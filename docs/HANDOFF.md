# Handoff

State of 2026-10-01: stage 1 for the launcher, the main program, the
two data disks' games and the two intros; BATTLE.EXE's and BI.EXE's
code reached to 97% and 96%, DESERT.EX2's as BATTLE.EXE's, MOON.EXE's
to 98%, the intros' to 92%.
`src/BI.hints`, `src/BATTLE.hints`, `src/DESERT.hints`,
`src/MOON.hints`, `src/INTVGA.hints` and `src/INTEGA.hints` rebuild
their programs byte for byte
(`doskit/tools/check.py`: all ok). BI.EXE is read in full (which
intro, then BATTLE.EXE); the others are not understood yet
beyond the segments and a few routines; the port is the template's.

## Start here (next session)

- AGENTS.md and PROVENANCE.md have the rules: read them first.
- `game/` holds the installed GOG folder as it is (no CD image: GOG
  ships the three games as folders for DOSBox); it is not in the
  repository.
- doskit is at 11c6135, which is pushed. Not pushed yet when this was
  written: the project's commits after a5d390a.
- On a machine without `game/` in the checkout, `DOSKIT_GAME` names the
  installed game's folder (GOG's, with ISLE, DESERT and MOON in it); a
  fresh clone needs `git submodule update --init doskit`.
- Scratch scripts of the last session are in `build/scratch` (ignored,
  not part of the project; they may be gone): `rd.py A B` prints lines
  of build/BATTLE.ASM with the compiler's table indexing folded
  (utype, unit, ground, shop, player), `uses.py BASE SIZE` lists the
  code lines naming F27EE addresses in a range, `rwfields.py LOG` sorts
  a `-rwatch` report by table field and routine.
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
| `*.SND`, `*.PND`, `*.FXX`, `*.PXX` | sound or animation data by their names; not looked at |
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
  alternate, the orders are carried out at the change. In 600 s the
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

## Next

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
   (`screens.py`); next in screens.py the other screens of SHOP.LIB's
   window (the callers of draw_shop_window and draw_box in T1479: the
   buildings' screens, by the texts 03h..09h), the menus' texts (T1090,
   in the program's data by the looks, drawn by draw_text24), who draws
   BB.DAT's sprites, then the rest of the screen: the cursor's entries, the overview's frame, the
   frame's texts, what the windows show of a map (scrolling, the units'
   directions, a unit hidden from the other player).
4. The port: `symmap.py`, then the program over `rmem.h` routine by
   routine, compared with the runner.
5. Later: the AdLib sound refined (`-oplwav` against the game in GOG's
   DOSBox; what differs goes into doskit's runtime/opl.c, whose
   modulation depth, attack curve and drums are choices). Whether the
   music uses the rhythm mode (register BDh bit 5) is not looked into.
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

1. The game's state flags: F27EE:250C (bits 1, 2, 4, 8, 40h, 80h, 100h,
   200h, 1000h beside the known 10h, 20h, 400h, 2000h, 4000h), F27EE:250E
   (bit 1 set against the computer, presumably; bit 2) and F27EE:243E
   bit 2 (which player the computer is, presumably). BATTLE.hints at
   play_anim.
2. A real end of a map: what a won map does besides what the pokes
   showed (after_map, T15AC:0007; the map record's +8 bit 0 for the last
   map; the STATS.IFF screen after a lost battle, which routine).
3. The clock: the map's loop runs every 4 ticks of timer_keys, 18.2
   times a second (above). Left: whether timer_add's (T2354:0713) period
   is the PIT's count, the loops of the menus and the title, the values
   2 and 0 of F27EE:251B, why the title menu takes keys only from about
   30 s.
4. The palette level: fade_in and fade_out give set_palette its levels
   (above; their callers are not read), where the animations' 6-bit
   palettes are set.
5. Saving and the high scores: where save_game writes (save_file or its
   own), where the name "00" comes from, the .HI file (28h bytes, named
   by F27EE:251B, which after_map sets to the map's number and the map's
   loop to its 4 ticks; a real end of a map not run).
6. The animations: play_anim's names from ANIM\anim.fx (presumably),
   the .Axx/.FX/.PX formats, end_credits (T25A6:0655) and ab.fx.
7. Sound: where sound_init's mode comes from, whether the music uses
   the OPL's rhythm mode.
8. The libraries' colour bases and palettes where not seen (BIGUNIT,
   FIGHT, the fonts; libfiles.py draws them at base 0 in 00.PAL), and
   what happens with BIGUNIT's missing MAA and with MOON's BIGUNIT
   without a .DAT (its PART and UNIT are taken in the library's order:
   seen).

9. The maps (mapfiles.py): the .SHP's kind 3 (F27EE:0B64), its bytes
   +3, +4 and its 27 bits; the .COM's records and why the loader swaps
   their bytes 2 and 3; whether owner/player 0 is the player of the
   arrow keys (the status screen's ONE is red, as player 0's dots);
   MOON.EXE's loader (MOON's 16.FIN has a two-square unit without its
   second half) and its overview without a .PMP; the overview's frame
   and cursor; the unit flag 200h (+4) that gives a dot AMOK's +20h
   colour; why the status screen counts 6 units where the .FIN has 5.

10. The tables (datfiles.py): the fight's formula (fight_reckon,
    T2190:000D: how the counts, armour, hit values, the units' +1, the
    two percentages and the ground's kind give the losses); what a type's
    move counts (squares or the path's cost) and how find_path's result
    limits a move; the targets word's bits 40h, 80h and above; flag 1 of
    a type's +0Eh; the low bits and 1000h, 2000h, 8000h of its +10h; its
    +16h (a sound, presumably) and +41h..+43h (the fight scene); the
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
