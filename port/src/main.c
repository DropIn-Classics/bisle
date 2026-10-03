/* main.c - Battle Isle: a native compatibility implementation requiring an
 * installed copy of the original game.
 *
 *     battle-isle [-game DIR | -gog FILE|FOLDER|SETUP.exe] [-title isle|desert|moon] [/s] [/m]
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
#include "audiofx.h"
#include "cdimage.h"
#include "frame.h"
#include "hud.h"
#include "inno.h"
#include "launcher.h"
#include "pad.h"
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
    int prog;               /* BI_GAME or BI_MOON: which names' column */
    uint16_t psp, paragraphs;    /* what the program keeps of its memory (INT 21h AH=4Ah, last call, seen in the runner) */
} titles[] = {
    { "isle", "Battle Isle", "ISLE", "BATTLE.EXE", BATTLE_SIZE, BATTLE_SHA256, "save", BI_GAME, 0x0067, 0x3080 },
    { "desert", "Scenario Disk 1 - Air-Land-Sea", "DESERT", "DESERT.EX2", 212242ul,
      "1042af4eb941908cae776cc2118b4664c16b56975ec9353ffa05633626a1cff4", "save-desert", BI_GAME, 0x0067, 0x3080 },
    { "moon", "Scenario Disk 2 - Moon", "MOON", "MOON.EXE", 216720ul,
      "b8a8ed7a342d3bf1d15082d8ec71c880f846e574cf04c8ad87738ac8d2b6c3b3", "save-moon", BI_MOON, 0x0066, 0x3180 },
};

#define TITLES ((int)(sizeof titles / sizeof titles[0]))

/* ---- the setup screen (launcher.h; doskit/docs/LAUNCHER.md) ---- */

enum { ACT_START = 1, ACT_PAGE };

static const char *const no_yes[] = { "No", "Yes", NULL };

static int set_fullscreen, set_updates, set_title, set_skip, set_quit = 1;
/* the titles whose folders are there, for the setup screen's choice */
static const char *title_labels[TITLES + 1];
static int title_of[TITLES];
static UpdateInfo newer;
static char newer_label[64];

/* ---- the sound, the keys and the controller (the port's, not the game's) ---- */

static int set_volume = 10, set_headphone, muted;

