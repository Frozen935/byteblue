#ifndef __BASE_POLL_H__
#define __BASE_POLL_H__

#include <stdint.h>

#include <base/queue/bt_queue.h>

#include <osdep/os.h>
#include <utils/bt_dlist.h>

/* private - types bit positions */
enum _poll_types_bits {
	/* can be used to ignore an event */
	_POLL_TYPE_IGNORE,

	/* to be signaled by bt_poll_signal_raise() */
	_POLL_TYPE_SIGNAL,

	/* semaphore availability */
	_POLL_TYPE_SEM_AVAILABLE,

	/* queue/FIFO data availability */
	_POLL_TYPE_DATA_AVAILABLE,

	/* msgq data availability */
	_POLL_TYPE_MSGQ_DATA_AVAILABLE,

	/* pipe data availability */
	_POLL_TYPE_PIPE_DATA_AVAILABLE,

	_POLL_NUM_TYPES
};

#define BT_POLL_TYPE_BIT(type) (1U << ((type) - 1U))

/* private - states bit positions */
enum _poll_states_bits {
	/* default state when creating event */
	_POLL_STATE_NOT_READY,

	/* signaled by bt_poll_signal_raise() */
	_POLL_STATE_SIGNALED,

	/* semaphore is available */
	_POLL_STATE_SEM_AVAILABLE,

	/* data is available to read on queue/FIFO */
	_POLL_STATE_DATA_AVAILABLE,

	/* queue/FIFO wait was cancelled */
	_POLL_STATE_CANCELLED,

	/* data is available to read on a message queue */
	_POLL_STATE_MSGQ_DATA_AVAILABLE,

	/* data is available to read from a pipe */
	_POLL_STATE_PIPE_DATA_AVAILABLE,

	_POLL_NUM_STATES
};

#define BT_POLL_STATE_BIT(state) (1U << ((state) - 1U))

#define _POLL_EVENT_NUM_UNUSED_BITS                                                                \
	(32 - (0 + 8                                    /* tag */                                  \
	       + _POLL_NUM_TYPES + _POLL_NUM_STATES + 1 /* modes */                                \
	       ))

/* end of polling API - PRIVATE */

/**
 * @defgroup poll_apis Async polling APIs
 * @ingroup kernel_apis
 * @{
 */

/* Public polling API */

/* public - values for bt_poll_event.type bitfield */
#define BT_POLL_TYPE_IGNORE              0
#define BT_POLL_TYPE_SIGNAL              BT_POLL_TYPE_BIT(_POLL_TYPE_SIGNAL)
#define BT_POLL_TYPE_SEM_AVAILABLE       BT_POLL_TYPE_BIT(_POLL_TYPE_SEM_AVAILABLE)
#define BT_POLL_TYPE_DATA_AVAILABLE      BT_POLL_TYPE_BIT(_POLL_TYPE_DATA_AVAILABLE)
#define BT_POLL_TYPE_FIFO_DATA_AVAILABLE BT_POLL_TYPE_DATA_AVAILABLE
#define BT_POLL_TYPE_MSGQ_DATA_AVAILABLE BT_POLL_TYPE_BIT(_POLL_TYPE_MSGQ_DATA_AVAILABLE)
#define BT_POLL_TYPE_PIPE_DATA_AVAILABLE BT_POLL_TYPE_BIT(_POLL_TYPE_PIPE_DATA_AVAILABLE)

/* public - polling modes */
enum bt_poll_modes {
	/* polling thread does not take ownership of objects when available */
	BT_POLL_MODE_NOTIFY_ONLY = 0,

	BT_POLL_NUM_MODES
};

