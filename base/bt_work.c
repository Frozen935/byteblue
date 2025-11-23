
#include <stdint.h>

#include <base/bt_work.h>
#include <osdep/os.h>

struct bt_work_q main_work_q;

static inline void flag_clear(uint32_t *flagp, uint32_t bit)
{
	*flagp &= ~BIT(bit);
}

static inline void flag_set(uint32_t *flagp, uint32_t bit)
{
	*flagp |= BIT(bit);
}

static inline bool flag_test(const uint32_t *flagp, uint32_t bit)
{
	return (*flagp & BIT(bit)) != 0U;
}

static inline bool flag_test_and_clear(uint32_t *flagp, int bit)
{
	bool ret = flag_test(flagp, bit);

	flag_clear(flagp, bit);

	return ret;
}

static inline void flags_set(uint32_t *flagp, uint32_t flags)
{
	*flagp = flags;
}

static inline uint32_t flags_get(const uint32_t *flagp)
{
	return *flagp;
}

/* Lock to protect the internal state of all work items, work queues,
 * and pending_cancels.
 */
static os_mutex_t lock;

/* Invoked by work thread */
static void handle_flush(struct bt_work *work)
{
}

static inline void init_flusher(struct bt_work_flusher *flusher)
{
	struct bt_work *work = &flusher->work;
	os_sem_init(&flusher->sem, 0, 1);
	bt_work_init(&flusher->work, handle_flush);
	flag_set(&work->flags, BT_WORK_FLUSHING_BIT);
}

/* List of pending cancellations. */
static bt_slist_t pending_cancels;

static inline void init_work_cancel(struct bt_work_canceller *canceler, struct bt_work *work)
{
	os_sem_init(&canceler->sem, 0, 1);
	canceler->work = work;
	bt_slist_append(&pending_cancels, &canceler->node);
}

static void finalize_flush_locked(struct bt_work *work)
{
	struct bt_work_flusher *flusher = CONTAINER_OF(work, struct bt_work_flusher, work);

	flag_clear(&work->flags, BT_WORK_FLUSHING_BIT);

	os_sem_give(&flusher->sem);
};

static void finalize_cancel_locked(struct bt_work *work)
{
	struct bt_work_canceller *wc, *tmp;
	bt_snode_t *prev = NULL;

	/* Clear this first, so released high-priority threads don't
	 * see it when doing things.
	 */
	flag_clear(&work->flags, BT_WORK_CANCELING_BIT);

	BT_SLIST_FOR_EACH_CONTAINER_SAFE(&pending_cancels, wc, tmp, node) {
		if (wc->work == work) {
			bt_slist_remove(&pending_cancels, prev, &wc->node);
			os_sem_give(&wc->sem);
			break;
		}
		prev = &wc->node;
	}
}

void bt_work_init(struct bt_work *work, bt_work_handler_t handler)
{
	__ASSERT_NO_MSG(work != NULL);
	__ASSERT_NO_MSG(handler != NULL);

	*work = (struct bt_work)BT_WORK_INITIALIZER(handler);
}

static inline int work_busy_get_locked(const struct bt_work *work)
{
	return flags_get(&work->flags) & BT_WORK_MASK;
}

int bt_work_busy_get(const struct bt_work *work)
{
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	int ret = work_busy_get_locked(work);

	os_mutex_unlock(&lock);

	return ret;
}

static void queue_flusher_locked(struct bt_work_q *queue, struct bt_work *work,
				 struct bt_work_flusher *flusher)
{
	init_flusher(flusher);

	if ((flags_get(&work->flags) & BT_WORK_QUEUED) != 0U) {
		bt_slist_insert(&queue->pending, &work->node, &flusher->work.node);
	} else {
		bt_slist_prepend(&queue->pending, &flusher->work.node);
	}
}

static inline void queue_remove_locked(struct bt_work_q *queue, struct bt_work *work)
{
	if (flag_test_and_clear(&work->flags, BT_WORK_QUEUED_BIT)) {
		(void)bt_slist_find_and_remove(&queue->pending, &work->node);
	}
}

static inline bool notify_queue_locked(struct bt_work_q *queue)
{
	bool rv = false;

	if (queue != NULL) {
		rv = os_sem_give(&queue->notifyq);
	}

	return rv;
}

