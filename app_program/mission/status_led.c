/*
 *    status_led.c
 *    Shows the mission state as one static colour on both onboard WS2812
 *    RGB LEDs (GP18). PIO0 state machine 0 generates the 800 kHz bit timing,
 *    so a frame costs the CPU two FIFO writes. The SDK CYW43 allocator
 *    searches PIO1 first, so the radio never takes this block.
 *    shortcut: PIO0 is not claimed in the SDK allocator; claim it there if
 *    another PIO user is ever added.
 */

#include "status_led.h"

/* PIO0 registers (RP2040 datasheet, section 3.7). */
#define LED_PIO_BASE            0x50200000u
#define LED_PIO_CTRL            (LED_PIO_BASE + 0x000u)
#define LED_PIO_FSTAT           (LED_PIO_BASE + 0x004u)
#define LED_PIO_TXF0            (LED_PIO_BASE + 0x010u)
#define LED_PIO_INSTR_MEM0      (LED_PIO_BASE + 0x048u)
#define LED_PIO_SM0_CLKDIV      (LED_PIO_BASE + 0x0C8u)
#define LED_PIO_SM0_EXECCTRL    (LED_PIO_BASE + 0x0CCu)
#define LED_PIO_SM0_SHIFTCTRL   (LED_PIO_BASE + 0x0D0u)
#define LED_PIO_SM0_INSTR       (LED_PIO_BASE + 0x0D8u)
#define LED_PIO_SM0_PINCTRL     (LED_PIO_BASE + 0x0DCu)

#define LED_PIO_SM0_ENABLE      (1u << 0)
#define LED_PIO_SM0_RESTART     (1u << 4)
#define LED_PIO_SM0_CLKRESTART  (1u << 8)
#define LED_PIO_SM0_TXEMPTY     (1u << 24)

/* 10 PIO cycles per bit at 800 kHz = 8 MHz; 125 MHz / 8 MHz = 15 + 160/256. */
#define LED_PIO_CLKDIV          ((15u << 16) | (160u << 8))
/* Wrap from instruction 3 back to 0. */
#define LED_PIO_EXECCTRL        (3u << 12)
/* Join the FIFOs for TX, autopull 24 bits, shift left (MSB first). */
#define LED_PIO_SHIFTCTRL       ((1u << 30) | (24u << 25) | (1u << 17))
/* One side-set pin and one SET pin, both GP18. */
#define LED_PIO_PINCTRL         ((1u << 29) | (1u << 26) | \
    (STATUS_LED_GPIO << 10) | (STATUS_LED_GPIO << 5))

#define LED_PIO_SET_PINS_LOW    0xE000u /* set pins, 0 */
#define LED_PIO_SET_PINDIR_OUT  0xE081u /* set pindirs, 1 */
#define LED_PIO_JMP_START       0x0000u /* jmp 0 */

/* Bounded wait for the PIO0 reset release; it takes a few cycles. */
#define LED_RESET_POLLS         1000u

enum
{
    LED_PROGRAM_LENGTH = 4
};

/* WS2812 program at offset 0, side-set 1 pin:
   0: out x, 1   side 0 [2]
   1: jmp !x, 3  side 1 [1]
   2: jmp 0      side 1 [4]   '1' bit: 7 cycles high, 3 low
   3: nop        side 0 [4]   '0' bit: 2 cycles high, 8 low */
static const uint16_t led_program[LED_PROGRAM_LENGTH] = {
    0x6221u, 0x1123u, 0x1400u, 0xA442u};

/* 0xRRGGBB at quarter brightness. Reacquire, bypass and scan share orange. */
static const uint32_t state_colours[MISSION_STATE_COUNT] = {
    [MISSION_READY] = 0x004000u,       /* green */
    [MISSION_FOLLOW_LINE] = 0x200040u, /* purple */
    [MISSION_NAVIGATE] = 0x000040u,    /* blue */
    [MISSION_SCAN] = 0x401000u,        /* orange */
    [MISSION_BYPASS] = 0x401000u,      /* orange */
    [MISSION_REACQUIRE] = 0x401000u,   /* orange */
    [MISSION_STOPPED] = 0x400000u};    /* red */

/* Mission task only: PIO0 state machine 0 is running. */
static struct
{
    uint8_t ready : 1;
} led;

ER status_led_init(void)
{
    clr_w(RESETS_RESET, RESETS_RESET_PIO0);
    uint32_t polls = 0u;
    while (0u == (in_w(RESETS_RESET_DONE) & RESETS_RESET_PIO0))
    {
        polls++;
        if (polls >= LED_RESET_POLLS)
        {
            return E_TMOUT;
        }
    }

    clr_w(LED_PIO_CTRL, LED_PIO_SM0_ENABLE);
    for (uint32_t i = 0u; i < (uint32_t)LED_PROGRAM_LENGTH; i++)
    {
        out_w(LED_PIO_INSTR_MEM0 + (4u * i), led_program[i]);
    }
    out_w(LED_PIO_SM0_CLKDIV, LED_PIO_CLKDIV);
    out_w(LED_PIO_SM0_EXECCTRL, LED_PIO_EXECCTRL);
    out_w(LED_PIO_SM0_SHIFTCTRL, LED_PIO_SHIFTCTRL);
    out_w(LED_PIO_SM0_PINCTRL, LED_PIO_PINCTRL);
    set_w(LED_PIO_CTRL, LED_PIO_SM0_RESTART | LED_PIO_SM0_CLKRESTART);
    out_w(LED_PIO_SM0_INSTR, LED_PIO_SET_PINS_LOW);
    out_w(LED_PIO_SM0_INSTR, LED_PIO_SET_PINDIR_OUT);
    out_w(LED_PIO_SM0_INSTR, LED_PIO_JMP_START);

    /* Output pad, no pulls, then hand the pin to PIO0 already driven low. */
    out_w(GPIO(STATUS_LED_GPIO), GPIO_DRIVE_4MA);
    out_w(GPIO_CTRL(STATUS_LED_GPIO), GPIO_CTRL_FUNCSEL_PIO0);
    set_w(LED_PIO_CTRL, LED_PIO_SM0_ENABLE);
    led.ready = 1u;
    return E_OK;
}

bool status_led_show(MissionState_t state)
{
    /* An empty FIFO means the last frame left at least one release ago, far
       longer than the 50 us WS2812 latch gap. */
    if ((0u == led.ready) || ((uint32_t)state >= (uint32_t)MISSION_STATE_COUNT)
        || (0u == (in_w(LED_PIO_FSTAT) & LED_PIO_SM0_TXEMPTY)))
    {
        return false;
    }
    uint32_t rgb = state_colours[state];
    /* WS2812 takes green, red, blue, MSB first, in the top 24 bits. */
    uint32_t grb = ((rgb & 0x00FF00u) << 8) | ((rgb & 0xFF0000u) >> 8) |
        (rgb & 0x0000FFu);
    for (uint32_t i = 0u; i < STATUS_LED_COUNT; i++)
    {
        out_w(LED_PIO_TXF0, grb << 8);
    }
    return true;
}
