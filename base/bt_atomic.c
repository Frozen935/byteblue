#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "osdep/atomic/atomic.h"

bt_atomic_val_t bt_atomic_get(const bt_atomic_t *target)
{
	if (!target) {
		return 0;
	}
	return (bt_atomic_val_t)os_atomic_word_load((const unsigned long *)target,
						    OS_ATOMIC_SEQ_CST);
}

bt_atomic_val_t bt_atomic_set(bt_atomic_t *target, bt_atomic_val_t value)
{
	if (!target) {
		return 0;
	}
	unsigned long old = os_atomic_word_exchange((unsigned long *)target, (unsigned long)value,
						    OS_ATOMIC_SEQ_CST);
	return (bt_atomic_val_t)old;
}

bt_atomic_val_t bt_atomic_inc(bt_atomic_t *target)
{
	if (!target) {
		return 0;
	}
	unsigned long old =
		os_atomic_word_fetch_add((unsigned long *)target, 1UL, OS_ATOMIC_SEQ_CST);
	return (bt_atomic_val_t)old;
}

bt_atomic_val_t bt_atomic_dec(bt_atomic_t *target)
{
	if (!target) {
		return 0;
	}
	unsigned long old =
		os_atomic_word_fetch_sub((unsigned long *)target, 1UL, OS_ATOMIC_SEQ_CST);
	return (bt_atomic_val_t)old;
}

bool bt_atomic_cas(bt_atomic_t *target, bt_atomic_val_t expected, bt_atomic_val_t desired)
{
	if (!target) {
		return false;
	}
	return os_atomic_word_compare_exchange((unsigned long *)target, (unsigned long)expected,
					       (unsigned long)desired, OS_ATOMIC_SEQ_CST);
}

void bt_atomic_add(bt_atomic_t *target, bt_atomic_val_t value)
{
	if (!target) {
		return;
	}
	(void)os_atomic_word_fetch_add((unsigned long *)target, (unsigned long)value,
				       OS_ATOMIC_SEQ_CST);
}

void bt_atomic_sub(bt_atomic_t *target, bt_atomic_val_t value)
{
	if (!target) {
		return;
	}
	(void)os_atomic_word_fetch_sub((unsigned long *)target, (unsigned long)value,
				       OS_ATOMIC_SEQ_CST);
}

static inline size_t _atomic_word_index(int bit)
{
	return ((size_t)bit) / ATOMIC_BITS;
}

static inline bt_atomic_t _atomic_bit_mask(int bit)
{
	return (bt_atomic_t)(1UL << (bit % ATOMIC_BITS));
}

bool bt_atomic_test_bit(const bt_atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return false;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long word =
		os_atomic_word_load((const unsigned long *)&target[idx], OS_ATOMIC_SEQ_CST);
	return (word & (unsigned long)_atomic_bit_mask(bit)) != 0;
}

void bt_atomic_set_bit(bt_atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return;
	}
	size_t idx = _atomic_word_index(bit);
	(void)os_atomic_word_fetch_or((unsigned long *)&target[idx],
				      (unsigned long)_atomic_bit_mask(bit), OS_ATOMIC_SEQ_CST);
}

void bt_atomic_set_bit_to(bt_atomic_t *target, int bit, bool val)
{
	if (val) {
		bt_atomic_set_bit(target, bit);
	} else {
		bt_atomic_clear_bit(target, bit);
	}
}

void bt_atomic_clear_bit(bt_atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long mask = (unsigned long)_atomic_bit_mask(bit);
	(void)os_atomic_word_fetch_and((unsigned long *)&target[idx], ~mask, OS_ATOMIC_SEQ_CST);
}

bool bt_atomic_test_and_set_bit(bt_atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return false;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long mask = (unsigned long)_atomic_bit_mask(bit);
	unsigned long old =
		os_atomic_word_fetch_or((unsigned long *)&target[idx], mask, OS_ATOMIC_SEQ_CST);
	return (old & mask) != 0;
}

bool bt_atomic_test_and_clear_bit(bt_atomic_t *target, int bit)
{
	if (!target || bit < 0) {
		return false;
	}
	size_t idx = _atomic_word_index(bit);
	unsigned long mask = (unsigned long)_atomic_bit_mask(bit);
	unsigned long old =
		os_atomic_word_fetch_and((unsigned long *)&target[idx], ~mask, OS_ATOMIC_SEQ_CST);
	return (old & mask) != 0;
}

/* Atomic pointer helpers mapped to os/atomic implementation */
void *bt_atomic_ptr_get(bt_atomic_ptr_t *ptr)
{
	return os_atomic_ptr_get((void *volatile *)ptr);
}

void *bt_atomic_ptr_set(bt_atomic_ptr_t *ptr, void *val)
{
	return os_atomic_ptr_set((void *volatile *)ptr, val);
}

void *bt_atomic_ptr_clear(bt_atomic_ptr_t *ptr)
{
	return os_atomic_ptr_clear((void *volatile *)ptr);
}

bool bt_atomic_ptr_cas(bt_atomic_ptr_t *ptr, void *expected, void *desired)
{
	return os_atomic_ptr_cas((void *volatile *)ptr, expected, desired);
}
