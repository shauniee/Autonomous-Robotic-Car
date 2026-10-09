#include "barcode_fixtures.h"
#include "../../app_program/subsystem3/barcode_capture.h"
#include <math.h>
#include <stddef.h>
#define REQUIRE(condition) do { if (!(condition)) { return false; } } while (0)

S3BarcodeConfig barcode_fixture_config(void)
{
    return (S3BarcodeConfig){.configured = true, .black_high = false,
        .left_polarity = 1, .right_polarity = 1,
        .narrow_min_counts = 8u, .narrow_max_counts = 24u,
        .wide_ratio_min = 2.5f, .wide_ratio_max = 3.5f,
        .narrow_variation = 1.5f, .quiet_counts = 80u, .rearm_counts = 100u,
        .frame_timeout_ms = 1000u, .event_age_ms = 500u,
        .mapping = {[0] = S3_NAV_LEFT, [1] = S3_NAV_RIGHT,
            [2] = S3_NAV_STRAIGHT, [3] = S3_NAV_UTURN}};
}
void barcode_fixture_widths(uint16_t middle, unsigned scale, uint32_t *widths)
{
    const uint16_t masks[3] = {0x094u, middle, 0x094u};
    for (unsigned group = 0u; group < 3u; ++group)
    {
        for (unsigned i = 0u; i < 9u; ++i)
        {
            widths[group * 10u + i] = scale *
                ((masks[group] & (1u << (8u - i))) != 0u ? 30u : 10u);
        }
        if (group < 2u)
        {
            widths[group * 10u + 9u] = scale * 10u;
        }
    }
}
bool barcode_fixture_symbols(void)
{
    S3BarcodeConfig config = barcode_fixture_config();
    const uint16_t patterns[] = {0x109u, 0x049u, 0x148u, 0x019u, 0x0d0u};
    const char expected[] = "ABCDZ";
    uint32_t widths[S3_BARCODE_WIDTHS];
    for (unsigned scale = 1u; scale <= 2u; ++scale)
    {
        for (unsigned i = 0u; i < 5u; ++i)
        {
            barcode_fixture_widths(patterns[i], scale, widths);
            char symbol = '?';
            REQUIRE(s3_code39_decode(widths, &config, &symbol) == S3_BC_OK);
            REQUIRE(symbol == expected[i]);
        }
    }
    return true;
}
bool barcode_fixture_rejection(void)
{
    S3BarcodeConfig config = barcode_fixture_config();
    uint32_t widths[S3_BARCODE_WIDTHS];
    uint32_t reverse[S3_BARCODE_WIDTHS];
    barcode_fixture_widths(0x109u, 1u, widths);
    for (unsigned i = 0u; i < S3_BARCODE_WIDTHS; ++i)
    {
        reverse[i] = widths[S3_BARCODE_WIDTHS - i - 1u];
    }
    char symbol = '?';
    REQUIRE(s3_code39_decode(reverse, &config, &symbol) == S3_BC_REVERSE);
    REQUIRE(symbol == '?');
    config.allow_reverse = true;
    REQUIRE(s3_code39_decode(reverse, &config, &symbol) == S3_BC_OK);
    REQUIRE(symbol == 'A');
    widths[9] = 30u;
    REQUIRE(s3_code39_decode(widths, &config, &symbol) != S3_BC_OK);
    barcode_fixture_widths(0x109u, 1u, widths);
    widths[0] = 0u;
    REQUIRE(s3_code39_decode(widths, &config, &symbol) == S3_BC_BAD_WIDTH);
    config.wide_ratio_min = NAN;
    REQUIRE(!s3_barcode_config_valid(&config));
    REQUIRE(s3_code39_decode(widths, &config, &symbol) == S3_BC_UNCONFIGURED);
    return true;
}
static void advance(S3BarcodeEdge *e, uint32_t counts)
{
    e->left_count += counts / 2u;
    e->right_count += counts / 2u;
    e->captured_us += 500u + counts * 3u; /* Deliberately changing speed. */
}
static void encounter(S3BarcodeDecoder *d, S3BarcodeEdge *e, uint16_t pattern)
{
    uint32_t widths[S3_BARCODE_WIDTHS];
    barcode_fixture_widths(pattern, 1u, widths);
    advance(e, 100u);
    e->high = false;
    s3_barcode_decoder_edge(d, e);
    for (unsigned i = 0u; i < S3_BARCODE_WIDTHS; ++i)
    {
        advance(e, widths[i]);
        e->high = !e->high;
        s3_barcode_decoder_edge(d, e);
    }
    advance(e, 100u);
    s3_barcode_decoder_poll(d, e, e->captured_us / 1000u);
}
bool barcode_fixture_events(void)
{
    S3BarcodeConfig config = barcode_fixture_config();
    S3BarcodeDecoder d;
    s3_barcode_decoder_init(&d, &config, 1u);
    S3BarcodeEdge edge = {.high = true, .encoder_valid = true,
        .context = 7u, .generation = 1u};
    s3_barcode_decoder_poll(&d, &edge, 0u);
    encounter(&d, &edge, 0x109u);
    REQUIRE(d.status.state == S3_BC_REARM);
    S3NavEvent event;
    REQUIRE(s3_barcode_decoder_take(&d, &event));
    REQUIRE((event.symbol == 'A') && (event.action == S3_NAV_LEFT));
    REQUIRE((event.context == 7u) && (event.id == 1u));
    REQUIRE(event.acquired_ms <= edge.captured_us / 1000u);
    REQUIRE(event.captured_us < edge.captured_us);
    s3_barcode_decoder_poll(&d, &edge, edge.captured_us / 1000u);
    REQUIRE(!s3_barcode_decoder_take(&d, &event));
    REQUIRE(d.status.state == S3_BC_QUIET);
    encounter(&d, &edge, 0x0d0u);
    REQUIRE(d.status.symbol == 'Z');
    REQUIRE(d.status.result == S3_BC_UNMAPPED);
    REQUIRE(!s3_barcode_decoder_take(&d, &event));
    return true;
}
bool barcode_fixture_overflow(void)
{
    S3BarcodeConfig config = barcode_fixture_config();
    S3BarcodeDecoder d;
    s3_barcode_decoder_init(&d, &config, 1u);
    S3BarcodeEdge edge = {.high = true, .encoder_valid = true,
        .context = 7u, .generation = 1u};
    s3_barcode_decoder_poll(&d, &edge, 0u);
    for (unsigned i = 0u; i <= S3_NAV_CAPACITY; ++i)
    {
        encounter(&d, &edge, 0x109u);
        if (i < S3_NAV_CAPACITY)
        {
            REQUIRE(d.count == i + 1u);
            s3_barcode_decoder_poll(&d, &edge, edge.captured_us / 1000u);
        }
    }
    REQUIRE(d.status.state == S3_BC_FAULT);
    REQUIRE(d.status.result == S3_BC_QUEUE_OVERFLOW);
    S3NavEvent event;
    REQUIRE(!s3_barcode_decoder_take(&d, &event));
    s3_barcode_decoder_init(&d, &config, 2u);
    edge.generation = 2u;
    edge.ambiguous = true;
    s3_barcode_decoder_edge(&d, &edge);
    REQUIRE(d.status.result == S3_BC_CAPTURE_OVERFLOW);
    return true;
}
bool barcode_fixture_wrap(void)
{
    S3BarcodeConfig config = barcode_fixture_config();
    S3BarcodeEdge from = {.left_count = UINT32_MAX - 4u,
        .right_count = UINT32_MAX - 4u, .encoder_valid = true};
    S3BarcodeEdge to = {.left_count = 5u, .right_count = 5u,
        .encoder_valid = true};
    uint32_t counts;
    REQUIRE(s3_barcode_travel(&config, &from, &to, &counts));
    REQUIRE(counts == 20u);
    REQUIRE(!s3_barcode_travel(&config, &to, &from, &counts));
    S3BarcodeDecoder d;
    s3_barcode_decoder_init(&d, &config, 1u);
    S3BarcodeEdge edge = {.high = true, .encoder_valid = true,
        .generation = 1u, .context = 1u, .captured_us = UINT32_MAX - 200u};
    s3_barcode_decoder_poll(&d, &edge, 0u);
    advance(&edge, 100u);
    edge.high = false;
    s3_barcode_decoder_edge(&d, &edge);
    REQUIRE(d.status.state == S3_BC_CAPTURE);
    edge.captured_us += config.frame_timeout_ms * 1000u;
    s3_barcode_decoder_poll(&d, &edge, config.frame_timeout_ms);
    REQUIRE(d.status.result == S3_BC_PARTIAL);
    return true;
}