static inline int queue_submit_locked(struct bt_work_q *queue, struct bt_work *work)
{
	if (queue == NULL) {
		return -EINVAL;
	}

	int ret;
	bool chained = (os_thread_self() == queue->thread_id);
	bool draining = flag_test(&queue->flags, BT_WORK_QUEUE_DRAIN_BIT);
	bool plugged = flag_test(&queue->flags, BT_WORK_QUEUE_PLUGGED_BIT);

	/* Test for acceptability, in priority order:
	 *
	 * * -ENODEV if the queue isn't running.
	 * * -EBUSY if draining and not chained
	 * * -EBUSY if plugged and not draining
	 * * otherwise OK
	 */
	if (!flag_test(&queue->flags, BT_WORK_QUEUE_STARTED_BIT)) {
		ret = -ENODEV;
	} else if (draining && !chained) {
		ret = -EBUSY;
	} else if (plugged && !draining) {
		ret = -EBUSY;
	} else {
		bt_slist_append(&queue->pending, &work->node);
		ret = 1;
		(void)notify_queue_locked(queue);
	}

	return ret;
}

static int submit_to_queue_locked(struct bt_work *work, struct bt_work_q **queuep)
{
	int ret = 0;

	if (flag_test(&work->flags, BT_WORK_CANCELING_BIT)) {
		/* Disallowed */
		ret = -EBUSY;
	} else if (!flag_test(&work->flags, BT_WORK_QUEUED_BIT)) {
		/* Not currently queued */
		ret = 1;

		/* If no queue specified resubmit to last queue.
		 */
		if (*queuep == NULL) {
			*queuep = work->queue;
		}

		/* If the work is currently running we have to use the
		 * queue it's running on to prevent handler
		 * re-entrancy.
		 */
		if (flag_test(&work->flags, BT_WORK_RUNNING_BIT)) {
			__ASSERT_NO_MSG(work->queue != NULL);
			*queuep = work->queue;
			ret = 2;
		}

		int rc = queue_submit_locked(*queuep, work);

		if (rc < 0) {
			ret = rc;
		} else {
			flag_set(&work->flags, BT_WORK_QUEUED_BIT);
			work->queue = *queuep;
		}
	} else {
		/* Already queued, do nothing. */
	}

	if (ret <= 0) {
		*queuep = NULL;
	}

	return ret;
}

static int work_submit_to_queue(struct bt_work_q *queue, struct bt_work *work)
{
	__ASSERT_NO_MSG(work != NULL);
	__ASSERT_NO_MSG(work->handler != NULL);

	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	int ret = submit_to_queue_locked(work, &queue);

	os_mutex_unlock(&lock);

	return ret;
}

int bt_work_submit_to_queue(struct bt_work_q *queue, struct bt_work *work)
{
	int ret = work_submit_to_queue(queue, work);

	/* submit_to_queue_locked() won't reschedule on its own
	 * (really it should, otherwise this process will result in
	 * spurious calls to z_swap() due to the race), so do it here
	 * if the queue state changed.
	 */
	if (ret > 0) {
		os_thread_yield();
	}

	return ret;
}

int bt_work_submit(struct bt_work *work)
{
	int ret = bt_work_submit_to_queue(&main_work_q, work);

	return ret;
}

static bool work_flush_locked(struct bt_work *work, struct bt_work_flusher *flusher)
{
	bool need_flush = (flags_get(&work->flags) & (BT_WORK_QUEUED | BT_WORK_RUNNING)) != 0U;

	if (need_flush) {
		struct bt_work_q *queue = work->queue;

		__ASSERT_NO_MSG(queue != NULL);

		queue_flusher_locked(queue, work, flusher);
		notify_queue_locked(queue);
	}

	return need_flush;
}

bool bt_work_flush(struct bt_work *work, struct bt_work_sync *sync)
{
	__ASSERT_NO_MSG(work != NULL);
	__ASSERT_NO_MSG(!flag_test(&work->flags, BT_WORK_DELAYABLE_BIT));
	__ASSERT_NO_MSG(sync != NULL);

	struct bt_work_flusher *flusher = &sync->flusher;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	bool need_flush = work_flush_locked(work, flusher);

	os_mutex_unlock(&lock);

	/* If necessary wait until the flusher item completes */
	if (need_flush) {
		os_sem_take(&flusher->sem, OS_TIMEOUT_FOREVER);
	}

	return need_flush;
}

