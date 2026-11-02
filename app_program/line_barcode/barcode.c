/* Use the port guard when mixing ISO C headers with tk/syslib.h. */
#define PROHIBIT_DEF_SIZE_T
#include "line_barcode.h"
#include <stddef.h>
#include "../motion/motion.h"
#include "../common/robot_data.h"

#ifndef HOST_TEST
#include <stdatomic.h>
#include "../app.h"
#include "../common/drv_gpio_irq.h"
#include "../comms/comms.h"
#endif

/* ---- Code 39 decoder and frame assembler ---- */
/* Code 39 standard patterns: bit 8 is the first element, 1 means wide.
   Verified against the A/Z project patterns and ZXing Code39Reader's
   published encoding table. Only A-Z plus framing '*' are supported. */
static const uint16_t alphabet[BARCODE_ALPHABET] =
{
    0x109u, 0x049u, 0x148u, 0x019u, 0x118u, 0x058u, 0x00du,
    0x10cu, 0x04cu, 0x01cu, 0x103u, 0x043u, 0x142u, 0x013u,
    0x112u, 0x052u, 0x007u, 0x106u, 0x046u, 0x016u, 0x181u,
    0x0c1u, 0x1c0u, 0x091u, 0x190u, 0x0d0u
};

static uint16_t reverse_pattern(uint16_t pattern)
{
    uint16_t reversed = 0u;
    for (unsigned i = 0u; i < 9u; ++i)
    {
        reversed = (uint16_t)((reversed << 1) | (pattern & 1u));
        pattern >>= 1;
    }
    return reversed;
}

/* True when the widest `wide` of `count` widths are clearly wider than the rest,
   and the smallest wide width is returned. Bars and spaces are judged apart
   because the threshold makes one thinner and the other wider (INT-152). */
static bool wide_cut(const uint16_t *widths, unsigned count, unsigned wide, uint16_t *cut)
{
    uint16_t sorted[5];
    for (unsigned i = 0u; i < count; ++i)
    {
        sorted[i] = widths[i * 2u];
        for (unsigned j = i; (j > 0u) && (sorted[j] < sorted[j - 1u]); --j)
        {
            uint16_t value = sorted[j];
            sorted[j] = sorted[j - 1u];
            sorted[j - 1u] = value;
        }
    }
    *cut = sorted[count - wide];
    return (uint32_t)sorted[count - wide] * 2u >= (uint32_t)sorted[count - wide - 1u] * 3u;
}

/* Every A-Z and '*' has two wide bars and one wide space. */
static bool barcode_group(const uint16_t widths[9], uint16_t *pattern,
    uint16_t *bar_cut_out, uint16_t *space_cut_out)
{
    uint16_t bar_cut, space_cut;
    if (!wide_cut(widths, 5u, 2u, &bar_cut) || !wide_cut(&widths[1], 4u, 1u, &space_cut))
    {
        return false;
    }
    *bar_cut_out = bar_cut;
    *space_cut_out = space_cut;
    *pattern = 0u;
    for (unsigned i = 0u; i < 9u; ++i)
    {
        uint16_t cut = (i % 2u == 0u) ? bar_cut : space_cut;
        *pattern = (uint16_t)((*pattern << 1) | (widths[i] >= cut ? 1u : 0u));
    }
    return true;
}

barcode_result_t barcode_decode(const uint16_t widths[BARCODE_ELEMENTS],
    char *symbol, barcode_orientation_t *orientation)
{
    uint16_t patterns[2], bar_cut, space_cut;
    /* Elements 5 and 15 are the gaps. A one-wheel hump just before a print
       corrupts the start of the first guard, so only its last 5 elements are
       needed; they stop a run lost inside the letter reading as another letter
       (INT-155). P and C can complete early: the letter is right, the
       orientation is not known. */
    for (unsigned group = 0u; group < 2u; ++group)
    {
        if (!barcode_group(&widths[6u + group * 10u], &patterns[group], &bar_cut, &space_cut))
        {
            return BARCODE_NO_RESULT;
        }
    }
    uint16_t lead = 0u;
    for (unsigned i = 0u; i < 5u; ++i)
    {
        /* These widths did not set the cut, which sits at the narrowest wide. */
        uint32_t cut = i % 2u == 0u ? bar_cut : space_cut;
        lead = (uint16_t)((lead << 1) | ((uint32_t)widths[i] * 4u >= cut * 3u ? 1u : 0u));
    }
    const uint16_t star = 0x094u;
    barcode_orientation_t direction;
    uint16_t payload = patterns[0];
    if ((patterns[1] == star) && (lead == (star & 0x1Fu)))
    {
        direction = BARCODE_FORWARD;
    }
    else if ((patterns[1] == reverse_pattern(star)) && (lead == (reverse_pattern(star) & 0x1Fu)))
    {
        direction = BARCODE_REVERSE;
        payload = reverse_pattern(payload);
    }
    else
    {
        return BARCODE_NO_RESULT;
    }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        if (alphabet[i] == payload)
        {
            *symbol = (char)('A' + i);
            *orientation = direction;
            return BARCODE_DECODED;
        }
    }
    return BARCODE_BAD_SYMBOL;
}

