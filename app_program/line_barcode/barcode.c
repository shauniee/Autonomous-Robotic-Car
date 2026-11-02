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

bool barcode_config_valid(const barcode_config_t *c)
{
    if ((c == NULL) || (c->narrow_min == 0u) ||
        (c->narrow_min > c->narrow_max) ||
        (c->wide_ratio_min_milli <= 1000u) ||
        (c->wide_ratio_min_milli > c->wide_ratio_max_milli) ||
        (c->wide_ratio_max_milli > 10000u) ||
        (c->width_tolerance_milli == 0u) ||
        (c->width_tolerance_milli >= 500u))
    {
        return false;
    }
    /* Classes must not overlap even at the configured tolerance edges. */
    return (uint64_t)c->wide_ratio_min_milli *
        (1000u - c->width_tolerance_milli) >
        (uint64_t)1000u * (1000u + c->width_tolerance_milli);
}

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

static bool fits(uint32_t width, uint32_t reference, uint32_t tolerance)
{
    uint64_t difference = width > reference ? width - reference :
        reference - width;
    return difference * 1000u <= (uint64_t)reference * tolerance;
}

barcode_result_t barcode_decode(const uint32_t widths[BARCODE_ELEMENTS],
    const barcode_config_t *config, char *symbol,
    barcode_orientation_t *orientation)
{
    if ((widths == NULL) || (symbol == NULL) || (orientation == NULL) ||
        !barcode_config_valid(config))
    {
        return BARCODE_BAD_CONFIG;
    }
    uint32_t sorted[BARCODE_ELEMENTS];
    for (unsigned i = 0u; i < BARCODE_ELEMENTS; ++i)
    {
        sorted[i] = widths[i];
        for (unsigned j = i; (j > 0u) && (sorted[j] < sorted[j - 1u]); --j)
        {
            uint32_t value = sorted[j];
            sorted[j] = sorted[j - 1u];
            sorted[j - 1u] = value;
        }
    }
    /* *X*: twenty narrow elements including gaps, nine wide elements. */
    uint32_t narrow = (uint32_t)(((uint64_t)sorted[9] + sorted[10]) / 2u);
    uint32_t wide = sorted[24];
    if ((narrow < config->narrow_min) ||
        (narrow > config->narrow_max) || (sorted[19] >= sorted[20]) ||
        ((uint64_t)wide * 1000u <
            (uint64_t)narrow * config->wide_ratio_min_milli) ||
        ((uint64_t)wide * 1000u >
            (uint64_t)narrow * config->wide_ratio_max_milli))
    {
        return BARCODE_BAD_WIDTH;
    }
    uint16_t patterns[3] = {0};
    unsigned wide_counts[3] = {0};
    for (unsigned i = 0u; i < BARCODE_ELEMENTS; ++i)
    {
        bool is_narrow = fits(widths[i], narrow,
            config->width_tolerance_milli);
        bool is_wide = fits(widths[i], wide, config->width_tolerance_milli);
        if (is_narrow == is_wide)
        {
            return BARCODE_BAD_WIDTH;
        }
        if ((i == 9u) || (i == 19u))
        {
            if (!is_narrow)
            {
                return BARCODE_BAD_FRAME;
            }
            continue;
        }
        unsigned group = i / 10u;
        patterns[group] = (uint16_t)((patterns[group] << 1) |
            (is_wide ? 1u : 0u));
        wide_counts[group] += is_wide ? 1u : 0u;
    }
    if ((wide_counts[0] != 3u) || (wide_counts[1] != 3u) ||
        (wide_counts[2] != 3u))
    {
        return BARCODE_BAD_FRAME;
    }
    const uint16_t star = 0x094u;
    barcode_orientation_t direction;
    uint16_t payload = patterns[1];
    if ((patterns[0] == star) && (patterns[2] == star))
    {
        direction = BARCODE_FORWARD;
    }
    else if ((patterns[0] == reverse_pattern(star)) &&
        (patterns[2] == reverse_pattern(star)))
    {
        direction = BARCODE_REVERSE;
        payload = reverse_pattern(payload);
    }
    else
    {
        return BARCODE_BAD_FRAME;
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
    f->state = BARCODE_FRAME_WAIT_QUIET;
    f->count = 0u;
    f->min_quiet = BARCODE_QUIET_COUNTS;
    f->have_edge = false;
}

void barcode_frame_init(barcode_frame_t *f, const barcode_config_t *config)
{
    *f = (barcode_frame_t){.config = *config};
    barcode_frame_reset(f);
}

static void drop(barcode_frame_t *f)
{
    f->state = BARCODE_FRAME_WAIT_QUIET;
    f->count = 0u;
    f->min_quiet = BARCODE_QUIET_COUNTS;
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
    int32_t left = enc_travel_signed(left_count, f->prev_left);
    int32_t right = enc_travel_signed(right_count, f->prev_right);
    bool run_was_black = f->prev_black;
    f->prev_left = left_count;
    f->prev_right = right_count;
    f->prev_black = black;
    if (black == run_was_black)
    {
        /* Two edges of one polarity: an edge was missed. Resynchronise. */
        drop(f);
        return BARCODE_BAD_FRAME;
    }
    if (left < 0 || right < 0)
    {
        drop(f);
        return BARCODE_REVERSED_TRAVEL;
    }
    uint32_t width = ((uint32_t)left + (uint32_t)right) / 2u;

    if (f->state == BARCODE_FRAME_WAIT_QUIET)
    {
        /* A white run that is long enough, ended by a black edge, starts a frame. */
        if (!run_was_black && black && width >= f->min_quiet)
        {
            f->state = BARCODE_FRAME_CAPTURE;
            f->count = 0u;
        }
        return BARCODE_NO_RESULT;
    }

    if (width > BARCODE_MAX_ELEMENT_COUNTS)
    {
        drop(f);
        /* A long white run can also be the next frame's quiet zone. */
        if (!run_was_black && black && width >= BARCODE_QUIET_COUNTS)
        {
            f->state = BARCODE_FRAME_CAPTURE;
        }
        return BARCODE_BAD_WIDTH;
    }
    f->widths[f->count++] = width;
    if (f->count < BARCODE_ELEMENTS)
    {
        return BARCODE_NO_RESULT;
    }
    barcode_result_t result = barcode_decode(f->widths, &f->config, symbol, orientation);
    f->state = BARCODE_FRAME_WAIT_QUIET;
    f->count = 0u;
    /* After a decode demand the longer rearm gap before the next frame. */
    f->min_quiet = result == BARCODE_DECODED ? BARCODE_REARM_COUNTS : BARCODE_QUIET_COUNTS;
    return result;
}

#ifndef HOST_TEST
/* ---- Edge capture ---- */
static barcode_edge_t ring[BARCODE_EDGE_CAPACITY];
static _Atomic unsigned head;		/* handler; task seeds with IRQ masked */
static _Atomic unsigned tail;		/* written by the task */
static _Atomic uint32_t overflows;	/* written by the handler */
static _Atomic uint32_t order_lost;	/* written by the handler */
static bool claimed;

static void edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)pin;
    (void)sampled_us;	/* edge positions are encoder counts; time is not needed */
    if (edges == (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))
    {
        /* Both flags together: ordering was lost, never guess two edges. */
        atomic_store_explicit(&order_lost, atomic_load_explicit(&order_lost, memory_order_relaxed) + 1u,
            memory_order_release);
        return;
    }
    unsigned write = atomic_load_explicit(&head, memory_order_relaxed);
    unsigned read = atomic_load_explicit(&tail, memory_order_acquire);
    if ((unsigned)(write - read) >= BARCODE_EDGE_CAPACITY)
    {
        atomic_store_explicit(&overflows, atomic_load_explicit(&overflows, memory_order_relaxed) + 1u,
            memory_order_release);
        return;
    }
    ring[write % BARCODE_EDGE_CAPACITY] = (barcode_edge_t){
        .left_count = (barcode_count_t)drv_encoder_count(ENCODER_LEFT),
        .right_count = (barcode_count_t)drv_encoder_count(ENCODER_RIGHT),
        .high = edges == GPIO_IRQ_EDGE_RISE};
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
        ring[write % BARCODE_EDGE_CAPACITY] = (barcode_edge_t){
            .left_count = (barcode_count_t)drv_encoder_count(ENCODER_LEFT),
            .right_count = (barcode_count_t)drv_encoder_count(ENCODER_RIGHT),
            .high = gpio_get_val(BARCODE_GPIO) != 0};
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