static int cancel_async_locked(struct bt_work *work)
{
	/* If we haven't already started canceling, do it now. */
	if (!flag_test(&work->flags, BT_WORK_CANCELING_BIT)) {
		/* Remove it from the queue, if it's queued. */
		queue_remove_locked(work->queue, work);
	}

	/* If it's still busy after it's been dequeued, then flag it
	 * as canceling.
	 */
	int ret = work_busy_get_locked(work);

	if (ret != 0) {
		flag_set(&work->flags, BT_WORK_CANCELING_BIT);
		ret = work_busy_get_locked(work);
	}

	return ret;
}

static bool cancel_sync_locked(struct bt_work *work, struct bt_work_canceller *canceller)
{
	bool ret = flag_test(&work->flags, BT_WORK_CANCELING_BIT);

	if (ret) {
		init_work_cancel(canceller, work);
	}

	return ret;
}

int bt_work_cancel(struct bt_work *work)
{
	__ASSERT_NO_MSG(work != NULL);
	__ASSERT_NO_MSG(!flag_test(&work->flags, BT_WORK_DELAYABLE_BIT));

	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	int ret = cancel_async_locked(work);
	os_mutex_unlock(&lock);

	return ret;
}

bool bt_work_cancel_sync(struct bt_work *work, struct bt_work_sync *sync)
{
	__ASSERT_NO_MSG(work != NULL);
	__ASSERT_NO_MSG(sync != NULL);
	__ASSERT_NO_MSG(!flag_test(&work->flags, BT_WORK_DELAYABLE_BIT));

	struct bt_work_canceller *canceller = &sync->canceller;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	bool pending = (work_busy_get_locked(work) != 0U);
	bool need_wait = false;

	if (pending) {
		(void)cancel_async_locked(work);
		need_wait = cancel_sync_locked(work, canceller);
	}

	os_mutex_unlock(&lock);

	if (need_wait) {
		os_sem_take(&canceller->sem, OS_TIMEOUT_FOREVER);
	}

	return pending;
}

/* Loop executed by a work queue thread.
 *
 * @param workq_ptr pointer to the work queue structure
 */
static void work_queue_main(void *workq_ptr)
{
	struct bt_work_q *queue = (struct bt_work_q *)workq_ptr;

	queue->thread_id = os_thread_self();

	while (true) {
		bt_snode_t *node;
		struct bt_work *work = NULL;
		bt_work_handler_t handler = NULL;
		os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
		bool yield;

		/* Check for and prepare any new work. */
		node = bt_slist_get(&queue->pending);
		if (node != NULL) {
			/* Mark that there's some work active that's
			 * not on the pending list.
			 */
			flag_set(&queue->flags, BT_WORK_QUEUE_BUSY_BIT);
			work = CONTAINER_OF(node, struct bt_work, node);
			flag_set(&work->flags, BT_WORK_RUNNING_BIT);
			flag_clear(&work->flags, BT_WORK_QUEUED_BIT);

			/* Static code analysis tool can raise a false-positive violation
			 * in the line below that 'work' is checked for null after being
			 * dereferenced.
			 *
			 * The work is figured out by CONTAINER_OF, as a container
			 * of type struct bt_work that contains the node.
			 * The only way for it to be NULL is if node would be a member
			 * of struct bt_work object that has been placed at address NULL,
			 * which should never happen, even line 'if (work != NULL)'
			 * ensures that.
			 * This means that if node is not NULL, then work will not be NULL.
			 */
			handler = work->handler;
		} else if (flag_test_and_clear(&queue->flags, BT_WORK_QUEUE_DRAIN_BIT)) {
			/* Not busy and draining: move threads waiting for
			 * drain to ready state.  The held spinlock inhibits
			 * immediate reschedule; released threads get their
			 * chance when this invokes z_sched_wait() below.
			 *
			 * We don't touch BT_WORK_QUEUE_PLUGGABLE, so getting
			 * here doesn't mean that the queue will allow new
			 * submissions.
			 */
			os_sem_give(&queue->drainq);
		} else if (flag_test(&queue->flags, BT_WORK_QUEUE_STOP_BIT)) {
			/* User has requested that the queue stop. Clear the status flags and exit.
			 */
			flags_set(&queue->flags, 0);
			os_mutex_unlock(&lock);
			return;
		} else {
			/* No work is available and no queue state requires
			 * special handling.
			 */
			;
		}

		if (work == NULL) {
			/* Nothing's had a chance to add work since we took
			 * the lock, and we didn't find work nor got asked to
			 * stop.  Just go to sleep: when something happens the
			 * work thread will be woken and we can check again.
			 */

			os_mutex_unlock(&lock);
			os_sem_take(&queue->notifyq, OS_TIMEOUT_FOREVER);
			continue;
		}

		os_mutex_unlock(&lock);

		__ASSERT_NO_MSG(handler != NULL);
		handler(work);

		/* Mark the work item as no longer running and deal
		 * with any cancellation and flushing issued while it
		 * was running.  Clear the BUSY flag and optionally
		 * yield to prevent starving other threads.
		 */
		os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

		flag_clear(&work->flags, BT_WORK_RUNNING_BIT);
		if (flag_test(&work->flags, BT_WORK_FLUSHING_BIT)) {
			finalize_flush_locked(work);
		} else if (flag_test(&work->flags, BT_WORK_CANCELING_BIT)) {
			finalize_cancel_locked(work);
		}

		flag_clear(&queue->flags, BT_WORK_QUEUE_BUSY_BIT);
		yield = !flag_test(&queue->flags, BT_WORK_QUEUE_NO_YIELD_BIT);
		os_mutex_unlock(&lock);

		/* Optionally yield to prevent the work queue from
		 * starving other threads.
		 */
		if (yield) {
			(void)os_thread_yield();
		}
	}
}