/* public - values for bt_poll_event.state bitfield */
#define BT_POLL_STATE_NOT_READY           0
#define BT_POLL_STATE_SIGNALED            BT_POLL_STATE_BIT(_POLL_STATE_SIGNALED)
#define BT_POLL_STATE_SEM_AVAILABLE       BT_POLL_STATE_BIT(_POLL_STATE_SEM_AVAILABLE)
#define BT_POLL_STATE_DATA_AVAILABLE      BT_POLL_STATE_BIT(_POLL_STATE_DATA_AVAILABLE)
#define BT_POLL_STATE_FIFO_DATA_AVAILABLE BT_POLL_STATE_DATA_AVAILABLE
#define BT_POLL_STATE_MSGQ_DATA_AVAILABLE BT_POLL_STATE_BIT(_POLL_STATE_MSGQ_DATA_AVAILABLE)
#define BT_POLL_STATE_PIPE_DATA_AVAILABLE BT_POLL_STATE_BIT(_POLL_STATE_PIPE_DATA_AVAILABLE)
#define BT_POLL_STATE_CANCELLED           BT_POLL_STATE_BIT(_POLL_STATE_CANCELLED)

/* public - poll signal object */
struct bt_poll_signal {
	/** PRIVATE - DO NOT TOUCH */
	bt_dlist_t poll_events;

	/**
	 * 1 if the event has been signaled, 0 otherwise. Stays set to 1 until
	 * user resets it to 0.
	 */
	unsigned int signaled;

	/** custom result value passed to bt_poll_signal_raise() if needed */
	int result;
};

#define BT_POLL_SIGNAL_INITIALIZER(obj)                                                             \
	{                                                                                          \
		.poll_events = BT_DLIST_STATIC_INIT(&obj.poll_events), .signaled = 0,             \
		.result = 0,                                                                       \
	}
/**
 * @brief Poll Event
 *
 */

struct bt_poller {
	bool is_polling;
	os_sem_t sem;
};

struct bt_poll_event {
	/** PRIVATE - DO NOT TOUCH */
	bt_dnode_t _node;

	/** PRIVATE - DO NOT TOUCH */
	struct bt_poller *poller;

	/** optional user-specified tag, opaque, untouched by the API */
	uint32_t tag: 8;

	/** bitfield of event types (bitwise-ORed BT_POLL_TYPE_xxx values) */
	uint32_t type: _POLL_NUM_TYPES;

	/** bitfield of event states (bitwise-ORed BT_POLL_STATE_xxx values) */
	uint32_t state: _POLL_NUM_STATES;

	/** mode of operation, from enum bt_poll_modes */
	uint32_t mode: 1;

	/** unused bits in 32-bit word */
	uint32_t unused: _POLL_EVENT_NUM_UNUSED_BITS;

	/** per-type data */
	union {
		/* The typed_* fields below are used by BT_POLL_EVENT_*INITIALIZER() macros to ensure
		 * type safety of polled objects.
		 */
		void *obj, *typed_BT_POLL_TYPE_IGNORE;
		struct bt_poll_signal *signal, *typed_BT_POLL_TYPE_SIGNAL;
		struct bt_queue *queue, *typed_BT_POLL_TYPE_DATA_AVAILABLE;
	};
};

#define BT_POLL_EVENT_INITIALIZER(_event_type, _event_mode, _event_obj)                             \
	{                                                                                          \
		.poller = NULL, .type = _event_type, .state = BT_POLL_STATE_NOT_READY,              \
		.mode = _event_mode, .unused = 0,                                                  \
		{                                                                                  \
			.typed_##_event_type = _event_obj,                                         \
		},                                                                                 \
	}

#define BT_POLL_EVENT_STATIC_INITIALIZER(_event_type, _event_mode, _event_obj, event_tag)           \
	{                                                                                          \
		.tag = event_tag, .type = _event_type, .state = BT_POLL_STATE_NOT_READY,            \
		.mode = _event_mode, .unused = 0,                                                  \
		{                                                                                  \
			.typed_##_event_type = _event_obj,                                         \
		},                                                                                 \
	}

void bt_poll_event_init(struct bt_poll_event *event, uint32_t type, int mode, void *obj);
int bt_poll(struct bt_poll_event *events, int num_events, os_timeout_t timeout);

void bt_poll_handle_obj_events(bt_dlist_t *events, uint32_t state);
int bt_poll_signal_raise(struct bt_poll_signal *sig, int result);

#endif /* __BASE_POLL_H__ */
