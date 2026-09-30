# Handoff

State of 2026-09-30: stage 1 for the launcher, the main program, the
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
| `MAP/NN.PMP`, `.SHP`, `.FIN`, `.COM` | per map; the `.COM` files are no programs: they begin with `TPWM` (a packed file, presumably) |
| `LIB/*.LIB`, `LIB/*.DAT` | graphics libraries by their names (units, parts, patterns, fonts, cursor); not looked at |
| `*.IFF`, `*.LBM`, `*.PAL`, `NN.PAL` | pictures and palettes by their names; not looked at |
| `*.SND`, `*.PND`, `*.FXX`, `*.PXX` | sound or animation data by their names; not looked at |
| `*.DAT`, `*.TXT` | tables and texts (`UNIT.DAT`, `CODES.DAT`, `GAME.TXT`, ...); not looked at |
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
3. A tool for each data format (`tools/NAMEfiles.py`): TPWM and the
   palettes are done (`tpwmfiles.py --out build/unpacked` gives the
   unpacked files); next the `.IFF`/`.LBM` pictures and the `.LIB`
   libraries, reading the unpacked files.
4. The port: `symmap.py`, then the program over `rmem.h` routine by
   routine, compared with the runner.
5. Later: the AdLib sound refined (`-oplwav` against the game in GOG's
   DOSBox; what differs goes into doskit's runtime/opl.c, whose
   modulation depth, attack curve and drums are choices). Whether the
   music uses the rhythm mode (register BDh bit 5) is not looked into.
   MOON.EXE's and the intros' last library gaps only if a run reaches
   them.
