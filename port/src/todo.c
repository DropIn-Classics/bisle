/* todo.c - the routines of BATTLE.EXE the port does not have yet: each
 * ends the program with its name.  A routine moves from here into its
 * module when it is translated. */
#include "bi.h"

#define TODO(name) bi_todo(#name)

int change_phase(fptr a, fptr b, fptr c, fptr d) { (void)a, (void)b, (void)c, (void)d; TODO(change_phase); return -1; }
void history_add(int units0, int units1) { (void)units0, (void)units1; TODO(history_add); }
void after_map(fptr buffer) { (void)buffer; TODO(after_map); }
int save_game(fptr buffer) { (void)buffer; TODO(save_game); return 0; }
void load_game(fptr buffer) { (void)buffer; TODO(load_game); }
void play_anim(int number, fptr buffer, int a, int b, int c, fptr path) { (void)number, (void)buffer, (void)a, (void)b, (void)c, (void)path; TODO(play_anim); }
void end_credits(fptr work, fptr path) { (void)work, (void)path; TODO(end_credits); }
void computer_start(int side) { (void)side; TODO(computer_start); }
void computer_step(int side) { (void)side; TODO(computer_step); }
void computer_unit_new(int unit) { (void)unit; }
void t262a_000e(fptr text) { (void)text; TODO(t262a_000e); }
