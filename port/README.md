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
hints' names (`src/gen/names.h`, written by doskit's `symmap.py`).

Translated so far (2026-10-01): the start, the Blue Byte logo, the title
with its running lines, the sound (the songs and the effects on the
AdLib), the menus with the code typed, the scores and their name, a
map's setup and the map's loop with the cursors on the map (a unit's
line, a unit chosen, its reach and targets, an attack order, the
overview), the screens over a window (status, a unit, a building with
its slots), a move carried out, the change of phase with the fights and
their scene, the films, a game saved and loaded, a map's end with the
statistics, the last map's ending and the credits, the computer
player. That is all of BATTLE.EXE the game calls, the rare paths too
(2026-10-03: the question for a disk when a file does not open,
T2695:02BB; a picture larger than 360 by 240, T2550:02E5; restore_sprites'
block from the page at A400h, T2467:007B; these three translated from
BATTLE.ASM and not run, since none of the game's files and no code of
BATTLE.EXE leads there with the game installed whole; the modes of 360
pixels left out, the program never asks for them). Not in the port: the game's other
programs (the starter BI.EXE, INTVGA's intro of 16 colours (not wanted),
MOON.EXE of the second scenario disk), the PC speaker's sound (not
wanted), the joystick (the port answers as a PC without one). The
window's mouse is connected when the setup screen's "Mouse" is on
(2026-10-06; below), not tried in a window yet.
The intro of 256 colours, INTEGA/INTRO.EXE, is in (below).

