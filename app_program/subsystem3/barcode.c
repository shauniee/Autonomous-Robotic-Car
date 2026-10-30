#include "barcode.h"
#include <stdatomic.h>
#include "drv_barcode.h"
#include "../common/robot_data.h"

static rd_word_t mapping_word;			/* writer: Mission via barcode_set_mapping */
static _Atomic bool run_wanted;			/* writer: Mission (barcode_start/stop) */
static barcode_frame_t frame;			/* S3 task only */
static barcode_event_t events[BARCODE_EVENT_CAPACITY];
static _Atomic unsigned event_head, event_tail;	/* head: S3 task, tail: Mission */
static uint32_t seen_overflows, seen_order_lost;	/* S3 task only */
static _Atomic bool initialised;		/* writer: S3 task, once */
static bool running;				/* S3 task only */

bool barcode_init(void)
{
    const barcode_config_t config = {
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
    if (atomic_load_explicit(&run_wanted, memory_order_acquire) || !rd_mapping_pack(letters, &word))
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
    bool wanted = !cancel && atomic_load_explicit(&run_wanted, memory_order_acquire);
    if (wanted != running)
    {
        /* Only this task touches the scanner state and the Line fault byte. */
        drv_barcode_enable(false);
        barcode_frame_reset(&frame);
        seen_overflows = drv_barcode_overflow_count();
        seen_order_lost = drv_barcode_order_lost_count();
        rd_fault_clear(RD_OWNER_LINE, RD_FAULT_BARCODE_CAPTURE_OVERFLOW | RD_FAULT_BARCODE_EVENT_OVERFLOW);
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
    for (unsigned budget = 0u; budget < BARCODE_SERVICE_BUDGET && drv_barcode_take(&edge); ++budget)
    {
        char symbol;
        barcode_orientation_t orientation;
        bool black = edge.high == (BARCODE_BLACK_HIGH != 0);
        barcode_result_t result = barcode_frame_edge(&frame, black, edge.left_count, edge.right_count,
            &symbol, &orientation);
        if (result == BARCODE_DECODED)
        {
            barcode_event_t event = {.left_count = edge.left_count, .right_count = edge.right_count};
            barcode_event_set(&event, symbol, barcode_action_for(rd_word_read(&mapping_word), symbol),
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
