/* audio.c - the port's sound output: what the game writes to the AdLib's
 * registers (sound.c) goes to doskit's OPL (runtime/opl.h), which the
 * platform's audio stream plays.
 *
 * The game writes on the port's clock (dos.c), the stream runs on the
 * sound device's; a write is put into a queue with its time in samples
 * and the stream takes it when it has played that far, a little behind
 * the game (LEAD), so that notes keep their distances whatever the size
 * of the device's buffer.
 *
 * BI_OPLLOG=FILE writes every register and value to FILE, a line each
 * (for comparisons with the runner's -log of the original's writes).
 */
#include <stdio.h>
#include <stdlib.h>
#include "bi.h"
#include "opl.h"
#include "platform.h"

#define RATE 48000
#define LEAD (RATE / 20)                /* 50 ms behind the game */
#define LEAD_MAX (RATE / 4)
#define QUEUE 16384

static OPL chip;
static struct { int64_t at; uint8_t reg, value; } queue[QUEUE];
static unsigned head, count;
static int64_t played, offset;
static int playing, started;
static FILE *reg_log;

static void fill(int16_t *out, int frames, void *user)
{
    static int16_t mono[1024];
    int i, n;

    (void)user;
    while (frames > 0) {
        n = frames < 1024 ? frames : 1024;
        while (count && queue[head].at <= played) {
            opl_write(&chip, queue[head].reg, queue[head].value);
            head = (head + 1) % QUEUE;
            count--;
        }
        if (count && queue[head].at - played < n)
            n = (int)(queue[head].at - played);
        opl_render(&chip, mono, n);
        for (i = 0; i < n; i++) {
            out[2 * i] = mono[i];
            out[2 * i + 1] = mono[i];
        }
        out += 2 * n;
        frames -= n;
        played += n;
    }
}

static void audio_start(void)
{
    const char *path = getenv("BI_OPLLOG");

    started = 1;
    if (path)
        reg_log = fopen(path, "w");
    opl_init(&chip, RATE);
    playing = plat_audio_start(RATE, fill, NULL);
}

void adlib_out(unsigned reg, unsigned value)
{
    int64_t at;

    if (!started)
        audio_start();
    if (reg_log)
        fprintf(reg_log, "%02X %02X\n", reg, value);
    if (!playing)
        return;
    plat_audio_lock();
    at = (int64_t)(clock_seconds() * RATE) + offset;
    if (at < played + LEAD / 2 || at > played + LEAD_MAX) {
        /* the two clocks have drifted apart, or the game stood still */
        offset += played + LEAD - at;
        at = played + LEAD;
    }
    if (count && queue[(head + count - 1) % QUEUE].at > at)
        at = queue[(head + count - 1) % QUEUE].at;
    if (count < QUEUE) {
        unsigned i = (head + count) % QUEUE;

        queue[i].at = at;
        queue[i].reg = (uint8_t)reg;
        queue[i].value = (uint8_t)value;
        count++;
    }
    plat_audio_unlock();
}
