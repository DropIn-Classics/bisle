# port: Battle Isle in C

A native compatibility implementation requiring an installed copy of the
original game, on doskit's runtime (doskit/runtime). The game's data is
read from the player's copy at run time; none of it is in this
repository.

## State

The port runs the game's main program, `ISLE/BATTLE.EXE`, as C over the
program's own memory image (doskit/docs/METHOD.md, stage 3): the player's
file is loaded into a megabyte of memory where the runner loads it (PSP
0067h; its SHA-256 is checked), and each routine of the original is a C
function of the hints' name that reads and writes that memory by the
hints' names (`src/gen/names.h`, written by doskit's `symmap.py`). A part
that is not translated yet ends the program with a message that names it.

Translated so far (2026-10-01): the start, the Blue Byte logo, the title
with its running lines, the sound (the songs and the effects on the
AdLib), the menus with the code typed, the scores and their name, a
map's setup and the map's loop with the cursors on the map (a unit's
line, a unit chosen, its reach and targets, an attack order, the
overview), the screens over a window (status, a unit, a building with
its slots), a move carried out. Not yet (`todo.c`: the program ends
with the routine's name): the change of phase with the fights, the computer player, saving and
loading, the animations, what follows a map.

| File | The original's | What |
|---|---|---|
| `main.c` | - | finds the game's files, loads BATTLE.EXE, starts it |
| `bi.h` | - | the names, far pointers, what the modules share |
| `dos.c` | DOS, BIOS, PIT | files, the keyboard, the clock (timer interrupt, retrace) |
| `timer.c` | T2354 | the timers, the keys, the players' input |
| `files.c` | T2619..T2728, T164D | memory blocks, files, TPWM unpacking, paths |
| `gfx.c` | T23DC..T259F | pages, sprites, pictures, palette, text, lines |
| `lib.c` | T0CEB | the sprite libraries (`sort_lib` not yet) |
| `text.c` | T164D | text in the large letters |
| `title.c` | T1727 | the logo and the title |
| `menu.c` | T1090 | the menus, a code typed, the scores, the name for them |
| `map.c` | T0E9B | a map's files, the windows, squares drawn again, the overview |
| `cursor.c` | T0D36 | the cursors: input, what fire offers, a unit chosen |
| `units.c` | T169E | units' and buildings' records made and given up |
| `reach.c` | T0BA0 | where a unit can move and fire, the way to a square |
| `orders.c` | T0B70, T11FD | an attack order, the line below a window |
| `shop.c` | T1479, T24D8, T2701 | the screens over a window: status, a unit, a building |
| `move.c` | T122D | a move: the way marked, the steps, what arriving does, taken back |
| `phase.c` | T0408 | the score (the change of phase not yet) |
| `battle.c` | T0708 | main: the start, a map's setup, the map's loop, after a map |
| `todo.c` | - | what is not translated yet |
| `sound.c` | CODE:0215..1B79 | the AdLib's driver, the songs' player, the effects |
| `audio.c` | - | the values written to the AdLib into doskit's OPL and out |

The game's files (doskit's template): a GOG release found by itself is
copied only when the player agrees, in the kit's dialog about the game's
files (`launcher.h`), which also shows the copy's progress and says what
to do when nothing was found; `-gog` copies without asking. The dialog
itself was not started yet. Files the game writes (saved games, scores)
go to the data folder's `save`.

What the port does otherwise than a PC:

- Time is counted in the PIT's counts and moves only where the program
  waits (the retrace, a count of ticks, the scan lines of `set_palette`);
  the timer's interrupt runs at those points, not between any two
  instructions.
- A loop that does not wait for the timer (the menus) makes a pass a
  picture, 70 a second, as on a fast PC (GOG's DOSBox runs at
  `cycles=max`); in the runner, at 6 million instructions a second, the
  menus' pass takes two pictures. What follows from the time between two
  passes (how far an effect has sounded when the next begins) differs
  from a run of the runner for that.
- The sound is the AdLib's whatever the switches: the PC speaker's
  (`/s`, and what the original does without an AdLib) is not in the port.
- No joystick and no mouse (INT 33h): the original's code for them is
  translated as far as it is reached without them.
- The original's routines keep scratch values in their code segments;
  the port keeps them in C variables.
- The template's question about looking for newer releases (update.h) is
  not asked: it comes back with the setup screen (doskit/docs/LAUNCHER.md).

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/battle-isle -game game

Both scripts define `PORT_VERSION` (a string) for the compiler when
there is a version: the environment's `PORT_VERSION`, else the tag of the
commit built; the workflow sets it for a tag's build. Without one it
stays undefined.
`PORT_UPDATE_URL` likewise, from the environment only: where a release
looks for newer ones (doskit/runtime/update.h); the workflow sets it to
the latest release's `latest.json`.

`/m` is the original's switch (another text at the start; what else it
does is not known). Its `/s`, the PC speaker's sound, is not taken.

## Releases

`.github/workflows/build.yml` builds the packages doskit/docs/RELEASE.md
prescribes; a pushed tag `vX.Y` makes a release of them. `dist/README.txt`
is the players' README in each package: fill in the game's keys and
anything the game needs before the first release.

(which packages were started from a download, on which systems)

