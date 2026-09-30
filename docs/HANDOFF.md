# Handoff

State of 2026-09-30: stage 1 for the launcher and the main program.
`src/BI.hints` and `src/BATTLE.hints` rebuild BI.EXE and BATTLE.EXE byte
for byte (`doskit/tools/check.py`: all ok). Nothing is understood yet
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
| `DESERT/DESERT.EX2` | data disk 1's game (Borland C++, the same size of header and number of relocations as BATTLE.EXE: presumably the same engine, not checked) |
| `MOON/MOON.EXE` | data disk 2's game (Borland C++, 4585 relocations; names `MAPINFO.DAT`, `MAP02.DAT`, `MAP04.DAT`, `HQ.PAL`, `TOT.PAL`, `END.PAL`/`.SND` besides BATTLE.EXE's files) |
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
- The analysis reaches about 70% of BATTLE.EXE's code bytes: 48 KB of
  non-zero bytes in 110 gaps are not reached yet (the largest at
  T1C04:00CD, 11 KB; `doskit/tools/gaps.py src/BATTLE.hints --seg T1C04`).
  In BI.EXE most of the segments other than CODE are not reached yet.

## Next

1. Gaps in BATTLE.EXE (and BI.EXE): far pointers in data (tables of
   `DD` handlers), switch tables; `gaps.py --seg SEG` per segment. The
   segment classes of the far data segments.
2. BATTLE.EXE in the runner: `run.py -until 20 -shot 12 build/shots/b12.png
   -dos ISLE/BATTLE.EXE` shows the title screen at 12 s; before it the
   program prints "Color." and "Speaker." and reads CHAR6.DAT, BB.DAT,
   BB.IFF, TITEL.IFF, LIB\char24.LIB, TITEL.TXT, TITEL.PND. (The shot
   asked for at 20 s was not written; not looked into.) Then names for the main loop, the file loading, the
   graphics output.
3. DESERT.EX2 and MOON.EXE by `doskit/tools/xfer.py` from BATTLE.hints,
   the INTRO programs on their own.
4. A tool for each data format (`tools/NAMEfiles.py`), starting with
   the palettes, `.IFF`/`.LBM` pictures and `.LIB` libraries.
5. The port: `symmap.py`, then the program over `rmem.h` routine by
   routine, compared with the runner.