void bt_work_queue_init(struct bt_work_q *queue)
{
	__ASSERT_NO_MSG(queue != NULL);

	*queue = (struct bt_work_q){
		.flags = 0,
	};
}

void bt_work_queue_run(struct bt_work_q *queue, const struct bt_work_queue_config *cfg)
{
	__ASSERT_NO_MSG(!flag_test(&queue->flags, BT_WORK_QUEUE_STARTED_BIT));

	uint32_t flags = BT_WORK_QUEUE_STARTED;

	if ((cfg != NULL) && cfg->no_yield) {
		flags |= BT_WORK_QUEUE_NO_YIELD;
	}

	if ((cfg != NULL) && (cfg->name != NULL)) {
		os_thread_name_set(&queue->thread, cfg->name);
	}

	bt_slist_init(&queue->pending);
	os_sem_init(&queue->notifyq, 0, 1);
	os_sem_init(&queue->drainq, 0, 1);
	queue->thread_id = os_thread_self();
	flags_set(&queue->flags, flags);
	work_queue_main(queue);
}

void bt_work_queue_start(struct bt_work_q *queue, size_t stack_size, int prio,
			  const struct bt_work_queue_config *cfg)
{
	__ASSERT_NO_MSG(queue);
	__ASSERT_NO_MSG(!flag_test(&queue->flags, BT_WORK_QUEUE_STARTED_BIT));

	uint32_t flags = BT_WORK_QUEUE_STARTED;

	bt_slist_init(&queue->pending);
	os_sem_init(&queue->notifyq, 0, 1);
	os_sem_init(&queue->drainq, 0, 1);

	if ((cfg != NULL) && cfg->no_yield) {
		flags |= BT_WORK_QUEUE_NO_YIELD;
	}

	/* It hasn't actually been started yet, but all the state is in place
	 * so we can submit things and once the thread gets control it's ready
	 * to roll.
	 */
	flags_set(&queue->flags, flags);

	(void)os_thread_create(&queue->thread, work_queue_main, queue, cfg ? cfg->name : NULL,
			       prio, stack_size);

	if ((cfg != NULL) && (cfg->name != NULL)) {
		os_thread_name_set(&queue->thread, cfg->name);
	}

	os_thread_start(&queue->thread);
}

int bt_work_queue_drain(struct bt_work_q *queue, bool plug)
{
	__ASSERT_NO_MSG(queue);

	int ret = 0;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	if (((flags_get(&queue->flags) & (BT_WORK_QUEUE_BUSY | BT_WORK_QUEUE_DRAIN)) != 0U) ||
	    plug || !bt_slist_is_empty(&queue->pending)) {
		flag_set(&queue->flags, BT_WORK_QUEUE_DRAIN_BIT);
		if (plug) {
			flag_set(&queue->flags, BT_WORK_QUEUE_PLUGGED_BIT);
		}

		notify_queue_locked(queue);
		os_mutex_unlock(&lock);
		os_sem_take(&queue->drainq, OS_TIMEOUT_FOREVER);
	} else {
		os_mutex_unlock(&lock);
	}

	return ret;
}