bool barcode_fixture_capture_ring(void)
{
    static S3BarcodeCapture c;
    s3_barcode_capture_init(&c);
    /* Force both indices across UINT32 wrap in the actual SPSC algorithm. */
    atomic_store(&c.written, UINT32_MAX - 100u);
    atomic_store(&c.consumed, UINT32_MAX - 100u);
    for (unsigned i = 0u; i < S3_BARCODE_EDGE_CAPACITY; ++i)
    {
        S3BarcodeEdge edge = {.captured_us = i};
        REQUIRE(s3_barcode_capture_push_isr(&c, &edge));
    }
    S3BarcodeEdge edge = {0};
    REQUIRE(!s3_barcode_capture_push_isr(&c, &edge));
    REQUIRE(atomic_load(&c.overflow));
    for (unsigned i = 0u; i < S3_BARCODE_EDGE_CAPACITY; ++i)
    {
        REQUIRE(s3_barcode_capture_pop_task(&c, &edge));
        REQUIRE(edge.captured_us == i);
    }
    REQUIRE(s3_barcode_capture_empty(&c));
    REQUIRE(!s3_barcode_capture_pop_task(&c, &edge));
    s3_barcode_capture_reset_task(&c);
    REQUIRE(!atomic_load(&c.overflow));
    return true;
}
