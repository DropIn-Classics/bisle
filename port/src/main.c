/* main.c - Battle Isle: a native compatibility implementation requiring an
 * installed copy of the original game.
 *
 *     battle-isle [-game DIR | -gog FILE|FOLDER|SETUP.exe] [-title isle|desert] [/s] [/m]
 *
 * DIR is the game's unpacked files: -game, else $BATTLE_ISLE_GAME, else the first
 * folder `game` holding ISLE/BI.EXE beside the program, in the current
 * directory or in the data folder (sys_find_game).  When there is none,
 * the installed GOG release's image is unpacked into the data folder's
 * `game`, or, installed as a folder, that folder copied there (cdimage.h);
 * not installed, GOG's Windows installer (setup_*.exe) lying about is
 * unpacked instead (inno.h).  -gog names the image, the folder or the
 * installer instead of looking for it.  Found by itself, the release is
 * copied only when the player agrees, asked in the kit's dialog about the
 * game's files (launcher.h: "Copy the files" or "Quit"), which also shows
 * the copy's progress and says what to do when nothing was found; the
 * headless build shows it only in a run scripted with keys (DK_KEYS,
 * plat_null.c) and copies without asking otherwise.
 *
 * Then the setup screen (the kit's launcher.h; this file gives it one page
 * of items and draws nothing): start, full screen, the original's /m, and
 * in a release build whether to look for newer releases (update.h).
 *
 * Then the game's main program, ISLE/BATTLE.EXE, is loaded from the
 * player's file into a megabyte of memory as DOS loaded it, and the C of
 * this port runs over that memory (bi.h, battle.c); /s and /m are the
 * original's switches.  Files the game writes go to the data folder's
 * `save`.
 *
 * The first data disk is the same program with its own files (`titles`
 * below): chosen on the setup screen when its folder is there, or by
 * -title, it is loaded from DESERT/DESERT.EX2 and writes to the data
 * folder's `save-desert`.
 *
 * For comparisons with the original (doskit/tools/memcmp.py):
 * BI_SKIP_INTRO and BI_QUIT_YZ set the setup screen's two choices of the
 * same names without it.
 * BI_BREAK=NAME#N ends the program at the Nth pass of a place of that
 * name (bi_at), after writing the memory to $BI_RAM and the video memory
 * to $BI_VRAM, as the runner's -break, -ram and -vram do.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdimage.h"
#include "inno.h"
#include "launcher.h"
#include "platform.h"
#include "sys.h"
#include "update.h"
#include "bi.h"

#ifndef PORT_VERSION
#define PORT_VERSION ""
#endif
#ifndef PORT_UPDATE_URL
#define PORT_UPDATE_URL ""
#endif

static const GogRelease release = {
    /* GOG's folder name */
    "Battle Isle",
    /* the CD image's path in that folder ("": game.gog) */
    "",
    /* the Mac release's image, its path in /Applications ("": none) */
    "",
    /* GOG's product ID, the number in goggame-ID.info ("": not known) */
    "1207660993",
    /* on the CD: images of other games are passed over */
    "ISLE/BI.EXE",
};

/* what earlier versions wrote beside the program (sys_data_migrate) */
static const char *const old_files[] = { "game", NULL };

static const LauncherApp app = { "Battle Isle", "battle-isle", PORT_VERSION };

/* The titles this program plays: the folder in the game's files, the
 * program in it, its size and SHA-256 (the GOG release's), where its own
 * files go in the data folder.
 * DESERT.EX2 is BATTLE.EXE but for three bytes of data (the key that
 * answers QUIT THE GAME in both key sets and its letter in the message)
 * and its end: the file stops before the stack, which is zeros in
 * BATTLE.EXE.  Its starter DESERT.EXE (read from its code) only makes the
 * folders BLUEBYTE, BI1D1 in it and MAP in that on C: and, while
 * DESERT.EX2 runs, sends the saved games NN.DAT and the scores MAP/NN.HI
 * there (INT 21h's create, open, delete, attributes and find first of
 * those names): the port's folder for what a title writes does the same. */