int bt_work_queue_unplug(struct bt_work_q *queue)
{
	__ASSERT_NO_MSG(queue);

	int ret = -EALREADY;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	if (flag_test_and_clear(&queue->flags, BT_WORK_QUEUE_PLUGGED_BIT)) {
		ret = 0;
	}

	os_mutex_unlock(&lock);

	return ret;
}

int bt_work_queue_stop(struct bt_work_q *queue, os_timeout_t timeout)
{
	__ASSERT_NO_MSG(queue);

	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	if (!flag_test(&queue->flags, BT_WORK_QUEUE_STARTED_BIT)) {
		os_mutex_unlock(&lock);
		return -EALREADY;
	}

	if (!flag_test(&queue->flags, BT_WORK_QUEUE_PLUGGED_BIT)) {
		os_mutex_unlock(&lock);
		return -EBUSY;
	}

	flag_set(&queue->flags, BT_WORK_QUEUE_STOP_BIT);
	notify_queue_locked(queue);
	os_mutex_unlock(&lock);
	if (os_thread_join(&queue->thread, timeout)) {
		os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
		flag_clear(&queue->flags, BT_WORK_QUEUE_STOP_BIT);
		os_mutex_unlock(&lock);
		return -ETIMEDOUT;
	}

	return 0;
}

static void work_timeout(os_timer_t *timer, void *arg)
{
	struct bt_work_delayable *dw = CONTAINER_OF(timer, struct bt_work_delayable, timeout);
	struct bt_work *wp = &dw->work;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	struct bt_work_q *queue = NULL;

	/* If the work is still marked delayed (should be) then clear that
	 * state and submit it to the queue.  If successful the queue will be
	 * notified of new work at the next reschedule point.
	 *
	 * If not successful there is no notification that the work has been
	 * abandoned.  Sorry.
	 */
	if (flag_test_and_clear(&wp->flags, BT_WORK_DELAYED_BIT)) {
		queue = dw->queue;
		(void)submit_to_queue_locked(wp, &queue);
	}

	os_mutex_unlock(&lock);
}

void bt_work_init_delayable(struct bt_work_delayable *dwork, bt_work_handler_t handler)
{
	__ASSERT_NO_MSG(dwork != NULL);
	__ASSERT_NO_MSG(handler != NULL);

	*dwork = (struct bt_work_delayable){
		.work =
			{
				.handler = handler,
				.flags = BT_WORK_DELAYABLE,
			},
	};
	os_timer_create(&dwork->timeout, work_timeout, NULL);
	// TODO :delete timer when dwork is destroyed
}

static inline int work_delayable_busy_get_locked(const struct bt_work_delayable *dwork)
{
	return flags_get(&dwork->work.flags) & BT_WORK_MASK;
}

int bt_work_delayable_busy_get(const struct bt_work_delayable *dwork)
{
	__ASSERT_NO_MSG(dwork != NULL);

	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	int ret = work_delayable_busy_get_locked(dwork);

	os_mutex_unlock(&lock);
	return ret;
}

static int schedule_for_queue_locked(struct bt_work_q **queuep, struct bt_work_delayable *dwork,
				     os_timeout_t delay)
{
	int ret = 1;
	struct bt_work *work = &dwork->work;

	if (TIMEOUT_EQ(delay, OS_TIMEOUT_NO_WAIT)) {
		return submit_to_queue_locked(work, queuep);
	}

	flag_set(&work->flags, BT_WORK_DELAYED_BIT);
	dwork->queue = *queuep;

	/* Add timeout */
	os_timer_start(&dwork->timeout, delay);

	return ret;
}

static inline bool unschedule_locked(struct bt_work_delayable *dwork)
{
	bool ret = false;
	struct bt_work *work = &dwork->work;

	/* If scheduled, try to cancel.  If it fails, that means the
	 * callback has been dequeued and will inevitably run (or has
	 * already run), so treat that as "undelayed" and return
	 * false.
	 */
	if (flag_test_and_clear(&work->flags, BT_WORK_DELAYED_BIT)) {
		ret = os_timer_stop(&dwork->timeout) == 0;
	}

	return ret;
}

static int cancel_delayable_async_locked(struct bt_work_delayable *dwork)
{
	(void)unschedule_locked(dwork);

	return cancel_async_locked(&dwork->work);
}