The first scenario disk (2026-10-02): its game program
`DESERT/DESERT.EX2` is BATTLE.EXE's code and data but for three bytes
(the key that answers QUIT THE GAME in both key sets, and its letter in
the message), so the port plays it as it is: `main.c` loads that file
instead (its size and SHA-256 checked) and gives the program DESERT's
folder. Its starter `DESERT.EXE` (read from its code, 1750 bytes) makes
a folder on C: and, while the game runs, sends the saved games `NN.DAT`
and the scores `MAP\NN.HI` there; the port has the data folder's
`save-desert` for what DESERT writes instead. The title is chosen on
the setup screen (the item "Title", offered for each title whose folder
is in the game's files) or by `-title isle|desert`.

The second scenario disk (2026-10-02, begun): `MOON/MOON.EXE` is chosen
as the others are (`-title moon`, the setup screen's "Game"), `main.c`
loads it with its own PSP (0066h: the image at 0076h, a paragraph before
BATTLE.EXE's, which is why `LOAD_SEG` is `rm_psp + 10h` now) and the
names' column of `gen/names.h` for MOON (`bi_program(BI_MOON)`). Its
title runs on the shared C (the logo, the title, the menu's drawing) and
is the original's at the menu's entry (`menu_pass`, the 50th pass; the
runner at T1151:058E, the title's loop at T1800:0331, space at its 200th
pass; `pcmp.py` with `OEXE=MOON/MOON.EXE HINTS=src/MOON.hints
TITLE=moon`): all of video memory the same, of the program's memory the
timers' and keys' counts, the C library's variables (`DATA:004C..007F`,
`DATA:1216..14FE`) and the routines' scratch variables in the library's
code segments, as BATTLE.EXE's. MOON has no `LIB\*.DAT`: its `load_lib`
(T0CEB's counterpart) is called with the flag 0 everywhere, so `lib.c`
does not sort there (read, then a headless run: the libraries load).
MOON's `map_setup` follows MOON.EXE's setup where it differs (the handoff
lists how: no `.PMP`, only the ground parts the map has stored behind the
pages, the files of its own overview read, the song and effects last).
START shows the first map, the original's at its 200th pass (`pcmp.py`
with MOON's names): the computer's tables and the far data segments the
same, video memory the same (MOON's loop draws the two cursors with the
colour base 40h, which map_loop does for MOON), the code segments the
same but for the library's scratch bytes, DATA below 2000h 149 bytes, each
looked at (the handoff lists them): the C library's startup (saved
vectors, argv, exit hooks, stream records at DATA:1226..150E, which
nothing of the port reads: it does not run that startup), the mouse the
runner has, and the clock's phase. MOON's image is loaded at 0076h, so
`pcmp.py` takes `-- --load 76` for it (memcmp.py's default 0077h is
BATTLE's). MOON's overview (fire with right on an empty square) is
drawn as MOON.EXE does it (`moon_draw_overview` in map.c, from MAPINFO.DAT
and MAP02/04.DAT, scale 2 or 1 by the map's size, a frame, the dots; the
frame's start and steps follow the scale in `loop_overview`): opened,
the frame moved down and right, and closed by fire, the port is the
original's in video memory and all of memory at passes 700 and 730 of the
first map (scale 2) and, for scale 1 (a width above 20h or a height above
28h: map 13 only, started by a poke of map_number), both pages at pass 110
of map 13 (the rest of that map differs: handoff); keys given by pass,
`map_pass@LT070B_13DD=N:KEY+,..`: by seconds they were a pass off. MOON's fights run now (the fight scene with MOON's own places for its
values and scripts, `unit_script` and `place_units` as MOON.EXE has them, the
overview's files read again after a scene as MOON.EXE does, in phase.c): a
game of two computers on map 0 is the original's in all of memory at passes
957, 958, 1000, 1300, 2000, 2250, 2375, 2440, 2470, 2500, 3000 and 6000
(MOON's computer step 0Ah sub 3 has one check more: `much_weaker`); video memory (all 256 KB) is the
original's at passes 958, 2000 and 3000 too (`map_loop` reads the cursor library's table at each draw, as
the original, since the change of phase loads other files over the buffer; the scene's ground pieces are
drawn in colour base 60h for MOON, 5Fh for BATTLE; not run for 6000, nor for fights of other units).
The scene's pieces 3 and 4 where two grounds meet: BATTLE puts them at ground kinds 2 and 5, MOON
(T2250:037A) at kind 5 only (fight.c `edged`); MOON's map 20, two computers, pass 5654 (a fight scene; its
grounds' kinds not looked at): the lists of covered areas differed by 291 bytes, now all video memory the same; maps 5, 7 and 9, whose lists differed at their ends, now the same there.
MOON's other maps, two computers each (scratch mcvall.sh, 2026-10-03), compared at pass 1000 (far data
segments and video memory): three more differences of MOON's computer found and done: computer_assess
step 1 (T18B8:0FF9) leaves out the three +19h for a type with 20h in its +10h and adds the values of
the units one holds (computer.c); computer_plan step 1 sub 1 leaves out units by 0C040h, not 0C048h,
and step 0Bh subs 1 and 2 do not leave out a unit with 20h in its +6, step 1 sub 3's 46h for a
unit whose +1 is above 0, not above 1 (plan.c); cost_map makes a building's square (ground 540h)
no way for a unit of two squares (reach.c, T0C0E:0BBB); a type's big picture is placed by MOON's
two tables of signed bytes, `bigunit_dx` and `bigunit_dy` (shop.c, T154F:0C9E), not by the
type's +18h. Found by the first pass that differs (scratch mbis.sh) and the routines' code
compared with BATTLE's (scratch rdiffs.py). With them all 34 maps (0 to 33) are the
original's at pass 1000 in the far data segments and the video memory (one run each with the same
build). At pass 3000: the units in a building get the task 5 (out) unless their +6 has 4 (BATTLE:
24h; computer.c `tasks_out`, MOON's T1C22:04D8); with it all 34 maps the same at pass 3000 (28 of
them run with the build before this change, which differs from it only there). At pass 6000 (2026-10-04): a holder's cargo
with 4 in its +6 becomes 6 energy a point in MOON, 8 in BATTLE (phase.c, T03EB:1290; map 6, pass
5963); then all 34 maps the same at pass 6000 (map 6 run again with the change, the others with the build
before it) in the far data segments, and in the video memory
but for map 20: from its pass 5654 on, 291 bytes of the lists of what sprites covered (A7E8h on)
differ, the two pages and all memory the same; not found yet.
A game of two computers on map 0 played to its end (pass 8826, round 16, the headquarters taken;
scratch mend.sh): at the statistics' wait (key_wait's 100th pass) memory and video memory the same,
the film before it included.
Maps 1 to 23 so too (2026-10-04): the same at their ends (passes 14988 to 57414) in memory and
both pages; later maps 19 and 32 too, and maps 5, 7, 9 also in the video memory's lists after the
change to the fight scene's edge pieces: all 25 maps a computer can play the same at their ends.
Maps 24 to 31 and 33, which only two people can play (2026-10-04, scratch mpvp.sh): both cursors'
state poked to 5 and F1 given ten passes later, every 60 map passes from 30, 15 times; at map pass
1000 (round 7) memory and all video memory the same on all nine, but the music's place. With both
people's cursors driven between the changes by a fixed pseudo-random walk of keys (25 changes, 120
passes apart): at pass 3000 (round 12) the same on all nine (the input counters' clock phase aside);
units moved, but whether a fight happened and the maps' ends are not checked.
A game saved and loaded in MOON (2026-10-04, scratch msave.sh and mload.sh): MOON's header is 14h
bytes (3, 7, the map's number, a byte not written, then BATTLE's fields; save.c), and the message
box before load_game takes its text from the setup's data (`text_insert_save`, F280C:000Eh; the
port showed bytes of BATTLE.EXE's place before). Saved from map 0 of two computers, both cursors'
state poked to 5 and player 0 made a person at pass 30 (with two computers MOON changes the phase
at once and never asks for D), D, a key, the digit 0: 00.DAT the original's byte for byte (37817
bytes), memory and video memory the same at pass 100. Loaded from the menus (keys by time: 14
space, 33 down, 34 down, 35 enter, 37 enter, 39 0, 42 enter, 55 n at INSERT SAVE DISK; space
there is player 0's fire and reached the map by time): the same at passes 1, 10 and 300 but for
the video memory's lists (28930 bytes, the same number of non-zero bytes in another order; not
looked into; neither page shows them). Found later by segdiff.py (T13CA against T148F): before
the map is set up MOON shows the box once more, reads the save's header (T148F:0B21, `moon_save_map`)
and takes the map's number from it, and sets game_flags 400h by CODES.DAT's entry of that map as the
menu's code does (battle.c). mload.sh again: the same at pass 10 as before. A save loaded while the
menu holds another map was not run. MOON's films at a map's end are its own routines (T070B:4703 `moon_win_film`, HQ.ANI or TOT.ANI, and
T070B:49ED `moon_end_film`, END.ANI and five lines of text, in battle.c where BATTLE.EXE plays play_anim
and end_credits). Checked with the map's end poked in (game_flags, F2902:4155, linear 2D8D5, 04CFh at
pass 30 of map 0, two computers, made 04DDh, 24DDh for the other film, 04FDh for the last map): the
1st, 40th and 100th frame of HQ.ANI, the 1st, 60th and 120th of TOT.ANI, the 1st, 40th and 76th of
END.ANI, the text before its wait of 2000 (T070B:4D1D) and the menu's next entry (T1151:0003, 2nd):
video memory and the far data segments the same as the original's, DATA but for the kinds above and
the C library's. The sound was not compared, a real win (not poked) not run. MOON's statistics
after a map (T1682:000B) are BATTLE's but for the song, started after the fade-in (after.c);
compared with the end poked at pass 5000 of map 0 (two computers, round 6; scratch mstat.sh), at
the wait's 100th pass (T0DA1:0D0F): won (DDh: rating 542, the next code LUNAR) and lost (CDh),
the video memory and the far data segments the same, the rest but for the kinds above. The
sound not compared. MOON.hints' carried names are
unchecked beyond what the setup uses (see the handoff). The names MOON.hints lacked for
the port are its own lines now (`name` lines above the carried block:
make_path's tables, the menus' texts, the statistics' and the logo's
names, the sound's variables at DATA:1514 + the offset of BATTLE.EXE's
ZEROS, found by the 173 matched instructions of the library that address
them, all 5BCh lower; the rest by the bytes their place holds in
BATTLE.EXE), 195 others stay `(not mapped)` and are 0xFFFF in the
column: a write through one lands far outside the program, so a run that
needs one shows garbage first (`build/scratch/moonnames.py` and
`moonvotes.py` list the candidates by bytes and votes).

The intro (2026-10-02): `ISLE/INTEGA/INTRO.EXE`, the intro of 256
colours that GOG's start runs before the game (BI.EXE's branch for the
EGA; the folder names are the wrong way round, docs/HANDOFF.md), runs
before the game in a window unless "Skip logo, intro, and title" is on
(headless only with BI_INTRO=1, or BI_INTRO=only for the intro alone, so
that the comparisons of the game are as they were). Esc ends it. It
is loaded as the game is (kept paragraphs 0AC0h, what its startup leaves:
the first block DOS gives is then where the runner's is), `intro_main`
(T03FA:122D) runs the show over the memory image with the game's timer,
sound and file modules. Checked against the original in the runner, the
video memory of both pages, the program's own data (F0728, F07D4, DATA
but for the library's startup values and the timer's counts) and the
blocks of memory (a block's header of 16 bytes keeps what its name field
held in the port, zeros in the runner: no matter): at the 20th, 150th,
1200th and last (2460th, at 200 s) call of its wait (T03FA:0288, `bi_at
"intro_wait"`); 0 bytes of video memory differ at each, the data as
said. Not checked: the window build looked at and listened to by a person
(the song and its fade, the effects), the OPL's writes against the
original's (oplcmp.py is for the game), `/s` (the PC speaker: not
wanted), the Ctrl-Break handler and the texts before the mode is set
(left out).

