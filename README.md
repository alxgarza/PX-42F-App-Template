# PX-42F App Template

Start here to write a native app or game for the **PX-42F** calculator (the
RP2350 board running PX-42F-Plus). An app is a `.bin` file you copy onto the
calculator's drive. It runs from **Settings → Games & Apps** at full speed,
with the screen, keyboard, sound, clock, files, USB serial and infrared LED
at its disposal.

**[`framework_manual.html`](framework_manual.html) is the full manual**: every function with
examples, the keyboard map, the fonts' characters, recipes, the simulator and
debugging. Open it in a browser.

This template builds a small demo, `HELLO.bin`. It shows the last key you
pressed, the clock and the battery, and keeps a run counter in a file. Copy the
folder and replace `src/app.c` with your own app.

## What you need

- The Raspberry Pi Pico SDK's ARM toolchain, as the VS Code Pico extension
  installs it: `~/.pico-sdk/toolchain/15_2_Rel1`. Another location works with
  `make PICO_TOOLCHAIN=/path/to/toolchain`.
- `make`, `python3` and a C compiler for the computer (`cc`), for the simulator.
- VS Code with the C/C++ extension (optional: everything is `make`).

## Start a new app

1. Copy this folder, for example to `PX-42F-Tetris`.
2. In the `Makefile`, set `NAME` (the file name and the name in the menu: up to 28
   letters, digits, `_` or `-`) and `KIND` (`GAMES` or `APPS`: the folder it goes in).
3. Replace `src/app.c` with your code. You can add more `.c` and `.h` files in `src/`.
4. Build it, try it in the simulator, then install it (below).

## Build, try, install

| VS Code task (Terminal → Run Task) | Command | What it does |
|---|---|---|
| Build (⇧⌘B) | `make` | Builds `build/NAME.bin` and prints its size and the RAM left. |
| Simulate | `make sim` | Runs the app on your computer with the `SIM_SCRIPT` from the `Makefile`. Screenshots are saved in `build/sim-out`. |
| Install on the PX-42F | `make install` | Copies the `.bin` onto the calculator's drive. |

**To install by hand.** On the calculator, open Settings → USB Drive → Connect
to Computer. Copy `build/NAME.bin` into `BIN/GAMES` or `BIN/APPS` on the
`PX-42F` drive, then eject it. The app then appears under
Settings → Games & Apps.

## The simulator

`make sim` compiles your app for your computer, against a stand-in for the
calculator, and runs it through a script of keys and pauses. Edit `SIM_SCRIPT`
in the `Makefile`. These are the words it understands:

| Word | Meaning |
|---|---|
| `R0C0` … `R6C5` | Press the key at that place (row, column). |
| `SIGMA` `XEQ` `ENTER` `BACKSPACE` `UP` `DOWN` `SHIFT` `RS` | Press that key by name. |
| `250` | Let 250 ms pass. |
| `shot:NAME` | Save the screen as `build/sim-out/NAME.png`. |
| `serial:TEXT` | TEXT arrives on the USB console, then Enter; `_` is a space. Quote a word with `( ) " ;` in it: `'serial:PRINT_SQR(2)'`. |
| `ON` | Press ON: the app ends (`PX_KEY_QUIT`), or after `px->on_key(1)` gets `PX_KEY_ON`. |
| `QUIT` | End the app, as the Sleep Timer does. The end of the script also does. |

Time in the simulator is virtual: a 10-second pause takes no time to run.
Sounds, serial output and IR transmissions are printed instead of played.

**The simulated drive.** `sim/drive` stands for the calculator's drive. Put the
files your app reads there, at the path they'd have on the calculator.
`make sim` works on a copy of it, `build/sim-drive`, so whatever the app
writes stays out of your project.

## Writing an app

Include `px.h`. The firmware's functions are in the table `px`, and your
entry point is `app_main()`:

```c
#include "px.h"

int app_main(void) {
    px->clear();
    px->text(4, 4, "Hello!", PX_FONT_LARGE, 0);
    px->show();
    while (px->key(-1) != PX_KEY_QUIT)
        ;
    return PX_EXIT_MENU;
}
```

`px42f/px_app.h` documents every function. In short:

| Area | Functions |
|---|---|
| Screen (250 × 122, 1 bit) | `clear` `pixel` `get_pixel` `fill` `line` `bitmap` `text` `text_width` `font_height` `show`, plus `screen`, the framebuffer itself |
| Fonts | `PX_FONT_SMALL` (8×12), `PX_FONT_LARGE` (10×16), `PX_FONT_MINI` (6×8), and `PX_FONT_CHOSEN_*` for the fonts chosen in Settings → Fonts. Characters are the HP-42S's: `"\x05"` is Σ, `"\x13"` is °. |
| Keys | `key(wait_ms)`: the next key going down (`PX_KEY(row, col)`) or up (`\| PX_KEY_RELEASED`), `PX_KEY_NONE` when the wait runs out, `PX_KEY_QUIT` when the app must end |
| Time | `millis` `sleep` `datetime` `random` |
| Sound | `tone(0-10)`, `play("/path/file.wav")` wait until the sound has played; `tone_async`, `play_async` don't (the second processor plays it); `sound_playing`, `sound_stop` |
| Files on the USB drive | `load` `save` (a whole file), `open` `read` `write` `close`, `remove` `mkdir` `list` |
| Serial | `serial_write` `serial_read`. `printf()` also goes to the USB console. |
| Infrared | `ir_send(burst)` |
| Power | `battery_mv` `usb_connected`; `on_key` (version 3): ON as a key, for an app that asks before leaving |
| Decimal numbers (version 2) | `dec` (arithmetic, powers, logarithms, trigonometry, Gamma…), `dec_from_int` `dec_to_int` `dec_from_string` `dec_to_string` `dec_cmp` `dec_class`: Plus42's own 34-digit decimals |

**Keys are named by where they sit.** Row 0 is the top row, column 0 is the
left (`PX_KEY(0, 5)` is XEQ). The top two rows have 6 keys, the others 5.
ENTER is `PX_KEY(2, 0)`. Names for the common ones are in `px_app.h`.

**Paths.** A path that starts with `/` is from the top of the drive
(`/SOUNDS/ON.wav`). Any other path is in the app's own folder: for
`/BIN/GAMES/SNAKE.bin` that's `/BIN/GAMES/SNAKE/`, made the first time the app
saves something there.

## Rules

- **Wait in `px->key()` or `px->sleep()`.** A game waits for its next frame
  there, never in a busy loop. The calculator sleeps while waiting, which keeps
  the battery current low. If 100 ms pass without a call into `px`, the
  watchdog restarts the calculator.
- **Return when `key()` says `PX_KEY_QUIT`.** It does when ON is pressed, the
  Sleep Timer runs out or the battery is empty. An app with its own way out
  can take ON as an ordinary key instead (`px->on_key(1)`, API version 3).
  Return `PX_EXIT_OFF` to turn the calculator off, or `PX_EXIT_MENU` to go
  back to the menu.
- **RAM.** Code, data and variables share 128 KB; `malloc()` gets what's left.
  `make` prints how much that is. The stack is the firmware's (32 KB, part of
  it already in use): keep big arrays `static`, not local.
- **Flash.** Saving writes the calculator's flash: save a high score, not
  every frame.
- **In a game, use `tone_async()` and `play_async()`.** `tone()` and `play()`
  wait until the sound has played, and the game with them.
- **Crashes are safe.** Before the app starts, the calculator saves its memory
  to the drive. If the app crashes or hangs, the calculator restarts with its
  memory intact and shows where the crash was.

## The files

| Path | What it is |
|---|---|
| `src/app.c` | The app. Replace it. |
| `px42f/px_app.h` | The contract with the firmware: every function, every constant. Copied from the firmware (PX-42F-Plus); don't edit it. |
| `px42f/px.h` | What your code includes. |
| `px42f/runtime.c` | The `.bin`'s header and entry, `malloc()`'s memory, and `printf()` to the console. |
| `px42f/app.ld` | Places the app at 0x20060000, where the calculator loads it. |
| `px42f/pxbin.py` | Turns the linked app into the `.bin`: size and checksum. |
| `sim/` | The simulator: `sim.c`, the calculator's fonts and PNG writer, and `drive/`. |

## Versions

`px_app.h` carries `PX_APP_API_VERSION`. A newer firmware only adds to the
table, so an app keeps working on later firmware. A calculator whose firmware
is older than the app says "Needs a newer firmware". To use functions a newer
firmware adds, copy its `px_app.h` into `px42f/`.