barcode_action_t barcode_action_for(uint32_t word, char symbol)
{
    rd_map_action_t action;
    if (symbol < 'A' || symbol > 'Z' || !rd_mapping_lookup(word, (uint8_t)(symbol - 'A'), &action))
    {
        return BARCODE_UNMAPPED;
    }
    switch (action)
    {
    case RD_MAP_LEFT: return BARCODE_LEFT;
    case RD_MAP_RIGHT: return BARCODE_RIGHT;
    case RD_MAP_STRAIGHT: return BARCODE_STRAIGHT;
    default: return BARCODE_U_TURN;
    }
}

void barcode_frame_reset(barcode_frame_t *f)
{
    *f = (barcode_frame_t){0};
}

/* Travel of the sensor itself: it sits left of the axle centre, so while the
   robot steers it moves almost with the left wheel, not at the mean speed. */
static uint32_t sensor_travel(uint32_t left, uint32_t right)
{
    const uint32_t half = (uint32_t)TRACK_WIDTH_MM / 2u;
    return (left * (half + BARCODE_SENSOR_LEFT_MM) +
        right * (half - BARCODE_SENSOR_LEFT_MM)) / (2u * half);
}

barcode_result_t barcode_frame_edge(barcode_frame_t *f, bool black,
    barcode_count_t left_count, barcode_count_t right_count, char *symbol,
    barcode_orientation_t *orientation)
{
    if (!f->have_edge)
    {
        f->have_edge = true;
        f->prev_left = left_count;
        f->prev_right = right_count;
        f->prev_black = black;
        return BARCODE_NO_RESULT;
    }
    f->last_width = 0u;
    int32_t left = enc_travel_signed(left_count, f->prev_left);
    int32_t right = enc_travel_signed(right_count, f->prev_right);
    bool run_was_black = f->prev_black;
    f->prev_left = left_count;
    f->prev_right = right_count;
    f->prev_black = black;
    if (black == run_was_black)
    {
        /* Two edges of one polarity: an edge was missed. Resynchronise. */
        f->count = 0u;
        f->carry = 0u;
        return BARCODE_BAD_FRAME;
    }
    if (left < 0 || right < 0)
    {
        f->count = 0u;
        f->carry = 0u;
        return BARCODE_REVERSED_TRAVEL;
    }
    uint32_t width = sensor_travel((uint32_t)left, (uint32_t)right) + f->carry;
    f->carry = 0u;
    f->last_width = 0u;
    if (width < BARCODE_MIN_ELEMENT_COUNTS)
    {
        /* Sensor chatter: this run and the next belong to the previous element. */
        if (f->count > 0u)
        {
            f->carry = (uint16_t)(f->widths[--f->count] + width);
        }
        return BARCODE_NO_RESULT;
    }
    f->last_width = width > 0x7FFFu ? 0x7FFFu : (uint16_t)width;
    if (width > BARCODE_MAX_ELEMENT_COUNTS)
    {
        f->count = 0u; /* open floor: nothing before it belongs to a symbol */
        return BARCODE_BAD_WIDTH;
    }
    if (f->count == BARCODE_ELEMENTS)
    {
        for (unsigned i = 1u; i < BARCODE_ELEMENTS; ++i)
        {
            f->widths[i - 1u] = f->widths[i];
        }
        --f->count;
    }
    f->widths[f->count++] = (uint16_t)width;
    /* A print ends in a bar, and one guard alone would match when misaligned by
       a run. INT-155 confirmed the polarity on real passes. */
    if (f->count < BARCODE_ELEMENTS || !run_was_black)
    {
        return BARCODE_NO_RESULT;
    }
    barcode_result_t result = barcode_decode(f->widths, symbol, orientation);
    if (result == BARCODE_DECODED)
    {
        f->count = 0u; /* one event per print */
    }
    return result;
}