The intro showed a black screen in the window (the user, 2026-10-02).
Cause, found with a headless run that goes through the setup screen
(`DK_KEYS="20:1C 22:9C" BI_INTRO=1`, pictures 8 by 1 pixels, no timer
interrupt for thousands of pictures): the intro waits 50 ticks
(`wait_ticks(0x32)`) before it sets its mode, and until then the VGA's
registers were zero, its refresh rate absurd, so the clock presented
thousands of pictures for one timer tick. `intro_main` sets text mode 3
first, as DOS has it when a program starts. After the change the first
wait ends at once (picture 400 of a headless run is the first text
card). Checked headless only (the shots above, one run through the setup
screen); not looked at in the window, not compared with the runner again.

The port runs more than one program now (2026-10-02, begun for the
intro INTEGA/INTRO.EXE, later MOON.EXE): `gen/names.h` holds a column a
program (`symmap.py ... BATTLE=src/BATTLE.hints INTEGA=src/INTEGA.hints`),
`prog.c` turns the columns into the variables A_name and S_name for the
program chosen with `bi_program`, and the shared modules (the sound, the
files, the timer) read the memory through them. Checked after the change:
map 03 at its 100th pass (the scratch m03.sh) is as before in the video
memory (0 bytes differ) and the heap (the same one byte); the other
comparisons were not run again. The intro itself is not in yet.

| File | The original's | What |
|---|---|---|
| `main.c` | - | finds the game's files, the setup screen's page, loads BATTLE.EXE or DESERT.EX2, starts it |
| `bi.h` | - | the names, far pointers, what the modules share |
| `prog.c` | - | the names' addresses in the program loaded (the game or the intro) |
| `intro.c` | INTEGA/INTRO.EXE: T0529, T03FA | the intro of 256 colours: the screen (mode 13h unchained) and the show |
| `dos.c` | DOS, BIOS, PIT | files, the keyboard, the clock (timer interrupt, retrace) |
| `timer.c` | T2354 | the timers, the keys, the players' input |
| `mouse.c` | T2683, T263D, T267C, T268A, T2354:0397 | the mouse as a player's directions and fire; its driver (the port's) |
| `files.c` | T2619..T2728, T164D | memory blocks, files, TPWM unpacking, paths |
| `gfx.c` | T23DC..T259F | pages, sprites, pictures, palette, text, lines |
| `lib.c` | T0CEB | the sprite libraries |
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
| `phase.c` | T0408 | the change of phase, the score |
| `fight.c` | T1F3C..T223C | the fight: its reckoning, the scene with the units and the shots |
| `anim.c` | T2248 | the films of ANIM\ with their sounds (the frames' decoder is gfx.c's) |
| `save.c` | T13CA | a game saved and loaded |
| `credits.c` | T25A6 | the text typed after the last map's end |
| `after.c` | T15AC | the numbers of units kept at each change, the statistics after a map |
| `battle.c` | T0708 | main: the start, a map's setup, the map's loop, after a map |
| `ai.h`, `computer.c` | T178C, T17C0, T1ABC, T1B01 | the computer player: its step a pass, the two sides assessed, the tasks handed out |
| `plan.c` | T1C04, T1ED2 | the computer player's plan: a task for every unit |
| `command.c` | T1938 | the computer player's commands carried out as keys |
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
- No joystick: the original's code for it is translated as far as it is
  reached without one. The mouse's routines are translated in full
  (mouse.c) over a driver of the port's own in place of INT 33h, which
  answers only when the setup screen's "Mouse" is on (off by default)
  or `BI_MOUSE=1`, `BI_MOUSEAT` or `BI_MOUSEMOVE` is set (scripted
  runs); otherwise the port is a PC without a mouse as before. With
  "Mouse" on (or `BI_MOUSE=window`) the driver takes the window's mouse
  from the kit's `plat_mouse_motion`: the buttons held, and the
  movement added to its place by its rates (INT 33h's counts for 8
  pixels, which the game sets by its menu's speed); the game sets the
  place back to its middle as in the original. While the game has the
  mouse it is kept to the window (`plat_mouse_grab`). One count of the
  platform is taken as one of the driver's: a choice. The driver's
  doubling threshold is not applied.
- The original's routines keep scratch values in their code segments;
  the port keeps them in C variables.
- Before the game the kit's setup screen is shown (doskit/docs/LAUNCHER.md:
  `main.c` gives `launcher_run` a menu and two pages of items and draws
  nothing; the menu: start, the choices Game (the title: Battle Isle or a
  data disk whose folder is there) and Full screen, then the pages
  Quality of Life changes (the two choices below) and This port (whether to look for newer releases, update.h)). The settings are kept in the data folder's
  `battle-isle.cfg`. Looking for newer releases is not asked about at the
  first start: it is off until the player switches it on there (a
  question of its own would be a dialog the kit does not have). A newer
  release known when the screen opens gets a line that opens its page.
