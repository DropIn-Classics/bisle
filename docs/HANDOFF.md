# Handoff

State of 2026-10-01: stage 1 for the launcher, the main program, the
two data disks' games and the two intros; BATTLE.EXE's and BI.EXE's
code reached to 97% and 96%, DESERT.EX2's as BATTLE.EXE's, MOON.EXE's
to 98%, the intros' to 92%.
`src/BI.hints`, `src/BATTLE.hints`, `src/DESERT.hints`,
`src/MOON.hints`, `src/INTVGA.hints` and `src/INTEGA.hints` rebuild
their programs byte for byte
(`doskit/tools/check.py`: all ok). BI.EXE is read in full (which
intro, then BATTLE.EXE); of BATTLE.EXE the data formats, the screens,
the menus, the map's screen, the fight's reckoning and its scene, a unit's reach, path and
targets, the carrying out of a move, the change of phase and a map's
end with its statistics, and the computer player's assessment, plan and
commands, and how it steers its cursor to carry the commands out, and
the sound (the songs and the effects, with the AdLib) are read and
checked against runs (below); the PC speaker's sound is read, not run;
the animations of ANIM\ are not read. The port (stage 3) has begun:
BATTLE.EXE's start, the logo and the title run as C over the program's
memory image and are the original's in memory and video memory at the
menu's entry (port/README.md; "The port" below).

## Start here (next session)

- AGENTS.md and PROVENANCE.md have the rules: read them first.
- `game/` holds the installed GOG folder as it is (no CD image: GOG
  ships the three games as folders for DOSBox); it is not in the
  repository.
- The kit is at a69c62b (its last step, from ebd96fa, is the launcher's
  design written down, doskit/docs/LAUNCHER.md, and AGENTS.md's rule 9
  from the template; no code changed): since b08dc28 the runtime has `hud.h` (a box
  at the top of the picture for the volume keys) and, in `launcher.h`,
  the dialog about the game's files every port shows (the copy from the
  GOG release offered, its progress, what to do when nothing is found)
  and a `LauncherApp` for the title bar (`launcher_run` takes it now);
  the tools and the runner did not change, check.py was not run again
  for it. port/src/main.c and the build scripts took the template's
  changes (below, port/README.md). Before that, at b08dc28 (after the
  merge 2fca87a it got, for ports, the
  runtime's cdaudio, a CD's audio tracks from a cue sheet, the VGA's
  start address latched at the retrace, the VESA modes 100h, 101h and
  103h, cd_copy_disc and launcher.h, a setup screen; nothing of the
  tools or the runner changed); check.py says all ok with it, the port's
  comparisons were not run again. Not pushed yet when this was written:
  the project's commits made after a5d390a on the Windows machine
  (60857eb to fc687ae) and their merge with d680857.
- The subagents come from the kit's plugin now (`doskit/agents/`,
  switched on in `.claude/settings.json`, which names `./doskit` as the
  marketplace): `doskit:collector`, `doskit:cmd-digest` (new: runs a
  command with long output and reports only the result),
  `doskit:git-committer`. Seen in headless sessions (`claude -p`): the
  first session after a checkout only makes the marketplace known, the
  agents are there from the second on. Not checked: an interactive
  session's question about trusting the marketplace, and two projects
  on one machine each naming their own `doskit/` as the marketplace
  `doskit` (Claude Code keeps one path for a name).
- On a machine without `game/` in the checkout, `DOSKIT_GAME` names the
  installed game's folder (GOG's, with ISLE, DESERT and MOON in it); a
  fresh clone needs `git submodule update --init doskit`.
- Scratch scripts of the last session are in `build/scratch` (ignored,
  not part of the project; they may be gone): `rd.py A B` prints lines
  of build/BATTLE.ASM with the compiler's idioms folded (the segment
  loads, LES BX before an operand, the table indexing: about a third of
  the lines), `turns.py NAME step|change N CODE EVENT..` runs a map
  stopped at the Nth entry and end of move_step or change_phase and
  then tools/turn.py on the two memories (KEYS=file takes a key file
  instead of the events; an event T:phase is both players asking for
  the change and F1), `brun.py NAME UNTIL
  EVENT..` runs a map (CODE=demon for another than MARSS) with keys
  given as T:KEY or T:fire:DIR and leaves build/NAME.ram, .vram and a
  shot, `cur.py RAM..` prints both cursor records and the building's
  record, `bld.py NN` lists a map's buildings.
- A build of BATTLE.hints takes about 15 s once it is identical, 30 to
  60 s while build.py still has rounds to go.

## The game's files

GOG's DOSBox configuration (`__support/app/dosboxBI1.conf`) offers three
games, each its own folder mounted as C: and started there:

| Folder / file | What |
|---|---|
| `ISLE/BI.EXE` | Battle Isle's launcher (10 KB, assembly, linked by TLINK): chooses VGA or EGA (`/V`, `/E`, the marker files `BIDISK.EGA`/`.VGA`, the BIOS), runs `INTVGA\INTRO.EXE` or `INTEGA\INTRO.EXE`, then `BATTLE.EXE` (read from the code, seen in runs; see below) |
| `ISLE/BATTLE.EXE` | the game (208 KB, Borland C++ 1991, large model, 4630 relocations) |
| `ISLE/INTVGA/INTRO.EXE` | an intro (Borland C++) in EGA's mode 0Dh despite the folder's name, with `INTRO.A00`..`A12`, `.FX`, `.PX`, `.MDI`, `.PDI`, `CHAR_BIG.DAT`, `CHAR_SMA.DAT` |
| `ISLE/INTEGA/INTRO.EXE` | an intro (Borland C++) in VGA's mode 13h despite the folder's name, with `INTRO0..12.VGA`, `IPA0..12.VGA`, `CBIG`/`CSMA`/`BTAB`/`STAB.VGA`, `INTRO.FX`, `.PX`, `.MDI`, `.PDI` |
| `ISLE/INSTALL.EXE`, `MOON/INSTALL.EXE` | installers (Borland C++); `MOON/INST.DAT` |
| `DESERT/DESERT.EXE` | data disk 1's starter (2 KB, not Borland): names `DESERT.EX2`, `C:\BLUEBYTE\BI1D1\...`, `MAP\NN.HI`, `NN.DAT` (strings only; what it does is not checked) |
| `DESERT/DESERT.EX2` | data disk 1's game: BATTLE.EXE's code byte for byte, the same relocations; 3 bytes of far data differ (F2789, F27EE), the file ends before the stack |
| `MOON/MOON.EXE` | data disk 2's game (Borland C++ with other encodings than BATTLE.EXE's, linked by TLINK 5.0, 4585 relocations, 96 code segments as BATTLE.EXE but sized and laid out otherwise; names `MAPINFO.DAT`, `MAP02.DAT`, `MAP04.DAT`, `HQ.PAL`, `TOT.PAL`, `END.PAL`/`.SND` besides BATTLE.EXE's files) |
| `MAP/NN.FIN`, `.SHP`, `.COM`, `.PMP` | per map, all packed: the map, what its buildings hold, data for the computer player, the picture of the map's overview (`tools/mapfiles.py`); MOON has no `.PMP` |
| `LIB/*.LIB`, `LIB/*.DAT` | graphics libraries: sprites of units, terrain, cursor, frame, fonts, fight scenes (`tools/libfiles.py`); a `.DAT` gives the order of a library's entries |
| `*.IFF`, `*.LBM`, `*.PAL`, `NN.PAL` | pictures and palettes by their names; not looked at |
| `*.SND`, `*.PND`, the intros' `*.MDI`, `*.PDI` | the songs, MIDI files with the AdLib's own events; `.PND`/`.PDI` the PC speaker's (`tools/sndfiles.py`) |
| `*.FXX`, `*.PXX`, ANIM's and the intros' `*.FX`, `*.PX` | the sound effects, records of 40h bytes; the P files are the same bytes (`tools/sndfiles.py`) |
| `UNIT.DAT`, `GROUND.DAT` | the tables of unit types and ground, packed (`tools/datfiles.py`) |
| `CODES.DAT` | the maps' codes and order, packed (`tools/datfiles.py`) |
| `AMOK.DAT` | the buildings' ground values, colours (`tools/datfiles.py`); packed only in MOON |
| `CHAR6.DAT`, `GAME.TXT`, `TITEL.TXT` | the font of 6x6 pixels, the screens' texts, the title's running lines, packed (`tools/txtfiles.py`) |
| `BB.DAT` | a library of 7 small sprites (`tools/libfiles.py`), all three games |
| other `*.DAT` | tables (MOON's `MAPINFO.DAT`, `MAP02.DAT`, `MAP04.DAT`; DESERT's `UNITU.DAT`, `UNITP.DAT`, which begin INFO DEPO as a .PMP begins INFO ILBM); not looked at |
| `*.pdf`, `goggame-*` | GOG's manuals and metadata |

## What was learned

- The launcher and all the game programs were linked by Borland's TLINK
  (its mark at 1Ch, relocations from 3Eh: `linker tlink 30`). TLINK
  writes the relocation table in the order of the object records, not
  by address; `relocorder original` takes that order from the player's
  file.
- The programs are made of modules whose code segments are BYTE or WORD
  aligned: each begins in the middle of a paragraph, where the one
  before ends. `start=` on each segment line (found as the lowest
  offset below 10h a far CALL/JMP enters it by, else 0: right for all of
  BATTLE.EXE's; two of BI.EXE's were found by hand from build.py's
  undefined labels)
  and a `prefix=` per segment, so that labels of the same offset in two
  segments do not collide. doskit got both (and `linker tlink`) for this.
- BATTLE.EXE: the startup code at CODE:0000 loads DS with DGROUP, frame
  2D8D (`DATA`). Frame 0000 (`CODE`) holds the run time library's
  modules, which share one segment (`_TEXT`, presumably), some compiled,
  some written in assembly; the game's modules each have their own
  segment (`T0408` .. `T2728`). Frames 2736 .. 2D8C are far data
  (`F....`, class FAR_DATA as a guess; which of them are BSS is not
  known). SS:SP = 2F32:0080, and 220h zero bytes follow the stack in the
  file (`ZEROS`, what it is is not known).
- Borland C's code generator encodes some instructions otherwise than
  TASM: AND and OR with a small constant in the word form, XCHG AX,reg
  as 87h /r, TEST r,r with the first register in r/m (`asm` line in
  BATTLE.hints); array[BX] with the array's offset written in two bytes
  (disasm.py now takes such a displacement for an address). The library's
  assembly modules keep TASM's forms: 36 `raw` lines. Four jumps at the
  edge of the short range were sized otherwise than TASM's estimate:
  `raw` too.
- The analysis reaches 97% of BATTLE.EXE's code bytes (155904 of
  160608 as instructions); 4.1 KB of non-zero bytes in about 100 gaps
  are left, nearly all in CODE (the run time library, 3.3 KB); in the
  game's modules they are mostly switch tables (data) and a few
  routines nothing calls (T2354:05B1 sets the PIT's channel 0,
  T2354:0856, T164D:00F6). None of them runs in seven runs of 300 s
  (the menus at random, the first map with player 1's keys at random,
  the first map idle; the seven -cover files merged, then `gaps.py
  src/BATTLE.hints --seg all --cover FILE`: 0 gaps ran); unused library
  code, presumably, or code of parts of the game the runs did not
  reach (a battle, loading and saving were not reached on purpose).
  What reached the rest (from 48 KB left):
  - 35 compiled switches, `CMP BX,N-1 / JA / SHL BX,1 / JMP CS:[BX+table]`
    with the table of near offsets in the code segment right before the
    next routine (`words` hints); most of the gaps were behind them,
    and switches inside switches.
  - Far pointers made of two immediates, the offset plain and the
    segment relocated: `MOV AX,handler / MOV DX,seg` before a far call
    to T2354:0713 (`timer_add`, a timer callback and its period in
    CX:BX, presumably), a record at T0708:10F6 filled with seven far
    pointers to routines; `ptr` hints. doskit's `ptr` and `words` seeded
    code only in the segment named CODE; now in every segment of class
    CODE (doskit 1c03e90).
  - T2354 sets INT 08h (T2354:0662), INT 09h (T2354:04C4) and INT 24h
    (T2354:0520) with INT 21h AH=25h, DS = CS.
  - T2248 begins at offset 4 (`start=4`): T223C's switch table runs up
    to there; the build was 16 bytes too long without it.
  - The startup code's table of routines it calls before main
    (DATA:1A32, records of 6 bytes, CODE:01A5 walks it): three near
    routines (CODE:297B, 2E2F, 3F48), a `words ... stride=6` hint. A run
    with `-cover` (doskit 795b46d) showed they ran though the analysis
    did not reach them; a `-trace` of the first 500 instructions showed
    the CALL WORD PTR ES:[BX+2] at CODE:01E0 that enters them.
  - T25A6 begins at offset D: T259F's last routine runs on over its
    frame's end to a RETF at T25A6:000C (found by the same run).
  - A far jump within CODE (CODE:3FFF, JMP FAR PTR) that doskit's
    assembler wrote as a short jump (fixed in doskit ccbdc27).
  - The far pointers the analysis finds in data (relocated segment
    words) are nearly all inside unreached code, CALL FAR instructions;
    only DATA:1770/1774/1778 are real (all to CODE:1D7F, a routine that
    does nothing).
- BI.EXE: 96% of its code bytes as instructions (5346 of 5584). Its
  own code (CODE, S00FF, S0102, S0107, S012B, S015B) was reached
  already; the rest is S0022 and the segments only S0022 calls. Nothing
  calls S0022 (no relocation outside it names it): a library module
  linked in and not used, presumably, the same timer and keyboard
  module as BATTLE.EXE's T2354 by its first routine (not compared in
  full). Its routines are `code` hints, so that it reads. Left: data in
  S0107 (addressed through CS) and single bytes at segments' ends.