static const struct {
    const char *arg, *label, *folder, *exe;
    unsigned long size;
    const char *sha256, *save;
} titles[] = {
    { "isle", "Battle Isle", "ISLE", "BATTLE.EXE", BATTLE_SIZE, BATTLE_SHA256, "save" },
    { "desert", "Data disk 1 (Desert)", "DESERT", "DESERT.EX2", 212242ul,
      "1042af4eb941908cae776cc2118b4664c16b56975ec9353ffa05633626a1cff4", "save-desert" },
};

#define TITLES ((int)(sizeof titles / sizeof titles[0]))

/* ---- the setup screen (launcher.h; doskit/docs/LAUNCHER.md) ---- */

enum { ACT_START = 1, ACT_PAGE };

static const char *const no_yes[] = { "no", "yes", NULL };
static const char *const off_on[] = { "off", "on", NULL };

static int set_fullscreen, set_m, set_updates, set_title, set_skip, set_quit = 1;
/* the titles whose folders are there, for the setup screen's choice */
static const char *title_labels[TITLES + 1];
static int title_of[TITLES];
static UpdateInfo newer;
static char newer_label[64];

/* the menu, and a page for each group of settings (doskit/docs/LAUNCHER.md) */
enum { PAGE_MENU, PAGE_GAME, PAGE_PICTURE, PAGE_QOL, PAGE_PORT };

static LauncherItem menu_items[] = {
    { LI_ACTION, "Start the game", NULL, NULL, NULL, ACT_START, NULL },
    { LI_HEAD, "", NULL, NULL, NULL, 0, NULL },
    { LI_PAGE, "Game", NULL, NULL, NULL, PAGE_GAME, "Which game, and the original's switch." },
    { LI_PAGE, "Picture", NULL, NULL, NULL, PAGE_PICTURE, "Full screen." },
    { LI_PAGE, "Quality of life fixes", NULL, NULL, NULL, PAGE_QOL,
      "Choices the original does not have." },
    { LI_PAGE, "This port", NULL, NULL, NULL, PAGE_PORT, "New versions." },
};

static LauncherItem game_items[] = {
    { LI_CHOICE, "Title", "title", title_labels, &set_title, 0,
      "Battle Isle, or a data disk whose folder is in the game's files." },
    { LI_CHOICE, "Switch /m", "m", off_on, &set_m, 0,
      "The original's /m: the map in its other palette." },
};

static LauncherItem picture_items[] = {
    { LI_CHOICE, "Full screen", "fullscreen", no_yes, &set_fullscreen, 0,
      "Alt+Enter changes it while the game runs." },
};

static LauncherItem qol_items[] = {
    { LI_CHOICE, "Skip logo, intro, and title", "skip_intro", no_yes, &set_skip, 0,
      "Starts at the main menu (the intro is not in the port yet)." },
    { LI_CHOICE, "Quit key Y and Z", "quit_yz", no_yes, &set_quit, 0,
      "QUIT THE GAME takes Y and Z: the original has one, by layout." },
};

static LauncherItem port_items[] = {
    { LI_CHOICE, "Look for new versions", NULL, no_yes, &set_updates, 0,
      "One small file from GitHub, at most once a day; nothing is sent." },
    /* the line of a newer release: counted only when there is one */
    { LI_ACTION, newer_label, NULL, NULL, NULL, ACT_PAGE, "Opens the release's page in the browser." },
};

#define PORT_ITEMS ((int)(sizeof port_items / sizeof port_items[0]) - 1)

static LauncherPage pages[] = {
    { "Setup", menu_items, (int)(sizeof menu_items / sizeof menu_items[0]) },
    { "Game", game_items, (int)(sizeof game_items / sizeof game_items[0]) },
    { "Picture", picture_items, (int)(sizeof picture_items / sizeof picture_items[0]) },
    { "Quality of life fixes", qol_items, (int)(sizeof qol_items / sizeof qol_items[0]) },
    { "This port", port_items, PORT_ITEMS },
};

