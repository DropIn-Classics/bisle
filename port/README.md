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
player. That is all of BATTLE.EXE the game calls; a few rare paths
still end the program with a message that names them (`bi_todo`: a
picture larger than a page, a block of the second page in
restore_sprites, the question for another disk, a timer's handler that
is not one of the program's). Not in the port: the game's other
programs (the starter BI.EXE, the intros, MOON.EXE of the second
scenario disk), the PC speaker's sound (not wanted), joystick
and mouse (the port answers as a PC without them).

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

| File | The original's | What |
|---|---|---|
| `main.c` | - | finds the game's files, the setup screen's page, loads BATTLE.EXE or DESERT.EX2, starts it |
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
- No joystick and no mouse (INT 33h): the original's code for them is
  translated as far as it is reached without them.
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
  no key on the map, no save; the answer to QUIT THE GAME with
  DESERT's key not tried.
  Games of two computers (the menu keys as for ISLE, the map's code
  from DESERT's CODES.DAT; the scratch `dcv.sh MAP`): the maps 8, 17
  and 32 ran to their ends (the passes 8507, 19969 and 29107, a wait in
  key_wait) and the original DESERT.EX2 in the runner and the port
  were compared one pass later at map_pass: the video memory the same,
  the heap the same, the data segment above DATA:2000 and the code
  segments the same but for the kinds of differences above (the mouse,
  the timers, input counts, the interrupt's stack leftovers). The other
  maps (9 to 16, 18 to 31) not run. The setup screen's item "Title" was looked
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
    leaves the map and 2Ch does not, with it both do. DESERT's key not
    tried; the message's text is not changed.
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
  sure, to be heard against the original: HANDOFF.md's Next, point 5).
  Not tried: a controller.

Not built: macOS, Linux.
