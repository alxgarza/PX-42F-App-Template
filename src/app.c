/* The template app: one of each of the framework's parts (px42f/px_app.h).
 * A dot bounces across the screen (frames, waiting in px->key()); the last
 * key pressed is shown by its place and printed on the USB console; ENTER
 * plays a sound; the clock and the battery are shown; and how many times
 * the app has run is kept in a file in its folder. ON ends it.
 * Replace this file with your own app_main(). */
#include <stdio.h>

#include "px.h"

#define FRAME_MS 50

/* The file RUNS in the app's folder (/BIN/APPS/HELLO/): how many times it
 * has started. */
static int count_runs(void) {
    int n = 0;
    px->load("RUNS", &n, sizeof(n));
    n++;
    px->save("RUNS", &n, sizeof(n));
    return n;
}

/* The next key before time t (px->millis()), or PX_KEY_NONE at t. A frame
 * that's late doesn't wait at all: a negative wait would be "forever". */
static int key_before(uint32_t t) {
    int32_t left = (int32_t) (t - px->millis());
    return px->key(left > 0 ? (int) left : 0);
}

static void draw(int dot_x, int dot_y, const char *last_key, int runs) {
    char line[40];
    px_datetime_t now;
    px->datetime(&now);

    px->clear();
    px->fill(0, 0, PX_SCREEN_W, 12, 1);
    px->text(2, 0, "PX-42F App Template", PX_FONT_SMALL, 1);
    snprintf(line, sizeof(line), "%02ld:%02ld", (long) now.hour, (long) now.min);
    px->text(PX_SCREEN_W - 2 - px->text_width(line, PX_FONT_SMALL), 0, line, PX_FONT_SMALL, 1);

    px->text(4, 18, "Hello!", PX_FONT_LARGE, 0);
    snprintf(line, sizeof(line), "Last key: %s", last_key);
    px->text(4, 40, line, PX_FONT_SMALL, 0);
    snprintf(line, sizeof(line), "Run %d   Battery %d.%02d V", runs, px->battery_mv() / 1000,
             px->battery_mv() % 1000 / 10);
    px->text(4, 54, line, PX_FONT_SMALL, 0);

    px->line(0, 70, PX_SCREEN_W - 1, 70, 1);
    px->fill(dot_x, dot_y, 4, 4, 1);
    px->line(0, 108, PX_SCREEN_W - 1, 108, 1);
    px->text(4, 112, "ENTER: a sound    ON: back to the menu", PX_FONT_MINI, 0);
    px->show();
}

int app_main(void) {
    int runs = count_runs();
    char last_key[16] = "none";
    int x = 10, y = 80, dx = 3, dy = 2;
    uint32_t next_frame = px->millis();
    printf("Hello from the app template, run %d\n", runs);

    while (1) {
        draw(x, y, last_key, runs);

        /* Wait for the next frame -- asleep, unless a key comes first. */
        next_frame += FRAME_MS;
        int key;
        while ((key = key_before(next_frame)) != PX_KEY_NONE) {
            if (key == PX_KEY_QUIT)
                return PX_EXIT_MENU;
            if (key & PX_KEY_RELEASED)
                continue;
            snprintf(last_key, sizeof(last_key), "R%dC%d", PX_KEY_ROW(key), PX_KEY_COL(key));
            printf("key %s\n", last_key);
            draw(x, y, last_key, runs);
            if (key == PX_KEY_ENTER)
                px->play("/SOUNDS/ON.wav");
        }

        x += dx;
        y += dy;
        if (x < 0 || x > PX_SCREEN_W - 4)
            dx = -dx, x += 2 * dx;
        if (y < 73 || y > 103)
            dy = -dy, y += 2 * dy;
    }
}