#ifndef HOST_TEST
/* ---- Edge capture ---- */
static barcode_edge_t ring[BARCODE_EDGE_CAPACITY];
static _Atomic unsigned head;		/* handler; task seeds with IRQ masked */
static _Atomic unsigned tail;		/* written by the task */
static _Atomic uint32_t overflows;	/* written by the handler */
static _Atomic uint32_t edge_total;	/* written by the handler: every edge seen */
static bool claimed;
static bool capture_high;	/* level of the newest ring entry; handler, seeded with IRQ masked */

static void edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)pin;
    (void)sampled_us;	/* edge positions are encoder counts; time is not needed */
    atomic_store_explicit(&edge_total, atomic_load_explicit(&edge_total, memory_order_relaxed) +
        (edges == (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE) ? 2u : 1u), memory_order_release);
    /* INT-147: the edge flags cannot order sensor chatter, and trusting them
       produced runs of one polarity. The pin says where the level really is. */
    bool high = gpio_get_val(BARCODE_GPIO) != 0;
    if (high == capture_high)
    {
        return; /* a pulse came and went: its travel stays in the current run */
    }
    unsigned write = atomic_load_explicit(&head, memory_order_relaxed);
    unsigned read = atomic_load_explicit(&tail, memory_order_acquire);
    if ((unsigned)(write - read) >= BARCODE_EDGE_CAPACITY)
    {
        atomic_store_explicit(&overflows, atomic_load_explicit(&overflows, memory_order_relaxed) + 1u,
            memory_order_release);
        return;
    }
    capture_high = high;
    ring[write % BARCODE_EDGE_CAPACITY] = (barcode_edge_t){
        .left_count = (barcode_count_t)drv_encoder_count(ENCODER_LEFT),
        .right_count = (barcode_count_t)drv_encoder_count(ENCODER_RIGHT),
        .high = high};
    atomic_store_explicit(&head, write + 1u, memory_order_release);
}

bool drv_barcode_init(void)
{
    if (drv_gpio_irq_init() < E_OK || gpio_set_pin(BARCODE_GPIO, GPIO_MODE_IN) < E_OK)
    {
        return false;
    }
    /* The mounted comparator must supply a verified safe level: no pulls. */
    clr_w(GPIO(BARCODE_GPIO), GPIO_PUE | GPIO_PDE);
    claimed = drv_gpio_irq_claim(BARCODE_GPIO, edge);
    return claimed;
}

void drv_barcode_enable(bool enabled)
{
    if (!claimed)
    {
        return;
    }
    (void)drv_gpio_irq_enable(BARCODE_GPIO, false);
    if (enabled)
    {
        UW state;
        DI(state);
        unsigned write = atomic_load_explicit(&head, memory_order_relaxed);
        atomic_store_explicit(&tail, write, memory_order_release);
        (void)drv_gpio_irq_enable(BARCODE_GPIO, true);
        /* Seed the white approach; the first real edge alone loses that gap. */
        capture_high = gpio_get_val(BARCODE_GPIO) != 0;
        ring[write % BARCODE_EDGE_CAPACITY] = (barcode_edge_t){
            .left_count = (barcode_count_t)drv_encoder_count(ENCODER_LEFT),
            .right_count = (barcode_count_t)drv_encoder_count(ENCODER_RIGHT),
            .high = capture_high};
        atomic_store_explicit(&head, write + 1u, memory_order_release);
        EI(state);
    }
}

bool drv_barcode_take(barcode_edge_t *out)
{
    unsigned read = atomic_load_explicit(&tail, memory_order_relaxed);
    if (read == atomic_load_explicit(&head, memory_order_acquire))
    {
        return false;
    }
    *out = ring[read % BARCODE_EDGE_CAPACITY];
    atomic_store_explicit(&tail, read + 1u, memory_order_release);
    return true;
}

uint32_t drv_barcode_edge_count(void)
{
    return atomic_load_explicit(&edge_total, memory_order_acquire);
}

uint32_t drv_barcode_overflow_count(void)
{
    return atomic_load_explicit(&overflows, memory_order_acquire);
}

/* ---- Scanner service ---- */
static rd_word_t mapping_word;  /* writer: Mission via barcode_set_mapping */
static _Atomic bool run_wanted; /* writer: Mission (barcode_start/stop) */
static barcode_frame_t frame;   /* S3 task only */
static barcode_event_t events[BARCODE_EVENT_CAPACITY];
static _Atomic unsigned event_head,
    event_tail; /* head: S3 task, tail: Mission */