- Driving BATTLE.EXE in the runner: the credits after the title run on
  until a key (`-key 14 space`); the title menu (START, OPTIONS, DISK,
  EXIT) takes keys from about 30 s on (`-key 33 enter` starts the first
  map; keys before that do nothing, why is not looked into). Down and
  up move, enter chooses or changes an entry (OPTIONS: FIRST, SETTING,
  PLAYER, OK; its SETTING: ALL SHOPS / HIDE SHOP, NO LIMIT / 4 TURNS /
  8 TURNS, PALETTE; DISK: LOAD, MOUSE, RATING, OK), Esc does nothing
  there. The menu (T1090) reads the key the timer callback got from
  INT 16h (DATA:0404) against a set of keys (a far pointer at
  F27EE:2498: Esc, space, backspace, Del, Ctrl, Tab, enter, up, down,
  right, left, F1..F8 by their scancodes) and ORs in the joystick's
  directions. In a map the game's INT 09h keeps a bitmap of the keys
  and each player has a table of keys with directions (DATA:0AE9,
  0AEB: records of 7 bytes, a scancode and left, right, up, down, fire
  as read from T2354's joystick code): player 1 the arrows, the keypad,
  space and enter, player 2 letters, Alt and Ctrl.
- Saving and loading, driven in the runner: in the first map each
  player asks for the change of phase (player 1: cursor two left onto
  an empty square, `space+`, `left+`, `space-`, `left-`; player 2 the
  same with `lctrl` and `x`; fire must be let go while left is still
  held), then D shows INSERT SAVE DISK / PRESS SPACE and a key saves
  `ISLE\00.DAT` (T13CA:000F `save_game`). The runner keeps it in
  `build/run/state`, so later runs find it: DISK, LOAD, `0`, enter
  loads it (T13CA:0462 `load_game`) and the map comes back as saved.
  The key times of that run: 14 space, 33 enter (the map is up at about
  45 s), 48 left, 49 left, 50 space+, 50.6 left+, 51.5 space-, 52 left-,
  53 x, 54 x, 55 lctrl+, 55.6 x+, 56.5 lctrl-, 57 x-, 59 d, 62 0; for
  loading 14 space, 33 down, 34 down, 35 enter, 37 enter, 39 0, 42
  enter. The save asks for no position; where its name "00" comes from
  is not looked into. The path was found from the code (the key set,
  the cursor record's state 5, the message table; BATTLE.hints). Esc, Y leaves the map without a
  save question. Neither run reached any of the 4.1 KB of gaps (-cover,
  gaps.py: 0 ran).
- DESERT.EX2 and MOON.EXE: `doskit/tools/xfer.py` carries BATTLE.hints
  to them (it handles programs of many code segments now: doskit
  23cbe74). DESERT.hints has BATTLE's segment lines, ZEROS not stored
  (`size=220`, the header's minimum allocation 2Bh paragraphs), and all
  127 hints carried. MOON.hints' segment lines come from its own
  relocations (the classes by position: 96 code frames from 0000 to
  27FC, far data from 280A, DATA 2EAE as a guess from BATTLE's layout;
  start= the lowest offset below 10h a far CALL/JMP enters by, 0 for
  T2084, T238E, T249A, T260B where none does, which the build accepts).
  Its compiler encoded AND/OR with a small constant in the byte form
  and XCHG AX,reg in one byte (as TASM does; BATTLE.EXE's the other
  way), 9 single instructions otherwise (`raw`). TLINK 5.0's header is
  400h bytes longer than its relocations need (zeros; MOON/INSTALL.EXE
  200h, DESERT.EXE none: no rule seen), so `linker tlink 50
  header=original` (doskit, new). xfer.py matched 48452 of BATTLE's
  58980 instructions; 71 hints carried, 56 not (switch tables in
  T1C04, T1938, T17C0 and others, the handler pointers of T0708's
  record, 12 raw lines of CODE: MOON's library differs, its CODE is
  39B0h bytes, BATTLE's 4080h). The 50 carried raw lines are not needed
  with MOON's encodings, but harmless.
- MOON.EXE from 73% to 98% (161105 of 164647 code bytes as
  instructions): 36 compiled switches found by their bytes (`D1 E3 2E
  FF A7`, SHL BX,1 / JMP CS:[BX+table], the CMP BX,N-1 before giving N;
  a scratch script, not a tool), 2 of them carried already; T237F's
  table runs 4 bytes into T238E's frame, so T238E has `start=4` (the
  build was 16 bytes long without it, as BATTLE's T2248). The startup
  table is at DATA:1508 (two near routines, CODE:20F5 and CODE:2CE5);
  CODE:1C5D is the routine that does nothing (DATA:1226/122A/122E).
  T1867:034E is a far routine nothing calls and BATTLE.EXE does not
  have (a `code` hint). T2579:019D..0346 is data in the code segment
  (a table of 43h records of 6 bytes from 01A7, read through CS).
  T070B:10F6 fills the record of seven far pointers to routines, as
  BATTLE's T0708 (`ptr` hints; two of them, CODE:20B0 and CODE:1C14,
  were not reached before). DATA is 2EAE: a run loaded the program at
  0076 and had DS = 2F24 at CODE:0116. What is left (3.5 KB, 161150
  bytes reached) is mostly the library in CODE (1.6 KB): routines
  nothing calls and data read through CS (CODE:2694's table, the
  words at 26F0/26F2). A run (space at
  14 s, enter at 33 s, 90 s) reaches MOON's first map with BATTLE.EXE's
  keys; gaps.py with its -cover: 0 gaps ran.
- The intros, INTVGA/INTRO.EXE and INTEGA/INTRO.EXE: the folder names
  seem the wrong way round but are what BI.EXE means (see below; a
  run: INTVGA's sets mode 0Dh, 16 colours, INTEGA's mode 13h, 256
  colours, and reads the `.VGA` files). Both are
  Borland C++ linked by TLINK 3.0, the same run time library in CODE
  (3FA0h bytes, the same offsets; it runs 10 bytes into T03FA's frame,
  so T03FA has `start=A`, found by a run) and the same timer module as
  BATTLE.EXE's T2354 (INTVGA's T0536, INTEGA's T0559 8 bytes further
  on: INT 08h and 24h handlers, a timer callback reading INT 16h, no
  INT 09h). Their segment lines come from the relocations (a scratch
  script, as MOON's); the header's minimum allocation needs zeros after
  the stack that are not in the file (`ZEROS size=`, 210h and 220h, the
  exact sizes not known). The library's encodings are MOON.EXE's (the
  byte form of AND/OR, 10 `raw` lines of the word form). 92% of the code
  reached (INTVGA 26956 of 29179 bytes, INTEGA 27281 of 29552): the
  switches, the startup table, the timer handlers, and the library's
  timer callbacks `MOV AX,handler` before timer_add, found as gaps that
  ran in a run of each intro to its end (178 s and 200 s, no keys);
  after them no gap ran. The rest is unused library code, 4 KB.
  BI.EXE runs INTVGA's on a card it takes for VGA (below).
- Which intro BI.EXE runs (BI.hints, `check_card`): `/V` or `/v` in its
  command tail means VGA, `/E` or `/e` EGA; without either, BIDISK.EGA
  there means EGA, else BIDISK.VGA VGA, else INT 10h AX=1A00h decides
  (AL=1Ah VGA). VGA runs INTVGA's intro (16 colours), EGA INTEGA's (256
  colours), so the folder names match what the launcher takes the card
  for, not what the intros show. Then CD .., and BATTLE.EXE runs if the
  same marker file opens (else INSERT DISC, three tries). Both get
  BI.EXE's command tail. GOG's ISLE folder has all three marker files
  and starts `bi.exe` without arguments: EGA, INTEGA's 256-colour intro
  (runs: no arguments, `/v`, `/e`, and one to BATTLE.EXE's start after
  the intro's exit at 200 s). BIDISK.INT is not opened by BI.EXE;
  BATTLE.EXE, DESERT.EX2 and MOON.EXE hold the string BIDISK.VGA (what
  for is not looked into), none of them .EGA or .INT.
- BATTLE.EXE and BIDISK.VGA (BATTLE.hints, `check_vga_disk`): it loads
  the file before the animations of ANIM\ and at one place in T0408;
  if that fails, "ERROR: cannot load file." and the end (`fatal_error`).
  A disk check, presumably; no run reached it (menus, first map, saving).
  With `/v`, `/e` or no argument it starts the same (mode 13h, the same
  files, 6 s each): no switch read there, as far as seen.
- Sound (BATTLE.hints, `sound_init`, `adlib_probe`): BATTLE.EXE probes
  for an AdLib at 388h through the OPL's timer status (0, then C0h after
  timer 1 runs) and prints "AdLib." or "Speaker.". doskit's runner answered 06h
  there, so the runs before doskit 60f8f68 printed "Speaker." and ran the
  speaker code; the GOG game plays AdLib (the user heard it). doskit has
  the OPL2's timers now, and an FM synthesizer of its own
  (runtime/opl.c, for the port as well): BATTLE.EXE prints "AdLib." and
  `-oplwav FILE` writes what it plays (49716 Hz mono from t=0). A run
  (the save key script, 80 s): music from 4 s (the title) to 16 s (space)
  and from 33 s (the map) on, no sample clipped. By ear (the user, against
  the original): the map's music clearly recognizable, good for now,
  refinement later; the title music not judged separately. With
  -cover no gap of BATTLE.hints ran: the AdLib code was reached by the
  analysis already. The intros and MOON.EXE load 388h too (not looked
  into).
- Battles, driven in the runner: one player against the computer
  (OPTIONS, the first entry, the code CONRA from GOG's
  Quick_Ref_Card.pdf, OK, START; keys 14 space, 33 down, 34 enter, 36
  enter, 37 c, 37.5 o, 38 n, 38.5 r, 39 a, 40 enter, 43/44/45 down, 46
  enter, 48.5 enter; the map is MAP\16, up at about 50 s). Then player
  1 asks for the change of phase (66 left, 67 left, then every 20 s from
  70: space+, +0.6 left+, +1.5 space-, +2 left-) and presses F1 4 s
  later: F1 changes the phase once both players asked (the computer
  asks at once; "F1 : CHANGE MODE" is shown, after it a new request
  reads "REQUESTING MOVE MODE"). Movement and action (attack) phases
  alternate, the attacks are carried out at the change (a move at once,
  in the mover's own map: turn.py, below). In 600 s the
  computer attacked (fight scenes at 200 s and 500 s in the shots) and
  won (STATS.IFF, LOOSER.SND at 534 s, then the menu). With -cover one
  gap ran: T1F5A:0000, a RETF that ends T1F3C's last routine (T1F5A
  `start=1` now, in DESERT.hints too). Neither BIDISK.VGA nor ANIM\ was
  opened: the animations check_vga_disk guards are not the fight scenes.
- A map's end, poked in (BATTLE.hints at `play_anim`): in the battle
  against the computer, `-poke LT0708_4088#401 F27EE:26CB 07` and
  `-poke LT0708_4088#401 F27EE:26D6 0F00` (at about 62 s) give player 1
  state 7 and result 0Fh: "VICTORY !! HQ IS YOURS" / "YOU LOST YOUR HQ
  !!", and after a key (space at 70 s) check_vga_disk opens BIDISK.VGA
  and `play_anim` (T2248:0F28) plays ANIM\qa (the HQ blown up); result
  11h plays ANIM\hs. Then MENU.IFF, CODES.DAT, "TYPE NAME FOR TOP FOUR";
  the name (keys, enter) is written to MAP\04.HI (28h bytes, not
  decoded; no .HI ships with the game, the runner keeps it in
  `build/run/state`; why 04 for map 16 is not looked into), then MAP\16.HI
  is read and map 16 starts again. With -cover one gap ran: T2354:0000,
  POP SI / POP BP / RETF, the end of T2248's last routine (T2354
  `start=3` now, in DESERT.hints too). Animation 4 (F27EE:250C bit 20h,
  the last map, presumably) was poked in as well (`-poke LT0708_4551
  F27EE:250C FD24`): ANIM\es in five parts (a self-destruction sequence,
  about 47 s), then `end_credits` (T25A6:0655) opens ANIM\ab.fx and types
  the staff credits; no gap ran. F27EE:251B was 4 when map 16 ended: the
  04 of 04.HI, presumably. That a poke is no real win: what a real one
  does besides is not known.
- The file routines (BATTLE.hints, from the code, their calls seen in
  -dos traces): `file_open` (T2690:0004), `file_close` (T2628:0004),
  `file_size` (T2653:0000), `save_file` (T26D2:000A) and `load_file`
  (T2695:0004), which unpacks the TPWM files: "TPWM", the unpacked
  length (a long), then flag bytes whose bits (from the top) each say
  a byte as it is (0) or a copy from earlier output (1: two bytes, the
  length in the low nibble + 3, the distance in 12 bits). A scratch
  script with that reading unpacks all 244 TPWM files of ISLE\ to
  their header's length, each ending at the file's end.
- `tools/tpwmfiles.py` (the first format tool): parses each TPWM file
  of the three games (600) into its items and writes it back identical,
  unpacks it (`--out DIR`), and packs the unpacked bytes again. Two
  packers are seen, both greedy with the longest match (3..18 bytes,
  up to FFFh back): ISLE's and DESERT's take the farthest of equal
  matches and measure one byte past the end (00, in 6 files 01..03:
  whatever the packer's buffer held, presumably; the tool tries all
  256); MOON's takes the nearest and never copies from position 0; in
  31 of MOON's files the last copy runs past the end, its length and
  distance from bytes there that are not known. All 600 files are
  reproduced that way (449 ISLE's packer, 120 MOON's, 31 MOON's but
  the last item). About 20 s for all.
- `tools/palfiles.py`: the 34 .PAL files, all packed, 768 bytes
  unpacked, written back identical, shown as PNG (`--png DIR`). Two
  kinds: 8-bit values (00..02, MENU, MOON's ANIM\END/HQ/TOT) that
  `set_palette` (T251F:000E) scales by a level, (value * level) >> 10;
  on map 16 the DAC was that of level 252 exactly (a run, `-vgastate`).
  The other ANIM\ palettes hold 6-bit values (where the game sets them
  is not looked into). Many entries are (255, 0, 0): unused, presumably.
  `tools/png.py` writes indexed PNGs for the format tools.
- `tools/ifffiles.py`: the 16 pictures (.IFF, BIGMASK.LBM; all packed
  but MOON's TITEL.IFF) are IFF "PBM " of 320x200, 352x256 or 360x240,
  read as BATTLE.EXE's `load_picture` (T2550:0006) reads them, written
  back identical, shown as PNG; packing the pixels anew gives every row
  as stored. The map's frame in video memory was GAME.IFF's pixels but
  colour 64, the windows for the maps (a run, `-vram`). The runner's
  PNGs take a 6-bit value v as (v << 2) | (v >> 4), the tools' as
  (v * 255 + 31) / 63: they differ by 1 at some values.
- `tools/libfiles.py`: the 40 libraries (LIB\*.LIB, all packed as
  files): a long (the directory's offset), the entries, a directory of
  12-byte records (name, offset). An entry: a label, the transparent
  value, the kind 'P' (two 4-bit pixels a byte) or 'U' (a byte a
  pixel), x and y offsets, width, height, the pixels plane by plane
  (unchained 256 colours). Drawn with a colour base added
  (T2470:0008 `draw_entry`, T2506:000A `draw_unit24` for the 24x24
  units, T24B5:000E `store_part` for the terrain; read from the code).
  In a run to the first map the video memory held UNIT.LIB's sprites as
  the tool decodes them, base 20h for player 1 and 30h for player 2,
  PART.LIB's terrain (base 0), a cursor, RAND.LIB's frame (base 40h,
  the colour 64 of the map's frame); in a MOON run its units of width
  14 as 24 pixels. The loader (T0CEB:0008 `load_lib`) sorts unit, part
  and bigunit by NAME.DAT (8-byte names); BIGUNIT.DAT names MAA, which
  the library lacks. MOON's BIGUNIT and FIGHT pack each entry on its
  own (MOON's packer), MOON.EXE unpacks them before drawing (T261E:0008).
  All 40 written back identical, the packed entries too. ISLE's UNITB,
  BIGUNITB, 00 and 01.LIB have entries of another kind (not decoded)
  that no program names: left over, presumably. BB.DAT (all three games,
  packed) is a library as well: 7 entries of kind U named 06 .. 00, 34x39,
  34x39, 29x36, 22x27, 1x19, 11x11 and 19x1 pixels; libfiles.py takes it
  with the .LIB files and writes it back identical. They are the Blue
  Byte logo that runs before the title: `title` (T1727:0004) calls
  T2433:0006 with BB.IFF and BB.DAT; that unpacks BB.DAT (load_file),
  draws BB.IFF (load_picture), sets its palette (set_palette at level
  FFh) and draws 67 steps from a table at T2433:01A7 (read through CS:
  records of 6 bytes, x, y and the entry's offset in the unpacked
  file; draw_entry at x+3, y-1 with the sprites' own offsets), in the
  table's order: 19 of the 19x1 entry (named 00) along y 170 from x 109
  in steps of 5, one of the 11x11 entry (01) at (203, 160), 28 of the
  1x19 entry (02) at x 213 from y 152 up to 14, then the star at
  (202, -3): entries 03, 04, 05, 06, 05, 04, three steps each, and 03
  once more (19 steps).
  All steps drawn over BB.IFF with its palette
  (a scratch script) give the logo card with the star at the top right;
  not compared with a run's video memory, the timing not read.
- `tools/mapfiles.py`: the 103 maps of the three games (ISLE 00..33,
  DESERT 00..33, MOON 00..33 and STATMAP), all files written back
  identical. T0708 loads them when a map starts (-dos: .FIN, .SHP, .COM,
  .PMP; make_path's extensions 0, 2, 9, 1). `load_fin` (T0E9B:02B6): the
  width and height, then two bytes a square, a GROUND.DAT index (its
  flags 400h, 100h, 40h make a building, owner by flags 1 and 2) and a
  unit (F4h up none, else type = byte >> 1 and player = bit 0); ISLE's
  types 15, 17, 19, 21 take two squares, the next type in the row below
  or above by player. Every map has one 40h building per player (the
  headquarters, presumably). `load_shp` (T0E9B:007B): 27 bits for the
  unit types, then records of 12 bytes (owner, which building table,
  index, two bytes, seven unit types). ISLE's 11.SHP ends 4 bytes into
  its last record. The .COM (27 records of 6 bytes; ISLE's 16..31) is
  loaded only with F27EE:250C bit 400h: in the run against the computer
  (map 16) it was. The .PMP (W = 2w+4 by H = 2h+4 pixels, a name: M00..
  in ISLE, CLOCK, LOSAG, .. in DESERT) is the map's overview (below).
- `tools/datfiles.py`: UNIT.DAT (27 unit types of 44h bytes) and
  GROUND.DAT (ground records of 6 bytes; 110 in ISLE and DESERT, 150 in
  MOON) of the three games, written back identical. T0708 loads them to
  F27EE:001F and F27EE:0751 when a map starts, over the same bytes in
  the program's own data: BATTLE.EXE holds ISLE's two files there, and
  DESERT's UNIT.DAT is ISLE's (compared). The fields as BATTLE.EXE's
  code uses them (the tool's docstring has them all): a type's move,
  armour, counts (6 in a unit; the ships 1 and a second count of 6),
  the ground it can be on (a mask ANDed with the ground's), the classes
  it can fire at with two ranges (at air units, at the others) and three
  hit values (air, land, sea), its class word (land 4, sea 8, air 10h,
  20h; two squares; holds others), what can hold it and what it holds,
  two weights for the path search, the big picture, two names, a cost,
  the room it has and the size it takes; a ground record's flags, the
  mask, two costs (the second for air units) and the fight scene's
  kind. Named from this: `make_unit` (T169E:000E, a unit's record of
  1Ah bytes at F27EE:2898 from its type), `find_path` (T0BA0:0E2E),
  `square_distance` (T0E9B:1E90), `fight_reckon` (T2190:000D, the
  fight's outcome; its formula is not read yet), `list_makeable`
  (T1479:11B4). A battle of 600 s against the computer with `-rwatch
  F27EE:001F 9C6` (both tables; keys: the CONRA script above, then 66
  left, 67 left, and every 20 s from 70 space+, +0.6 left+, +1.5
  space-, +2 left-, +4 f1; the computer won at about 520 s) showed 97
  reading instructions after the unpacking: those in the routines named
  here read the fields as described (the tool says "seen"); the others,
  most of them in the computer player's modules (T17C0, T1ABC, T1B01,
  T1C04) and the fight scene's (T1F5A, T2112), are not read yet. +17h of
  a type and the names' last bytes were read by nothing, a ground's
  second cost not in this run (no air unit moved, presumably). Not
  checked against what the game shows on its screens.
- CODES.DAT (`tools/datfiles.py --codes`): 34 records of 10 bytes, a
  map each: its code (5 letters, each less 30h), +6 the players (2, or
  1: the menu's T1090:110F then sets F27EE:250C bit 400h, a map against
  the computer; in each game the maps with 1 are those with a .COM
  file), +7 what after_map adds to the map's number after a win (1; 0
  in the last map of a row), +8 bit 0 the last map, which sets bit 20h
  (the ending). The menu looks the typed code up there and the record's
  number is the map (F27EE:2523). +5 (2) and +9 (0) have no reader in
  the code as far as read. All read from the code; no run made for it
  (the CONRA run of earlier steps went this way).
- AMOK.DAT (`tools/datfiles.py`; 36 bytes, packed only in MOON; loaded
  to F27EE:24E8), all read from the code: values the program has no
  constants for. +0..+7 the ground a building's square gets when the
  building changes its owner (T0408:16B3): by owner 0, 1, 2 for a
  building record with flag 8 in its +19h (in all three games the
  GROUND.DAT records of flag 400h and that owner), the same for flag 10h
  (ground flag 100h), and by owner 0, 1 for flag 4 (ground flag 40h);
  the tool checks that. +8 (6) the steps of an animation on a square
  (T0408:0C86 counts a square's record down from it; the explosion,
  presumably). +9..+17h colours of texts and boxes (+9, +0Ah the status
  screen's counts of the two players: seen; the others as read, +0Ch and
  +15h..+17h without a reader). +18h..+1Bh four ground values a unit's
  order 0Bh gives four squares of ground flag 8000h (a building site,
  presumably), +1Ch..+1Fh those of order 0Dh, with which a depot record
  is made and the first square gets +3 or +4: the pioneers build a
  depot so, presumably (read, not run). +20h..+23h the colours of the
  units' dots in the overview (next item).
- The .PMP is the map's overview, and what the cursor's directions do
  with fire on an empty square (the save run's keys with another
  direction for left: 50 space+, 50.6 DIR+, 51.5 space-, 52 DIR-; shots
  at 55 s): right shows the overview in the player's window, down a
  status screen, up nothing (left asks for the change of phase, above).
  The .PMP is a library of one entry ("INFOILBM" its label), a byte a
  pixel, 2 pixels a square and 2 around; `draw_overview` (T0E9B:0931)
  draws it with colour base 70h in the middle of the window and a dot
  of four pixels for each unit (AMOK's +22h by player; no dot for the
  types shown to their own player only). `mapfiles.py --png` writes the
  68 overviews, `--overview VRAM NN` compares: in the run the picture
  with its nine dots was in video memory at x 59, y 78 but for 160
  pixels, a frame around a part of it (the part the window shows,
  presumably; not read). All 68 .PMP files are written back identical
  as libraries. MOON.EXE has overview_dot and put_pixel too (carried),
  but its own routine around them: MOON has no .PMP (not read).
- The status screen (`draw_status`, T1479:0DC5; the run with down):
  ROUND, LEVEL, MODE, HIGH, ACTUAL; UNIT, FACTORY, DEPOT for ONE, TWO and
  MAP; TURN, LIMIT. Its three counts a player are F27EE:243E +2, +3, +4
  (17h bytes a player); +3 counts the buildings of ground flag 400h and
  +4 those of 100h (load_fin, T0408:16B3), so by the screen's rows 400h
  is a factory and 100h a depot (40h, of which each player has one, the
  headquarters). On ISLE's first map the screen showed 6, 6 and 12
  units; its .FIN has 5 a player (the one in each headquarters'
  record, presumably; not looked into) and no factory or depot.
- The maps drawn (`tools/mapfiles.py --png DIR`, all 103): `draw_window`
  (T0E9B:0A9B) draws a player's window column by column, a square 24x24
  at x = 16 * column and y = 24 * row, 12 further down in the odd
  columns; the ground is PART.LIB's entry of that number in PART.DAT's
  order, of which `draw_hexagon` (T24DF:0002) copies a hexagon of 384
  pixels through the latches, whatever the entry's transparent value
  (DESERT's entries are full squares; ISLE's and MOON's are clear outside
  the hexagon); the unit is UNIT.LIB's entry type * 6 + direction (the
  unit's +0Fh, +10h, one per window; 3 for player 0 and 0 for the others
  when a unit is made), base 30h for player 1, else 20h. Runs to the
  first map of each game (space at 14 s, enter at 33 s, `-vram` at 60 s;
  `mapfiles.py --match VRAM 00 game/ISLE/MAP`): in both windows
  (GAME.IFF's colour 64, 24192 pixels each) all pixels were the drawn
  map's but those of one square, the cursor's (ISLE 240 in each window,
  DESERT 228, MOON 0 and 384). DESERT.EX2 runs by itself in the runner
  (from its folder, the same keys; it opens the files BATTLE.EXE opens)
  and MOON has no .DAT files in LIB: MOON.EXE's ground and units are the
  libraries' entries in their own order (that run).
- doskit's `-rwatch` kept 64 readers (an instruction and the byte it
  read), which the unpacker's own reads of the tables filled; it keeps
  up to 65536 now (doskit 1676994, with a test, RWATCH.EXE).
- xfer.py can carry a name to the wrong routine where the two programs
  differ around it: a name at BATTLE.EXE's T1479:0C93 (the type's big
  picture and name) went to MOON.EXE's T1724:000E, a routine that draws
  a text. The name is left out of BATTLE.hints for that; the other names
  carried in this step (make_unit, find_path, fight_reckon) were looked
  at in MOON.ASM and read the tables as BATTLE.EXE's do. Earlier carried
  names were not looked at that way.
- `tools/txtfiles.py`: CHAR6.DAT, GAME.TXT and TITEL.TXT of the three
  games (the first two the same bytes in all three, TITEL.TXT another in
  MOON), written back identical. CHAR6.DAT is the font: 255 characters
  of six words, a row each, the leftmost pixel bit 7 of the first byte;
  `draw_chars` (T2525:000A) sets the pixels of the set bits in the
  colour of the drawing record (DATA:00D6, to which the far pointer
  DATA:0092 points; the font's far pointer is its +206h), 6 pixels a
  character, 7Ch or 0Dh a new line. GAME.TXT: 31 texts, each its lines
  ended by 0, then 2 and a byte; `draw_text` (T164D:0140: x, y, number,
  colour) draws one, 6 rows a line, `draw_number` (T164D:01DB) a number.
  TITEL.TXT: the title's lines in CHAR24.LIB's characters (the entry's
  number plus 3, 1 a gap), drawn by `draw_text24` (T164D:000C, 24 pixels
  a character; the menus of T1090 use it too); `title` (T1727:0004) runs
  three of them up the screen at a time, each centred, inside y 132 to
  199. A run to the status screen (the keys above with down, -vram at
  56 s; `txtfiles.py --match`): the texts 0Ah, 0Bh, 0Dh, 0Fh and 10h in
  video memory pixel for pixel as the tool draws them, in colour 49h
  (AMOK's +0Dh), on both pages (the second 4000h bytes into a plane).
  The title's lines were seen in a shot at 25 s, not compared pixel for
  pixel; the other texts were not seen in a run. MOON.EXE has draw_chars,
  draw_text24, next_line and title (carried, looked at in MOON.ASM);
  draw_text and draw_number were not mapped by xfer.py and are not looked
  for there. T164D:0100 is a routine nothing calls (next_line a number
  of times; left as bytes).
- doskit on Windows: xfer.py wrote the hints with CR LF there (every
  line of DESERT.hints and MOON.hints differed); it writes LF now. The
  kit's selftest was written for cc and `sh` only; it runs with MSVC now
  (selftest ok on Windows 11 with Visual Studio 2022 and on Ubuntu 24.04
  in WSL with gcc 13; macOS not run, doskit's new workflow
  `.github/workflows/selftest.yml` runs the three once pushed). What it
  found on the way: `runtime/inno.c` and the template's `port/src/main.c`
  gave warnings at MSVC's /W4 (this project's main.c too, fixed here),
  update.c on Windows had no file:// for the test.
- The status screen in full (`tools/screens.py --status RAM --vram
  VRAM`, BATTLE.hints at draw_status): the tool draws it from the game's
  files and a run's memory as draw_status does: the window
  (`draw_shop_window`, T1479:0004: SHOP.LIB's entries 0, 3, 1, 2 with
  base 40h and a filled rectangle), three boxes (`draw_box`, T1479:0130:
  filled, a light row above and column left, a dark column right and row
  below; `fill_rect`, `draw_row`, `draw_column`), the texts, the numbers
  and SHOP.LIB's entry 5, all colours AMOK.DAT's. A run with both
  players' screens up (the keys of the overview run with down; player
  1's: 53 x, 54 x, 55 lctrl+, 55.6 c+, 56.5 lctrl-, 57 c-; -ram and
  -vram at 61 s): all 24160 pixels drawn for each player were in video
  memory, on both pages (player 0 MOVE, player 1 ATTACK). Other values
  and a limit were not seen. Two things the comparison showed:
  draw_entry_u clips to the drawing record's rectangle with the right
  column and the lower row left out (0, 0, 320, 179 in the run, so row
  179 of the window's lower edge is not drawn), and the cursor is in the
  hexagon of entry 5.
- The unit's screen (`draw_unit_info`, T1479:05AF, BATTLE.hints): in the
  first map of ISLE, the cursor one up from its start onto a unit, fire
  with down held (`-key 48 up -key 49 up -key 50 space+ -key 50.6 down+
  -key 51.5 space- -key 52 down-`) opens it (-log: T1479:05AF once,
  draw_status not; shot at 55 s): the unit's big picture and name ("T-3
  SCORPION"), a column of numbers with icons and a small unit. Fire alone
  on a unit shows only the bottom line ("5 3RD ARM. VEHICLES"). From the
  code (T0D36:10D4, 1484..158B; not checked beyond the run): the cursor
  record's +1Bh 2 and then state +17h 4 come from fire with down when the
  cursor's +1Ch has bit 4 (set with 0Eh when +18h is 1); the map's loop
  calls the routine at T0708:25FF. `tools/screens.py --unit RAM --vram
  VRAM` draws it from the game's files and a run's memory (the same
  moment, -ram and -vram at 55 s; with player 1's keys added `-key 48 d
  -key 49 d -key 50 lctrl+ -key 50.6 c+ -key 51.5 lctrl- -key 52 c-`):
  the window, two boxes, the numbers (T1479:030A: the type's hit values
  against land, air and sea, the unit's +0 halved, the armour, the
  ranges less 1, as the code reads them; whether they are what the
  manual calls them is not checked), the big picture and the name
  (T1479:0C93), the ground's hexagon with the unit over it and the
  cursor. All 24160 pixels drawn were in the video memory on both
  pages, for player 0's T-3 SCORPION (ground 3) and player 1's SC-T
  PROVIDER (ground 64); player 1's screen is 160 pixels to the right,
  as the status screen. Not run: a unit of the other player (text 1
  instead of the numbers), a unit that holds others (the word +4 with
  bit 40h), the message line at the bottom (T11FD:0103). The other
  callers of draw_shop_window (T1479:0AA6 and T13CA:002E, the save and
  load messages) are not run yet. MOON.hints did not get the name (not
  mapped). mapfiles.py's sorted_entries stops before BIGUNIT.DAT's MAA
  now (the loader's list does).
- A building's screen (T1479:0AA6, drawn by `screens.py --building` for
  the poked headquarters run below: window, the two boxes, the seven
  slots with their arrows (PATT.LIB's entry 2, from `make_path`'s name at
  F27EE:0A59), the title, and what T0708 draws after it (the big picture,
  the numbers box, the bar and number by the title, the cursor on the
  slot; T0708:2AF0..2F77) match the video memory in all 24160 pixels, on
  both pages; the unit screen's pictures were rechecked after sharing
  code): the
  cursor record in state +17h 2 with +18h 9 (T0708:2846..291B picks the
  record the screen shows: F27EE:259C + 1Ch * the building's number for
  a factory or depot, F27EE:24B0 + 1Ch * 0 or 1 for a square of ground
  flag 40h (the headquarters), F27EE:0B64 + 1Ch * n for a unit that holds
  others; in the record's +24h, +26h as far pointer, +22h 1 or 2). The
  record's first 7 bytes are the units inside (FFh none) for player 0's
  window, the next 7 for player 1's. Reached by a poke, not by keys (the
  cursor's start square is empty: fire there gives the status screen with
  down, nothing with the other directions; the route by keys to a
  building was not found): `-poke LT0708_4088#401 F27EE:26CB 02 -poke
  LT0708_4088#401 F27EE:26CC 09 -poke LT0708_4088#401 F27EE:26D6 0100
  -poke LT0708_4088#401 F27EE:26D8 B024 -poke LT0708_4088#401 F27EE:26DA
  6528` (about 46 s into the map; 6528 is the load segment 0077 plus
  27EEh, written as the run stores it) then `-log T1479:0AA6` ran once and
  the shot at 56 s showed the headquarters screen: a box and a column of
  seven slots on the left (T1479:0931: SHOP.LIB's entry 4 at each, the
  unit in it by UNIT.LIB, an arrow icon from F27EE:0A67 for some), the
  title "HQ" with a number, and below the box of the unit in slot 0 (a
  T-4 GLADIATOR, unit 10) with its numbers, in a box at 56, 125 (not the
  unit screen's place, 43, 124). T0708 draws the rest after 0AA6 (the
  state 2, +18h 0Ah loop from T0708:2A4C on: the choice among the
  slots, the costs and the messages by texts 03h..09h, T1479:028E and
  show_message): not read.
- The cursor and the map's clock (BATTLE.hints at copy_page): at the end
  of each pass of the map's loop (T0708:4385) each player's cursor is
  drawn, CURSOR.LIB's entry of the cursor record's +1Bh at its +10h,
  +12h, then `copy_page` (T2479:000A) copies one page to the other. The
  loop then waits until the tick count DATA:0304 reaches F27EE:251B, 4
  in a map; `timer_keys` counts it, added with a period of 4000h (72.8
  a second if that is the PIT's count, presumably): 18.2 passes a
  second. Two dumps 4 s apart had 397 and 470 passes (the long
  F27EE:251F): 18.25 a second. F27EE:251B is also the number in the .HI
  file's name, which after_map sets to the map's: the 04 of 04.HI after
  the poked end of map 16 was the loop's 4, presumably. `fade_in` and
  `fade_out` (T247D:0004, 002B) call set_palette at the levels 0, 4, ..
  252 and 255, 251, .. 3 (read, not run; 252 is the level seen on the
  map). MOON.hints got the eight names by xfer.py; not looked at in
  MOON.ASM.
- Player 1's keys (the table at DATA:0B50 in a run's memory): x left, v
  right, d and f up, c down, Alt and left Ctrl fire.
- doskit 11c6135 has `-keyat ADDR[#N] KEY+` and `-keysat ADDR FILE`: a
  key at the Nth pass of an address, not at a time. Not used here yet;
  with the loop's end (T0708:4385) as the address the key scripts would
  no longer hang on the timing.
- The runner read port 201h as F0h, axis bits that fall at once: a
  joystick held up and left. BATTLE.EXE took it for an attached one
  (DATA:0374/0376 = FFFFh) and its menu saw "up" all the time, so down
  did nothing. doskit's runner reads FFh now, as a PC without a joystick
  (doskit, with a test, GAMEPORT.EXE).

- The menus' texts (`screens.py --menu RAM --id N`, T1090:0EDE): the
  menu records are 6 bytes at F2740:0079 (item numbers, +5 the count),
  the items 2Eh bytes from F2740:00A3 (a word of flags, bit 20h not
  drawn; texts of 10 bytes from +2, the one chosen by +2Bh), a text is
  CHAR24.LIB's entries by byte less 3 (1 a gap; A is 17, 0..9 are 2..11).
  Menu 0 OPTIONS (FIRST, SETTING, PLAYER, OK), 1 DISK (LOAD, MOUSE,
  RATING, OK), 2 PLAYER (HUMAN/COMPUTER twice, OK), 3 the title menu
  (START, OPTIONS, DISK, EXIT), 4 SETTING (ALL SHOPS/HIDE SHOP, NO LIMIT/
  .. TURNS, PALETTE 1/.., OK; a digit's byte is the digit plus 5, entries
  2..11: the tool printed digits 3 too high before, the "7 TURNS" and
  "PALETTE 4/5" of an earlier note), 5 EXIT/CANCEL, 6 SIDE ONE/TWO, SLOW/
  MEDIUM, OK, each item's text at x 100, y 50 + 34 an item. In a run of
  the title menu (-key 14 space, -ram -vram at 40 s) the four texts were
  in the video memory pixel for pixel on page 1, and on page 0 but for
  EXIT's rows (not examined); the picture behind and the sphere cursor
  are not drawn, the other menus not seen on a screen.
  The routines are named in BATTLE.hints (draw_building, draw_slots,
  draw_bar, draw_menu, edit_text) and carried to DESERT.hints and
  MOON.hints by xfer.py: in MOON.hints draw_slots, draw_bar, draw_menu
  and edit_text were looked at in MOON.ASM and are the same routines
  (the menu records there are at F2850:007D, not 0079: the comments are
  BATTLE's addresses); draw_building was not mapped, and what MOON does
  with its building screen is not looked at.
- The menus' screen in full and their loop (`screens.py --menu RAM --id N
  --sel ITEM`, BATTLE.hints at `menu`, T1090:000E): MENU.IFF's upper left
  320 by 200 behind, the texts, and the cursor, CHAR24.LIB's entry 40 (a
  sphere) at x 66 and the chosen item's y. Four runs (space at 14 s, from
  33 s down and enter to the menu and down to an item, -ram -vram at 40
  or 42 s): the title menu on START, OPTIONS on SETTING, DISK on RATING,
  SETTING on its second item: all 64000 pixels in the video memory on
  the page shown. That answers the EXIT rows above: the game draws to
  one page (DATA:0350, A000 or A400) and shows the other (DATA:0352);
  each pass `restore_sprites` (T2467:0006) puts back what the sprites
  covered, from a list kept in video memory behind the two pages, the
  pass draws anew and `flip_page` (T259F:0004) changes the pages at the
  vertical retrace. A dump in the middle of a pass has the page drawn to
  half done (three of the four dumps). Read from the code, not run: what
  fire does by an item's flags (a submenu, the next text, a field for
  the code, an action), the code typed and looked up in CODES.DAT, LOAD's
  position (a digit, F27EE:2512) and messages, the sounds of MENU.FXX
  (moved, chosen, refused), and what the menu leaves for the map:
  F27EE:243E and 2455 bit 2 (that player is the computer), F27EE:250E bit
  1 (one computer), 4 (two), 2 (none), 8 (HIDE SHOP), the limit of turns
  (4, 8, 16 or FFh none) in F27EE:2454 and 246B, the highest score in
  F27EE:2593.
- The .HI file (`load_scores`, T1090:15F3; `show_scores`, T1090:12B7;
  read, not run but for the poked end above): 28h bytes, four longs (the
  scores, held within 0 and 7EF4h) and from +10h four names of 6 bytes
  (5 letters typed), a file a map in MAP. After a map the menu loads
  the file numbered F27EE:251B and, when the score (the long F27EE:2597,
  the status screen's ACTUAL) is above the lowest of the four, asks for
  the name ("TYPE NAME FOR TOP FOUR"), puts it in the lowest's place,
  saves and shows the four from the highest down; RATING in the DISK
  menu shows the current map's. xfer.py carried menu, show_scores,
  load_scores, restore_sprites, flip_page, clear_page and wait_retrace to
  MOON.hints (their beginnings looked at in MOON.ASM: the same);
  T1090:110F went to another routine there and has no name for that.
  The scores' screen in a run (`screens.py --scores RAM [--hi FILE]`;
  the DISK run's keys, then enter on RATING at 39 s, -ram -vram at 44 s,
  -dos): without a .HI file (00.HI in MAP was asked for when the menu
  began, at 15.5 s, and is not there) the code FIRST and four times
  00000 EMPTY over MENU.IFF, all 64000 pixels in the video memory on the
  page shown. A file's scores and names, and the name typed after a
  map, were seen in later runs (below).
- The menus' other screens in runs (`screens.py --menu`; space at 14 s,
  the keys from 33 s, -ram -vram at the end, all 64000 pixels in the
  video memory on the page shown in each): PLAYER (`--id 2 --sel 0`:
  HUMAN, HUMAN, OK; keys 33 down, 34 enter, 36 down, 36.5 down, 37.5
  enter, until 41), EXIT/CANCEL (`--id 5 --sel 0`; 33, 34, 35 down, 36
  enter, until 40), a code typed (`--id 0 --sel 0`; 33 down, 34 enter, 36
  enter, 37 c, 37.5 o, 38 n, until 41: the item hidden by flag 20h,
  edit_text draws CON at the item's place and the sphere, CHAR24.LIB's
  entry 40, right after it, the cursor's sphere still left of it), and
  LOAD's messages (`--message position`: SELECT / POSITION / 0 TO 9 at x
  64, y 50, 84, 118; 33 down, 34 down, 35 enter, 37 enter, until 40;
  `--message insert`: PLEASE / INSERT / DISK at x 87, y 55, 91, 127; the
  same with 39 0, until 42), both over the picture alone. Not run: the
  mouse's menu (6), the items' other texts (COMPUTER, the limits, HIDE
  SHOP), a wrong code's return to the old one.
- The name for the scores and a .HI file in runs (`screens.py --menu
  --message name --typed TEXT`, `--scores RAM --hi FILE --map N`): the
  poked end of map 16 (the CONRA keys, the two pokes at
  LT0708_4088#401, space at 70 s; each run with its own `-state` folder)
  has "TYPE NAME FOR TOP FOUR" up from about 84 s (MAP\04.HI asked for at
  80.9 s); the score, the long F27EE:2597, was 495. Five runs, all 64000
  pixels in the video memory on the page shown in each: nothing typed
  (-ram -vram at 90 s), h, a, n typed (86, 87, 88 s; at 91 s: HAN and the
  sphere after it at x 102, y 164), the name HANS and enter at 89 s (at
  95 s: the game created MAP\04.HI, 28h bytes, at 89.0 s, and shows 00495
  HANS and three times 00000 EMPTY), the same once more over that file
  with OTTO (the file then holds both, each 495; HANS, the file's first,
  is shown first), and RATING with the first file put in as MAP\00.HI
  (`-put`; the DISK run's keys, at 44 s: FIRST, 00495 HANS). The file is
  as read from the code: four longs, four names of 6 bytes in the
  menus' character codes; a new name takes the place of the lowest
  score's (the first of equal ones). The code on the scores' screen is
  CODES.DAT's record of show_scores' second argument, not the menu's
  text: the chosen map (F27EE:2523) from RATING, F27EE:251B after a map,
  so the poked end showed EAGLE (record 4) for map 16. Not run: a name
  of fewer than 5 letters over a longer one, a score not above the
  lowest (no question, as read), backspace, a real end of a map.
- The buildings' screens by keys (`screens.py --building`; this corrects
  the headquarters item above, which found no route by keys and gave
  259C for a factory or depot): fire on a building's square with left
  held opens its screen (BATTLE.hints at draw_building: the first pass
  with fire sets bit 400h of the cursor's +1Ch on ground of flag 80h,
  200h or 40h, left is then function 0Ah, state 6 and after it state 2
  with +18h 0Ah). On ISLE's map 03 (code MARSS: the CONRA keys with m, a,
  r, s, s; two players, the map up at about 50 s; `mapfiles.py --grid
  03`) the cursors start at (5, 6) and (17, 6), a square below each
  headquarters (5, 5) and (17, 5); player 0's depot is at (6, 14), a
  factory of nobody's at (5, 19). A tapped key (0.15 s) moves the cursor
  two squares, a key held 0.06 s one (0.03 s: up one, right none); from
  the third down on the window scrolls and each down is two rows. The
  keys, each `KEY+` and 0.06 s later `KEY-`, then `space+`, +0.6 `left+`,
  +1.5 `space-`, +2 `left-`, -ram -vram 5 s later: the headquarters 52
  up, fire at 54; player 1's with it 52 d, 54 lctrl+, 54.6 x+, 55.5
  lctrl-, 56 x-; the depot 52..56 down, 57 right, fire at 59; the factory
  52..59 down, 60 up, fire at 62. All 24160 pixels drawn were in the
  video memory on both pages in each: the headquarters with seven empty
  slots (title HQ, the number 20, FREE PART in the numbers' place: text
  2, added to the tool), the depot (record F27EE:259C, flags 10h, DEPOT
  15, an R-1 DEMON in slot 0) and the factory (record F27EE:2780, flags
  0Ah, FACTORY 35, a T-3 SCORPION), the cursor record's +22h 1 in all.
  The message line below the window (a number and the unit's second
  name) is not drawn by the tool. Not run: moving among the slots, what
  fire does there (the loop from T0708:2A4C: not read), a unit with bit
  400h or 800h (texts 8, 9), +22h 2, HIDE SHOP, how a unit is built or
  taken out.
- The building's screen in use (BATTLE.hints at draw_building has the
  loop, T0708:2928..3C75, state by state; `screens.py --building` draws
  what it leaves): up and down choose a slot; fire held shows what a
  direction would do by the cursor's picture, and fire let go with the
  direction still held does it: right leaves; up, in the move phase,
  takes the slot's unit out onto the map to be moved (while the player
  has turns left: message 21h otherwise, read); down, in the attack
  phase, repairs the unit for 3 of the building's energy (19h for some
  units; the number by the title is that energy, the record's +16h +
  player); left, in the attack phase on an empty slot of a factory the
  player owns, opens the list of the types the energy pays for
  (list_makeable), where up and down move and scroll, fire and left
  builds the type into the slot for its cost and fire and right goes
  back. Runs: ISLE's map 03 as before (player 0, in the move phase, at
  his depot: 52..56 down, 57 right, fire with left at 59; player 1, in
  the attack phase, at his, (20, 14): 52..56 c, 57, 57.5, 58 v, lctrl with
  x at 60) and map 14 (code DEMON; player 1's factory at (43, 32): x at
  52, 52.5 .. 54.5, c at 55 .. 57, lctrl with x at 59; a key held 0.06 s
  each). Then: down twice (slot 2, FREE PART); fire at 64 with up: +1Ch
  12h and picture 4 while held, then the unit out (its slot FFh, state 0,
  +18h 16h); with right: the map again; with down in the move phase:
  nothing; player 1's lctrl with c: +1Ch 102h, picture 8, and for a whole
  unit message 1Eh NOT DAMAGED ! (on the line when c and lctrl are let go
  together; with c still held the slot moves and the line is cleared);
  in the factory c at 64 (an empty slot), lctrl with x at 66: the list
  (ten types for the energy 15), c twice or eight times (scrolled by 2),
  lctrl with x at 70: the unit built (make_unit once, unit ACh in slot 1,
  the energy 5, the player's units 54h from 53h, no type left for 5),
  lctrl with v at 70: back, nothing changed. `screens.py --building` had
  all 25312 pixels of each of 14 dumps in the video memory (the message
  line with them now; one dump taken with fire held had one page in the
  middle of a pass). A message's text is not in the memory, only the
  player's bit 4 or 8 of F27EE:250C while it is up (about 1.4 s: 19h
  passes, presumably; the bit was set 0.5 s and clear 1.5 s after):
  `--line 1E` names it, `--line clear` draws the empty line after it.
  A slot of a building is two bytes, one for each player's window (+0..6
  and +7..13): a unit built or taken out changed only the acting
  player's; what the other's is for is not read. A new unit has 11 at
  +9 (the number before "th" on its line), the maps' units 1, 3, 11.
  Not run: a repair that repairs (messages 18h, 1Fh), no turn left
  (21h), a unit that holds others (+22h 2), the unit moved after it is
  out (+18h 16h on), EXP.LIB's entries on the unit's line (+1 of a unit
  not 0), the same in DESERT and MOON (MOON.hints got unit_line,
  draw_type_list and draw_unit_numbers by xfer.py; their beginnings
  looked at in MOON.ASM: the same routines).
- The runner writes no shot asked for at the time of `-until` itself
  (seen again: `-shot 70` with `-until 70` gave no file, `-shot 69.8`
  did): the run ends before the shot's turn, presumably. Open question
  15 is that.
- The fight (`tools/fight.py`, BATTLE.hints at fight_reckon): the formula
  is read in full and done again by the tool from a run's memory. When
  the phase changes, T0408:000B goes through a player's attack orders,
  fills the fight record F27EE:2716 (the two units, their types, the
  ground records of their squares, whether they stand next to each
  other, two percentages) and runs the fight scene (`fight_step`,
  T1F3C:000A), whose first call reckons the outcome (`fight_reckon`,
  T2190:000D). Each side has an attack (its count times its type's hit
  value against the other's class) and a defence (count times armour),
  both raised by the unit's experience (+1) and moved by the ground it
  stands on (two signed bytes a ground scene at F2D65:000E); the
  attacker's attack less the target's defence, divided by the target's
  armour times a factor of the attacker's experience, is what the target
  loses, and the other way round; at least 1 when the attack is above
  what one of the other's withstands, at most twice the firing side's
  count, then one more or less by a random number (srand with the two
  units' addresses XOR the count of the map loop's passes, so the same
  for the same units in the same pass). The target's defence is reckoned
  with a divisor of 512 where the three others have 256 (kept as it is).
  A target that is not next to the attacker, cannot fire at its class or
  has 40h or 80h in its targets word does not answer: the attacker loses
  nothing. The two percentages (`fight_bonus`, T0408:29C6) are for units
  standing around: the attacker's hit value rises by the bytes
  F27EE:000E.. for each unit of its side beside the target that can fire
  at it, the target's armour by 80 (at most 150) for each unit of its
  side on the two squares beside both. After the fight a unit left with
  0 is taken off the map, a unit that holds others passes its losses on
  to those inside, and the experience rises by one for losses caused and
  by one more for an enemy destroyed, to 6 at most. Checked: the battle
  against the computer (the CONRA keys, the change of phase asked for
  every 20 s; ten fights in 600 s, at 154, 158, 194, 198, 202, 294, 374,
  454, 458 and 494 s), each fight stopped at fight_reckon's entry and at
  its end (`-break fight_reckon#N`, `-break LT2190_0ABA#N`, -ram):
  all ten give both units' counts and both percentages as the tool
  reckons them (losses of 0 to 6, percentages 0, 25, 50, 75 and 80,
  three units destroyed; the units T-3 SCORPION, T-4 GLADIATOR and R-1
  DEMON, all on ground of scene 6, all next to each other and
  answering). Not seen: a
  target that does not answer, air or sea units, ships (the count in
  +3), other ground scenes, a unit that holds others in a fight. ISLE's
  armours are 20 to 100, so the divisor 0 (armour 0, or 1 against an
  attacker of experience 6: a divide error) does not come up there.
- Moving and firing (`tools/moves.py`, BATTLE.hints at T0BA0): a type's
  move is points, not squares. `reach` (T0BA0:0323) gives each square of
  the map a cost for the unit (`cost_map`, T0BA0:098F: the ground's +3,
  or +4 for an air unit, never where the type's ground mask and the
  ground's do not meet, never onto a unit of the other side or of
  nobody, 2 into a unit of the own side that holds others) and spreads
  the unit's points from its square; a square beside an enemy is entered
  with nothing left, so a unit stops there. The squares with 0 or more
  left are marked in F27EE:1339 (68 rows of 64 bytes: bit 1 or 2 in
  reach by side, 4 or 8 a target). `find_path` (T0BA0:0E2E) then finds
  the way over the marked squares, best first by the distance to the aim
  and the ground's cost with the type's two weights. `fire_reach`
  (T0BA0:05C8) marks the units of the other side within the range (one
  less than the type's number) whose class the type's targets name; a
  type with 40h there cannot fire at the six squares around it. All
  three are done again by moves.py from a run's memory: reach in two
  runs (a unit out of the depot on map 03; fire with up on a unit of the
  first map: `-key 48 up -key 49 up -key 50 space+ -key 50.6 up+ -key
  51.5 space- -key 52 up-`, -ram at 56 s), the buffer's 1104h bytes and
  all marks the game's; find_path in six calls of the battle against the
  computer (`-break LT0BA0_138A#N` for N 1, 2, 3, 6, 7, 12; paths of 2
  to 14 squares), fire_reach in six (`-break fire_reach#N` and
  `-break LT0BA0_0988#N`, N 1..6), each as the game's. So fire with up on
  a unit of one's own in the move phase chooses it to be moved. Not
  seen: an air unit, a unit of two squares, a path not found, a range
  above 2. Read, not run: `stop_check` (T122D:0F50; BATTLE.hints has it
  in full), which says what stopping on a square would be: a plain move,
  taking a unit in, going into a unit of the own side that holds others
  or into a building (by the slots free and the types' sizes and room),
  taking a building that is not the side's, or a message why not (run
  in the next item but one). Not read: what the ground flags 3 and 6
  that reach's callers give stand for (the other player's buildings,
  presumably).
- xfer.py carried the new names to DESERT.hints (all) and MOON.hints
  (neighbours64, cost_map, reach, fire_reach, find_path, list_reach,
  fight_step, flankers, mod6, srand, rand, count_slots; not mapped
  there: clear_marks, neighbours, off_map, square_distance, fight_bonus,
  same_side, random, stop_check, cargo_size, find_building). MOON's were not looked at beyond the first lines
  of reach, find_path and flankers (the same stack frames).
- A move carried out, the change of phase and a map's end
  (`tools/turn.py`, BATTLE.hints at change_phase and move_step; the
  tool's docstring has the rules in full). Each player has a map of his
  own (F27EE:4152 and 4156) and every unit a square and a direction in
  each: what a player does is done in his map at once, and the other's
  gets it when the phase changes. So an earlier note is put right: only
  the attacks are carried out at the change, a move right away. A unit
  is chosen with fire and up on it (or taken out of a building), the
  cursor put on a square in reach and fire pressed: T122D:02D8
  (`move_aim`: stop_check, find_path, the path marked); fire again on
  the same square: T122D:05B8 starts the move, and `move_step`
  (T122D:0713) takes the unit a square further each time timer 4 runs
  out, after the type's +16h passes of the map's loop (UNIT.DAT's +16h
  is the pace of a move, not a sound). At the aim `move_arrive`
  (T122D:0ABD) does what stop_check found: a plain move, a unit taken in
  or gone into, a building of the own side entered, or one of the other
  side or of nobody stood on, which is taken when the phase changes.
  `change_phase` (T0408:000B) runs when both players asked and F1 is
  pressed: the fights of the attacking player's orders, the dead units
  removed (a dead unit's move taken back), the moves' ends (a unit taken
  in changes its player; a building taken: the units inside and the
  building change their player, its square's ground comes from
  AMOK.DAT, a headquarters makes the routine return the taker), the
  slots and energy of the buildings and the mover's units copied into
  the attacker's view and that map copied to the mover's, the modes
  exchanged, the units' flags cleared (a type with flag 1 in +0Eh,
  ISLE's 0 and 7, keeps "has moved" for the phase after), the end's
  test (a player with no unit that counts has lost: results 11h and
  12h, 15h both for none at all) and the score: the armour of all units
  that count, of both players (+ 100 with HIDE SHOP, times 4, 3, 2 for a
  limit of 4, 8, 16 turns). All of it is done again by turn.py from a
  run's memory at a routine's entry and compared with the memory at its
  end, byte for byte over the units, both maps, the cargo and building
  records, the players' counts, the marks, the types' serials, and for a
  change the cursors' modes, states and results, the round, the score
  and the last fight's record. Runs, all the same in every byte (the
  keys after the MARSS or CONRA keys above, a key held 0.06 s unless
  said):
  - ISLE's map 03, the unit out of the depot (52..56 down, 57 right,
    fire with left at 59, fire with up at 64), 68 down, space at 70 and
    at 74 (0.3 s each): two steps (`-break LT122D_0713#1`, `#2`, the end
    `-break LT122D_0AB6#N`), kind 3; then fire with right at 78 (the
    cursor is back in the depot's screen) and at 82 the change (space+,
    82.6 left+, 83.5 space-, 84 left-, 85 lctrl+, 85.6 x+, 86.5 lctrl-,
    87 x-, 89 f1; `-break LT0408_000B#1`, the end `-break LT0408_23F4#1`).
  - The unit at (6, 5) into its own headquarters (5, 5): 52 right, 53
    up, 55 space+, 55.6 up+, 56.5 space-, 56.53 up- (up let go right
    after fire, or the cursor runs on upwards), 59 left, space at 61 and
    64: one step, kind 5; 67 down, the change at 69.
  - The other player's headquarters taken: the unit 10h put on (17, 6),
    below player 1's headquarters, by pokes (`-poke LT0708_4088#401
    F27EE:2A43 43014301`, and the unit byte of its old and new square in
    both maps: 31F99 FF, 31FDF 10, 33F9D FF, 33FE3 10, linear), the
    cursor there by 64..71 right (a tap a second: 1, 1, 1, then 2 a tap)
    and 73 left, 75 space+, 75.6 up+, 76.5 space-, 76.53 up-, 79 up,
    space at 81 and 84: one step, kind 4; 87 down, the change at 89:
    change_phase plays ANIM\br in the taker's half of the screen
    (play_anim 1) and returns 0. Then "VICTORY !! HQ IS YOURS" and "YOU
    LOST YOUR HQ !!", after a key ANIM\qa, after_map, MENU.IFF and the
    name for the scores.
  - The battle against the computer (fights.keys): the changes 5, 7, 11,
    15, 19, 21 and 22 (at 154, 194, 294, 374, 454, 494 and 515 s; 10
    fights, three units dead; 22 changes in all, one every 20 s) and
    the 6th (175 s, no fight): in the 22nd the computer's unit takes the
    player's headquarters and the routine returns 1, a unit inside
    becomes the computer's.
  Not seen: a unit of two squares, a unit that holds others moving, a
  unit taken in or gone into (kinds 1, 2), a full building taken (a
  factory and a depot taken are seen in the second battle, below), the pioneers' orders (0Bh, 0Dh; the tool stops at
  0Dh), a repair (8000h in a unit's +6: where it is set is not read), a
  map's end by units (11h, 12h, 15h), a limit of turns, the same in
  DESERT and MOON. Not read: where the limit of turns counts, the
  animation's records.
- An attack order by keys (BATTLE.hints at give_order): in the attack
  phase fire with up on a unit of one's own marks its targets
  (fire_reach), fire on a target gives the order (`give_order`,
  T0B70:000E: the unit's +11h the target's square, +13h 9, +14h 2, the
  unit into the player's orders). Run: unit 10h put on (17, 3) among
  player 1's units by pokes (as above, the squares 31F4F and 33F53),
  `64:phase`, the cursor by 76..83 right, 85 left, 86, 87, 88 up, then 90
  space+, 90.6 up+, 91.5 space-, 91.53 up-, 94 down, space at 96, the
  change at 99: the second change fights unit 10h against unit 0Dh (6 ->
  3 and 6 -> 5) and turn.py has every byte as the game's, with player 0
  attacking this time. What fire offers on a square (the cursor's +1Ch)
  and the choice of a unit to move or fire are read (BATTLE.hints
  there); the pioneers' orders are read, not run.
- The computer player (`tools/computer.py`, BATTLE.hints at
  computer_step; the tool's docstring has the rules in full). It is a
  state machine of which the map's loop does one small step a pass
  (`computer_step`, T178C:000D), in five stages: it assesses (the two
  sides' strength, how much each enemy unit threatens, a list of aims:
  the enemy's buildings, squares by the own headquarters, units of
  nobody), plans (a task and a score for every unit: the nearest fitting
  unit to each aim, weakened units to a building, the units that fire
  from afar, up to four units around the enemy that threatens most, the
  rest towards an enemy or after a unit with a task), hands the tasks
  out as commands (a unit to a square, a unit fires at a unit, a unit
  out of a building, a repair, a type made in a factory, the change of
  phase), and carries each command out by steering its own cursor: it
  makes up directions and fire (the word F2C0A:11EE) that the map's loop
  takes as it takes a player's keys, so the computer moves, fires and
  builds through the routines read before (move_aim, give_order, the
  building's screen). The map's .COM file is the types' worth and flags
  for it (27 records of 6 bytes at F2C0A:0000: +0 the worth of a type as
  a threat, +2 flags: 1 a type that takes buildings, 2 one that fires
  from afar, 20h one that wants company, 40h one that guards, 80h..200h
  those that carry, 400h one the computer builds; +4 how many go with
  it; read from the code's use, the flags' names are the tool's).
  tools/computer.py does the first three stages again from a run's
  memory, call by call, and with `--calls` a whole visit of a stage from
  one memory (the calls up to the one that returns 1). In the battle
  against the computer (fights.keys; `build/scratch/aic.py STAGE ENTRY
  EXIT N` stops a run at the Nth entry, lets the tool say how many calls
  the visit has and stops a second run at that call's end):
  - the assessment (`-break LT17C0_0A0D#N`, the end `LT17C0_177C#N`):
    the calls 1..8, 11, 12, 19, 20, 27, 28, 43, 44, 59, 60, 75, 76, one
    by one: all the records and the aims as the game's;
  - the plan (`LT1C04_000D#N`, `LT1C04_2C58#N`): the twelve visits from
    the calls 3, 90 (the attack phase: 2 calls), 92, 191, 312, 432, 544,
    643, 731, 816, 911 and 1000 (83 to 119 calls each, the game's
    numbers): every byte of the units' records, the aims and the plan's
    own values as the game's;
  - the commands (`LT17C0_0004#N`, `LT17C0_09C2#N`): the chains from the
    calls 1, 8, 44, 51 and 230 (2 to 20 calls), each up to the command it
    adds: the records, the queue of commands, the path's length and the
    record of stop_check as the game's.
  A second battle, ISLE's map 24 (code MAGIC, `build/scratch/magic.keys`:
  the same keys with the code and without the two lefts; 24 by 24, four
  factories and a depot of nobody, 23 units of the computer): the plan's
  visits from the calls 3, 435, 894 and 1340 (430, 457, 444 and 462
  calls, the game's numbers), the commands' chains from 8 (68 calls),
  299, 926 and 932 (78 calls, a move running meanwhile), the assessment's
  calls 3..7: all as the game's; and its first ten changes of phase with
  turn.py (the computer takes a depot and two factories, three changes
  with fights, a unit dead): every byte the game's, which adds a factory
  and a depot taken to what turn.py was seen with. While a move runs the
  map's loop carries it on between the calls, so the tool leaves the
  units, the marks, the path's length and the move's record out of such
  a chain's comparison and ends the chain before a call that only waits
  (the chain from 616 of this battle, done before the tool did so, had
  only such differences; it was not done again: the machine ran short
  of memory and the last runs were stopped, as were the chains from 283
  on of the first battle).
  Not seen in the two battles: aims of kind 4 (units of nobody), a unit
  that needs another to the end (T1ED2:0005 was called, its steps 5 and
  6 not looked for), what the computer builds (the random choice: no
  factory was the computer's with energy in the attack phase as far as
  seen), ships and aircraft, the tasks 5 and 6 (units in buildings).
  The cursor's steering is the next item. Read, not run: what the
  human's request for the change does to it (F27EE:250E bit 80h,
  T0708:1480: computer_step in a loop of its own, BATTLE.hints). MOON.hints got the names by xfer.py
  (computer_step, computer_assess, computer_plan, computer_hand_out and
  their helpers, in MOON's own segments in the same order; not looked
  at).
- How the computer carries its commands out (`tools/computer.py --carry`
  and `--script`, BATTLE.hints at command_step; the tool's docstring has
  the seven commands step by step). A command is a row of steps, one a
  call of `command_step` (T1938:012F), each of which waits or writes
  keys into the player's script; `script_step` (T1938:000F) plays the
  script a byte a pass into the word the map's loop takes as that
  player's keys (T0708:155C: bits 1, 2, 4, 8 up, down, left, right, 10h
  fire; a script byte 80h..83h is fire with a direction for one pass and
  a pass of nothing). The cursor goes to a square step by step, left or
  right and up or down at once (`cursor_steer`, T1938:157E); to a square
  five or more columns or rows away by the overview: fire with right,
  the overview's window steered until the square is in its middle, fire
  (T1938:1245, 140D). A unit is moved as a player moves it (fire with up
  on it, the square, fire twice), fires the same way, comes out of a
  building by the building's screen (fire with left, its slot, fire with
  up, the first square list_reach gives, fire twice, fire with right), is
  repaired there (fire with down) and made there (a free slot, fire with
  left, down the list to the type, fire with left); for the change of
  phase the cursor goes to the square below the own headquarters and
  fire with left asks for it. A choice that cannot be made (the square
  not in reach, stop_check's message, the target not marked, no square
  to stop on) sets bit 8 of the player's record. Two things read on the
  way: computer_step's stages follow one another within a call (a
  command handed out is begun and its first key played in the same
  pass), and it sets the script's place to 0 before script_step.
  Checked, a call each, the memory at the routine's entry against the
  one at its end (the scripts and keys, both players' records, both
  cursors, the commands, their own values at F2D33:001F, the aims, the
  marks, the units, the move's record): 48 calls of command_step in the
  first battle (fights.keys; the calls 1, 2, 4, 5, 7, 8, 30, 416..431,
  465..490, 1089..1103, 2676..2690, 2984, 3013, 3421, 3422, 3567) and 25
  in the second (magic.keys; 454..458, 844..853, 2384, 2494, 3989..4050,
  4471, 5485, 5499), chosen from a log of every step's label
  (`build/scratch/cslog.py`) so that each step of each command is among
  them: the commands 1, 2, 3, 4 and 6 in the first battle, 7 in the
  second (the computer made a unit in a factory at about 350 s, four
  times in 600 s: what the item above had not seen), every step of
  theirs and of the cursor's way by the overview but those named below;
  and 18 calls of script_step (the first battle's 1, 2, 440..453, 1150,
  1151: directions, fire, a row of fire with up, the script's end). All
  as the game's in every byte. Not seen: command 2's step 4 (a target
  not marked) and any choice given up (bit 8 was set in none of the
  calls), command 5 (no caller hands it out as far as read), a holder
  not found, the script bytes 6..9 and 84h (in the tables, written by
  no step). Only single calls are done again: between two the map's loop
  moves the cursor. xfer.py carried command_move, command_fire,
  cursor_steer and find_holder to MOON.hints (their beginnings looked at
  in MOON.ASM: routine starts); five others it put 8 to 10h bytes before
  MOON's routines and are left without a name in BATTLE.hints (the
  comment there has both programs' addresses).
- The fight scene (`tools/scene.py`, BATTLE.hints at fight_step; the
  tool's docstring has the rules in full). change_phase loads FIGHT.LIB,
  BUM.LIB, RAND.LIB and FIGHT.FXX before a change's first fight (-dos)
  and calls fight_step once a pass (2 ticks) until it returns 1; each
  pass ends with flip_page, copy_page and restore_sprites. The scene is
  in the attacker's half of the screen: the first two calls draw the
  ground (FIGHT.LIB's pieces by the two squares' ground kinds, the
  target's above; colour base 5Fh; black columns of random height
  between the two when the target is not next to the attacker) and the
  frame (RAND.LIB); then the units come in (UNIT.LIB's entries, the
  map's own sprites, as many as each side's count, each along a script
  of places, directions and lengths chosen by the unit or its ground, at
  a pace by its type); then every unit fires a shot (BUM.LIB) at one of
  the other side, as many hitting as the other side lost, the others
  aimed off at random, each after a random wait; a shot ends in an
  explosion of six pictures and a unit that is hit is no longer drawn.
  A fight of 6 against 6 took 62 calls, about 2 s. scene.py does a call
  again from a run's memory at fight_step's entry (the reckoning of the
  first call by fight.py) and compares with the memory and the video
  memory at the next entry: the scene's values (F2D37:0008..0284), the
  count of calls, both units' records and rand's long, and every pixel
  it drew on the page shown (27208: the half's ground, frame and
  sprites). Runs (the battle against the computer, fights.keys; `-break
  fight_step#N`, -ram -vram; `build/scratch/fsc.py N..`, then `fsv.py`):
  - the first fight (154 s; T-3 SCORPION 6 against 6, ground kind 6,
    next to each other, 5 and 5 left), the calls 1..5, 20..45, 50, 55
    and 58..62 of its 62 (38): all the same, bytes and pixels;
  - the same fight with values poked in before its first call (`-poke
    LT0408_0981#1 F27EE:OFF HEX`; `build/scratch/fplan.py NAME OFF=HEX..`
    does the whole scene on a copy of the first dump to choose the
    calls: the first three, some of the coming in, the shots' first
    calls, some of the flight and the explosions, the last): not next to
    each other (the record's +22h 0: the columns, the target does not
    answer); the attacker an XA-7 RAVEN in the air (type 10 and 10h in
    its +4: the shadow, the target cannot fire at its class); the
    attacker a W-1 FORTRESS (type 21, 8 in its +4, counts 1 and 6: two
    squares, the sea's script, one unit against six, three hits: one
    shot and two that are not drawn); the same as the target; the
    target's count 2 (6 shots at 2 units, both gone) and the attacker's
    2; the grounds' kinds 4 and 5 with 4 and 20h in the units' +6 (the
    script 128h: the units stand at once), 2 and 7, 5 and 1, 7 and 3 (the
    pieces between two grounds, the other scripts): 5 to 11 pairs of
    calls each, 90 in all, all the same, bytes and pixels.
  Not seen: a fight of other units than T-3 SCORPION without a poke, a
  target that answers nothing by its own type, shots of the target that
  are not drawn, MOON's and DESERT's scenes (MOON's entries are packed:
  draw_packed). Not done: the sounds (the scene writes records of 6
  bytes behind the far pointer F2D8A:000C and calls CODE:16F8; the
  type's +41h..+43h: BATTLE.hints), the squares' explosions on the map
  after the fight (change_phase, F27EE:2472).
- The map's whole screen (`tools/screens.py --field RAM`, BATTLE.hints
  at draw_marks): GAME.IFF, each player's window of his own map (the
  cursor record's +2 its first square; draw_window), the marks over it
  (`draw_marks`, T0E9B:0C65: PATT.LIB's entry 1 for a mark of C0h, 2 for
  one of 30h, entry 0 in the player's colours for one of 0Fh, a unit's
  reach and targets), the screen the cursor's state names over the
  window (a building's, the status, a unit's), the line below and the
  cursor. The line of a window showing the map is read too
  (T0708:1607..1829): with no key held and the cursor at rest on a unit,
  that unit's line (unit_line; of the other player's units too, unless
  hidden), kept up by a timer of 5 passes; in the attack phase the aim
  of a unit that has an order is marked for that time (`redraw_square`
  with the mark set for the call). Single squares are drawn again by
  `redraw_cursor_square`, `redraw_square` and `undraw_marks` (the marks
  taken off the screen and out of the memory). Checked against 44 dumps
  (the 42 map dumps of the earlier steps still in build/, ISLE's maps
  00, 03 and 14, and two new ones: the attack order's keys above with
  the pokes of that run, -ram -vram at 93 s, the targets marked, and
  with 97.5 up after the order, at 100 s, the cursor back on the unit
  and the aim marked): all 64000 pixels on the page shown in 43. The
  one: a depot's screen that came up when a unit moved in had 15 pixels
  of row 179 of another ground: a screen over the window does not draw
  that row (the clipping at 179, which draw_unit24 and draw_hexagon do
  not have: the odd columns' squares end in it), so it keeps what the
  window had before, and the tool draws the map as it is now. p1msg
  needs `--line1 1E`, p1msg2 and p1msg3 `--line1 clear` (a message's
  text is not in the memory). Not seen: marks of player 1, what the
  marks 40h and 10h stand for (T0708:3297 sets 40h or 80h), a unit with
  2 in its +6 (hidden from the other side), the squares' explosions
  after a fight, scrolling as such (the windows at other first squares
  are seen), why the page drawn to lacked the aim's mark in its dump.
  xfer.py carried the four names to MOON.hints (T0F3E:0E2A, 0F9D, 1208,
  141C: routine starts with the same stack frames, looked at in
  MOON.ASM). Scratch: `fieldall.py` (--field on every dump), `fat.py`
  (the two new runs), `fdiff.py NAME` (the pixels that differ).
- The overview over a window (the cursor's state 3; BATTLE.hints at
  draw_overview, `screens.py --field`): the map's loop puts the picture
  in the window's middle (the record's +4, +6), draws draw_shop_window
  and draw_overview and makes the cursor a frame (CURSOR.LIB's entry 6)
  over the part the window shows; the record's +0Ch, +0Eh are the
  window's first column and row meanwhile (the cursor's own kept in +8,
  +0Ah), a direction moves them by two within the map, fire ends it and
  the window is drawn from there. Runs of map 03 (scratch `ov.py`: fire
  with right at 52 s for player 0; then down twice and right three
  times; fire with v for player 1, then d and x, which are his right,
  up and left; down, right and fire): all 64000 pixels of both pages in
  the four dumps, and the 44 before as they were. The picture is taken
  from the game's .PMP that the memory holds (its pointer is in the
  loop's frame). The frame came up 4 steps right of the window's place:
  the direction is still held when fire is let go. Not seen: a dot in
  AMOK's +20h colour (a unit with 200h in its +4), MOON.EXE's overview.
  Not read: T24D3:0000, which the loop calls after a move of the frame
  (a stored block to the page drawn to; what was under the cursor,
  presumably).
- What chooses the cursor's entry (the record's +1Bh; BATTLE.hints at
  "What fire on a square offers"): at rest 5; while fire is held 1, or
  by the direction and what the square offers (+1Ch) 4 or 0 (up: a move
  or an attack of the unit there), 2 (down: the status or the unit's
  screen), 9 or 0Ah (left: the change of phase, or a building), 3
  (right: the overview); letting fire go sets the state the entry
  stands for, and the overview's frame is entry 6. Runs of map 03 with
  fire held (scratch `ce.py`): the entries 1, 2, 3, 9, 0 and 4 were in
  the memory as read and on the screen as `screens.py --field` draws
  them (all pixels; in the run for 4 its unit is poked in and the other
  player's window, not drawn since, lacks it: 144 pixels). Not seen:
  0Ah over the map, message 21h (the turn's limit reached, presumably:
  the counts' +15h against +16h).
- A unit under way in a move on the screen (BATTLE.hints at move_step):
  nothing of its own. move_step takes the unit a whole square on and
  draws the old and the new square again (redraw_square), so the screen
  is the map as the memory has it, and the path move_aim marks has the
  reach's bit and looks as the reach does. Runs of map 03 (scratch
  `mvw.py`: the cursor to unit 00 at 3, 1, fire with up, down held
  twice for 0.15 s, fire at 70 s and at 72 s; dumps at 69.5, 71.5, 72.2
  to 73.4 every 0.2 s, 80): the reach, the path, the unit two of its
  three squares on (F27EE:2510 C0h) and at the aim; `screens.py --field`
  had all 64000 pixels of the page shown in each, those after the
  arrival with `--line0 6` (the message is not in the memory). The page
  drawn to lacked 115 pixels in player 1's window in the dump with the
  path (not looked into). Seen on the way and not looked into: with a
  unit chosen (the cursor's +18h 16h) taps of 0.06 s did not move the
  cursor, keys held 0.15 s moved it one or two squares. Not seen: a
  unit of two squares under way, a move that leaves the window (whether
  the window follows).
- The squares' explosions after a fight (BATTLE.hints at "The fights",
  `screens.py --field RAM --explosions N`): while the fight scene has
  the attacking player's half, the other player's half shows his map
  around the attacker (in the run), the attacker's square hatched (the mark 40h or
  80h, PATT.LIB's entry 1) and the target's ringed (1 or 2, entry 0);
  change_phase sets these marks before the fight and clears them. When
  a unit is gone, its square shows the ground and BUM.LIB's entries 5
  down to 1, one every 4 ticks, then the bare ground (unit_explode's
  records at F27EE:2472). Runs (scratch `ex.py`: atc.keys with at.pk's
  pokes and `-poke LT0408_000B#2 F27EE:29EC 01`, the target's count 1;
  `-break LT0408_25C8#1..6` and `LT0408_0E4A#1`, -ram -vram): player
  1's half, 32000 pixels, the same on both pages in all seven with
  `--explosions 1 --line1 unit:D` (the line is the target's, by
  unit_line in change_phase). Not compared: the two halves together
  (the other is scene.py's), a dead attacker (the record 0), a unit of
  two squares, an explosion outside the window. This also says what
  two marks of the map's screen are: 40h/80h the attacker during a
  fight; 10h was on a unit with an order in its own player's window
  (seen, the code that sets it not read).
- The sound (`tools/sndfiles.py`, BATTLE.hints at sound_init; the
  tool's docstring has the formats and the rules in full). All of it is
  in CODE:0215 .. 1B79, three layers: a driver for the AdLib's registers
  (18 operators of 14 values, nine voices or six and five drums, a
  timbre, a volume, a pitch bend, note on and off), a player of MIDI
  files, and the effects. A song (.SND; the intros' .MDI) is a MIDI
  file of format 0, one track, 420 ticks a quarter note, with the
  AdLib's own events in it (FF 7F 00 00 3F: a voice's timbre of 28
  values, the percussion mode, the bend's range); the player has a
  timer of its own with a period of 8 ticks and drops the rest of a
  wait divided by 8, and begins the song again at its end. An effect
  file (.FXX) is records of 40h bytes: ticks, a frequency, a step, a
  timbre. The game asks for effects in four records of 6 bytes
  (F2D8A:0010: the effect, the volume, a count; FFFFh silences) which
  effects_start takes in the passes of the game's loops; the four
  channels are the voices 2, 4, 3 and 5, taken from the song meanwhile,
  and a timer of 72.8 a second adds the step to the frequency. Unless
  the AdLib is used the names' letter after the dot becomes a P: .PND
  and .PXX are the PC speaker's (the songs arranged otherwise, the
  effects the same bytes); with the speaker a song's notes take turns
  on the one voice. The mode is main's: `/s` the speaker, else the
  probe; `/m` another text than "Color." and F2740:02C8 = 2 (what for is
  not read).
  The program's timers (T2354, read in full; BATTLE.hints at timer_add):
  up to 16, a period each in counts of the PIT (so timer_keys' 4000h is
  72.8 a second, which was a guess before), the PIT run at the shortest
  period, a timer called at the first interrupt at or after its time.
  sndfiles.py reads the 68 song and effect files of the three games and
  writes them back identical (ISLE's and DESERT's LOOSER.PND is cut
  short at 3000h bytes, no end event: the speaker's song after a lost
  map would run past its buffer, presumably; MOON's .PND are its .SND,
  its TITEL ISLE's), and `--run LOG` does the routines again from a
  run's log (-log on the register write's two outputs, the song's start,
  stop and step, the effects asked for and their tick, the loudness; the
  tables from the player's BATTLE.EXE) and compares every value written
  to the AdLib: the title for 30 s (1889 writes) and the battle against
  the computer to 215 s (fights.keys: the title, the menu's and the
  map's effects, GAME.SND, two changes with fights; 51525 writes), all
  the same. Seen on the way:
  - a register write is two outputs and the timer's interrupt can fall
    between them: twice in the battle a value of the main program's
    went to register B5h, which the interrupt's routines had written
    last (the original's doing: a wrong key-off or octave of voice 5);
  - set_palette has the interrupts off while it writes the 256 colours,
    each after a wait for the display's blank (a scan line: some 8 ms
    in all, about the song's period), so in a fade the timers lose
    ticks: the waits between the song's steps were its periods to a
    millisecond but for two of 284 and 304 ms too long, at 4.5 s and
    13.8 s (the title's fade_in and fade_out: a run with -log on them
    and on set_palette), and 79 of 1185 off by up to 2.7 ms, each made
    up by the next;
  - GAME.SND sets the percussion mode (register BDh had bit 5 in the
    run), which answers whether the music uses it;
  - effects_start writes a volume as a word at DATA:1CCA + voice instead
    of + 2 * voice (the song's own volumes of two voices spoilt; kept).
  Not run: the speaker (`/s`), a song's end and beginning again,
  WINNER.SND and LOOSER.SND, a volume below 0, the animations' and the
  intros' sounds (the intros have the same file formats; their code is
  not compared), DESERT.EX2, MOON.EXE. Not looked for: whether a pitch
  bend other than none came in the runs' steps. xfer.py carried all the
  names to MOON.hints, each 7Eh further on in its CODE (a steady shift;
  not looked at in MOON.ASM), timer_remove and timer_period to T249A at
  the same offsets.
  The log of a run: `run.py -until T -keys FILE -dos -log CODE:14BC -log
  CODE:14C7 -log CODE:0B76 -log CODE:168D -log CODE:1334 -log CODE:1711
  -log CODE:172B -log CODE:1738 -log CODE:188A -log CODE:1792
  ISLE/BATTLE.EXE > LOG` (22 MB for 215 s). Scratch: `snd1.py`,
  `snd2.py` (the driver's tables printed), `snd3.py` (which files are
  the same; the effects a log asks for), `sndt.py`, `sndlate.py` (the
  steps' times).
- MOON.hints: T2084 has `start=8`: MOON's fight_step (T2066) ends in the
  frame's first 8 bytes (read; the build is identical either way).
  xfer.py carried the scene's names to MOON.hints (their beginnings
  looked at in MOON.ASM: the same routines) but two: T1F5A:0001 went to
  another routine (T1BCB:0001; MOON's is T2084:0008) and T1F5A:093C 5
  bytes before MOON's (T2084:09CF); both are left without a name in
  BATTLE.hints. history_rows, draw_curve and draw_line were carried
  right (looked at).
- Background runs: a batch of the session's shell is stopped after 30
  minutes unless a longer time is asked for; two batches at once ran
  without trouble (a run to 154 s takes 40 to 60 s then).
- Scratch scripts of this step (build/scratch): `fsc.py N..` (the dumps
  at fight_step's Nth entry; ARGS for runner options, PRE for the files'
  prefix), `fsv.py [PRE]` (scene.py on every pair of dumps), `fplan.py`
  (a poked variant planned), `fpk.py` (the reckoning with pokes).
- A real end of a map and the statistics (BATTLE.hints at after_map; the
  run above with two changes first, so that a round is played: `64:phase
  76:phase`, the cursor's keys from 88, the change at 113, space at 140
  and 175, h, a, n, s, enter from 185; -dos): after the animation
  after_map loads STATS.IFF, the palette, WINNER.SND, STATS.LIB and
  CODES.DAT and shows the two players' curves of their units' counts
  (history_add puts a point at each change), "RATING : 1210 ROUNDS : 1
  SCALE : 1 - 1" and "HEADQUARTER REPORTS A NEW KEYWORD : EAGLE", the
  code of map 04 (CODES.DAT's +7 added to the map); then MENU.IFF, the
  name typed goes to MAP\03.HI and the scores show MARSS, 01210 HANS.
  With no round played (F27EE:251D 0) after_map does nothing: the run
  without the two changes went from the animation straight to the name,
  and the poked ends of map 16 before had their name in MAP\04.HI for
  that (F27EE:251B still the loop's 4). `tools/screens.py --stats`
  draws the statistics' screen from a run's memory (BATTLE.hints at
  after_map and draw_line): STATS.IFF, the two curves (a point a change
  of phase, 16, 8 or 4 pixels apart by their number; the height the
  count * 64 / the highest count of both; a line by draw_line,
  T2541:0004, and over it STATS.LIB's entry, a pixel wide, at every x)
  and the texts. Two runs, all 64000 pixels in the video memory on the
  page shown: this map won (-ram -vram at 170 s: level curves of 3
  points, SCALE 1 - 1) and the battle against the computer lost
  (fights.keys, the screen up from 534.6 s, -ram -vram at 545 s: MISSION
  NOT COMPLETED, RATING 0, ROUNDS 11, 22 points, SCALE 1 - 2, curves
  that fall and rise). Not seen: 32 points and more (SCALE 1 - 4), 64
  and more (the rows turned round), the last map's text. Seen on the
  way, not looked into: in that battle a tap of space at 530 s alone did
  not end "YOU LOST YOUR HQ" (nothing more until 600 s); fights.keys'
  request for the change at 530 s and F1 at 534 s did.
- The cursor's keys: taps in the same direction get faster (the third
  tap on moves two squares; T0D36:058B, the cursor record's +2Bh..+2Eh:
  not read), so a far square is best reached by a fixed row of taps and
  checked in the memory (the cursor record's +0 is its square's offset,
  +2 the window's first square).
- xfer.py carried two of this step's names to wrong routines of MOON.EXE
  (the start of a move, T122D:05B8, to MOON's T154F:0C9E, which draws a
  type's big picture; T0E9B:19AB to T0DA1:04D0): both are left without a
  name in BATTLE.hints, as T1479:0C93. Carried to MOON.hints and looked
  at there (the first lines: the same routines): change_phase,
  move_aim, move_step, unit_release, unit_remove, unit_flag,
  four_squares, sync_slots; not looked at: unit_explode, cargo_loses,
  drop_empty, direction; not mapped: score, cargo_follow, move_arrive,
  move_undo, cargo_move, timer_set, timer_due, history_add.
- A kit matter seen on the way: disasm.py names an operand `D0008` (as
  if of DATA) where the code has set DS to another far data segment by
  hand and the ASSUME is of that segment (T17C0, T1C04: `MOV AL,[BX+D0000]`
  is F2D33:0000); the build is identical all the same. Not looked into.
- Runs in parallel: three batches of runs at once and the session's
  own memory ran this machine short of memory (two batches were stopped
  for it); one batch at a time is safe. A chain of 400 calls of the plan
  takes the tool some minutes (the path search in Python).
- Scratch scripts of this step (build/scratch, ignored): `ais.py STAGE
  ENTRY EXIT N..` (single calls of a stage of the computer player),
  `aic.py STAGE ENTRY EXIT N` (a chain), KEYS=file for another battle;
  `fights.py log
  UNTIL` and `fights.py N ..` (the battle with the fights logged, or
  stopped at the Nth fight and fight.py run on it), `paths.py N ..` and
  `fires.py N ..` the same for find_path and fire_reach. A run to 600 s
  takes about 140 s here.

## The port

How it is made (port/README.md has what is translated and what was
compared):

- `port/src/gen/names.h` comes from `symmap.py port/src/gen/names.h BI
  BATTLE=src/BATTLE.hints` (check.py checks it); the C names every
  variable it uses, so a variable gets a `name` in BATTLE.hints first
  (the section "names of data, for the port" at its end), then build.py,
  symmap.py, and xfer.py for DESERT.hints and MOON.hints. A name must not
  be a routine's (`score` was taken: `score_now`). xfer.py maps a data
  name only where an instruction names the address, so some stay "(not
  mapped)" even in DESERT.hints, whose addresses are BATTLE's.
- The program is loaded at PSP 0067h and keeps 3080h paragraphs (its
  INT 21h AH=4Ah in a `-dos` run), so the first block it gets from DOS is
  at 30E8h, as in the runner; its one big block (493E0h bytes, T0708's
  main) is cut up by the compiler's huge pointer arithmetic (`hadd`).
- DATA:0092 on is a table of far pointers to the library's records in
  DATA (the drawing record 00D6, the keys 0404, the players' input
  03AC..); the compiled code goes through them (`LES BX,[0092]`), the C
  uses the records' names.
- The clock (dos.c): time moves only where the program waits. The title
  waits for 3 ticks a pass and a retrace in flip_page; the number of its
  passes until a key at a given time is not the original's, so keys for
  a comparison are given by passes (`-keysat` and `BI_KEYSAT`), not by
  time. With a key by time the two pages came out exchanged (one pass
  more or less).
- load_file (T2695) writes one byte more than a packed file's length
  says (it stops when the count goes below 0), and takes its bytes from
  the 1000h bytes it reads the file through: past the file's end what
  the buffer held before. The port reads through the same buffer in the
  program's memory, so the byte is the original's.
- load_picture returns -1 whatever happened, and looks for BODY from
  the segment's start, word by word.
- What the zeros after the stack are (ZEROS, open question 13): the
  sound's variables, DS:1AD0 to 1CE6 (the operators' values, the voices,
  the song's tracks; named in BATTLE.hints now), and above them the
  stack the program really uses (SS:SP is 2F32:0F8C at the menu's entry:
  the startup code moves it there). The sound keeps more in its code
  segment (the song's timer handle and wait, where the song and the
  effects' file are): named too, and kept there by the port, so that a
  comparison shows the sound's state.
- A pointer to a variable behind DATA is made with DS in the original
  (2D8D:1C30, not 2F3A:0160): the port's DP() does the same, or the
  pointers differ in a comparison though they point to the same byte.
- The sound's check is a list of the values written (BI_OPLLOG against
  the runner's -log on CODE:14BC and 14C7; scratch `oplcmp.py SECONDS`):
  the title's first 30 s the same. Once a second source writes (the
  effects' timer in the menus) the two lists fall apart where the port's
  passes are quicker than the runner's (port/README.md): the order
  within each source is to be compared then, not the whole list (not
  done).
- The menus' loop has no wait but flip_page's retrace: 2 pictures a
  pass in the runner (0.0285 s), one in the port and on a fast PC. The
  title waits for 3 ticks a pass (0.0411 s in both).
- Scratch (build/scratch): `rl.py LABEL [LINES]` prints a routine folded,
  `BATTLE.fold` is all of build/BATTLE.ASM folded so (rd.py 1 65518; made
  again after names change), `pb.sh` builds the port with MSVC, `pcmp.py
  NAME ORIG_BREAK PORT_PLACE [T:KEY..] [PLACE@ADDR=N:KEY,..]` runs both to
  a place and compares (memcmp.py; FILTER=0 shows all), `vcmp.py` the
  video memories by region, `heapcmp.py` the memory behind the program.
- The Bash tool's here-documents break on apostrophes and backslashes
  here: scripts and C files are written as files, not through `cat <<`.

## Next

0. The port: the sound and the menus are done; next a map's setup
   (T0708:0392 on: the libraries with sort_lib and store_part, load_fin,
   load_shp, the cursors) and its loop (T0708:135C), then what the loop
   calls. Each step compared with the runner as port/README.md says.
1. BATTLE.EXE's last 4.1 KB: menus, a map, saving, loading, battles
   against the computer and poked wins with their animations ran none
   of it (above) but for a RETF; unused library code and switch tables,
   presumably, as the ending and the credits (poked in) ran none either.
   Left: the segment classes of the far data segments.
2. BATTLE.EXE in the runner: `run.py -until 20 -shot 12 build/shots/b12.png
   -dos ISLE/BATTLE.EXE` shows the title screen at 12 s; before it the
   program prints "Color." and "Speaker." and reads CHAR6.DAT, BB.DAT,
   BB.IFF, TITEL.IFF, LIB\char24.LIB, TITEL.TXT, TITEL.PND. (The shot
   asked for at 20 s was not written; not looked into.) Then names for the main loop, the file loading, the
   graphics output.
3. A tool for each data format (`tools/NAMEfiles.py`): TPWM, the
   palettes, the pictures, the libraries, the maps and the tables of
   unit types and ground are done (`tpwmfiles.py --out build/unpacked`
   gives the unpacked files), and the maps are drawn as a run shows them
   (`mapfiles.py --png`), CODES.DAT and AMOK.DAT are read, the .PMP is
   the map's overview, the texts and the small font are read
   (`txtfiles.py`), the status screen is drawn in full
   (`screens.py`: the status screen and the unit's); next in screens.py
   the other screens of SHOP.LIB's window (the callers of
   draw_shop_window and draw_box in T1479: the buildings' screens are
   drawn as they open and in use, by keys, with the message line; left
   there a repair that repairs and a unit that holds others; the
   statistics after a map, the menus, a code typed, LOAD's messages,
   the scores with a file and their name are drawn in full, and the
   fight scene pass by pass, `scene.py`; the map's whole screen with
   both windows, marks, lines, cursors and the overview, `screens.py
   --field`, a move under way too), then what is left of the screen: a unit hidden from the other
   player. The sounds are read and done again (`sndfiles.py`); the
   PC speaker's sound (`/s`) stays as read: the user does not want it,
   the port plays the AdLib's only.
4. The port: `symmap.py`, then the program over `rmem.h` routine by
   routine, compared with the runner.
5. Later: the AdLib sound refined (`-oplwav` against the game in GOG's
   DOSBox; what differs goes into doskit's runtime/opl.c, whose
   modulation depth, attack curve and drums are choices). The music
   uses the rhythm mode (GAME.SND; above), so the drums matter.
   MOON.EXE's and the intros' last library gaps only if a run reaches
   them.

## Open questions

What the notes above and the hints mark as "not looked into", "not
checked" or "presumably", in one place. They were left because each step
had another aim, not because they do not matter: before the port takes
over a routine, its questions here are answered against runs of the
original. The first group matters for the port, the second for
understanding the programs, the third hardly.

For the port (behaviour):

1. The game's state flags: F27EE:250C (bits 1, 2, 4, 8, 40h, 80h,
   1000h beside the known 10h, 20h, 100h, 200h, 400h, 2000h, 4000h;
   100h and 200h as the menu sets them, BATTLE.hints at menu: who reads
   100h is not looked into). F27EE:250E and the players' bit 2 are the
   menu's (above); what the map does with them is not read.
   BATTLE.hints at play_anim.
2. A map's end: a headquarters taken is run and read, the statistics
   are drawn (above); left: an end by units, the last map (CODES.DAT's
   +8 bit 0) without a poke, statistics of 32 points and more, what the
   map's loop does with F27EE:250E's bits at the end against the
   computer, and which key ends "YOU LOST YOUR HQ" there.
3. The clock: the map's loop runs every 4 ticks of timer_keys, 18.2
   times a second (above); timer_add's period is the PIT's count (read,
   and the song's steps timed in runs). Left: the loops of the menus and
   the title, the values
   2 and 0 of F27EE:251B, why the title menu takes keys only from about
   30 s.
4. The palette level: fade_in and fade_out give set_palette its levels
   (above; their callers are not read), where the animations' 6-bit
   palettes are set.
5. Saving and the high scores: where save_game writes (save_file or its
   own), where the name "00" comes from (the position asked for at LOAD
   is F27EE:2512, and T26EA:000F makes the name from it: not read), the
   .HI file after a real end of a map (written and read back in runs of
   a poked end, above; named by F27EE:251B, which after_map sets to the
   map's number and the map's loop to its 4 ticks, and the scores'
   screen takes its code by the same number).
6. The animations: play_anim's names from ANIM\anim.fx (presumably),
   the .Axx/.FX/.PX formats, end_credits (T25A6:0655) and ab.fx.
7. Sound (the AdLib's; the speaker's is left out on purpose): what
   the port does where the original loses the timers' ticks in a fade
   and where a value goes to the wrong register (both are the original's
   timing, not its rules); what F2740:02C8 (`/m`) is for.
8. The libraries' colour bases and palettes where not seen (the
   fonts; libfiles.py draws all at base 0 in 00.PAL; the fight scene's
   are seen: FIGHT 5Fh, RAND 40h, BUM 0 and 20h or 30h, a shadow 50h), and
   what happens with BIGUNIT's missing MAA and with MOON's BIGUNIT
   without a .DAT (its PART and UNIT are taken in the library's order:
   seen).

9. The maps (mapfiles.py): the .SHP's kind 3 (F27EE:0B64), its bytes
   +3, +4 and its 27 bits; the .COM's records and why the loader swaps
   their bytes 2 and 3; whether owner/player 0 is the player of the
   arrow keys (the status screen's ONE is red, as player 0's dots);
   MOON.EXE's loader (MOON's 16.FIN has a two-square unit without its
   second half) and its overview without a .PMP; the unit flag 200h (+4) that gives a dot AMOK's +20h
   colour; why the status screen counts 6 units where the .FIN has 5.

10. The tables (datfiles.py): the fight's formula, what a type's move
    counts and the targets word's bits 40h and 80h are answered (above:
    fight.py, moves.py); left of them: a fight of units the runs did not
    have (no answer, air, sea, ships, other ground), stop_check's kinds
    1 and 2 and its messages in a run; the
    targets word's bits above 80h; flag 1 of
    a type's +0Eh in a run of such a type; the low bits of its +10h and
    where a unit gets 8000h in its +6; its +41h..+43h (the fight scene's
    sounds: read, the sounds themselves not looked at); the
    ground's flags besides the buildings' and 4000h; the tables as the
    game shows them on its screens (not compared); MOON.EXE's reading of
    its UNIT.DAT and GROUND.DAT (MOON's masks differ, and one type has a
    flag 8 in +0Eh); AMOK.DAT's colours on the screen, and its building
    site and depot (orders 0Bh and 0Dh) in a run.

For understanding the programs:

11. make_path's first argument; the OR of DATA:0368 in file_open;
    T2695:02BB's message (DATA:0C88, "insert the disk", presumably);
    load_picture for pictures larger than 360x240 (T2550:02E5).
12. The record T0708 fills with far pointers to routines (+56 is
    draw_entry); CODE:2296's formatter (presumably).
13. The far data segments' classes (which are BSS); the zeros after the
    stack in BATTLE.EXE and the intros (ZEROS).
14. What BATTLE.EXE, DESERT.EX2 and MOON.EXE do with the string
    BIDISK.VGA besides check_vga_disk; what DESERT.EXE (the starter)
    does.

Hardly:

15. Why the runner did not write the shot asked for at 20 s (Next,
    item 2).
16. In which order the TPWM packer saw the files (the byte past the end
    in six files; the bytes past the end in 31 of MOON's).
