#ifndef __INCLUDE_BT_STACK_INIT_H__
#define __INCLUDE_BT_STACK_INIT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STACK_BASE_INIT
#define STACK_BASE_INIT 1
#endif
#ifndef STACK_RUN_INIT
#define STACK_RUN_INIT 2
#endif
#ifndef STACK_SVC_INIT
#define STACK_SVC_INIT 3
#endif

/* Function prototype for init callbacks */
typedef int (*stack_init_fn_t)(void);

struct stack_init_entry {
	stack_init_fn_t init; /* initializer function */
	uint16_t prio;        /* priority within level (ascending) */
	uint16_t level;       /* broad level ordering (ascending) */
	const char *name;     /* optional name for diagnostics */
};

/* Section name for registration; avoid leading dot for start/stop symbols */
#define STACK_INIT_SECTION "stack_init"

#define STACK_INIT(fn, level, prio)                                                                \
	const struct stack_init_entry                                                              \
		__attribute__((section("." STACK_INIT_SECTION), used)) __stack_init_entry_##fn = { \
			(fn), (uint16_t)(prio), (uint16_t)(level), #fn}

/* Public API: run all registered initializers */
int bt_stack_init_once(void);

#ifdef __cplusplus
}
#endif

#endif /* __STACK_INIT_H__ */