## Checked

How: the original stopped in the runner at an address and the port at
the same place (`bi_at` in the C, `BI_BREAK=NAME#N` with `BI_RAM` and
`BI_VRAM`), both with keys given by the passes of a loop, not by time
(the runner's `-keysat`, the port's `BI_KEYSAT`), then
`doskit/tools/memcmp.py` on the two memories and video memories.

- The title menu's entry (`menu`, T1090:000E) after the logo and the
  title, space at the 200th pass of the title's loop (T1727:031E;
  Windows 11, MSVC, the headless build): all 256 KB of video memory the
  same (both pages, the lists of what the sprites covered); of the
  program's memory all segments the same but
  - the routines' scratch variables in their code segments (T23FD,
    T2433, T2470, T2550, T265E, T2695, T26D2), the saved interrupt
    vectors (T2354:0008, DATA:0DE2), the stack;
  - the C library's own variables (DATA:005C..0090, DATA:1432..1A3E) and
    its startup code's (CODE:01A6..0207, CODE:38F9..3F47);
  - the sound: its two timers and their tables (DATA:0CBC..0DAC), its
    variables after the stack (ZEROS), CODE:14F0, CODE:164D;
  - what goes with the time: `tick_count`, `input_divider`, the timers'
    counts left and the counts of the players' input.
  The memory the program got from DOS (behind the program, 30E7:0 on:
  the font, the title's files) is the same in every byte but one, the
  size of the free block after it (the runner's memory ends at 9FC0h,
  the port's at A000h); memcmp.py does not look there, a scratch script
  did.
- The pictures of the logo and the title were looked at (the headless
  build's `DK_SHOTS`), not compared pixel for pixel on their way.
- The sound: every value the original writes to the AdLib in the first
  30 s (the start and the title's music; the runner's `-log` on
  opl_write's two outputs) against the port's (`BI_OPLLOG`): the 1889
  writes the same in order. With the sound translated the comparison at
  the menu's entry above no longer has the sound's differences: its
  variables behind the stack and in its code are the original's but
  `song_wait`, which goes with the time. A song's end and beginning
  again, a pitch bend and the percussion mode's drums were not looked
  for in these 30 s.
- The menus: the map's setup (T0708:0392) after space at the title's
  200th pass and enter (START) at the title menu's 30th pass
  (T1090:058B): all of the video memory the same, the memory the same
  but for the kinds of differences above and the effect still sounding
  (`fx_ticks`, `fx_count`, `fx_freq` of the fourth channel: the time
  between the passes, above). Not compared: the other menus, a code
  typed, LOAD, RATING, the name for the scores (their routines are
  translated; tools/screens.py drew those screens from runs of the
  original before).
- A map: ISLE's first map (START at the title menu), the original
  stopped at the loop's start (T0708:135C) and the port at the same
  pass: at the first pass, after the whole setup (the libraries sorted
  and the ground's parts stored, the map's files, the units and
  buildings made, both windows drawn), and at the 60th pass with the
  first player's cursor moved meanwhile (right held at the passes 20 to
  23, down 30 to 33, up 40 to 42: keys by passes): all of the video
  memory the same, all of the program's memory the same but for the
  kinds of differences above (at the 60th pass of what goes with the time
  only `input_divider`), the memory behind the program the same. Not
  compared: fire and what it chooses (a unit's reach, an order, the
  overview; cursor.c and reach.c are translated, tools/moves.py did them
  again from runs of the original before), player 1's keys.
- The screens over a window, the keys by the map loop's passes, the
  original and the port stopped at the same pass with the screen open:
  the status screen (ISLE's first map, fire and down on the empty start
  square), the unit's screen (one up, fire and down on the T-3
  SCORPION), and on map 03 (the code MARSS typed in the menus, which
  compares the code menu as well: the first pass of the map was the
  same) the headquarters' screen (one up, fire and left: seven empty
  slots) and the depot's (five down, one right, fire and left: a unit in
  the first slot, its big picture unpacked and its numbers). In all four
  the whole video memory and the program's memory were the same but for
  the kinds of differences above; `tools/screens.py` found each screen
  in the port's video memory. Not compared: the list of types in a
  factory (`list_makeable`, `draw_type_list`: the attack phase is not
  reached yet), a unit of two squares, another player's unit, player 1's
  window.
- A move, the keys by the map loop's passes, stopped at the same pass
  after the aim, during the steps and after the end: on the first map a
  T-3 SCORPION chosen (fire and up) and moved a square (fire on the
  square, fire again); on map 03 the unit beside the headquarters moved
  into it (move_record's kind 5), the unit taken out of the depot (fire
  and up in its screen) and moved two squares up (the cursor is back in
  the depot's screen afterwards), an aim on a square out of reach
  refused, and the unit given back into the depot after it (fire twice
  quickly). In each the whole video memory and the program's memory were
  the same but for the kinds of differences above. Not compared: a unit
  taken aboard or going aboard another (kinds 1 and 2), a building of
  the other side stood on (kind 4), a unit of two squares, `move_undo`
  (the change of phase calls it), player 1's moves.
- The window build was started for 8 s and ran (nothing looked at or
  heard: nobody was there).

Not built: macOS, Linux.