#define NPAGES ((int)(sizeof pages / sizeof pages[0]))

static void setting_changed(const LauncherItem *item)
{
    if (item->value == &set_fullscreen)
        plat_set_fullscreen(set_fullscreen);
    else if (item->value == &set_updates)
        update_set_consent(set_updates);
}

/* The setup screen until the game is started: 1, or 0 to quit.  The
 * settings are kept in the data folder's battle-isle.cfg; the answer
 * about new versions is update.h's (not asked: no until the player says
 * yes).  A newer release known at the start (update.h: fetched in the
 * background, so one found by this start's fetch shows at the next) gets
 * a line that opens its page. */
static int setup(const char *game, int *m, int *title)
{
    char data[SYS_PATH], cfg[SYS_PATH], dir[SYS_PATH];
    int r, i, n = 0;

    for (i = 0; i < TITLES; i++)
        if (sys_find(game, titles[i].folder, dir, sizeof dir)) {
            title_labels[n] = titles[i].label;
            title_of[n++] = i;
        }
    title_labels[n] = NULL;
    sys_data_dir(data, sizeof data);
    sys_join(cfg, sizeof cfg, data, "battle-isle.cfg");
    set_fullscreen = plat_fullscreen();
    launcher_load(cfg, pages, NPAGES);
    if (set_title < 0 || set_title >= n)
        set_title = 0;
    plat_set_fullscreen(set_fullscreen);
    set_updates = update_consent() > 0;
    for (;;) {
        update_start(PORT_VERSION, PORT_UPDATE_URL);
        pages[PAGE_PORT].count = PORT_ITEMS;
        if (update_poll(&newer)) {
            snprintf(newer_label, sizeof newer_label, "%.20s is out: its page", newer.version);
            pages[PAGE_PORT].count = PORT_ITEMS + 1;
        }
        r = launcher_run(&app, NULL, pages, NPAGES, setting_changed);
        if (r != ACT_PAGE)
            break;
        update_open(newer.page);
    }
    set_fullscreen = plat_fullscreen();
    launcher_save(cfg, "battle-isle: the setup screen's settings", pages, NPAGES);
    *m = set_m;
    bi_skip_intro = set_skip;
    bi_quit_yz = set_quit;
    *title = n ? title_of[set_title] : 0;
    return r == ACT_START;
}

/* the game's files: found, or from the GOG release (its CD image
 * unpacked, its installed folder copied, or its Windows installer
 * unpacked) once the player agreed in the kit's dialog about the game's
 * files (launcher.h); 1 if there, 0 after saying why not.  Not asked when
 * -gog named the release; headless the dialog is shown only when keys are
 * scripted. */
static int get_game(const char *given, const char *gog, char *out, size_t n)
{
    char from[SYS_PATH], data[SYS_PATH], err[256];
    int dialog = plat_has_window() || getenv("DK_KEYS"), r;
    int (*progress)(void *, const char *, long, long) = dialog ? launcher_copy_progress : NULL;
    LauncherCopy copy = { &app, LAUNCHER_GAME, 0, 0 };

    if (sys_find_game(given, "BATTLE_ISLE_GAME", "ISLE/BI.EXE", out, n))
        return 1;
    if (gog && !given)
        snprintf(from, sizeof from, "%s", gog);
    else if (given || (!gog_find(&release, from, sizeof from) &&
                       !gog_find_folder(&release, from, sizeof from) &&
                       !inno_find(&release, from, sizeof from))) {
        if (dialog)
            launcher_no_game(&app, NULL);
        else
            plat_message("The game's files were not found. This program needs an installed "
                         "copy of Battle Isle (the GOG release), or -game with its folder.");
        return 0;
    }
    sys_data_dir(data, sizeof data);
    sys_join(out, n, data, "game");
    if (dialog && !gog && !launcher_offer_copy(&app, LAUNCHER_GAME, from, out))
        return 0;
    if (sys_is_dir(from))
        r = gog_copy(from, out, "ISLE/BI.EXE", progress, &copy, err, sizeof err);
    else if (inno_is_setup(from))
        r = inno_unpack(from, out, "ISLE/BI.EXE", progress, &copy, err, sizeof err);
    else
        r = cd_unpack(from, out, "ISLE/BI.EXE", progress, &copy, err, sizeof err);
    if (r == 0)
        return 1;
    if (copy.closed)
        return 0;
    if (dialog)
        launcher_copy_failed(&app, from, err);
    else
        plat_message(err);
    return 0;
}

