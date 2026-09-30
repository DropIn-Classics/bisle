# Handoff

State of 2026-09-30: stage 1 for the launcher, the main program and the
two data disks' games; BATTLE.EXE's and BI.EXE's code reached to 97% and
96%, DESERT.EX2's as BATTLE.EXE's, MOON.EXE's to 98%.
`src/BI.hints`, `src/BATTLE.hints`, `src/DESERT.hints` and
`src/MOON.hints` rebuild their programs byte for byte
(`doskit/tools/check.py`: all ok). Nothing is understood yet
beyond the segments; the port is the template's.

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
| `ISLE/BI.EXE` | Battle Isle's launcher (10 KB, assembly, linked by TLINK): checks the marker files `BIDISK.VGA`/`.EGA`/`.INT` (1 byte each), runs `INTVGA\INTRO.EXE` or `INTEGA\INTRO.EXE`, then `BATTLE.EXE` (read from its strings; not checked by a run) |
| `ISLE/BATTLE.EXE` | the game (208 KB, Borland C++ 1991, large model, 4630 relocations) |
| `ISLE/INTVGA/INTRO.EXE`, `ISLE/INTEGA/INTRO.EXE` | the intro, VGA and EGA (Borland C++), with `INTRO.A00`..`A12`, `.FX`, `.PX`, `.MDI`, `.PDI`, `CHAR_BIG.DAT`, `CHAR_SMA.DAT` |
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
  What is left is mostly the library in CODE (1.7 KB). A run (space at
  14 s, enter at 33 s, 90 s) reaches MOON's first map with BATTLE.EXE's
  keys; gaps.py with its -cover: 0 gaps ran.
- The runner read port 201h as F0h, axis bits that fall at once: a
  joystick held up and left. BATTLE.EXE took it for an attached one
  (DATA:0374/0376 = FFFFh) and its menu saw "up" all the time, so down
  did nothing. doskit's runner reads FFh now, as a PC without a joystick
  (doskit, with a test, GAMEPORT.EXE).

## Next

1. BATTLE.EXE's last 4.1 KB: a battle (a unit moved onto an enemy's,
   the FIGHT screen) with `-cover` would tell whether any of it runs
   (saving and loading did not reach any); the first map's units stand
   far apart, so it takes several turns: a key script built turn by
   turn, saving on the way (the change of phase as above). The segment
   classes of the far data segments.
2. BATTLE.EXE in the runner: `run.py -until 20 -shot 12 build/shots/b12.png
   -dos ISLE/BATTLE.EXE` shows the title screen at 12 s; before it the
   program prints "Color." and "Speaker." and reads CHAR6.DAT, BB.DAT,
   BB.IFF, TITEL.IFF, LIB\char24.LIB, TITEL.TXT, TITEL.PND. (The shot
   asked for at 20 s was not written; not looked into.) Then names for the main loop, the file loading, the
   graphics output.
3. MOON.EXE: the other hints not carried (T0708's record of far
   pointers to routines, BATTLE's CODE raw lines) found again in MOON's
   own code where they matter; whether DATA is 2EAE (a run's DS would
   tell); the library's last 1.7 KB. Then the INTRO programs on their
   own.
4. A tool for each data format (`tools/NAMEfiles.py`), starting with
   the palettes, `.IFF`/`.LBM` pictures and `.LIB` libraries.
5. The port: `symmap.py`, then the program over `rmem.h` routine by
   routine, compared with the runner.