static uint32_t seen_overflows; /* S3 task only */
static _Atomic bool initialised;                 /* writer: S3 task, once */
static bool running;                             /* S3 task only */

#if BARCODE_DIAGNOSTICS
static barcode_diagnostic_t diagnostics[32];
static unsigned diagnostic_head, diagnostic_tail, diagnostic_dropped;
#endif
bool barcode_diagnostic_pop(barcode_diagnostic_t *record, unsigned *dropped)
{
#if BARCODE_DIAGNOSTICS
    *dropped = diagnostic_dropped;
    if (diagnostic_tail == diagnostic_head)
    {
        return false;
    }
    *record = diagnostics[diagnostic_tail++ % 32u];
    return true;
#else
    (void)record;
    (void)dropped;
    return false;
#endif
}

bool barcode_init(void)
{
    if (!drv_barcode_init())
    {
        return false;
    }
    barcode_frame_reset(&frame);
    initialised = true;
    return true;
}

bool barcode_set_mapping(const uint8_t letters[4])
{
    uint32_t word;
    if (atomic_load_explicit(&run_wanted, memory_order_acquire) ||
        !rd_mapping_pack(letters, &word))
    {
        return false;
    }
    rd_word_publish(&mapping_word, word);
    return true;
}

bool barcode_mapped(void)
{
    return rd_mapping_is_mapped(rd_word_read(&mapping_word));
}

bool barcode_start(void)
{
    if (!initialised || !barcode_mapped())
    {
        return false;
    }
    barcode_event_t stale;
    while (barcode_pop(&stale))
    {
        /* Mission is the consumer: drop events left over from an earlier run. */
    }
    atomic_store_explicit(&run_wanted, true, memory_order_release);
    return true;
}

void barcode_stop(void)
{
    atomic_store_explicit(&run_wanted, false, memory_order_release);
}

static void push_event(const barcode_event_t *event)
{
    unsigned head = atomic_load_explicit(&event_head, memory_order_relaxed);
    unsigned tail = atomic_load_explicit(&event_tail, memory_order_acquire);
    if ((unsigned)(head - tail) >= BARCODE_EVENT_CAPACITY)
    {
        rd_fault_set(RD_OWNER_LINE, RD_FAULT_BARCODE_EVENT_OVERFLOW);
        return;
    }
    events[head % BARCODE_EVENT_CAPACITY] = *event;
    atomic_store_explicit(&event_head, head + 1u, memory_order_release);
}

/* INT-146 diagnostics: raw run widths for the laptop, before any filtering. A
   full ring drops them; the sequence number shows the loss. */
enum { RUN_TRACE_DEPTH = 8u };
static uint8_t run_records[RUN_TRACE_DEPTH][MQTT_LEN_BARCODE_TRACE];
static _Atomic uint32_t run_head, run_tail;
static uint16_t run_pending[MQTT_BARCODE_TRACE_RUNS];
static unsigned run_count;
static uint8_t run_sequence;

static void run_trace_flush(void)
{
    uint32_t write = atomic_load_explicit(&run_head, memory_order_relaxed);
    if (run_count == 0u)
    {
        return;
    }
    while (run_count < MQTT_BARCODE_TRACE_RUNS)
    {
        run_pending[run_count++] = 0u; /* zero width: unused slot */
    }
    run_count = 0u;
    uint8_t sequence = run_sequence++;
    if (write - atomic_load_explicit(&run_tail, memory_order_acquire) >= RUN_TRACE_DEPTH)
    {
        return;
    }
    (void)mqtt_encode_barcode_trace(run_records[write % RUN_TRACE_DEPTH],
        MQTT_LEN_BARCODE_TRACE, sequence, run_pending);
    atomic_store_explicit(&run_head, write + 1u, memory_order_release);
}

/* One element the decoder accepted, or a long run it rejected; chatter is
   merged first, so these frames fit the bridge. */
static void run_trace_add(uint16_t width, bool black)
{
    if (width == 0u)
    {
        return;
    }
    run_pending[run_count++] = (uint16_t)((width & 0x7FFFu) | (black ? 0x8000u : 0u));
    if (run_count == MQTT_BARCODE_TRACE_RUNS || width > BARCODE_MAX_ELEMENT_COUNTS)
    {
        run_trace_flush();
    }
}

