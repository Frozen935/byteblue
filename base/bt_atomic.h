#ifndef __BASE_ATOMIC_H__
#define __BASE_ATOMIC_H__

typedef unsigned long bt_atomic_t;
typedef unsigned long bt_atomic_val_t;

/* Atomic pointer placeholder */
#ifndef bt_atomic_ptr_t
typedef void *bt_atomic_ptr_t;
#endif
#ifndef bt_atomic_ptr_val_t
typedef bt_atomic_ptr_t bt_atomic_ptr_val_t;
#endif

#define ATOMIC_BITS            (sizeof(bt_atomic_val_t) * BITS_PER_BYTE)
#define ATOMIC_MASK(bit)       BIT((unsigned long)(bit) & (ATOMIC_BITS - 1U))
#define ATOMIC_ELEM(addr, bit) ((addr) + ((bit) / ATOMIC_BITS))

#define ATOMIC_BITMAP_SIZE(num_bits) (ROUND_UP(num_bits, ATOMIC_BITS) / ATOMIC_BITS)

#define ATOMIC_DEFINE(name, num_bits) bt_atomic_t name[ATOMIC_BITMAP_SIZE(num_bits)]

/* Compare-and-swap */
bool bt_atomic_cas(bt_atomic_t *target, bt_atomic_val_t expected, bt_atomic_val_t desired);

/* Prototypes implemented in os_atomic.c */
bt_atomic_val_t bt_atomic_get(const bt_atomic_t *target);
bt_atomic_val_t bt_atomic_set(bt_atomic_t *target, bt_atomic_val_t value);
bt_atomic_val_t bt_atomic_inc(bt_atomic_t *target);
bt_atomic_val_t bt_atomic_dec(bt_atomic_t *target);
void bt_atomic_add(bt_atomic_t *target, bt_atomic_val_t value);
void bt_atomic_sub(bt_atomic_t *target, bt_atomic_val_t value);

static inline void bt_atomic_clear(bt_atomic_t *target)
{
	bt_atomic_set(target, (bt_atomic_val_t)0);
}

/* Atomic bit helpers */
bool bt_atomic_test_bit(const bt_atomic_t *target, int bit);
void bt_atomic_set_bit(bt_atomic_t *target, int bit);
void bt_atomic_clear_bit(bt_atomic_t *target, int bit);
void bt_atomic_set_bit_to(bt_atomic_t *target, int bit, bool val);
bool bt_atomic_test_and_set_bit(bt_atomic_t *target, int bit);
bool bt_atomic_test_and_clear_bit(bt_atomic_t *target, int bit);

/* atomic pointer helpers (compile-only prototypes) */
void *bt_atomic_ptr_get(bt_atomic_ptr_t *ptr);
void *bt_atomic_ptr_set(bt_atomic_ptr_t *ptr, void *val);
void *bt_atomic_ptr_clear(bt_atomic_ptr_t *ptr);
bool bt_atomic_ptr_cas(bt_atomic_ptr_t *ptr, void *expected, void *desired);

#endif /* __BASE_ATOMIC_H__ */