static const char *const volume_values[] = { "0 (off)", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", NULL };

/* The players' keys in a map: each player has a table of records (a
 * scancode, then left, right, up, down, fire; DATA:0AE9 and 0AEB point to
 * them, read from the program's data): player 1 the keypad (the arrows
 * send the same scancodes) and Space or Enter to fire, player 2 X, V,
 * D or F, C and Alt or the left Ctrl.  The player's key for one of them
 * is turned into that one (frame.h's keymap), so the game's other keys
 * for the same stay. */
enum { K_P1_UP, K_P1_DOWN, K_P1_LEFT, K_P1_RIGHT, K_P1_FIRE,
       K_P2_UP, K_P2_DOWN, K_P2_LEFT, K_P2_RIGHT, K_P2_FIRE, K_COUNT };
static const int game_key[K_COUNT] = { 0x48, 0x50, 0x4B, 0x4D, 0x39, 0x20, 0x2E, 0x2D, 0x2F, 0x1D };
static int player_key[K_COUNT] = { 0x48, 0x50, 0x4B, 0x4D, 0x39, 0x20, 0x2E, 0x2D, 0x2F, 0x1D };

/* what a controller's button gives: a game key above, or one of these
 * (Esc leaves and asks to quit, Enter and the arrows run the menus, F1
 * carries out the change of mode both players asked for: the original
 * reads it from the keyboard only, so a button must send it; Y and N
 * answer, D saves) */
enum { P_NONE, P_KEYS, P_ESC = P_KEYS + K_COUNT, P_ENTER, P_F1, P_Y, P_N, P_D, P_COUNT };
static const int other_key[] = { 0x01, 0x1C, 0x3B, 0x15, 0x31, 0x20 };
static const char *const pad_actions[] = {
    "nothing", "P1 up", "P1 down", "P1 left", "P1 right", "P1 fire",
    "P2 up", "P2 down", "P2 left", "P2 right", "P2 fire",
    "back / quit (Esc)", "Enter", "change mode (F1)", "yes (Y)", "no (N)", "save (D)", NULL
};
static int pad_choice[PAD_BUTTONS] = {
    [PAD_A] = P_KEYS + K_P1_FIRE, [PAD_B] = P_ESC, [PAD_X] = P_ENTER, [PAD_Y] = P_F1,
    [PAD_BACK] = P_N, [PAD_START] = P_Y, [PAD_LB] = P_KEYS + K_P1_FIRE,
    [PAD_RB] = P_KEYS + K_P1_FIRE, [PAD_UP] = P_KEYS + K_P1_UP,
    [PAD_DOWN] = P_KEYS + K_P1_DOWN, [PAD_LEFT] = P_KEYS + K_P1_LEFT,
    [PAD_RIGHT] = P_KEYS + K_P1_RIGHT,
};

static unsigned char keymap[256];
static PadKeys play_keys;

/* the keymap and the controller's table from the settings */
static void apply_keys(void)
{
    int i, k;

    for (i = 0; i < 256; i++)
        keymap[i] = (unsigned char)i;
    for (k = 0; k < K_COUNT; k++)
        if (player_key[k] && player_key[k] != game_key[k])
            keymap[player_key[k]] = (unsigned char)game_key[k];
    frame_set_keymap(keymap);
    memset(play_keys, 0, sizeof play_keys);
    for (i = 0; i < PAD_BUTTONS; i++) {
        int a = pad_choice[i], code = 0;

        if (a >= P_KEYS && a < P_ESC) {
            /* the player's key, which the keymap turns into the game's */
            k = a - P_KEYS;
            code = player_key[k] ? player_key[k] : game_key[k];
        } else if (a >= P_ESC && a < P_COUNT)
            code = other_key[a - P_ESC];
        play_keys[i][0] = (unsigned char)code;
    }
    pad_set_keys(&play_keys);
}

static void apply_sound(void)
{
    plat_audio_lock();
    audiofx_set(0, 0, 0, set_headphone);
    plat_audio_unlock();
}

float bi_volume_gain(void)
{
    return muted ? 0.0f : (float)set_volume / 10.0f;
}

/* the keypad's + and - and * (mute) while the game runs: doskit's hud.h
 * box at the top of the picture for two seconds */
static void hud_control(int c)
{
    if (c == PLAT_VOLUME_UP || c == PLAT_VOLUME_DOWN) {
        if (c == PLAT_VOLUME_UP && set_volume < 10)
            set_volume++;
        else if (c == PLAT_VOLUME_DOWN && set_volume > 0)
            set_volume--;
        muted = 0;
    } else if (c == PLAT_MUTE)
        muted = !muted;
    else
        return;
    if (muted)
        hud_show("MUTE", 0, 0, 140);
    else
        hud_show("VOLUME", set_volume, 10, 140);
}

/* the menu, and a page for each group of settings (doskit/docs/LAUNCHER.md) */
enum { PAGE_MENU, PAGE_SOUND, PAGE_KEYS, PAGE_PAD, PAGE_QOL, PAGE_PORT };

static LauncherItem menu_items[] = {
    { LI_ACTION, "Start the game", NULL, NULL, NULL, ACT_START, NULL },
    { LI_CHOICE, "Game", "title", title_labels, &set_title, 0,
      "Battle Isle or one of the data disks." },
    { LI_CHOICE, "Full screen", "fullscreen", no_yes, &set_fullscreen, 0,
      "Alt+Enter changes it while the game runs." },
    { LI_HEAD, "", NULL, NULL, NULL, 0, NULL },
    { LI_PAGE, "Sound", NULL, NULL, NULL, PAGE_SOUND, "Volume and headphones." },
    { LI_PAGE, "Keys", NULL, NULL, NULL, PAGE_KEYS, "The players' keys in a map." },
    { LI_PAGE, "Controller", NULL, NULL, NULL, PAGE_PAD, "What a controller's buttons do." },
    { LI_PAGE, "Quality of Life changes", NULL, NULL, NULL, PAGE_QOL,
      "Improvements to the gameplay experience." },
    { LI_PAGE, "This port", NULL, NULL, NULL, PAGE_PORT, "New versions." },
};

static LauncherItem qol_items[] = {
    { LI_CHOICE, "Skip logo, intro, and title", "skip_intro", no_yes, &set_skip, 0,
      "Starts at the main menu." },
    { LI_CHOICE, "Quit key Y and Z", "quit_yz", no_yes, &set_quit, 0,
      "QUIT THE GAME takes Y and Z on any keyboard layout." },
};

static LauncherItem sound_items[] = {
    { LI_CHOICE, "Volume", "volume", volume_values, &set_volume, 0,
      "In the game: keypad + and -, * mutes." },
    { LI_CHOICE, "Headphones", "headphone", no_yes, &set_headphone, 0,
      "A wider stereo picture for headphones." },
};

#define KEY_ITEM(k, label, name, help) { LI_KEY, label, name, NULL, &player_key[k], 0, help }
static LauncherItem key_items[] = {
    { LI_HEAD, "Player 1", NULL, NULL, NULL, 0, NULL },
    KEY_ITEM(K_P1_UP, "Up", "key_p1_up", "The arrow and the keypad's 8 stay too."),
    KEY_ITEM(K_P1_DOWN, "Down", "key_p1_down", "The arrow and the keypad's 2 stay too."),
    KEY_ITEM(K_P1_LEFT, "Left", "key_p1_left", "The arrow and the keypad's 4 stay too."),
    KEY_ITEM(K_P1_RIGHT, "Right", "key_p1_right", "The arrow and the keypad's 6 stay too."),
    KEY_ITEM(K_P1_FIRE, "Fire", "key_p1_fire", "Enter stays too."),
    { LI_HEAD, "Player 2", NULL, NULL, NULL, 0, NULL },
    KEY_ITEM(K_P2_UP, "Up", "key_p2_up", "F stays too."),
    KEY_ITEM(K_P2_DOWN, "Down", "key_p2_down", NULL),
    KEY_ITEM(K_P2_LEFT, "Left", "key_p2_left", NULL),
    KEY_ITEM(K_P2_RIGHT, "Right", "key_p2_right", NULL),
    KEY_ITEM(K_P2_FIRE, "Fire", "key_p2_fire", "Alt stays too."),
};

#define PAD_ITEM(b, label) { LI_CHOICE, label, NULL, pad_actions, &pad_choice[b], 0, NULL }
static LauncherItem pad_items[] = {
    PAD_ITEM(PAD_A, "A"), PAD_ITEM(PAD_B, "B"), PAD_ITEM(PAD_X, "X"), PAD_ITEM(PAD_Y, "Y"),
    PAD_ITEM(PAD_LB, "Left shoulder"), PAD_ITEM(PAD_RB, "Right shoulder"),
    PAD_ITEM(PAD_LT, "Left trigger"), PAD_ITEM(PAD_RT, "Right trigger"),
    PAD_ITEM(PAD_LSTICK, "Left stick pressed"), PAD_ITEM(PAD_RSTICK, "Right stick pressed"),
    PAD_ITEM(PAD_START, "Start"), PAD_ITEM(PAD_BACK, "Back"),
    PAD_ITEM(PAD_UP, "D-pad up"), PAD_ITEM(PAD_DOWN, "D-pad down"),
    PAD_ITEM(PAD_LEFT, "D-pad left"), PAD_ITEM(PAD_RIGHT, "D-pad right"),
};

/* the buttons' names in the settings file: pad_ and pad.h's name */
static void pad_names(void)
{
    static char names[PAD_BUTTONS][24];
    size_t i, b;

    for (i = 0; i < sizeof pad_items / sizeof pad_items[0]; i++) {
        b = (size_t)(pad_items[i].value - pad_choice);
        snprintf(names[b], sizeof names[b], "pad_%s", pad_button_name((int)b));
        pad_items[i].name = names[b];
    }
}

static LauncherItem port_items[] = {
    { LI_CHOICE, "Look for new versions", NULL, no_yes, &set_updates, 0,
      "One small file from GitHub, at most once a day; nothing is sent." },
    /* the line of a newer release: counted only when there is one */
    { LI_ACTION, newer_label, NULL, NULL, NULL, ACT_PAGE, "Opens the release's page in the browser." },
};

#define PORT_ITEMS ((int)(sizeof port_items / sizeof port_items[0]) - 1)

static LauncherPage pages[] = {
    { "Setup", menu_items, (int)(sizeof menu_items / sizeof menu_items[0]) },
    { "Sound", sound_items, (int)(sizeof sound_items / sizeof sound_items[0]) },
    { "Keys", key_items, (int)(sizeof key_items / sizeof key_items[0]) },
    { "Controller", pad_items, (int)(sizeof pad_items / sizeof pad_items[0]) },
    { "Quality of Life changes", qol_items, (int)(sizeof qol_items / sizeof qol_items[0]) },
    { "This port", port_items, PORT_ITEMS },
};

#define NPAGES ((int)(sizeof pages / sizeof pages[0]))

static void setting_changed(const LauncherItem *item)
{
    if (item->value == &set_fullscreen)
        plat_set_fullscreen(set_fullscreen);
    else if (item->value == &set_headphone)
        apply_sound();
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
    pad_names();
    launcher_load(cfg, pages, NPAGES);
    apply_sound();
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
    *m = 0;         /* /m is on the command line only: the game's own menu has the palettes */
    bi_skip_intro = set_skip;
    bi_quit_yz = set_quit;
    *title = n ? title_of[set_title] : 0;
    if (r != ACT_START)
        return 0;
    apply_keys();
    frame_set_hud(hud_draw, hud_control);
    return 1;
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
    if (rm_load_exe(exe, titles[title].size, titles[title].sha256, titles[title].psp, titles[title].paragraphs, err, sizeof err) != 0) {
        plat_message(err);
        return 0;
    }
    bi_program(titles[title].prog);
    rm_ds = bi_data_seg(titles[title].prog);
    sys_data_dir(data, sizeof data);
    sys_join(save, sizeof save, data, titles[title].save);
    sys_mkdir(save);
    dos_set_dirs(isle, save);
    return 1;
}

/* The intro of 256 colours, INTEGA/INTRO.EXE, which GOG's start runs
 * before the game (BI.EXE, there the branch for the EGA): loaded as the
 * game is, its own files in its folder, played to its end or to Esc.  The
 * game's program is loaded after it.  A folder without the intro starts
 * the game.  Only Battle Isle itself has it. */
static void run_intro(const char *game)
{
    char isle[SYS_PATH], dir[SYS_PATH], exe[SYS_PATH], data[SYS_PATH], save[SYS_PATH], err[256];

    if (!sys_find(game, titles[0].folder, isle, sizeof isle) || !sys_find(isle, "INTEGA", dir, sizeof dir) ||
        !sys_find(dir, "INTRO.EXE", exe, sizeof exe))
        return;
    /* what the program keeps of its memory (the startup's INT 21h AH=4Ah, seen in the runner: 0A65h, then 0A80h and 0AC0h) */
    if (rm_load_exe(exe, INTEGA_SIZE, INTEGA_SHA256, RM_LOAD_PSP, 0x0AC0, err, sizeof err) != 0) {
        plat_message(err);
        return;
    }
    bi_program(BI_INTRO);
    rm_ds = bi_data_seg(BI_INTRO);
    sys_data_dir(data, sizeof data);
    sys_join(save, sizeof save, data, titles[0].save);
    sys_mkdir(save);
    dos_set_dirs(dir, save);
    intro_main();
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
                fprintf(stderr, "-title: isle, desert or moon\n");
                return 2;
            }
            given_title = 1;
        } else if (argv[i][0] == '/' && nargs < 8)
            args[nargs++] = argv[i];
        else {
            fprintf(stderr, "usage: battle-isle [-game DIR | -gog FILE|FOLDER] [-title isle|desert|moon] [/s] [/m]\n");
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
    /* The intro in a window, unless skipped.  Headless it is for the
     * comparisons with the original only: BI_INTRO=only is the intro alone,
     * BI_INTRO=1 the intro and the game (the game's comparisons stay as
     * they were without it). */
    if (title == 0 && (getenv("BI_INTRO") || (plat_has_window() && !bi_skip_intro)))
        run_intro(game);
    if (getenv("BI_INTRO") && !strcmp(getenv("BI_INTRO"), "only")) {
        plat_shutdown();
        return 0;
    }
    if (!load_program(game, title)) {
        plat_shutdown();
        return 1;
    }
    battle_main(nargs, args);
    plat_shutdown();
    return 0;
}
