#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "CH9120_Test.h"
#include "square_4ch.pio.h"

#define BASE_PIN        2u
#define DIR_PIN         6u
#define DEFAULT_FREQ_HZ 100u
#define MAX_FREQ_HZ     10000000u

static PIO  g_pio;
static uint g_sm;
static uint g_offset;

// ... set_frequency() etc. ...

static bool parse_freq(const char *buf, uint32_t *out) { /* ... как было ... */ }

static bool parse_direction(const char *buf, uint *out_dir) {
    const char *p = strstr(buf, "\"direction\"");
    if (!p) return false;
    p += 12;
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    if (strncmp(p, "\"forward\"", 9) == 0) { *out_dir = 0; return true; }
    if (strncmp(p, "\"backward\"", 10) == 0) { *out_dir = 1; return true; }
    char *end;
    long v = strtol(p, &end, 10);
    if (end != p && (v == 0 || v == 1)) { *out_dir = (uint)v; return true; }
    return false;
}

int main(void) {
    CH9120_init();

    // Настройка пина направления
    gpio_init(DIR_PIN);
    gpio_set_dir(DIR_PIN, GPIO_IN);
    gpio_pull_down(DIR_PIN);  // по умолчанию forward

    g_pio    = pio0;
    g_sm     = 0;
    g_offset = pio_add_program(g_pio, &square_4ch_program);
    square_4ch_program_init(g_pio, g_sm, g_offset, BASE_PIN, DIR_PIN, DEFAULT_FREQ_HZ);

    char rx_buf[128];
    int  rx_len = 0;

    while (1) {
        while (uart_is_readable(UART_ID1)) {
            char c = (char)uart_getc(UART_ID1);
            if (c == '{') {
                rx_buf[0] = '{';
                rx_len = 1;
                continue;
            }
            if (rx_len > 0 && rx_len < (int)sizeof(rx_buf) - 1) {
                rx_buf[rx_len++] = c;
            }
            if (c == '}' && rx_len > 0) {
                rx_buf[rx_len] = '\0';
                uint32_t freq;
                if (parse_freq(rx_buf, &freq)) {
                    set_frequency(freq);
                    uint dir;
                    if (parse_direction(rx_buf, &dir)) {
                        gpio_put(DIR_PIN, dir);
                    }
                }
                rx_len = 0;
            }
        }
    }
}