- The player's settings (2026-10-03; doskit/docs/PLAYER-SETTINGS.md),
  pages Sound, Keys and Controller of the setup screen, applied only after
  it (so never in the headless comparisons): the volume 0..10 and in play
  the keypad's + and - and * (mute) with doskit's hud box; the headphone
  mix (audiofx.h, after the OPL's mono is made stereo in audio.c); the
  players' keys for up, down, left, right and fire, mapped onto the
  game's (player 1 keypad 8 2 4 6 and Space, player 2 D C X V and the
  left Ctrl: the tables at DATA:0AE9 and 0AEB point to, read from
  BATTLE.EXE's data), the game's other keys staying; a controller's
  buttons each one of the players' actions or Esc, Enter, F1 ("change
  mode": the original takes it from the keyboard only), Y, N, D. Player 1's fire on a button sends Enter with it: the
  menus (T1090) confirm with Enter only, not with the players' fire.
  While a controller is in use (pad.h's pad_in_use: the last press a
  button's) the question QUIT THE GAME (messages 0Ch and 0Dh) names the
  buttons that send Y and fire, as the port's texts drawn as the game
  draws its messages (show_text in orders.c, the text at 0050:0100 below
  the program); with the keyboard, and headless, the original's.
  The same for F1 : CHANGE MODE (message 20h, with 0Dh) and the boxes
  with PRESS ANY KEY (INSERT SAVE DISK at a saved game's load, the
  question for a disk): there the button that sends Enter is named
  (pad_box_text, the text copied to 0050:0140). Messages 0..4Fh were
  looked through for keys named: only 0Ch, 0Dh and 20h. Checked:
  the build, map 03 at its 100th pass headless as before (video memory 0
  bytes, the heap the one byte). Not checked: the window's sound, the
  keys and a controller in play.
  The players' keys cannot be chosen yet.

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

`/m` is the original's switch: it prints "Monochrome." in place of
"Color." at the start and sets F2740:02C8 to 2, which is passed to the
file name routine (T26EA:000F); the pictures are grey with it (the user
saw). It is the third of the game's own menu's palettes ("Palette 1",
"Palette 2", "Mono", as the user read there), so the setup screen has no
item for it (it had one, first guessed wrong as "other palette"); the
command line still takes it. Not compared with the original's own /m run. Its `/s`, the PC speaker's sound, is not taken.

## Releases

`.github/workflows/build.yml` builds the packages doskit/docs/RELEASE.md
prescribes; a pushed tag `vX.Y` makes a release of them. `dist/README.txt`
is the players' README in each package; its keys (2026-10-06) are the
ones the code reads (BATTLE.hints: the key set, the players' tables,
the cursor's functions, F7/F8, the quit key), not the manual's.

Linux, not from a download (2026-10-06, Debian 13, no display): the
package made by hand as the workflow's Pack step does, from SDL2
2.32.10's release source (configure, not cmake; `$ORIGIN` set at the
link, there was no patchelf), unpacked into a fresh folder: `ldd` finds
the SDL2 beside the program, RUNPATH `$ORIGIN` only, the newest glibc
symbol GLIBC_2.38 (this machine's; the workflow builds on Ubuntu
22.04). Started with SDL's offscreen video and an empty data folder it
ran 15 s and made its data folder; nothing more could be seen without a
window. The headless build of the same source, the same empty data
folder and the GOG installer in ~/Downloads: "The game's files" names
the installer, Enter copies, then the title, the menu and the first
map (pictures 90, 4400 and 5900). Not done yet: a package of the
workflow downloaded with a browser and started with a double click on
a desktop, Windows, macOS, the update check. The setup's "Look for new
versions" opens the release page on every platform (main.c:
update_open); RELEASE.md's point 7 asks for update_install on Windows
and Linux, which the port does not call.

- macOS (2026-10-06, macOS 15.7.3, x86_64): a package built by hand as
  the workflow's macos job builds it (SDL2 by its configure, no cmake
  on that machine), not one from a release page; unpacked with Archive
  Utility under a quarantine set by hand, Gatekeeper's verdict
  "rejected" as README.txt says, then started after `xattr -cr` with
  the game's folder named by -gog and an empty data folder: the app
  runs, copies the game's files and writes its settings there. NOT
  seen: the window (setup screen, menu, map), a double click, "Open
  Anyway", arm64. docs/HANDOFF.md, "The macOS package", has it in full.
- Done since on branch dropin-team/rc1-readme-linux: dist/README.txt's
  Keys are the game's keys as the code reads them; doskit/docs/RELEASE.md's
  point 6 is met.
`dist/uninstall.sh` and `dist/uninstall.cmd` go into the packages as
they are (the Mac package has the first as `uninstall.command`): they
remove what the port copied and wrote, after asking
(doskit/docs/RELEASE.md, point 8). Tried on macOS with a made-up data
folder only; the .cmd not at all here.

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
- The change of phase on the first map (both players ask with fire and
  left, then F1; keys by the map loop's passes), with no order given and
  after a move: at a pass after it the whole video memory, the program's
  memory but for the kinds of differences above, and the memory behind
  the program (the history's buffers) were the same. Not compared: a
  change with orders (the fight scene is not translated), a building
  taken (the film is not translated), a unit taken aboard, a road or a
  depot built, the map's end by it, the statistics after a map
  (`after_map` is translated and not run yet).
- A fight, on the first map: a unit of player 0 put beside one of player
  1 by pokes into both programs (the unit's record and the unit's byte
  of both maps; `BI_POKE`, the runner's `-poke`), player 1's attack
  order by keys, the change of phase. Stopped at the 3rd, 20th, 45th and
  60th call of fight_step and at a pass of the map's loop afterwards:
  the whole video memory and the program's memory (the scene's values,
  the units, the seed of rand) the same but for the kinds of differences
  above; the same with the target's count poked to 1, which the fight
  destroys (the explosion, the unit removed). Not compared: a fight over
  a distance, a unit in the air (its shadow), a unit of two squares, a
  unit that holds others, a target that cannot answer, the other
  grounds' pieces and scripts (tools/scene.py did those from runs of the
  original before).
- A game saved (both players ask for the change, D, a key, the digit 0;
  the keys of the two waits by the passes of their loops): the file
  `00.DAT` the port wrote is the original's byte for byte (37815 bytes),
  the memory and the video memory after it the same. Loaded again from
  the menus (DISK, LOAD, 0, a key at PLEASE INSERT DISK, a key at the
  message box): at the map's 10th pass the same.
  The same with the digit 3 (2026-10-06, keys by passes: title_pass 200
  space; menu_pass 30 enter; map_pass 50, 60 left, 70 space and 75 left,
  100, 110 x, 120 lctrl and 125 x, 150 d; key_wait 200 p; save_key 100
  3, never let go): both wrote `03.DAT`, byte for byte the same, once
  the port's data folder had the runner's `MAP\00.HI` (without it the
  file differs in score_best, F27EE:2593, which the menu reads from
  there); at map_pass 400 the video memory the same and the memory the
  same but for the kinds of differences above (DATA below 2000h, the
  timers' and interrupts' code segments, the stack). Loaded (menu_pass
  30, 40 down, 50, 60 enter; ask_key 100 3, let go at 400; key_wait 200
  p; box_key 100 p): number_asked 3 at load_game's entry in the
  original; at map_pass 10 the video memory the same and the memory as
  before. The port with only `03.DAT` in its folder gave the same
  memory; with no save it did not reach map_pass 10 in 300 s (not
  looked into what it waits at). save.c was not changed: it already
  took 1..9 and 0 from the scancode. The other digits in the next entry.
- The save's digit names the file, 0 and 9 in ISLE and 5 in DESERT
  (2026-10-06, keys by passes as in the 03.DAT entry of branch
  dropin-team/port-save-names; this branch
  dropin-team/save-digits-rest): save_key 100 the digit, never let go;
  LOAD's ask_key 100 the digit, let go at 400; each run with only that
  one save in the runner's and the port's folders. The original wrote
  00.DAT, 09.DAT (F27EE:249C "00", "09") and DESERT's 05.DAT, the port
  the same files byte for byte (37815 bytes); after the save (map_pass
  400) and after the load (map_pass 10) the video memory the same and
  the memory the same but for the kinds of differences above (DATA up
  to 1A3Eh, the timers' and interrupts' code segments, the stack).
  DESERT's memory was compared with DESERT's hints, its keys at the same
  labels (its code there reads as BATTLE's). The digits 1, 2, 4, 6,
  7 and 8 not run (the next entry runs MOON and no save).
- A save's digit in MOON and a digit with no save at LOAD (2026-10-06,
  branch dropin-team/save-leftovers). MOON, map 0 as the menu has it:
  both cursors' state (+17h, linear 2D9DA and 2DA0B) poked to 5 at
  map_pass 30 and 31, d at map_pass 150, p at key_wait 200, 7 at
  save_key 100 never let go: the original (key_wait LT0DA1_0D0F,
  save_key LT148F_0174) and the port wrote 07.DAT, byte for byte the
  same (37817 bytes); at map_pass 400 the video memory the same, the
  memory the same but for the kinds of differences above (DATA up to
  14FEh). Loaded with 7 (the original's keys by time: 14 space, 33 down,
  34 down, 35 enter, 37 enter, 39 7, 42 enter, 55 n; number_asked
  F2902:415B 7 at load_game's entry; the port's by passes: menu_pass
  30, 40 down, 50, 60 enter, ask_key 100 7, key_wait 200 n, box_key 100
  n): at map_pass 10 the video memory the same and the memory as
  before (DATA up to 153Dh). ISLE, LOAD with 5 and only 09.DAT there:
  the original goes back to the DISK menu without a message box; at
  menu_pass 1000 the video memory the same and the memory the same but
  for the kinds above. Not run: MOON's load by passes in the original
  (box_key's place LT26FE_0116 did not end the box with n or p at its
  passes; not looked into), MOON with a digit and no save.
- A map's end, on map 03 with a unit poked beside player 1's
  headquarters and moved onto it: the change of phase with the film of
  the building taken (ANIM\br; the 2nd and 25th frame), the message and
  the key, the film of the headquarters blown up (ANIM\qa), the
  statistics (`after_map`), the name for the scores, the scores, the
  menu again: at each of these places the whole video memory and the
  program's memory the same but for the kinds of differences above, and
  the scores' file written the same. For the scores' picture the port
  got the name's Enter a pass of the name's loop earlier than the
  original: at the same pass the two pages come out exchanged (when a
  key is seen by a loop that makes a picture a pass is a matter of a
  tick). With the last map's end poked in at the same place (game_flags
  24FDh before after_map): the ending's film (ANIM\es; its 150th frame
  overall) and the credits (the 1st and the 200th letter typed) the same
  but for the C library's jmp_buf; the port then went on to the menu
  (the original was not run that far).
