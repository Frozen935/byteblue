/*
 * STACK_INIT registry traversal (section-based aggregator)
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bt_stack_init.h"

extern int bt_driver_userchan_init(void);
extern int bt_work_main_work_init(void);
extern int bt_driver_h4_init(void);

int bt_stack_init_once(void)
{
	bt_work_main_work_init();
#ifdef CONFIG_OPENBLUE_BT_DRIVER_TYPE_USERCHAN
	bt_driver_userchan_init();
#endif

#ifdef CONFIG_OPENBLUE_BT_DRIVER_TYPE_H4
	bt_driver_h4_init();
#endif

	return 0;
}