int bt_work_schedule_for_queue(struct bt_work_q *queue, struct bt_work_delayable *dwork,
			       os_timeout_t delay)
{
	__ASSERT_NO_MSG(queue != NULL);
	__ASSERT_NO_MSG(dwork != NULL);

	struct bt_work *work = &dwork->work;
	int ret = 0;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	/* Schedule the work item if it's idle or running. */
	if ((work_busy_get_locked(work) & ~BT_WORK_RUNNING) == 0U) {
		ret = schedule_for_queue_locked(&queue, dwork, delay);
	}

	os_mutex_unlock(&lock);

	return ret;
}

int bt_work_schedule(struct bt_work_delayable *dwork, os_timeout_t delay)
{
	int ret = bt_work_schedule_for_queue(&main_work_q, dwork, delay);

	return ret;
}

int bt_work_reschedule_for_queue(struct bt_work_q *queue, struct bt_work_delayable *dwork,
				 os_timeout_t delay)
{
	__ASSERT_NO_MSG(queue != NULL);
	__ASSERT_NO_MSG(dwork != NULL);

	int ret;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	/* Remove any active scheduling. */
	(void)unschedule_locked(dwork);

	/* Schedule the work item with the new parameters. */
	ret = schedule_for_queue_locked(&queue, dwork, delay);

	os_mutex_unlock(&lock);

	return ret;
}

int bt_work_reschedule(struct bt_work_delayable *dwork, os_timeout_t delay)
{

	int ret = bt_work_reschedule_for_queue(&main_work_q, dwork, delay);

	return ret;
}

int bt_work_cancel_delayable(struct bt_work_delayable *dwork)
{
	__ASSERT_NO_MSG(dwork != NULL);

	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	int ret = cancel_delayable_async_locked(dwork);

	os_mutex_unlock(&lock);

	return ret;
}

bool bt_work_cancel_delayable_sync(struct bt_work_delayable *dwork, struct bt_work_sync *sync)
{
	__ASSERT_NO_MSG(dwork != NULL);
	__ASSERT_NO_MSG(sync != NULL);

	struct bt_work_canceller *canceller = &sync->canceller;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	bool pending = (work_delayable_busy_get_locked(dwork) != 0U);
	bool need_wait = false;

	if (pending) {
		(void)cancel_delayable_async_locked(dwork);
		need_wait = cancel_sync_locked(&dwork->work, canceller);
	}

	os_mutex_unlock(&lock);

	if (need_wait) {
		os_sem_take(&canceller->sem, OS_TIMEOUT_FOREVER);
	}

	return pending;
}

bool bt_work_flush_delayable(struct bt_work_delayable *dwork, struct bt_work_sync *sync)
{
	__ASSERT_NO_MSG(dwork != NULL);
	__ASSERT_NO_MSG(sync != NULL);

	struct bt_work *work = &dwork->work;
	struct bt_work_flusher *flusher = &sync->flusher;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	/* If it's idle release the lock and return immediately. */
	if (work_busy_get_locked(work) == 0U) {
		os_mutex_unlock(&lock);

		return false;
	}

	/* If unscheduling did something then submit it.  Ignore a
	 * failed submission (e.g. when cancelling).
	 */
	if (unschedule_locked(dwork)) {
		struct bt_work_q *queue = dwork->queue;

		(void)submit_to_queue_locked(work, &queue);
	}

	/* Wait for it to finish */
	bool need_flush = work_flush_locked(work, flusher);

	os_mutex_unlock(&lock);

	/* If necessary wait until the flusher item completes */
	if (need_flush) {
		os_sem_take(&flusher->sem, OS_TIMEOUT_FOREVER);
	}

	return need_flush;
}

struct bt_work_q *bt_work_main_work_queue(void)
{
	return &main_work_q;
}

static int main_work_init(void)
{
	static const struct bt_work_queue_config cfg = {
		.name = "main_work",
		.no_yield = IS_ENABLED(CONFIG_SYSTEM_WORKQUEUE_NO_YIELD),
	};

	bt_work_queue_start(&main_work_q, 2048, OS_PRIORITY(0), &cfg);

	return 0;
}

int bt_work_main_work_init(void)
{
	return main_work_init();
}

STACK_INIT(main_work_init, STACK_BASE_INIT, 1);
