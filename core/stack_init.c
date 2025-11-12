/*
 * STACK_INIT registry traversal (section-based aggregator)
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bt_stack_init.h"

/* Start/Stop symbols provided by GNU ld for the section named exactly
 * as used in the attribute. No custom linker script required.
 */
extern const struct stack_init_entry __start_stack_init[];
extern const struct stack_init_entry __stop_stack_init[];

extern const struct stack_init_entry __stack_init_entry_main_work_init;
extern const struct stack_init_entry __stack_init_entry_uc_init;

extern int bt_driver_userchan_init(void);
extern int bt_work_main_work_init(void);
int bt_stack_init_once(void)
{
	bt_work_main_work_init();
	bt_driver_userchan_init();

	return 0;
}