unsigned barcode_trace_take(uint8_t *out, unsigned capacity)
{
    uint32_t read = atomic_load_explicit(&run_tail, memory_order_relaxed);
    if (out == NULL || capacity < MQTT_LEN_BARCODE_TRACE ||
        read == atomic_load_explicit(&run_head, memory_order_acquire))
    {
        return 0u;
    }
    for (unsigned i = 0u; i < MQTT_LEN_BARCODE_TRACE; ++i)
    {
        out[i] = run_records[read % RUN_TRACE_DEPTH][i];
    }
    atomic_store_explicit(&run_tail, read + 1u, memory_order_release);
    return MQTT_LEN_BARCODE_TRACE;
}

void barcode_service(bool cancel)
{
    bool wanted =
        !cancel && atomic_load_explicit(&run_wanted, memory_order_acquire);
    if (wanted != running)
    {
        /* Only this task touches the scanner state and the Line fault byte. */
        drv_barcode_enable(false);
        barcode_frame_reset(&frame);
        seen_overflows = drv_barcode_overflow_count();
        rd_fault_clear(RD_OWNER_LINE, RD_FAULT_BARCODE_CAPTURE_OVERFLOW |
            RD_FAULT_BARCODE_EVENT_OVERFLOW);
        drv_barcode_enable(wanted);
        running = wanted;
    }
    if (!running)
    {
        return;
    }
    uint32_t overflows = drv_barcode_overflow_count();
    if (overflows != seen_overflows)
    {
        /* The handler keeps level order, so a drop only merges chatter into the
           current run: report it, but keep the print (INT-155). */
        seen_overflows = overflows;
        rd_fault_set(RD_OWNER_LINE, RD_FAULT_BARCODE_CAPTURE_OVERFLOW);
    }
    barcode_edge_t edge;
    for (unsigned budget = 0u;
        budget < BARCODE_SERVICE_BUDGET && drv_barcode_take(&edge); ++budget)
    {
        char symbol = 0;
        barcode_orientation_t orientation = BARCODE_FORWARD;
#if BARCODE_DIAGNOSTICS
        int32_t dl = enc_travel_signed(edge.left_count, frame.prev_left);
        int32_t dr = enc_travel_signed(edge.right_count, frame.prev_right);
        uint32_t width =
            frame.have_edge && dl >= 0 && dr >= 0 ?
            sensor_travel((uint32_t)dl, (uint32_t)dr) : 0u;
#endif
        bool black = edge.high == (BARCODE_BLACK_HIGH != 0);
        barcode_result_t result =
            barcode_frame_edge(&frame, black, edge.left_count, edge.right_count,
            &symbol, &orientation);
        /* Compile-time constant: TRACE=0 removes this work at optimisation
           without leaving the static diagnostic producer unused. */
        if (ROBOT_TRACE_ENABLED)
        {
            run_trace_add(frame.last_width, !black);
        }
#if BARCODE_DIAGNOSTICS
        if (diagnostic_head - diagnostic_tail < 32u)
        {
            diagnostics[diagnostic_head++ % 32u] = (barcode_diagnostic_t){
                .left = edge.left_count,
                    .right = edge.right_count,
                    .width = (uint16_t)(width > 65535u ? 65535u : width),
                    .result = result,
                    .orientation = orientation,
                    .letter = symbol ? (unsigned)(symbol - 'A') : 31u,
                    .action =
                    symbol ? barcode_action_for(rd_word_read(&mapping_word), symbol)
                : BARCODE_UNMAPPED};
        }
        else if (diagnostic_dropped != ~0u)
        ++diagnostic_dropped;
#endif
        if (result == BARCODE_DECODED)
        {
            barcode_event_t event = {.left_count = edge.left_count,
                    .right_count = edge.right_count};
            barcode_event_set(&event, symbol,
                barcode_action_for(rd_word_read(&mapping_word), symbol),
                orientation);
            push_event(&event);
        }
    }
}

bool barcode_pop(barcode_event_t *event)
{
    unsigned tail = atomic_load_explicit(&event_tail, memory_order_relaxed);
    if (tail == atomic_load_explicit(&event_head, memory_order_acquire))
    {
        return false;
    }
    *event = events[tail % BARCODE_EVENT_CAPACITY];
    atomic_store_explicit(&event_tail, tail + 1u, memory_order_release);
    return true;
}
#endif /* !HOST_TEST */
