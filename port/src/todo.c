/* todo.c - the routines of BATTLE.EXE the port does not have yet: each
 * ends the program with its name.  A routine moves from here into its
 * module when it is translated. */
#include "bi.h"

#define TODO(name) bi_todo(#name)

void computer_start(int side) { (void)side; TODO(computer_start); }
void computer_step(int side) { (void)side; TODO(computer_step); }
void computer_unit_new(int unit) { (void)unit; }
