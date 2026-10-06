# Proposal: removing what the port copied (for doskit, not yet made)

Status: a proposal (2026-10-06, branch dropin-team/uninstall-script). It
belongs to the kit (AGENTS.md rule 7: doskit's template, its workflow
and RELEASE.md), so nothing of it is in doskit yet; the script below
was tried on Linux only.

## What a port leaves behind

Everything goes into the data folder, `sys_data_dir`
(doskit/runtime/sys.c): `$DK_DATA_DIR` if set, else
`%LOCALAPPDATA%\NAME` on Windows, `~/Library/Application Support/NAME`
on a Mac, `$XDG_DATA_HOME/SLUG` or `~/.local/share/SLUG` on Linux.
For battle-isle it holds:

- `game/`: the game's files copied from the player's GOG release (the
  launcher's copy; 39 MB here). This is "the copied game data".
- `save/`, `save-desert/`, `save-moon/`: saved games and scores.
- `battle-isle.cfg`: the setup screen's settings.
- `update.cfg`, `latest.json`, `latest.part`, `update-package.part`:
  update.h's consent, the last fetched release list and downloads.

Outside it: a `game` folder beside the program from versions before
the data folder (`sys_data_migrate` moves it on the first start, so it
is normally gone), and update.c's staging folder `.doskit-update-*`
beside the program's folder, which only lives while an update installs.
Nothing is written to the registry or elsewhere.

## The proposal

1. The kit's template gets `port/dist/uninstall.sh` (Linux and macOS)
   and `port/dist/uninstall.cmd` with a PowerShell line (Windows); the
   workflow's Pack steps copy them into each package beside README.txt;
   RELEASE.md's package table lists them.
2. Default: a dry run that lists what would go and removes nothing.
   `--yes` removes the copied game files only (`game/` in the data
   folder and a leftover beside the program); `--all --yes` the whole
   data folder (saves and settings too). The program's own folder the
   player deletes by hand, as it was unpacked by hand.
3. README.txt gets a section "Removing" that says this, in the
   platform's words.

## The script (Linux and macOS), as tried

```sh
#!/bin/sh
# uninstall.sh - removes what SLUG copied and wrote: by default only the
# copied game files (the data folder's "game"); with --all the whole data
# folder (saves and settings too).  Nothing is removed without --yes: the
# default is a dry run that lists what would go.
set -eu
SLUG=battle-isle
NAME="Battle Isle"
all=0 yes=0
for a in "$@"; do
    case $a in
        --all) all=1 ;;
        --yes) yes=1 ;;
        *) echo "usage: $0 [--all] [--yes]" >&2; exit 2 ;;
    esac
done
# the data folder as sys_data_dir finds it (doskit/runtime/sys.c)
if [ -n "${DK_DATA_DIR:-}" ]; then data=$DK_DATA_DIR
elif [ "$(uname)" = Darwin ]; then data="$HOME/Library/Application Support/$NAME"
elif [ -n "${XDG_DATA_HOME:-}" ]; then data="$XDG_DATA_HOME/$SLUG"
else data="$HOME/.local/share/$SLUG"
fi
# a game folder beside the program (versions before the data folder)
here=$(cd "$(dirname "$0")" && pwd)
targets=""
add() { [ -e "$1" ] && targets="$targets
$1" || true; }
if [ $all = 1 ]; then add "$data"; else add "$data/game"; fi
add "$here/game"
targets=$(printf '%s\n' "$targets" | sed '/^$/d')
if [ -z "$targets" ]; then echo "Nothing to remove (data folder: $data)."; exit 0; fi
printf '%s\n' "$targets" | while IFS= read -r t; do
    printf '%s  (%s)\n' "$t" "$(du -sh "$t" 2>/dev/null | cut -f1)"
done
if [ $yes = 0 ]; then
    echo "Dry run: nothing removed. Run again with --yes to remove the above."
    exit 0
fi
printf '%s\n' "$targets" | while IFS= read -r t; do
    case $t in
        */game|"$data") rm -rf -- "$t" && echo "removed $t" ;;
        *) echo "skipped $t (not a folder of $SLUG)" >&2 ;;
    esac
done
```

Tried (Linux, Debian 13): in a made-up data folder (`XDG_DATA_HOME`
set; empty files, no game data) the dry run listed `game` and removed
nothing; `--all` listed the folder and removed nothing; `--yes` removed
`game` and kept `save/` and `battle-isle.cfg`; `--all --yes` removed
the folder; a second run said "Nothing to remove"; an unknown option
gave the usage and exit 2. A dry run against this machine's real
`~/.local/share/battle-isle` listed `game` (39M) and, with `--all`, the
folder, and removed nothing.

Not tried: macOS (the path with a space is quoted, not run), Windows
(the .cmd is not written yet), `DK_DATA_DIR`, a leftover `game` beside
the program, a game folder with read-only files. Open: whether the
update files belong to "game data" (here they go only with `--all`);
whether a Mac script must be a `.command` to start by a double click.
