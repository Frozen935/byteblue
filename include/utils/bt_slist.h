#ifndef __INCLUDE_LIST_SLIST_H__
#define __INCLUDE_LIST_SLIST_H__

#include <stdbool.h>
#include <stddef.h> /* for NULL */

/* Singly linked list node */
typedef struct sys_snode {
	struct sys_snode *next;
} bt_snode_t;

/* Singly linked list head/tail */
typedef struct bt_slist {
	bt_snode_t *head;
	bt_snode_t *tail;
} bt_slist_t;

static inline void bt_slist_init(bt_slist_t *list)
{
	list->head = NULL;
	list->tail = NULL;
}

static inline bool bt_slist_is_empty(const bt_slist_t *list)
{
	return list->head == NULL;
}

static inline void bt_slist_prepend(bt_slist_t *list, bt_snode_t *node)
{
	/* Insert at head */
	node->next = list->head;
	list->head = node;
	if (list->tail == NULL) {
		list->tail = node;
	}
}

static inline void bt_slist_append(bt_slist_t *list, bt_snode_t *node)
{
	/* Insert at tail */
	node->next = NULL;
	if (list->tail) {
		list->tail->next = node;
	} else {
		/* List was empty */
		list->head = node;
	}
	list->tail = node;
}

static inline void bt_slist_insert(bt_slist_t *list, bt_snode_t *prev, bt_snode_t *node)
{
	if (prev == NULL) {
		bt_slist_prepend(list, node);
		return;
	}

	node->next = prev->next;
	prev->next = node;

	if (list->tail == prev) {
		list->tail = node;
	}
}

static inline bt_snode_t *bt_slist_peek_head(const bt_slist_t *list)
{
	return list->head;
}

static inline bt_snode_t *bt_slist_peek_tail(const bt_slist_t *list)
{
	return list->tail;
}

static inline bt_snode_t *bt_slist_peek_next(const bt_snode_t *node)
{
	return node ? node->next : NULL;
}

static inline bt_snode_t *bt_slist_get(bt_slist_t *list)
{
	bt_snode_t *head = list->head;
	if (head == NULL) {
		return NULL;
	}

	list->head = head->next;
	if (list->head == NULL) {
		/* List became empty */
		list->tail = NULL;
	}

	/* Detach node */
	head->next = NULL;
	return head;
}

static inline bt_snode_t *bt_slist_get_not_empty(bt_slist_t *list)
{
	/* Caller guarantees non-empty; behave like bt_slist_get but skip checks */
	bt_snode_t *head = list->head;

	/* In case assertions are disabled in project, keep minimal safety */
	if (head == NULL) {
		return NULL;
	}

	list->head = head->next;
	if (list->head == NULL) {
		list->tail = NULL;
	}

	head->next = NULL;
	return head;
}

static inline void bt_slist_remove(bt_slist_t *list, bt_snode_t *prev, bt_snode_t *node)
{
	if (prev == NULL) {
		/* Removing head */
		if (list->head == node) {
			list->head = node->next;
			if (list->head == NULL) {
				list->tail = NULL;
			}
		}
	} else {
		/* Removing after prev */
		if (prev->next == node) {
			prev->next = node->next;
			if (node->next == NULL) {
				/* Removed tail */
				list->tail = prev;
			}
		}
	}

	/* Detach node */
	node->next = NULL;
}

static inline bool bt_slist_find(const bt_slist_t *list, const bt_snode_t *node,
				 bt_snode_t **prev_out)
{
	bt_snode_t *prev = NULL;
	bt_snode_t *cur = list->head;

	while (cur != NULL) {
		if (cur == node) {
			if (prev_out) {
				*prev_out = prev;
			}
			return true;
		}
		prev = cur;
		cur = cur->next;
	}

	if (prev_out) {
		*prev_out = NULL;
	}
	return false;
}

static inline bool bt_slist_find_and_remove(bt_slist_t *list, bt_snode_t *node)
{
	bt_snode_t *prev = NULL;

	if (bt_slist_find(list, node, &prev)) {
		bt_slist_remove(list, prev, node);
		return true;
	}

	return false;
}

static inline size_t bt_slist_len(const bt_slist_t *list)
{
	size_t len = 0;
	bt_snode_t *cur = list->head;

	while (cur != NULL) {
		len++;
		cur = cur->next;
	}

	return len;
}

/* Static initializer for an empty list (argument ignored; kept for API parity) */
#define BT_SLIST_STATIC_INIT(_list_ptr)                                                            \
	{                                                                                          \
		.head = NULL, .tail = NULL                                                         \
	}

/* Iterate raw nodes */
#define BT_SLIST_FOR_EACH_NODE(_list, _node)                                                       \
	for ((_node) = bt_slist_peek_head((_list)); (_node) != NULL;                               \
	     (_node) = bt_slist_peek_next((_node)))

#define BT_SLIST_FOR_EACH_NODE_SAFE(_list, _node, _tmp)                                            \
	for ((_node) = bt_slist_peek_head((_list)), (_tmp) = bt_slist_peek_next((_node));          \
	     (_node) != NULL; (_node) = (_tmp), (_tmp) = bt_slist_peek_next((_node)))

/* Peek first container from list */
#define BT_SLIST_PEEK_HEAD_CONTAINER(_list, _container, _member)                                   \
	(bt_slist_peek_head((_list))                                                               \
		 ? CONTAINER_OF(bt_slist_peek_head((_list)), __typeof__(*(_container)), _member)   \
		 : NULL)

#define BT_SLIST_PEEK_TAIL_CONTAINER(_list, _container, _member)                                   \
	(bt_slist_peek_tail((_list))                                                               \
		 ? CONTAINER_OF(bt_slist_peek_tail((_list)), __typeof__(*(_container)), _member)   \
		 : NULL)

/* Peek next container from current container */
#define BT_SLIST_PEEK_NEXT_CONTAINER(_container, _member)                                          \
	(bt_slist_peek_next(&(_container)->_member)                                                \
		 ? CONTAINER_OF(bt_slist_peek_next(&(_container)->_member),                        \
				__typeof__(*(_container)), _member)                                \
		 : NULL)

/* Iterate containers of type *_container using member field *_member */
#define BT_SLIST_FOR_EACH_CONTAINER(_list, _container, _member)                                    \
	for ((_container) = BT_SLIST_PEEK_HEAD_CONTAINER((_list), (_container), _member);          \
	     (_container) != NULL;                                                                 \
	     (_container) = BT_SLIST_PEEK_NEXT_CONTAINER((_container), _member))

/* Safe iteration variant that caches next before the loop body */
#define BT_SLIST_FOR_EACH_CONTAINER_SAFE(_list, _container, _tmp, _member)                         \
	for ((_container) = BT_SLIST_PEEK_HEAD_CONTAINER((_list), (_container), _member),          \
	    (_tmp) = (_container) ? BT_SLIST_PEEK_NEXT_CONTAINER((_container), _member) : NULL;    \
	     (_container) != NULL; (_container) = (_tmp),                                          \
	    (_tmp) = (_container) ? BT_SLIST_PEEK_NEXT_CONTAINER((_container), _member) : NULL)

#endif /* __INCLUDE_LIST_SLIST_H__ */