- The computer player: map 16 (the code CONRA), the human doing
  nothing, at the passes 30, 150, 500 and 1500 of the map's loop (the
  computer, in the attack phase first, assesses, hands out and asks for
  the change); and both players the computer (PLAYER in the menus set to
  COMPUTER twice: the two play by themselves, moves, fights and changes
  of phase), at the passes 300, 1000, 3000 and 5900 and at the map's end
  (the game ends by itself in round 14, before pass 6000): at each the
  whole video memory, the program's memory (the computer's state, the
  units, both maps, the seed of rand) but for the kinds of differences
  above, and the memory behind the program the same. Which parts ran in
  that game was recorded (`BI_SEEN=FILE`, `bi_seen` in the sources): the
  assessment's steps 0 to 5, the commands 1, 2, 3, 4 and 6, the tasks 1
  to 7 and most of the plan's steps. Translated and not reached, so not
  compared: the commands 5 and 7 (units made in a factory), the plan's
  steps 2 and 6, 7.4, 9.2 and 9.3 (a unit that fires from afar), B.7 to
  B.9, the helper (T1ED2) as a whole and the tasks 9 to 0Eh. The same
  game of two computers on the maps 17 to 20 (their codes read from the player's CODES.DAT), each compared
  once, where the map ends by itself (the passes 4196, 3423, 11910 and 14236): the same there
  too. Those games ran the helper's steps 0 to 7, the plan's steps 9.2,
  9.3 and B.7 to B.9 and the tasks 0Dh and 0Eh as well. And on the
  maps 21, 24, 25 and 27 (the ends at the passes 15102, 23839, 28207
  and 47608, the last after about 58 minutes of the original's time):
  the same again; these ran the command 7 (a unit made in a factory),
  the plan's step 6 and the tasks 9, 0Ah and 0Ch too. Not reached in
  any game: the command 5, the plan's step 2, the task 0Bh. The maps
  22, 23, 26, 28, 29 and 31 likewise, each compared once at its last
  pass (29136, 19796, 14270, 34878, 36001 and 35740; after it the game
  waits for a key, `key_wait`): the same in memory and video memory but
  for the kinds of differences above. Map 26 ran in the runner as the
  others before; the other five in a runner built at 07:59 on
  2026-10-02 from the kit's sources as another session had them, not
  committed, with a mouse driver (INT 33h): there the original finds
  the mouse and its variables differ from the port's (`mouse_on` and
  DATA:040E..044A), nothing else new. Map 30 did not end within
  the 400 s given to the port's run: the port reaches the map loop's
  pass 30304 (round 10) in about 20 s and not the pass 30305 within
  45 s, and no wait for a key, message box, film or map's end either
  (key_clear, key_wait, box_key, after_map not reached): it spins
  inside that pass. At the pass 30304 the original (there after 2132 s
  of its time) and the port are the same in memory and video memory
  but for the kinds of differences above. The original does not reach
  the pass 30305 either: run on to 5000 s of its time the loop's start
  still had its 30304 hits. So the game itself hangs there in a game of
  two computers and the port does as it does. Where it spins is not
  looked into; between the pass 30304 and the stop the original's
  memory has a unit's reach in `marks`, `path_count` 0 and
  `computer_state`+17h 1 (memcmp.py on the two dumps), the timers go
  on. Not known: whether a game of a human against the computer on
  that map can come to the same place. The place is `find_path`
  (reach.c; found with counters put into the port for a run): the aim
  (0036h from 0282h, unit 10, player 1) is not reached, the search
  goes on with the list's last node as with a square and the search
  for a new node's place goes round. The port does not follow the
  original here, at the user's wish: `find_path` counts that search's
  steps and gives "no path" beyond 316h, the most nodes a list has.
  With that the game on map 30 goes on and ends as the others do, in
  the wait for a key after the pass 36589 (round 13, player 0 without
  units); nothing of it after the pass 30304 can be compared with the
  original. Compared again with that change, at the ends of the maps
  17, 18 and 20 (the passes 4196, 3423 and 14236): the same as before.
  Not run again: the other comparisons of this list.
- DESERT (`-title desert`): its first map (START at the title menu;
  space at the title's 200th pass, enter at the menu's 30th), the
  original DESERT.EX2 in the runner and the port stopped at the map
  loop's 200th pass: all of the video memory the same, the memory the
  same but for the kinds of differences above (and the mouse's
  variables, the runner being the one with the mouse driver). Nothing
  else of DESERT was compared but the games of two computers (below):
  no key on the map.
  A map's end in DESERT (2026-10-03, the scratch dstat.sh): two
  computers on map 8, game_flags' low byte (linear 2AB5Ch) poked to DDh
  at the map loop's 30th pass, as for MOON's films. Compared with the
  original at after_map's entry, at the name for the scores
  (name_pass, its 100th pass) and, Enter given at its 150th, at the
  scores' wait (key_wait, its 100th): the video memory and the memory
  the same but for the kinds above; at the scores the two pages come out
  exchanged, as in ISLE (the Enter a pass of the name's loop apart).
  The statistics are left out there because after_map returns at once
  while the round is 0. Poked at the 5000th pass instead (round 5), the
  statistics' screen at its wait's 100th pass: won (DDh: rating 845, the
  next code WATCH) and lost (CDh: MISSION NOT COMPLETED, rating 0), the
  video memory the same and the memory the same but for the kinds above
  (the C library's ZEROS, 7 and 8 bytes). A game of two computers that ends by itself waits in key_wait
  for ever: neither a key given by time (Enter, space at 300 s) nor one
  at key_wait's passes (the loop clears it at once) ended it, in the
  original or the port; why was not looked into.
  QUIT THE GAME in DESERT (2026-10-03, without "Quit key Y and Z"): on
  the first map Esc at 25 s and the scancode 2Ch (Z on QWERTY) at 30 s
  leave the map for the title menu in the original, 15h does not; the
  original and the port compared at the menu's 200th pass: the video
  memory the same, the memory the same but for the passes' count
  F27EE:251F (85h against B4h: the keys go by time, the port had run more
  map passes by 25 s) and two bytes of the cursors (+2Ch, +5Dh, 0 against
  1; presumably from the same, not checked). The port with 15h not run.
  A game saved and loaded in DESERT (2026-10-02, the scratch pcmp.py):
  the first map, the cursors moved right twice (player 1 right, player 2
  down) and both asking for the change of phase (player 1 left with fire,
  player 2 left with fire, as in ISLE but the first move to the right:
  the left of the start square is a depot there), D, a key at key_wait
  (T0D36:0D0D), the digit 0 at save_key: the file the port wrote to
  `save-desert/00.DAT` is the original's byte for byte (37815 bytes),
  the video memory the same and the memory the same but for the kinds of
  differences above, at the map loop's 500th pass. Loaded again (DISK,
  LOAD, 0, two keys; the keys by time, 14 space, 33 down, 34 down, 35
  enter, 37 enter, 39 0, 42 enter): at the map loop's 10th pass the same
  in memory and video memory. That the loaded map is the saved one and not
  the first map anew was not looked at apart from the comparison with
  the original, which loads it too.
  Games of two computers (the menu keys as for ISLE, the map's code
  from DESERT's CODES.DAT; the scratch `dcv.sh MAP`): the maps 8, 17
  and 32 ran to their ends (the passes 8507, 19969 and 29107, a wait in
  key_wait) and the original DESERT.EX2 in the runner and the port
  were compared one pass later at map_pass: the video memory the same,
  the heap the same, the data segment above DATA:2000 and the code
  segments the same but for the kinds of differences above (the mouse,
  the timers, input counts, the interrupt's stack leftovers). The other
  maps 9 to 11, 13 to 16, 18 to 22 and 24 to 30 (2026-10-03, the same
way: the passes 29846, 32153, 45172, 18066, 7921,
12499, 22826, 7862, 23404, 16781, 9650, 44800, 14853, 42232, 65908,
18917, 16933, 50509 and 3794, and 31 at 91431; 11, 22, 25, 26 and 29
with 8000 s of the runner's time, 31 with 10000 s): the same as those.
On 12 and 23 the two computers play on without an end (12: round 331
at the pass 150000, 23: round 292). Map 12 compared at the passes
50000 and 100000 instead: at 50000 the same (the timers' slots too);
at 100000 the timers' slots differed (F27EE:2525.., 2534..). Found by
a trace of the timers set and run in both (the runner's -log at
T0D36:01E7 and T0708:4451, a temporary print in the port; the scratch
ttrace.sh, ttcmp.py): the same 911989 events to the pass 65534, then
the port's pass count went from FFFEh to 1FFFFh. bi.h's SD wrote a long
as two words and computed the value again for the second, so
SD(passes, GD(passes) + 1) saw the low word already written; SFP had
the same form. Both take their value once now (bi_sd). The count was
wrong for that one pass (the next step, 1FFFFh + 1, comes out 10000h
again), and there every timer waiting was due at once; on the maps 26
and 31 above (65908 and 91431 passes) that left nothing different at
their ends, on 12 the slots' order. After the
change map 12 at 100000 against the same dump of the original: F27EE
the same, the rest but for the kinds above. The comparisons above were
not run again. Map 23 at its pass 100000 (round 187), with the change:
the video memory the same, the memory but for the kinds above.
Formerly not compared: 12 and 23 (the port alone does
not reach key_wait within 550 s; map 12 is at round 66 at its pass
40000, so a long game, not a stall as far as seen; a longer run was
stopped by the host). The setup screen's item "Title" was looked
  at in the headless build's picture (`DK_DUMP`, Esc scripted): it is
  there under "The game", showing "Battle Isle"; changed to the data
  disk and started from there: not tried.
- Two choices of the setup screen that the original has not (both
  also set in scripted runs by BI_SKIP_INTRO and BI_QUIT_YZ):
  - "Skip logo, intro, and title" (`bi_skip_intro`): battle_main calls
    `title_skip` instead of `title`: the key set of the keyboard as
    space chooses it, the page cleared. Compared with the original
    (space at the title's 200th pass, enter at the menu's 30th) at the
    menu's entry (video memory of the pages the same; the sprite lists
    at A7E8 differ, 4842 bytes, not looked into) and at the first
    map's 200th pass (video memory the same, data above DATA:2000 the
    same; unlike before: the heap behind, 7754 bytes, which the
    title's files leave there, and T2515:0004..0005, not looked into).
    The intro of 256 colours is not in the port yet: the choice is to
    leave it out too when it is.
  - "Quit key Y and Z" (`bi_quit_yz`, on by default): QUIT THE GAME is
    answered by the key at the scancode 15h and by that at 2Ch. The
    original takes the one its key set has at index 18h: 15h in
    BATTLE.EXE (the message shows Y), 2Ch in DESERT.EX2 (shows Z). The
    keys are scancodes, so these are the letters on a QWERTY keyboard
    and the other way round on QWERTZ: the original is right on QWERTY
    and wrong on QWERTZ in both. Tried in the headless build on ISLE's
    map (Esc, then the scancode 15h and 2Ch): without the choice 15h
    leaves the map and 2Ch does not, with it both do. DESERT's key
    without the choice: above; the message's text is not changed.
- The setup screen, in the headless build with scripted keys (`DK_KEYS`,
  the pictures through `DK_SHOTS`, looked at): the menu and each of the
  four pages as the kit draws them (the design of pddnative's setup
  screen, which doskit/docs/LAUNCHER.md made the standard on 2026-10-02:
  the earlier tabs are gone); Esc on a page comes back to the menu and
  in the menu moves to Quit, Enter there ends the program. Earlier, with the earlier screen: `/m`
  chosen and `battle-isle.cfg` written and read back. Not checked since
  the new design: the settings file written and read again by this port,
  full screen (no window in that build), a release build's line for a
  newer release, a controller.
- The window build was started for 8 s and ran (nothing looked at or
  heard: nobody was there). The user then played it (Windows 11, by
  eye and ear, not beside the original): full screen works, the keys
  seem as the original's, the speed is that of a fast 386, the music
  seems right at first hearing; the percussion may be too quiet (not
  sure, to be heard against the original: HANDOFF.md's Next, point 5;
  measured since on the kit's synthesizer alone, no reference: the
  cymbal, snare, hi-hat and bass drum come out 23 to 32 dB below the
  tom, the cymbal as a 44 Hz tone; HANDOFF.md, "The AdLib's drums").
  Not tried: a controller.
- Fights by kind and stop_check (2026-10-07, macOS, the headless
  build; fight.c, move.c): games of two computers on ISLE's maps 16 to
  31 by the menu loop's passes, the original stopped at
  `fight_reckon#N` and at its end `LT2190_0ABA#N`, the port at the new
  places `fight_reckon#N` and `fight_reckon_end#N`. 31 fights (the
  list is in docs/HANDOFF.md): air units attacking and attacked, air
  against air, ships on either side, units that hold others, targets
  that do not answer beside the attacker and over a distance, ground
  of each scene 1 to 7. Video memory and all far data the same at both
  places in 30; in the one other (map 16, fight 1) two bytes differ at
  both places, F27EE:2593 (score_best: 0 in the original, 02E4h in the
  port: a score file for map 16 that earlier runs of the same morning
  had left in this machine's data folder; with `DK_DATA_DIR` naming an
  empty folder the fight is the same at both places, F27EE too). tools/fight.py gives both units' counts as the game's
  in all 31.
  stop_check at its end (the original's `LT122D_150F#N`, the port's
  new place `stop_check_end#N`), nine calls on the maps 17, 18, 27 and
  28: the kinds 1 and 2 and the messages 7, 8, 9 and 0Eh; AX in the
  original is the port's result, video memory and all far data the
  same. The message 5 by a poke (map 17's call 2, the unit on the
  square made the other side's at the routine's entry, the port's new
  place `stop_check#2` with `BI_POKE`): AX 5 in the original, memory
  the same; the port's result 5 read from a trace in a scratch build.
  Not run: DESERT, MOON.
- The window's mouse (2026-10-06, macOS, the headless build; mouse.c's
  bi_mouse_move): the original in the runner with a place set by
  `-mouse T X,Y,B` against the port moved to the same place by
  `BI_MOUSEMOVE="map_pass N:DX,DY,B ..."` (counts, turned into the
  place by the driver's rates: 8 and 16 on these maps, 50 with the left
  button held), on the first map of ISLE, DESERT and MOON (`--load 76`),
  keys by passes, both stopped at the map loop's 400th pass, the pass
  of each event read from the runner's `-log` as before. Video memory,
  the game's code segments and all far data the same, DATA as in a run
  of the port's driver without events (94, 95, 132 bytes), in each of:
  - 125 counts right at pass 151, 100 down at 231 (the original at 260,
    100 and 160, 180): the cursor a column on and a row down;
  - the left button at 151, 500 counts down with it at 191 (190 in
    MOON), let go at 251 (160, 180 held): the status screen;
  - the right button with 125 right at 151, back and let go at 211
    (209 in MOON, the third pass tried);
  - both buttons with 125 right at 151, back and let go at 231;
  - 32 counts right and 40 up at 151 (192, 80, under the distance that
    is a direction): nothing moves, the place stays; with 33 counts
    DATA differs by one byte more (the place is compared).
  Through the platform (`BI_MOUSE=window`, plat_null.c's
  `DK_MOUSEMOVE="P:32,-40,0"`, ISLE): at pictures 1100 and 1300 memory
  and video memory at the map's 400th pass are byte for byte those of
  the `BI_MOUSEMOVE` run; at 300 to 900 (before the game has the mouse)
  those of a run without an event. Not run: a window, a real mouse.
- The mouse (2026-10-06, macOS, the headless build; mouse.c): the
  original in the runner with its mouse driver against the port with
  its own (`BI_MOUSEAT="PLACE N:X,Y,B ..."`, the driver's place and
  buttons at a place's Nth pass; the runner's `-mouse T X,Y,B` goes by
  time, so the pass a time falls in was read from a `-log` of the map
  loop's start and the passes beside it tried), ISLE's first map, keys
  by passes as before, both stopped at the map loop's 400th pass. In
  each the video memory, the game's code segments and all far data
  segments the same, DATA 94 bytes (the 111 of a run without a mouse
  less the mouse's 17), the rest the kinds above:
  - a place 100 to the right at t=25 s (pass 132): the cursor a column
    on, the place back at 160, 100; the same with the port's event at
    pass 132 or 134 (at 131 and 133 the cursor's byte +2Ch differs, 1
    against 0, its animation's count);
  - the left button at t=25 s, 80 down with it at 25.6 s, let go at
    26.5 s (passes 132, 143, 159 in the original): the status screen
    open; the same with the port's events at 132, 143, 160 (with 159
    the same but for the cursor's +2Ch);
  - F7 by passes (down 150, up 153) and a place to the right at 30 s
    (pass 223): the mouse off, nothing moved; the same;
  - F8 twice by passes (150, 200) and the same place: the speed 2, the
    rates 3, the cursor a column on; the same with the port's event at
    222 or 224 (at 223 the cursor's +2Ch).
  The port's driver is at 160, 100, the game's middle, after a reset,
  not at half its limits as the runner's (319, 99). The game takes
  319 for "right" once and sets the place to its middle; in the
  original that falls into the menu or the loading before a map, in
  the port, whose clock stands while files load, it fell into the
  map's first passes and the cursor started a column to the right
  (seen before the change with `BI_MOUSE=1` alone at the first map's
  200th pass: cursor 00B2 against 00B0). With the reset at the middle
  the runs above were made again with nothing but their events
  (`BI_MOUSEAT="map_pass ..."`; the first time the driver had been put
  at the middle by a scripted place before the map's setup): the same
  results, and with `BI_MOUSE=1` alone the first map's 200th pass is
  the original's (DATA 94). What the original's one "right" does in
  the menu, if anything, the port does not do; not looked for.
  Without `BI_MOUSE` and `BI_MOUSEAT` the first maps of ISLE, DESERT
  and MOON at their 200th pass are as before the change (DATA 111, 112
  and 149 bytes, video memory the same); with the driver on DESERT and
  MOON are the original's there with 17 bytes fewer in DATA (95, 132).
  More of the mouse (2026-10-06, the same way, `BI_MOUSE=1`, stopped at
  the map loop's 400th pass; each: video memory, the game's code
  segments and all far data segments the same, no byte of the mouse's
  variables other):
  - both buttons with the place 100 to the right for 2 s (passes 132
    to 168): nothing moves and no fire; the same, DATA 94;
  - the right button alone with that place for 1.5 s (passes 132 to
    159): the cursor runs right, from 00B0 to 00BC (the place is not
    set back while a button is down); the same with the port's events
    at 132 and 158;
  - the left button, the right one with it, the left let go with the
    place to the right, all let go (t=25, 25.5, 26, 27 s; passes 132,
    141, 150, 168), which by the code sets mouse_input's count of ten
    passes (the count itself was not watched): the cursor at 00BC, the same with the port's events at 132, 141, 150,
    167;
  - the menus' MOUSE item by keys (DISK, MOUSE, Enter on SIDE ONE and on
    SLOW, OK, OK, START; keys by the menu's passes): SIDE TWO and MEDIUM,
    then the map with mouse_player 1, mouse_speed 1 and player 1's
    events record as mouse_record: the same, DATA 85; and with a place
    to the right and one down on that map (passes 197, 233): player 1's
    cursor from 019A to 01BC, player 0's where it was; the same;
  - F7 off and on again by passes (150, 200), then a place to the right
    (pass 223): here the port is NOT the original's. The original's
    cursor is at 00B4, the port's at 00B2: switching on resets the
    driver, the runner's place is then 319, 99 and the game takes one
    "right" from it on the map; the port's driver starts at the middle
    (above) and gives none. With 319, 99 scripted into the port at pass
    202 (`BI_MOUSEAT="map_pass 202:319,99,0 222:260,100,0"`) the two
    are the same, so that is all of the difference. Left as decided
    (the middle); what a real driver's reset place gives was not
    looked up.
  - DESERT and MOON (`-title desert`, `-title moon`; 2026-10-06, macOS,
    headless): the first map, stopped at the map loop's 400th pass,
    four cases a title, the places and buttons given to the original by
    time and to the port at the passes those times fall in (in
    brackets; DESERT's, then MOON's):
    - a place to the right, then one down, no button (151, 231; 151,
      231);
    - the left button, then a place down with it held, then released
      (151, 191, 251; 151, 190, 251);
    - the right button with a place to the right, then released at the
      middle (151, 211; 151, 209, the original's event at its pass
      210);
    - both buttons with a place to the right, then released (151, 231;
      151, 231): in the original this changes nothing against a run
      without events, in the port neither.
    All eight: video memory, the far data segments and the game's code
    segments the same; DATA differs in 95 bytes for DESERT and 132 for
    MOON, as with the driver and no events. DESERT's left button case
    also differs in 6 bytes of draw_chars' segment (T2525:00FC and
    0100, values the original keeps there and the port does not; ISLE's
    case with the left button has the same). The library's segments
    and the stack differ as without a mouse.
  Not checked: the menus steered by the mouse (only by keys with the
  mouse on), mouse_rates' effect (the runner's driver and the port's
  keep the rates and do nothing with them), a move or a fight by the
  mouse, DESERT's and MOON's menus, F7 and F8 and player 1 with the
  mouse (their first maps with events are above), Windows and Linux builds
  (build.bat got the file, not run), the ten passes' count read step
  by step in a run (only the result compared).

- macOS (2026-10-06; macOS 15.7.3 on x86_64, Apple clang 17.0.0 of the
  Command Line Tools, SDL2 2.32.10 as a framework in
  `~/Library/Frameworks`, found by doskit's `sdl2-flags.sh`):
  `sh port/build.sh` builds both programs with no error and no warning
  (`-Wall -Wextra`), no source changed for it. The headless build on
  ISLE's first map (space at the title's 200th pass, enter at the menu's
  30th, keys by passes) against the original in the runner (built here
  by run.py), both stopped at the map loop's 200th pass: all 256 KB of
  video memory the same, the game's own code segments and all far data
  segments the same, the rest the kinds of differences above (the C
  library's startup, the routines' scratch variables, the stack's
  leftovers, the timers' and input counts, `song_wait`, and the mouse's
  variables: the runner has a mouse). The first map's picture of a run
  with the setup screen passed by scripted keys was looked at
  (`DK_SHOTS`). The window build was started for 8 s and was still
  running then. Not checked: anything seen or heard in the window (the
  setup screen, the picture, the sound, full screen, the keys, a
  controller), DESERT and MOON, the intro, the other comparisons of
  this list, a build for arm64, a package as doskit/docs/RELEASE.md
  wants it (the binary finds SDL2 by an rpath into this machine's
  framework folder).
  DESERT and MOON on macOS (2026-10-06, the same machine and build, the
  headless program with `-title desert` and `-title moon`, each run
  once): the first map at the map loop's 200th pass against the
  original in the runner, the same keys by passes (DESERT.EX2 at
  BATTLE's places; MOON.EXE's title LT1800_0331, menu LT1151_058E, map
  loop LT070B_13DD, memcmp.py with `--load 76`). In both all 256 KB of
  video memory the same, the game's own code segments and all far data
  segments the same. What differs is the kinds above: DESERT CODE 13
  bytes, the library's segments T2354..T26D2 74, DATA 112, STACK 8
  (DESERT.hints has no ZEROS, so the bytes behind the stack were not
  compared); MOON CODE 11, the library's segments T249A..T27A6 82, DATA
  149 in 89 runs (the number the entry on MOON's first map above has),
  STACK 8. The differing bytes were counted by segment, not read one by
  one again here. Not checked on macOS: any other pass or map of
  either, a key on the map, a fight, a save, a map's end, the title
  chosen on the setup screen, the window.

- Linux (2026-10-06; Debian 13, kernel 6.12 on x86_64, gcc 14.2.0,
  SDL2 2.32.4 as pkg-config reports it, found by `sdl2-flags.sh`):
  `sh port/build.sh` builds both programs with no error, no source
  changed for it; gcc gives 10 `-Wrestrict` warnings (make_path in
  files.c, anim_file in anim.c: strcpy/strcat between two strings of the
  one memory image, which gcc cannot tell apart; the strings are at
  different fixed places, so presumably harmless, not changed). The
  headless build on ISLE's first map against the original in the runner,
  as on macOS (both at the map loop's 200th pass): all 256 KB of video
  memory the same, the segments T0408..T2248 the same; 13 bytes in CODE
  and a few in T2354..T2470 differ, not looked at (presumably the kinds
  of differences listed for macOS). The window build was started for 8 s without a
  display (no X or Wayland in this session) and was still running then;
  nothing seen or heard. Not checked: the window on a desktop, the
  sound, full screen, a controller, DESERT and MOON, the intro, a
  package as doskit/docs/RELEASE.md wants it.