uint32_t drv_barcode_overflow_count(void)
{
    return atomic_load_explicit(&overflows, memory_order_acquire);
}

uint32_t drv_barcode_order_lost_count(void)
{
    return atomic_load_explicit(&order_lost, memory_order_acquire);
}

/* ---- Scanner service ---- */
static rd_word_t mapping_word;  /* writer: Mission via barcode_set_mapping */
static _Atomic bool run_wanted; /* writer: Mission (barcode_start/stop) */
static barcode_frame_t frame;   /* S3 task only */
static barcode_event_t events[BARCODE_EVENT_CAPACITY];
static _Atomic unsigned event_head,
    event_tail; /* head: S3 task, tail: Mission */
static uint32_t seen_overflows, seen_order_lost; /* S3 task only */
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
    const barcode_config_t config =
    {
        .narrow_min = BARCODE_NARROW_MIN,
            .narrow_max = BARCODE_NARROW_MAX,
            .wide_ratio_min_milli = BARCODE_WIDE_RATIO_MIN_MILLI,
            .wide_ratio_max_milli = BARCODE_WIDE_RATIO_MAX_MILLI,
            .width_tolerance_milli = BARCODE_TOLERANCE_MILLI};
    if (!barcode_config_valid(&config) || !drv_barcode_init())
    {
        return false;
    }
    barcode_frame_init(&frame, &config);
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
        seen_order_lost = drv_barcode_order_lost_count();
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
    uint32_t order_lost = drv_barcode_order_lost_count();
    if (overflows != seen_overflows || order_lost != seen_order_lost)
    {
        /* Edges were lost: the partial frame cannot be trusted. */
        seen_overflows = overflows;
        seen_order_lost = order_lost;
        rd_fault_set(RD_OWNER_LINE, RD_FAULT_BARCODE_CAPTURE_OVERFLOW);
        barcode_frame_reset(&frame);
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
            frame.have_edge && dl >= 0 && dr >= 0 ? (uint32_t)(dl + dr) / 2u : 0u;
#endif
        bool black = edge.high == (BARCODE_BLACK_HIGH != 0);
        barcode_result_t result =
            barcode_frame_edge(&frame, black, edge.left_count, edge.right_count,
            &symbol, &orientation);
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
