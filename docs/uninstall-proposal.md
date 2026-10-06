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
2. Interactive, no parameters needed (the user's decision, 2026-10-06):
   started without anything it asks two yes/no questions, each naming
   the folder and its size: "Remove the copied game files in ...?"
   (`game/` in the data folder, and a leftover beside the program) and
   "Also remove the saves and settings in ...?" (the whole data
   folder). The default is no: only y or yes removes. Without a
   terminal to ask on nothing is removed and the questions are only
   listed. For scripts, `--yes` answers the first with yes and the
   second with no, `--yes --all` both with yes. The program's own
   folder the player deletes by hand, as it was unpacked by hand.
3. README.txt gets a section "Removing" that says this, in the
   platform's words.

## The script (Linux and macOS), as tried

```sh
#!/bin/sh
# uninstall.sh - removes what SLUG copied and wrote.  Started without
# anything it asks: first whether to remove the copied game files (the
# data folder's "game"), then whether to remove the saves and settings
# too.  Anything but y or yes keeps it.  Without a terminal to ask on it
# only lists what it would remove.  --yes (for scripts) answers the
# first question with yes and the second with no; with --all as well
# the second with yes.
set -eu
SLUG=battle-isle
NAME="Battle Isle"
yes=0 all=0
for a in "$@"; do
    case $a in
        --yes) yes=1 ;;
        --all) all=1 ;;
        *) echo "usage: $0 [--yes [--all]]" >&2; exit 2 ;;
    esac
done
# the data folder as sys_data_dir finds it (doskit/runtime/sys.c)
if [ -n "${DK_DATA_DIR:-}" ]; then data=$DK_DATA_DIR
elif [ "$(uname)" = Darwin ]; then data="$HOME/Library/Application Support/$NAME"
elif [ -n "${XDG_DATA_HOME:-}" ]; then data="$XDG_DATA_HOME/$SLUG"
else data="$HOME/.local/share/$SLUG"
fi
here=$(cd "$(dirname "$0")" && pwd)
size() { du -sh "$1" 2>/dev/null | cut -f1; }
# ask QUESTION DEFAULT-FOR---yes: 0 for yes
ask() {
    if [ $yes = 1 ]; then return $2; fi
    if [ ! -t 0 ]; then echo "$1 [y/N] (no terminal: not asked, kept)"; return 1; fi
    printf '%s [y/N] ' "$1"
    read -r answer || return 1
    case $answer in y|Y|yes|Yes|YES) return 0 ;; *) return 1 ;; esac
}
removed=0
for g in "$data/game" "$here/game"; do
    [ -d "$g" ] || continue
    if ask "Remove the copied game files in $g ($(size "$g"))?" 0; then
        rm -rf -- "$g" && echo "removed $g"; removed=1
    else echo "kept $g"; fi
done
if [ -d "$data" ]; then
    [ $all = 1 ] && second=0 || second=1
    if ask "Also remove the saves and settings in $data ($(size "$data"))?" $second; then
        rm -rf -- "$data" && echo "removed $data"; removed=1
    else echo "kept $data"; fi
fi
[ $removed = 1 ] || echo "Nothing removed."
```

Tried (Linux, Debian 13), in a made-up data folder (`XDG_DATA_HOME`
set; empty files, no game data), the answers typed through `script`
so that the script has a terminal: n, n kept everything; y, n removed
`game/` and kept `save/` and `battle-isle.cfg`; n, yes and y, y removed
the folder; without a terminal nothing was removed; `--yes` removed
`game/` only, `--yes --all` the folder; a run with nothing there said
"Nothing removed"; an unknown option gave the usage and exit 2.
Against this machine's real `~/.local/share/battle-isle` only without a
terminal: it listed `game` (39M) and the folder and removed nothing.

Not tried: macOS (the path with a space is quoted, not run), Windows
(the .cmd, with the same two questions, is not written yet), `DK_DATA_DIR`, a leftover `game` beside
the program, a game folder with read-only files. Open: whether the
update files belong to "game data" (here they go only with `--all`);
whether a Mac script must be a `.command` to start by a double click.
