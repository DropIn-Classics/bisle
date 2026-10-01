/* todo.c - the routines of BATTLE.EXE the port does not have yet: each
 * ends the program with its name.  A routine moves from here into its
 * module when it is translated. */
#include "bi.h"

#define TODO(name) bi_todo(#name)

int move_aim(fptr cur, fptr map, int side) { (void)cur, (void)map, (void)side; TODO(move_aim); return 0; }
int t122d_05b8(fptr cur, int side) { (void)cur, (void)side; TODO(t122d_05b8); return 0; }
void unit_release(fptr cur, fptr map, int side, int kind) { (void)cur, (void)map, (void)side, (void)kind; TODO(unit_release); }
void move_step(void) { TODO(move_step); }
int stop_check(int off, int side, fptr map) { (void)off, (void)side, (void)map; TODO(stop_check); return 0; }
void draw_shop_window(int side) { (void)side; TODO(draw_shop_window); }
void draw_unit_info(int side, int square, int unit, fptr map) { (void)side, (void)square, (void)unit, (void)map; TODO(draw_unit_info); }
void draw_status(int side) { (void)side; TODO(draw_status); }
void draw_building(int side, fptr rec) { (void)side, (void)rec; TODO(draw_building); }
void list_makeable(int energy) { (void)energy; TODO(list_makeable); }
void draw_slots(int x, int y, int side, fptr rec) { (void)x, (void)y, (void)side, (void)rec; TODO(draw_slots); }
void draw_unit_numbers(int x, int y, int unit, int side) { (void)x, (void)y, (void)unit, (void)side; TODO(draw_unit_numbers); }
void draw_type_list(int x, int y, int side, int first) { (void)x, (void)y, (void)side, (void)first; TODO(draw_type_list); }
void t1479_028e(int x, int y) { (void)x, (void)y; TODO(t1479_028e); }
void t1479_0c93(int x, int y, int type, int side, int owner) { (void)x, (void)y, (void)type, (void)side, (void)owner; TODO(t1479_0c93); }
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