/* the title's program of the game's folder into memory, the program's
 * files in the title's folder, its own in the data folder's `save` (or
 * the title's); 1, or 0 after saying why not */
static int load_program(const char *game, int title)
{
    char isle[SYS_PATH], exe[SYS_PATH], data[SYS_PATH], save[SYS_PATH], err[256];

    if (!sys_find(game, titles[title].folder, isle, sizeof isle) ||
        !sys_find(isle, titles[title].exe, exe, sizeof exe)) {
        snprintf(err, sizeof err, "%s/%s is not in the game's folder.", titles[title].folder, titles[title].exe);
        plat_message(err);
        return 0;
    }
    /* what the program keeps of its memory (INT 21h AH=4Ah, seen in the
     * runner): 3080h paragraphs from its PSP on */
    if (rm_load_exe(exe, titles[title].size, titles[title].sha256, RM_LOAD_PSP, 0x3080, err, sizeof err) != 0) {
        plat_message(err);
        return 0;
    }
    rm_ds = SEG(DATA);
    sys_data_dir(data, sizeof data);
    sys_join(save, sizeof save, data, titles[title].save);
    sys_mkdir(save);
    dos_set_dirs(isle, save);
    return 1;
}

int main(int argc, char **argv)
{
    const char *given = NULL, *gog = NULL;
    char game[SYS_PATH];
    char *args[8];
    int i, nargs = 1, title = 0, given_title = 0, m = 0;

    args[0] = argv[0];
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-game") && i + 1 < argc)
            given = argv[++i];
        else if (!strcmp(argv[i], "-gog") && i + 1 < argc)
            gog = argv[++i];
        else if (!strcmp(argv[i], "-title") && i + 1 < argc) {
            for (title = 0, i++; title < TITLES && strcmp(argv[i], titles[title].arg) != 0; title++)
                ;
            if (title == TITLES) {
                fprintf(stderr, "-title: isle or desert\n");
                return 2;
            }
            given_title = 1;
        } else if (argv[i][0] == '/' && nargs < 8)
            args[nargs++] = argv[i];
        else {
            fprintf(stderr, "usage: battle-isle [-game DIR | -gog FILE|FOLDER] [-title isle|desert] [/s] [/m]\n");
            return 2;
        }
    }
    sys_set_app("Battle Isle", "battle-isle");
    sys_data_migrate(old_files);
    if (!plat_init("Battle Isle"))
        return 1;
    if (!get_game(given, gog, game, sizeof game)) {
        plat_shutdown();
        return 1;
    }
    /* the setup screen: in a window; headless only in a run scripted with
     * keys that is not a comparison.  -title goes before its choice. */
    if (plat_has_window() || (getenv("DK_KEYS") && !getenv("BI_BREAK"))) {
        int chosen = 0;

        if (!setup(game, &m, &chosen)) {
            plat_shutdown();
            return 0;
        }
        if (!given_title)
            title = chosen;
        if (m && nargs < 8)
            args[nargs++] = "/m";
    }
    /* for scripted runs without the setup screen (the comparisons) */
    if (getenv("BI_SKIP_INTRO"))
        bi_skip_intro = 1;
    if (getenv("BI_QUIT_YZ"))
        bi_quit_yz = 1;
    if (!load_program(game, title)) {
        plat_shutdown();
        return 1;
    }
    battle_main(nargs, args);
    plat_shutdown();
    return 0;
}
