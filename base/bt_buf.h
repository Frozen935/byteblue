#ifndef __BASE_BUF_H__
#define __BASE_BUF_H__

#include <stddef.h>
#include <string.h>

#include <base/byteorder.h>
#include <utils/bt_slist.h>
#include <osdep/os.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Alignment needed for various parts of the buffer definition */
#define __bt_buf_align __aligned(sizeof(void *))

/**
 *  @brief Define a bt_buf_simple stack variable.
 *
 *  This is a helper macro which is used to define a bt_buf_simple object
 *  on the stack.
 *
 *  @param _name Name of the bt_buf_simple object.
 *  @param _size Maximum data storage for the buffer.
 */
#define BT_BUF_SIMPLE_DEFINE(_name, _size)                                                         \
	uint8_t bt_buf_data_##_name[_size];                                                        \
	struct bt_buf_simple _name = {                                                             \
		.data = bt_buf_data_##_name,                                                       \
		.len = 0,                                                                          \
		.size = _size,                                                                     \
		.__buf = bt_buf_data_##_name,                                                      \
	}

/**
 *
 * @brief Define a static bt_buf_simple variable.
 *
 * This is a helper macro which is used to define a static bt_buf_simple
 * object.
 *
 * @param _name Name of the bt_buf_simple object.
 * @param _size Maximum data storage for the buffer.
 */
#define BT_BUF_SIMPLE_DEFINE_STATIC(_name, _size)                                                  \
	static __noinit uint8_t bt_buf_data_##_name[_size];                                        \
	static struct bt_buf_simple _name = {                                                      \
		.data = bt_buf_data_##_name,                                                       \
		.len = 0,                                                                          \
		.size = _size,                                                                     \
		.__buf = bt_buf_data_##_name,                                                      \
	}

/**
 * @brief Simple network buffer representation.
 *
 * This is a simpler variant of the bt_buf object (in fact bt_buf uses
 * bt_buf_simple internally). It doesn't provide any kind of reference
 * counting, user data, dynamic allocation, or in general the ability to
 * pass through kernel objects such as FIFOs.
 *
 * The main use of this is for scenarios where the meta-data of the normal
 * bt_buf isn't needed and causes too much overhead. This could be e.g.
 * when the buffer only needs to be allocated on the stack or when the
 * access to and lifetime of the buffer is well controlled and constrained.
 */
struct bt_buf_simple {
	/** Pointer to the start of data in the buffer. */
	uint8_t *data;

	/**
	 * Length of the data behind the data pointer.
	 *
	 * To determine the max length, use bt_buf_simple_max_len(), not #size!
	 */
	uint16_t len;

	/** Amount of data that bt_buf_simple#__buf can store. */
	uint16_t size;

	/** Start of the data storage. Not to be accessed directly
	 *  (the data pointer should be used instead).
	 */
	uint8_t *__buf;
};

/**
 *
 * @brief Define a bt_buf_simple stack variable and get a pointer to it.
 *
 * This is a helper macro which is used to define a bt_buf_simple object on
 * the stack and the get a pointer to it as follows:
 *
 * struct bt_buf_simple *my_buf = BT_BUF_SIMPLE(10);
 *
 * After creating the object it needs to be initialized by calling
 * bt_buf_simple_init().
 *
 * @param _size Maximum data storage for the buffer.
 *
 * @return Pointer to stack-allocated bt_buf_simple object.
 */
#define BT_BUF_SIMPLE(_size)                                                                       \
	((struct bt_buf_simple *)(&(struct {                                                       \
		struct bt_buf_simple buf;                                                          \
		uint8_t data[_size];                                                               \
	}){                                                                                        \
		.buf.size = _size,                                                                 \
	}))

/**
 * @brief Initialize a bt_buf_simple object.
 *
 * This needs to be called after creating a bt_buf_simple object using
 * the BT_BUF_SIMPLE macro.
 *
 * @param buf Buffer to initialize.
 * @param reserve_head Headroom to reserve.
 */
static inline void bt_buf_simple_init(struct bt_buf_simple *buf, size_t reserve_head)
{
	if (!buf->__buf) {
		buf->__buf = (uint8_t *)buf + sizeof(*buf);
	}

	buf->data = buf->__buf + reserve_head;
	buf->len = 0U;
}

/**
 * @brief Initialize a bt_buf_simple object with data.
 *
 * Initialized buffer object with external data.
 *
 * @param buf Buffer to initialize.
 * @param data External data pointer
 * @param size Amount of data the pointed data buffer if able to fit.
 */
void bt_buf_simple_init_with_data(struct bt_buf_simple *buf, void *data, size_t size);

/**
 * @brief Reset buffer
 *
 * Reset buffer data so it can be reused for other purposes.
 *
 * @param buf Buffer to reset.
 */
static inline void bt_buf_simple_reset(struct bt_buf_simple *buf)
{
	buf->len = 0U;
	buf->data = buf->__buf;
}

/**
 * Clone buffer state, using the same data buffer.
 *
 * Initializes a buffer to point to the same data as an existing buffer.
 * Allows operations on the same data without altering the length and
 * offset of the original.
 *
 * @param original Buffer to clone.
 * @param clone The new clone.
 */
void bt_buf_simple_clone(const struct bt_buf_simple *original, struct bt_buf_simple *clone);

/**
 * @brief Prepare data to be added at the end of the buffer
 *
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to increment the length with.
 *
 * @return The original tail of the buffer.
 */
void *bt_buf_simple_add(struct bt_buf_simple *buf, size_t len);

/**
 * @brief Copy given number of bytes from memory to the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at the
 * end.
 *
 * @param buf Buffer to update.
 * @param mem Location of data to be added.
 * @param len Length of data to be added
 *
 * @return The original tail of the buffer.
 */
void *bt_buf_simple_add_mem(struct bt_buf_simple *buf, const void *mem, size_t len);

/**
 * @brief Add (8-bit) byte at the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at the
 * end.
 *
 * @param buf Buffer to update.
 * @param val byte value to be added.
 *
 * @return Pointer to the value added
 */
uint8_t *bt_buf_simple_add_u8(struct bt_buf_simple *buf, uint8_t val);

/**
 * @brief Add 16-bit value at the end of the buffer
 *
 * Adds 16-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be added.
 */
void bt_buf_simple_add_le16(struct bt_buf_simple *buf, uint16_t val);

/**
 * @brief Add 16-bit value at the end of the buffer
 *
 * Adds 16-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be added.
 */
void bt_buf_simple_add_be16(struct bt_buf_simple *buf, uint16_t val);

/**
 * @brief Add 24-bit value at the end of the buffer
 *
 * Adds 24-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be added.
 */
void bt_buf_simple_add_le24(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Add 24-bit value at the end of the buffer
 *
 * Adds 24-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be added.
 */
void bt_buf_simple_add_be24(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Add 32-bit value at the end of the buffer
 *
 * Adds 32-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be added.
 */
void bt_buf_simple_add_le32(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Add 32-bit value at the end of the buffer
 *
 * Adds 32-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be added.
 */
void bt_buf_simple_add_be32(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Add 40-bit value at the end of the buffer
 *
 * Adds 40-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be added.
 */
void bt_buf_simple_add_le40(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Add 40-bit value at the end of the buffer
 *
 * Adds 40-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be added.
 */
void bt_buf_simple_add_be40(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Add 48-bit value at the end of the buffer
 *
 * Adds 48-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be added.
 */
void bt_buf_simple_add_le48(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Add 48-bit value at the end of the buffer
 *
 * Adds 48-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be added.
 */
void bt_buf_simple_add_be48(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Add 64-bit value at the end of the buffer
 *
 * Adds 64-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be added.
 */
void bt_buf_simple_add_le64(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Add 64-bit value at the end of the buffer
 *
 * Adds 64-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be added.
 */
void bt_buf_simple_add_be64(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Remove data from the end of the buffer.
 *
 * Removes data from the end of the buffer by modifying the buffer length.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to remove.
 *
 * @return New end of the buffer data.
 */
void *bt_buf_simple_remove_mem(struct bt_buf_simple *buf, size_t len);

/**
 * @brief Remove a 8-bit value from the end of the buffer
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 8-bit values.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return The 8-bit removed value
 */
uint8_t bt_buf_simple_remove_u8(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 16 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 16-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from little endian to host endian.
 */
uint16_t bt_buf_simple_remove_le16(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 16 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 16-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from big endian to host endian.
 */
uint16_t bt_buf_simple_remove_be16(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 24 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 24-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from little endian to host endian.
 */
uint32_t bt_buf_simple_remove_le24(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 24 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 24-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from big endian to host endian.
 */
uint32_t bt_buf_simple_remove_be24(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 32 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 32-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 32-bit value converted from little endian to host endian.
 */
uint32_t bt_buf_simple_remove_le32(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 32 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 32-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 32-bit value converted from big endian to host endian.
 */
uint32_t bt_buf_simple_remove_be32(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 40 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 40-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 40-bit value converted from little endian to host endian.
 */
uint64_t bt_buf_simple_remove_le40(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 40 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 40-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 40-bit value converted from big endian to host endian.
 */
uint64_t bt_buf_simple_remove_be40(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 48 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 48-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 48-bit value converted from little endian to host endian.
 */
uint64_t bt_buf_simple_remove_le48(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 48 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 48-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 48-bit value converted from big endian to host endian.
 */
uint64_t bt_buf_simple_remove_be48(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 64 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 64-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 64-bit value converted from little endian to host endian.
 */
uint64_t bt_buf_simple_remove_le64(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 64 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_simple_remove_mem(), but a helper for operating
 * on 64-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 64-bit value converted from big endian to host endian.
 */
uint64_t bt_buf_simple_remove_be64(struct bt_buf_simple *buf);

/**
 * @brief Prepare data to be added to the start of the buffer
 *
 * Modifies the data pointer and buffer length to account for more data
 * in the beginning of the buffer.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to add to the beginning.
 *
 * @return The new beginning of the buffer data.
 */
void *bt_buf_simple_push(struct bt_buf_simple *buf, size_t len);

/**
 * @brief Copy given number of bytes from memory to the start of the buffer.
 *
 * Modifies the data pointer and buffer length to account for more data
 * in the beginning of the buffer.
 *
 * @param buf Buffer to update.
 * @param mem Location of data to be added.
 * @param len Length of data to be added.
 *
 * @return The new beginning of the buffer data.
 */
void *bt_buf_simple_push_mem(struct bt_buf_simple *buf, const void *mem, size_t len);

/**
 * @brief Push 16-bit value to the beginning of the buffer
 *
 * Adds 16-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_le16(struct bt_buf_simple *buf, uint16_t val);

/**
 * @brief Push 16-bit value to the beginning of the buffer
 *
 * Adds 16-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_be16(struct bt_buf_simple *buf, uint16_t val);

/**
 * @brief Push 8-bit value to the beginning of the buffer
 *
 * Adds 8-bit value the beginning of the buffer.
 *
 * @param buf Buffer to update.
 * @param val 8-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_u8(struct bt_buf_simple *buf, uint8_t val);

/**
 * @brief Push 24-bit value to the beginning of the buffer
 *
 * Adds 24-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_le24(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Push 24-bit value to the beginning of the buffer
 *
 * Adds 24-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_be24(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Push 32-bit value to the beginning of the buffer
 *
 * Adds 32-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_le32(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Push 32-bit value to the beginning of the buffer
 *
 * Adds 32-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_be32(struct bt_buf_simple *buf, uint32_t val);

/**
 * @brief Push 40-bit value to the beginning of the buffer
 *
 * Adds 40-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_le40(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Push 40-bit value to the beginning of the buffer
 *
 * Adds 40-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_be40(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Push 48-bit value to the beginning of the buffer
 *
 * Adds 48-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_le48(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Push 48-bit value to the beginning of the buffer
 *
 * Adds 48-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_be48(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Push 64-bit value to the beginning of the buffer
 *
 * Adds 64-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_le64(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Push 64-bit value to the beginning of the buffer
 *
 * Adds 64-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be pushed to the buffer.
 */
void bt_buf_simple_push_be64(struct bt_buf_simple *buf, uint64_t val);

/**
 * @brief Remove data from the beginning of the buffer.
 *
 * Removes data from the beginning of the buffer by modifying the data
 * pointer and buffer length.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to remove.
 *
 * @return New beginning of the buffer data.
 */
void *bt_buf_simple_pull(struct bt_buf_simple *buf, size_t len);

/**
 * @brief Remove data from the beginning of the buffer.
 *
 * Removes data from the beginning of the buffer by modifying the data
 * pointer and buffer length.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to remove.
 *
 * @return Pointer to the old location of the buffer data.
 */
void *bt_buf_simple_pull_mem(struct bt_buf_simple *buf, size_t len);

/**
 * @brief Remove a 8-bit value from the beginning of the buffer
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 8-bit values.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return The 8-bit removed value
 */
uint8_t bt_buf_simple_pull_u8(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 16-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from little endian to host endian.
 */
uint16_t bt_buf_simple_pull_le16(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 16-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from big endian to host endian.
 */
uint16_t bt_buf_simple_pull_be16(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 24 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 24-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from little endian to host endian.
 */
uint32_t bt_buf_simple_pull_le24(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 24 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 24-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from big endian to host endian.
 */
uint32_t bt_buf_simple_pull_be24(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 32 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 32-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 32-bit value converted from little endian to host endian.
 */
uint32_t bt_buf_simple_pull_le32(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 32 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 32-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 32-bit value converted from big endian to host endian.
 */
uint32_t bt_buf_simple_pull_be32(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 40 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 40-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 40-bit value converted from little endian to host endian.
 */
uint64_t bt_buf_simple_pull_le40(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 40 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 40-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 40-bit value converted from big endian to host endian.
 */
uint64_t bt_buf_simple_pull_be40(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 48 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 48-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 48-bit value converted from little endian to host endian.
 */
uint64_t bt_buf_simple_pull_le48(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 48 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 48-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 48-bit value converted from big endian to host endian.
 */
uint64_t bt_buf_simple_pull_be48(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 64 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 64-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 64-bit value converted from little endian to host endian.
 */
uint64_t bt_buf_simple_pull_le64(struct bt_buf_simple *buf);

/**
 * @brief Remove and convert 64 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_simple_pull(), but a helper for operating
 * on 64-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 64-bit value converted from big endian to host endian.
 */
uint64_t bt_buf_simple_pull_be64(struct bt_buf_simple *buf);

/**
 * @brief Get the tail pointer for a buffer.
 *
 * Get a pointer to the end of the data in a buffer.
 *
 * @param buf Buffer.
 *
 * @return Tail pointer for the buffer.
 */
static inline uint8_t *bt_buf_simple_tail(const struct bt_buf_simple *buf)
{
	return buf->data + buf->len;
}

/**
 * @brief Check buffer headroom.
 *
 * Check how much free space there is in the beginning of the buffer.
 *
 * buf A valid pointer on a buffer
 *
 * @return Number of bytes available in the beginning of the buffer.
 */
size_t bt_buf_simple_headroom(const struct bt_buf_simple *buf);

/**
 * @brief Check buffer tailroom.
 *
 * Check how much free space there is at the end of the buffer.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return Number of bytes available at the end of the buffer.
 */
size_t bt_buf_simple_tailroom(const struct bt_buf_simple *buf);

/**
 * @brief Check maximum bt_buf_simple::len value.
 *
 * This value is depending on the number of bytes being reserved as headroom.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return Number of bytes usable behind the bt_buf_simple::data pointer.
 */
uint16_t bt_buf_simple_max_len(const struct bt_buf_simple *buf);

/**
 * @brief Parsing state of a buffer.
 *
 * This is used for temporarily storing the parsing state of a buffer
 * while giving control of the parsing to a routine which we don't
 * control.
 */
struct bt_buf_simple_state {
	/** Offset of the data pointer from the beginning of the storage */
	uint16_t offset;
	/** Length of data */
	uint16_t len;
};

/**
 * @brief Save the parsing state of a buffer.
 *
 * Saves the parsing state of a buffer so it can be restored later.
 *
 * @param buf Buffer from which the state should be saved.
 * @param state Storage for the state.
 */
static inline void bt_buf_simple_save(const struct bt_buf_simple *buf,
				      struct bt_buf_simple_state *state)
{
	state->offset = (uint16_t)bt_buf_simple_headroom(buf);
	state->len = buf->len;
}

/**
 * @brief Restore the parsing state of a buffer.
 *
 * Restores the parsing state of a buffer from a state previously stored
 * by bt_buf_simple_save().
 *
 * @param buf Buffer to which the state should be restored.
 * @param state Stored state.
 */
static inline void bt_buf_simple_restore(struct bt_buf_simple *buf,
					 struct bt_buf_simple_state *state)
{
	buf->data = buf->__buf + state->offset;
	buf->len = state->len;
}

/**
 * Flag indicating that the buffer's associated data pointer, points to
 * externally allocated memory. Therefore once ref goes down to zero, the
 * pointed data will not need to be deallocated. This never needs to be
 * explicitly set or unset by the bt_buf API user. Such bt_buf is
 * exclusively instantiated via bt_buf_alloc_with_data() function.
 * Reference count mechanism however will behave the same way, and ref
 * count going to 0 will free the bt_buf but no the data pointer in it.
 */
#define BT_BUF_EXTERNAL_DATA BIT(0)

/**
 * @brief Network buffer representation.
 *
 * This struct is used to represent network buffers. Such buffers are
 * normally defined through the BT_BUF_POOL_*_DEFINE() APIs and allocated
 * using the bt_buf_alloc() API.
 */
struct bt_buf {
	/** Allow placing the buffer into bt_slist_t */
	bt_snode_t node;

	/** Fragments associated with this buffer. */
	struct bt_buf *frags;

	/** Reference count. */
	uint8_t ref;

	/** Bit-field of buffer flags. */
	uint8_t flags;

	/** Where the buffer should go when freed up. */
	uint8_t pool_id;
	struct bt_buf_pool *pool;

	/** Size of user data on this buffer */
	uint8_t user_data_size;

	/** Union for convenience access to the bt_buf_simple members, also
	 * preserving the old API.
	 */
	union {
		/* The ABI of this struct must match bt_buf_simple */
		struct {
			/** Pointer to the start of data in the buffer. */
			uint8_t *data;

			/** Length of the data behind the data pointer. */
			uint16_t len;

			/** Amount of data that this buffer can store. */
			uint16_t size;

			/** Start of the data storage. Not to be accessed
			 *  directly (the data pointer should be used
			 *  instead).
			 */
			uint8_t *__buf;
		};

		/** @cond INTERNAL_HIDDEN */
		struct bt_buf_simple b;
		/** @endcond */
	};

	/** System metadata for this buffer. Cleared on allocation. */
	uint8_t user_data[] __bt_buf_align;
};

/** @cond INTERNAL_HIDDEN */

struct bt_buf_data_cb {
	uint8_t *__must_check (*alloc)(struct bt_buf *buf, size_t *size, int32_t timeout);
	uint8_t *__must_check (*ref)(struct bt_buf *buf, uint8_t *data);
	void (*unref)(struct bt_buf *buf, uint8_t *data);
};

struct bt_buf_data_alloc {
	const struct bt_buf_data_cb *cb;
	void *alloc_data;
	size_t max_alloc_size;
	size_t alignment;
};

/** @endcond */

/**
 * @brief Network buffer pool representation.
 *
 * This struct is used to represent a pool of network buffers.
 */
struct bt_buf_pool {
	/** LIFO to place the buffer into when free */
	struct bt_lifo free;

	/** To prevent concurrent access/modifications */
	os_mutex_t lock;

	/** Number of buffers in pool */
	const uint16_t buf_count;

	/** Number of uninitialized buffers */
	uint16_t uninit_count;

	/** Size of user data allocated to this pool */
	uint8_t user_data_size;

	/** Optional destroy callback when buffer is freed. */
	void (*const destroy)(struct bt_buf *buf);

	/** Data allocation handlers. */
	const struct bt_buf_data_alloc *alloc;

	/** Start of buffer storage array */
	struct bt_buf *const __bufs;
};

#define BT_BUF_POOL_INITIALIZER(_pool, _alloc, _bufs, _count, _ud_size, _destroy)                  \
	{                                                                                          \
		.free = BT_LIFO_INITIALIZER(_pool.free), .lock = {}, .buf_count = _count,          \
		.uninit_count = _count, .user_data_size = _ud_size, .destroy = _destroy,           \
		.alloc = _alloc, .__bufs = (struct bt_buf *)_bufs,                                 \
	}

#define _BT_BUF_ARRAY_DEFINE(_name, _count, _ud_size)                                              \
	struct _bt_buf_##_name {                                                                   \
		uint8_t b[sizeof(struct bt_buf)];                                                  \
		uint8_t ud[_ud_size];                                                              \
	} __bt_buf_align;                                                                          \
	BUILD_ASSERT(_ud_size <= UINT8_MAX);                                                       \
	BUILD_ASSERT(offsetof(struct bt_buf, user_data) == offsetof(struct _bt_buf_##_name, ud),   \
		     "Invalid offset");                                                            \
	BUILD_ASSERT(__alignof__(struct bt_buf) == __alignof__(struct _bt_buf_##_name),            \
		     "Invalid alignment");                                                         \
	BUILD_ASSERT(sizeof(struct _bt_buf_##_name) == ROUND_UP(sizeof(struct bt_buf) + _ud_size,  \
								__alignof__(struct bt_buf)),       \
		     "Size cannot be determined");                                                 \
	static struct _bt_buf_##_name _bt_buf_##_name[_count] __noinit

#if 0
extern const struct bt_buf_data_alloc bt_buf_heap_alloc;
/** @endcond */

/**
 *
 * @brief Define a new pool for buffers using the heap for the data.
 *
 * Defines a bt_buf_pool struct and the necessary memory storage (array of
 * structs) for the needed amount of buffers. After this, the buffers can be
 * accessed from the pool through bt_buf_alloc. The pool is defined as a
 * static variable, so if it needs to be exported outside the current module
 * this needs to happen with the help of a separate pointer rather than an
 * extern declaration.
 *
 * The data payload of the buffers will be allocated from the heap using
 * k_malloc, so CONFIG_HEAP_MEM_POOL_SIZE must be set to a positive value.
 * This kind of pool does not support blocking on the data allocation, so
 * the timeout passed to bt_buf_alloc will be always treated as OS_TIMEOUT_NO_WAIT
 * when trying to allocate the data. This means that allocation failures,
 * i.e. NULL returns, must always be handled cleanly.
 *
 * If provided with a custom destroy callback, this callback is
 * responsible for eventually calling bt_buf_destroy() to complete the
 * process of returning the buffer to the pool.
 *
 * @param _name      Name of the pool variable.
 * @param _count     Number of buffers in the pool.
 * @param _ud_size   User data space to reserve per buffer.
 * @param _destroy   Optional destroy callback when buffer is freed.
 */
#define BT_BUF_POOL_HEAP_DEFINE(_name, _count, _ud_size, _destroy)                                 \
	_BT_BUF_ARRAY_DEFINE(_name, _count, _ud_size);                                             \
	struct bt_buf_pool bt_buf_pool##_name = BT_BUF_POOL_INITIALIZER(                           \
		_name, &bt_buf_heap_alloc, _bt_buf_##_name, _count, _ud_size, _destroy)
#endif
/** @cond INTERNAL_HIDDEN */

struct bt_buf_pool_fixed {
	uint8_t *data_pool;
};

extern const struct bt_buf_data_cb bt_buf_fixed_cb;

/** @endcond */

/**
 *
 * @brief Define a new pool for buffers based on fixed-size data
 *
 * Defines a bt_buf_pool struct and the necessary memory storage (array of
 * structs) for the needed amount of buffers. After this, the buffers can be
 * accessed from the pool through bt_buf_alloc. The pool is defined as a
 * static variable, so if it needs to be exported outside the current module
 * this needs to happen with the help of a separate pointer rather than an
 * extern declaration.
 *
 * The data payload of the buffers will be allocated from a byte array
 * of fixed sized chunks. This kind of pool does not support blocking on
 * the data allocation, so the timeout passed to bt_buf_alloc will be
 * always treated as OS_TIMEOUT_NO_WAIT when trying to allocate the data. This means
 * that allocation failures, i.e. NULL returns, must always be handled
 * cleanly.
 *
 * If provided with a custom destroy callback, this callback is
 * responsible for eventually calling bt_buf_destroy() to complete the
 * process of returning the buffer to the pool.
 *
 * @param _name      Name of the pool variable.
 * @param _count     Number of buffers in the pool.
 * @param _data_size Maximum data payload per buffer.
 * @param _ud_size   User data space to reserve per buffer.
 * @param _destroy   Optional destroy callback when buffer is freed.
 */
#define BT_BUF_POOL_FIXED_DEFINE(_name, _count, _data_size, _ud_size, _destroy)                    \
	_BT_BUF_ARRAY_DEFINE(_name, _count, _ud_size);                                             \
	static uint8_t __noinit bt_buf_data_##_name[_count][_data_size] __bt_buf_align;            \
	static const struct bt_buf_pool_fixed bt_buf_fixed_##_name = {                             \
		.data_pool = (uint8_t *)bt_buf_data_##_name,                                       \
	};                                                                                         \
	static const struct bt_buf_data_alloc bt_buf_fixed_alloc_##_name = {                       \
		.cb = &bt_buf_fixed_cb,                                                            \
		.alloc_data = (void *)&bt_buf_fixed_##_name,                                       \
		.max_alloc_size = _data_size,                                                      \
	};                                                                                         \
	struct bt_buf_pool _name = BT_BUF_POOL_INITIALIZER(                                        \
		_name, &bt_buf_fixed_alloc_##_name, _bt_buf_##_name, _count, _ud_size, _destroy)

/** @cond INTERNAL_HIDDEN */
#if 0
extern const struct bt_buf_data_cb bt_buf_var_cb;
/** @endcond */

/**
 *
 * @brief Define a new pool for buffers with variable size payloads
 *
 * Defines a bt_buf_pool struct and the necessary memory storage (array of
 * structs) for the needed amount of buffers. After this, the buffers can be
 * accessed from the pool through bt_buf_alloc. The pool is defined as a
 * static variable, so if it needs to be exported outside the current module
 * this needs to happen with the help of a separate pointer rather than an
 * extern declaration.
 *
 * The data payload of the buffers will be based on a memory pool from which
 * variable size payloads may be allocated.
 *
 * If provided with a custom destroy callback, this callback is
 * responsible for eventually calling bt_buf_destroy() to complete the
 * process of returning the buffer to the pool.
 *
 * @param _name      Name of the pool variable.
 * @param _count     Number of buffers in the pool.
 * @param _data_size Total amount of memory available for data payloads.
 * @param _ud_size   User data space to reserve per buffer.
 * @param _destroy   Optional destroy callback when buffer is freed.
 */
#define BT_BUF_POOL_VAR_DEFINE(_name, _count, _data_size, _ud_size, _destroy)                      \
	_BT_BUF_ARRAY_DEFINE(_name, _count, _ud_size);                                             \
	K_HEAP_DEFINE(bt_buf_mem_pool_##_name, _data_size);                                        \
	static const struct bt_buf_data_alloc bt_buf_data_alloc_##_name = {                        \
		.cb = &bt_buf_var_cb,                                                              \
		.alloc_data = &bt_buf_mem_pool_##_name,                                            \
		.max_alloc_size = 0,                                                               \
	};                                                                                         \
	struct bt_buf_pool bt_buf_pool##_name = BT_BUF_POOL_INITIALIZER(                           \
		_name, &bt_buf_data_alloc_##_name, _bt_buf_##_name, _count, _ud_size, _destroy)

/**
 *
 * @brief Define a new pool for buffers with variable size payloads. Align the
 *        length and start of the buffer to the specified alignment.
 *
 * Defines a bt_buf_pool struct and the necessary memory storage (array of
 * structs) for the needed amount of buffers. After this, the buffers can be
 * accessed from the pool through bt_buf_alloc. The pool is defined as a
 * static variable, so if it needs to be exported outside the current module
 * this needs to happen with the help of a separate pointer rather than an
 * extern declaration.
 *
 * The data payload of the buffers will be based on a memory pool from which
 * variable size payloads may be allocated.
 *
 * If provided with a custom destroy callback, this callback is
 * responsible for eventually calling bt_buf_destroy() to complete the
 * process of returning the buffer to the pool.
 *
 * Both the length and start of the buffer will be aligned to the specified
 * alignment. The alignment must be a power of 2 and the size of the
 * alignment must be less than or equal to the size of the data payload.
 *
 * @param _name      Name of the pool variable.
 * @param _count     Number of buffers in the pool.
 * @param _data_size Total amount of memory available for data payloads.
 * @param _ud_size   User data space to reserve per buffer.
 * @param _destroy   Optional destroy callback when buffer is freed.
 * @param _align     Alignment of the length and start of the buffer.
 */
#define BT_BUF_POOL_VAR_ALIGN_DEFINE(_name, _count, _data_size, _ud_size, _destroy, _align)        \
	_BT_BUF_ARRAY_DEFINE(_name, _count, _ud_size);                                             \
	K_HEAP_DEFINE(bt_buf_mem_pool_##_name, _data_size);                                        \
	static const struct bt_buf_data_alloc bt_buf_data_alloc_##_name = {                        \
		.cb = &bt_buf_var_cb,                                                              \
		.alloc_data = &bt_buf_mem_pool_##_name,                                            \
		.max_alloc_size = 0,                                                               \
		.alignment = _align,                                                               \
	};                                                                                         \
	struct bt_buf_pool bt_buf_pool##_name = BT_BUF_POOL_INITIALIZER(                           \
		_name, &bt_buf_data_alloc_##_name, _bt_buf_##_name, _count, _ud_size, _destroy)
#endif
/**
 *
 * @brief Define a new pool for buffers
 *
 * Defines a bt_buf_pool struct and the necessary memory storage (array of
 * structs) for the needed amount of buffers. After this,the buffers can be
 * accessed from the pool through bt_buf_alloc. The pool is defined as a
 * static variable, so if it needs to be exported outside the current module
 * this needs to happen with the help of a separate pointer rather than an
 * extern declaration.
 *
 * If provided with a custom destroy callback this callback is
 * responsible for eventually calling bt_buf_destroy() to complete the
 * process of returning the buffer to the pool.
 *
 * @param _name     Name of the pool variable.
 * @param _count    Number of buffers in the pool.
 * @param _size     Maximum data size for each buffer.
 * @param _ud_size  Amount of user data space to reserve.
 * @param _destroy  Optional destroy callback when buffer is freed.
 */
#define BT_BUF_POOL_DEFINE(_name, _count, _size, _ud_size, _destroy)                               \
	BT_BUF_POOL_FIXED_DEFINE(_name, _count, _size, _ud_size, _destroy)

/**
 * @brief Get a zero-based index for a buffer.
 *
 * This function will translate a buffer into a zero-based index,
 * based on its placement in its buffer pool. This can be useful if you
 * want to associate an external array of meta-data contexts with the
 * buffers of a pool.
 *
 * @param buf  Network buffer.
 *
 * @return Zero-based index for the buffer.
 */
int bt_buf_id(const struct bt_buf *buf);

/**
 * @brief Allocate a new fixed buffer from a pool.
 *
 * @param pool Which pool to allocate the buffer from.
 * @param timeout Affects the action taken should the pool be empty.
 *        If OS_TIMEOUT_NO_WAIT, then return immediately. If OS_TIMEOUT_FOREVER, then
 *        wait as long as necessary. Otherwise, wait until the specified
 *        timeout. Note that some types of data allocators do not support
 *        blocking (such as the HEAP type). In this case it's still possible
 *        for bt_buf_alloc() to fail (return NULL) even if it was given
 *        OS_TIMEOUT_FOREVER.
 *
 * @return New buffer or NULL if out of buffers.
 */
#if defined(CONFIG_BT_BUF_LOG)
struct bt_buf *__must_check bt_buf_alloc_fixed_debug(struct bt_buf_pool *pool, int32_t timeout,
						     const char *func, int line);
#define bt_buf_alloc_fixed(pool, timeout)                                                          \
	bt_buf_alloc_fixed_debug(pool, timeout, __func__, __LINE__)
#else
struct bt_buf *__must_check bt_buf_alloc_fixed(struct bt_buf_pool *pool, int32_t timeout);
#endif

/**
 * @copydetails bt_buf_alloc_fixed
 */
static inline struct bt_buf *__must_check bt_buf_alloc(struct bt_buf_pool *pool, int32_t timeout)
{
	return bt_buf_alloc_fixed(pool, timeout);
}

/**
 * @brief Allocate a new variable length buffer from a pool.
 *
 * @param pool Which pool to allocate the buffer from.
 * @param size Amount of data the buffer must be able to fit.
 * @param timeout Affects the action taken should the pool be empty.
 *        If OS_TIMEOUT_NO_WAIT, then return immediately. If OS_TIMEOUT_FOREVER, then
 *        wait as long as necessary. Otherwise, wait until the specified
 *        timeout. Note that some types of data allocators do not support
 *        blocking (such as the HEAP type). In this case it's still possible
 *        for bt_buf_alloc() to fail (return NULL) even if it was given
 *        OS_TIMEOUT_FOREVER.
 *
 * @return New buffer or NULL if out of buffers.
 */
#if defined(CONFIG_BT_BUF_LOG)
struct bt_buf *bt_buf_alloc_len_debug(struct bt_buf_pool *pool, size_t size, int32_t timeout,
				      const char *func, int line);
#define bt_buf_alloc_len(pool, size, timeout)                                                      \
	bt_buf_alloc_len_debug(pool, size, timeout, __func__, __LINE__)
#else
struct bt_buf *__must_check bt_buf_alloc_len(struct bt_buf_pool *pool, size_t size,
					     int32_t timeout);
#endif

/**
 * @brief Allocate a new buffer from a pool but with external data pointer.
 *
 * Allocate a new buffer from a pool, where the data pointer comes from the
 * user and not from the pool.
 *
 * @param pool Which pool to allocate the buffer from.
 * @param data External data pointer
 * @param size Amount of data the pointed data buffer if able to fit.
 * @param timeout Affects the action taken should the pool be empty.
 *        If OS_TIMEOUT_NO_WAIT, then return immediately. If OS_TIMEOUT_FOREVER, then
 *        wait as long as necessary. Otherwise, wait until the specified
 *        timeout. Note that some types of data allocators do not support
 *        blocking (such as the HEAP type). In this case it's still possible
 *        for bt_buf_alloc() to fail (return NULL) even if it was given
 *        OS_TIMEOUT_FOREVER.
 *
 * @return New buffer or NULL if out of buffers.
 */
struct bt_buf *__must_check bt_buf_alloc_with_data(struct bt_buf_pool *pool, void *data,
						   size_t size, int32_t timeout);

/**
 * @brief Destroy buffer from custom destroy callback
 *
 * This helper is only intended to be used from custom destroy callbacks.
 * If no custom destroy callback is given to BT_BUF_POOL_*_DEFINE() then
 * there is no need to use this API.
 *
 * @param buf Buffer to destroy.
 */
static inline void bt_buf_destroy(struct bt_buf *buf)
{
	struct bt_buf_pool *pool = buf->pool;

	if (buf->__buf) {
		if (!(buf->flags & BT_BUF_EXTERNAL_DATA)) {
			pool->alloc->cb->unref(buf, buf->__buf);
		}
		buf->__buf = NULL;
	}

	bt_lifo_put(&pool->free, buf);
}

/**
 * @brief Reset buffer
 *
 * Reset buffer data and flags so it can be reused for other purposes.
 *
 * @param buf Buffer to reset.
 */
void bt_buf_reset(struct bt_buf *buf);

/**
 * @brief Initialize buffer with the given headroom.
 *
 * The buffer is not expected to contain any data when this API is called.
 *
 * @param buf Buffer to initialize.
 * @param reserve How much headroom to reserve.
 */
void bt_buf_simple_reserve(struct bt_buf_simple *buf, size_t reserve);

/**
 * @brief Put a buffer into a list
 *
 * @param list Which list to append the buffer to.
 * @param buf Buffer.
 */
void bt_buf_slist_put(bt_slist_t *list, struct bt_buf *buf);

/**
 * @brief Get a buffer from a list.
 *
 * @param list Which list to take the buffer from.
 *
 * @return New buffer or NULL if the FIFO is empty.
 */
struct bt_buf *__must_check bt_buf_slist_get(bt_slist_t *list);

/**
 * @brief Decrements the reference count of a buffer.
 *
 * The buffer is put back into the pool if the reference count reaches zero.
 *
 * @param buf A valid pointer on a buffer
 */
#if defined(CONFIG_BT_BUF_LOG)
void bt_buf_unref_debug(struct bt_buf *buf, const char *func, int line);
#define bt_buf_unref(buf) bt_buf_unref_debug(buf, __func__, __LINE__)
#else
void bt_buf_unref(struct bt_buf *buf);
#endif
/**
 * @brief Increment the reference count of a buffer.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return the buffer newly referenced
 */
struct bt_buf *__must_check bt_buf_ref(struct bt_buf *buf);

/**
 * @brief Clone buffer
 *
 * Duplicate given buffer including any (user) data and headers currently stored.
 *
 * @param buf A valid pointer on a buffer
 * @param timeout Affects the action taken should the pool be empty.
 *        If OS_TIMEOUT_NO_WAIT, then return immediately. If OS_TIMEOUT_FOREVER, then
 *        wait as long as necessary. Otherwise, wait until the specified
 *        timeout.
 *
 * @return Cloned buffer or NULL if out of buffers.
 */
struct bt_buf *__must_check bt_buf_clone(struct bt_buf *buf, int32_t timeout);

/**
 * @brief Get a pointer to the user data of a buffer.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return Pointer to the user data of the buffer.
 */
static inline void *__must_check bt_buf_user_data(const struct bt_buf *buf)
{
	return (void *)buf->user_data;
}

/**
 * @brief Copy user data from one to another buffer.
 *
 * @param dst A valid pointer to a buffer gettings its user data overwritten.
 * @param src A valid pointer to a buffer gettings its user data copied. User data size must be
 *            equal to or exceed @a dst.
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_buf_user_data_copy(struct bt_buf *dst, const struct bt_buf *src);

/**
 * @brief Initialize buffer with the given headroom.
 *
 * The buffer is not expected to contain any data when this API is called.
 *
 * @param buf Buffer to initialize.
 * @param reserve How much headroom to reserve.
 */
static inline void bt_buf_reserve(struct bt_buf *buf, size_t reserve)
{
	bt_buf_simple_reserve(&buf->b, reserve);
}

/**
 * @brief Prepare data to be added at the end of the buffer
 *
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to increment the length with.
 *
 * @return The original tail of the buffer.
 */
static inline void *bt_buf_add(struct bt_buf *buf, size_t len)
{
	return bt_buf_simple_add(&buf->b, len);
}

/**
 * @brief Copies the given number of bytes to the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at
 * the end.
 *
 * @param buf Buffer to update.
 * @param mem Location of data to be added.
 * @param len Length of data to be added
 *
 * @return The original tail of the buffer.
 */
static inline void *bt_buf_add_mem(struct bt_buf *buf, const void *mem, size_t len)
{
	return bt_buf_simple_add_mem(&buf->b, mem, len);
}

/**
 * @brief Add (8-bit) byte at the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at
 * the end.
 *
 * @param buf Buffer to update.
 * @param val byte value to be added.
 *
 * @return Pointer to the value added
 */
static inline uint8_t *bt_buf_add_u8(struct bt_buf *buf, uint8_t val)
{
	return bt_buf_simple_add_u8(&buf->b, val);
}

/**
 * @brief Add 16-bit value at the end of the buffer
 *
 * Adds 16-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be added.
 */
static inline void bt_buf_add_le16(struct bt_buf *buf, uint16_t val)
{
	bt_buf_simple_add_le16(&buf->b, val);
}

/**
 * @brief Add 16-bit value at the end of the buffer
 *
 * Adds 16-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be added.
 */
static inline void bt_buf_add_be16(struct bt_buf *buf, uint16_t val)
{
	bt_buf_simple_add_be16(&buf->b, val);
}

/**
 * @brief Add 24-bit value at the end of the buffer
 *
 * Adds 24-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be added.
 */
static inline void bt_buf_add_le24(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_add_le24(&buf->b, val);
}

/**
 * @brief Add 24-bit value at the end of the buffer
 *
 * Adds 24-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be added.
 */
static inline void bt_buf_add_be24(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_add_be24(&buf->b, val);
}

/**
 * @brief Add 32-bit value at the end of the buffer
 *
 * Adds 32-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be added.
 */
static inline void bt_buf_add_le32(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_add_le32(&buf->b, val);
}

/**
 * @brief Add 32-bit value at the end of the buffer
 *
 * Adds 32-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be added.
 */
static inline void bt_buf_add_be32(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_add_be32(&buf->b, val);
}

/**
 * @brief Add 40-bit value at the end of the buffer
 *
 * Adds 40-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be added.
 */
static inline void bt_buf_add_le40(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_add_le40(&buf->b, val);
}

/**
 * @brief Add 40-bit value at the end of the buffer
 *
 * Adds 40-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be added.
 */
static inline void bt_buf_add_be40(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_add_be40(&buf->b, val);
}

/**
 * @brief Add 48-bit value at the end of the buffer
 *
 * Adds 48-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be added.
 */
static inline void bt_buf_add_le48(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_add_le48(&buf->b, val);
}

/**
 * @brief Add 48-bit value at the end of the buffer
 *
 * Adds 48-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be added.
 */
static inline void bt_buf_add_be48(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_add_be48(&buf->b, val);
}

/**
 * @brief Add 64-bit value at the end of the buffer
 *
 * Adds 64-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be added.
 */
static inline void bt_buf_add_le64(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_add_le64(&buf->b, val);
}

/**
 * @brief Add 64-bit value at the end of the buffer
 *
 * Adds 64-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be added.
 */
static inline void bt_buf_add_be64(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_add_be64(&buf->b, val);
}

/**
 * @brief Remove data from the end of the buffer.
 *
 * Removes data from the end of the buffer by modifying the buffer length.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to remove.
 *
 * @return New end of the buffer data.
 */
static inline void *bt_buf_remove_mem(struct bt_buf *buf, size_t len)
{
	return bt_buf_simple_remove_mem(&buf->b, len);
}

/**
 * @brief Remove a 8-bit value from the end of the buffer
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 8-bit values.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return The 8-bit removed value
 */
static inline uint8_t bt_buf_remove_u8(struct bt_buf *buf)
{
	return bt_buf_simple_remove_u8(&buf->b);
}

/**
 * @brief Remove and convert 16 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 16-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from little endian to host endian.
 */
static inline uint16_t bt_buf_remove_le16(struct bt_buf *buf)
{
	return bt_buf_simple_remove_le16(&buf->b);
}

/**
 * @brief Remove and convert 16 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 16-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from big endian to host endian.
 */
static inline uint16_t bt_buf_remove_be16(struct bt_buf *buf)
{
	return bt_buf_simple_remove_be16(&buf->b);
}

/**
 * @brief Remove and convert 24 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 24-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from big endian to host endian.
 */
static inline uint32_t bt_buf_remove_be24(struct bt_buf *buf)
{
	return bt_buf_simple_remove_be24(&buf->b);
}

/**
 * @brief Remove and convert 24 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 24-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from little endian to host endian.
 */
static inline uint32_t bt_buf_remove_le24(struct bt_buf *buf)
{
	return bt_buf_simple_remove_le24(&buf->b);
}

/**
 * @brief Remove and convert 32 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 32-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 32-bit value converted from little endian to host endian.
 */
static inline uint32_t bt_buf_remove_le32(struct bt_buf *buf)
{
	return bt_buf_simple_remove_le32(&buf->b);
}

/**
 * @brief Remove and convert 32 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 32-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 32-bit value converted from big endian to host endian.
 */
static inline uint32_t bt_buf_remove_be32(struct bt_buf *buf)
{
	return bt_buf_simple_remove_be32(&buf->b);
}

/**
 * @brief Remove and convert 40 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 40-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 40-bit value converted from little endian to host endian.
 */
static inline uint64_t bt_buf_remove_le40(struct bt_buf *buf)
{
	return bt_buf_simple_remove_le40(&buf->b);
}

/**
 * @brief Remove and convert 40 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 40-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 40-bit value converted from big endian to host endian.
 */
static inline uint64_t bt_buf_remove_be40(struct bt_buf *buf)
{
	return bt_buf_simple_remove_be40(&buf->b);
}

/**
 * @brief Remove and convert 48 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 48-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 48-bit value converted from little endian to host endian.
 */
static inline uint64_t bt_buf_remove_le48(struct bt_buf *buf)
{
	return bt_buf_simple_remove_le48(&buf->b);
}

/**
 * @brief Remove and convert 48 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 48-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 48-bit value converted from big endian to host endian.
 */
static inline uint64_t bt_buf_remove_be48(struct bt_buf *buf)
{
	return bt_buf_simple_remove_be48(&buf->b);
}

/**
 * @brief Remove and convert 64 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 64-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 64-bit value converted from little endian to host endian.
 */
static inline uint64_t bt_buf_remove_le64(struct bt_buf *buf)
{
	return bt_buf_simple_remove_le64(&buf->b);
}

/**
 * @brief Remove and convert 64 bits from the end of the buffer.
 *
 * Same idea as with bt_buf_remove_mem(), but a helper for operating on
 * 64-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 64-bit value converted from big endian to host endian.
 */
static inline uint64_t bt_buf_remove_be64(struct bt_buf *buf)
{
	return bt_buf_simple_remove_be64(&buf->b);
}

/**
 * @brief Prepare data to be added at the start of the buffer
 *
 * Modifies the data pointer and buffer length to account for more data
 * in the beginning of the buffer.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to add to the beginning.
 *
 * @return The new beginning of the buffer data.
 */
static inline void *bt_buf_push(struct bt_buf *buf, size_t len)
{
	return bt_buf_simple_push(&buf->b, len);
}

/**
 * @brief Copies the given number of bytes to the start of the buffer
 *
 * Modifies the data pointer and buffer length to account for more data
 * in the beginning of the buffer.
 *
 * @param buf Buffer to update.
 * @param mem Location of data to be added.
 * @param len Length of data to be added.
 *
 * @return The new beginning of the buffer data.
 */
static inline void *bt_buf_push_mem(struct bt_buf *buf, const void *mem, size_t len)
{
	return bt_buf_simple_push_mem(&buf->b, mem, len);
}

/**
 * @brief Push 8-bit value to the beginning of the buffer
 *
 * Adds 8-bit value the beginning of the buffer.
 *
 * @param buf Buffer to update.
 * @param val 8-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_u8(struct bt_buf *buf, uint8_t val)
{
	bt_buf_simple_push_u8(&buf->b, val);
}

/**
 * @brief Push 16-bit value to the beginning of the buffer
 *
 * Adds 16-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_le16(struct bt_buf *buf, uint16_t val)
{
	bt_buf_simple_push_le16(&buf->b, val);
}

/**
 * @brief Push 16-bit value to the beginning of the buffer
 *
 * Adds 16-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 16-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_be16(struct bt_buf *buf, uint16_t val)
{
	bt_buf_simple_push_be16(&buf->b, val);
}

/**
 * @brief Push 24-bit value to the beginning of the buffer
 *
 * Adds 24-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_le24(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_push_le24(&buf->b, val);
}

/**
 * @brief Push 24-bit value to the beginning of the buffer
 *
 * Adds 24-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 24-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_be24(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_push_be24(&buf->b, val);
}

/**
 * @brief Push 32-bit value to the beginning of the buffer
 *
 * Adds 32-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_le32(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_push_le32(&buf->b, val);
}

/**
 * @brief Push 32-bit value to the beginning of the buffer
 *
 * Adds 32-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 32-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_be32(struct bt_buf *buf, uint32_t val)
{
	bt_buf_simple_push_be32(&buf->b, val);
}

/**
 * @brief Push 40-bit value to the beginning of the buffer
 *
 * Adds 40-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_le40(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_push_le40(&buf->b, val);
}

/**
 * @brief Push 40-bit value to the beginning of the buffer
 *
 * Adds 40-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 40-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_be40(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_push_be40(&buf->b, val);
}

/**
 * @brief Push 48-bit value to the beginning of the buffer
 *
 * Adds 48-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_le48(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_push_le48(&buf->b, val);
}

/**
 * @brief Push 48-bit value to the beginning of the buffer
 *
 * Adds 48-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 48-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_be48(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_push_be48(&buf->b, val);
}

/**
 * @brief Push 64-bit value to the beginning of the buffer
 *
 * Adds 64-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_le64(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_push_le64(&buf->b, val);
}

/**
 * @brief Push 64-bit value to the beginning of the buffer
 *
 * Adds 64-bit value in big endian format to the beginning of the
 * buffer.
 *
 * @param buf Buffer to update.
 * @param val 64-bit value to be pushed to the buffer.
 */
static inline void bt_buf_push_be64(struct bt_buf *buf, uint64_t val)
{
	bt_buf_simple_push_be64(&buf->b, val);
}

/**
 * @brief Remove data from the beginning of the buffer.
 *
 * Removes data from the beginning of the buffer by modifying the data
 * pointer and buffer length.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to remove.
 *
 * @return New beginning of the buffer data.
 */
static inline void *bt_buf_pull(struct bt_buf *buf, size_t len)
{
	return bt_buf_simple_pull(&buf->b, len);
}

/**
 * @brief Remove data from the beginning of the buffer.
 *
 * Removes data from the beginning of the buffer by modifying the data
 * pointer and buffer length.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to remove.
 *
 * @return Pointer to the old beginning of the buffer data.
 */
static inline void *bt_buf_pull_mem(struct bt_buf *buf, size_t len)
{
	return bt_buf_simple_pull_mem(&buf->b, len);
}

/**
 * @brief Remove a 8-bit value from the beginning of the buffer
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 8-bit values.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return The 8-bit removed value
 */
static inline uint8_t bt_buf_pull_u8(struct bt_buf *buf)
{
	return bt_buf_simple_pull_u8(&buf->b);
}

/**
 * @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 16-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from little endian to host endian.
 */
static inline uint16_t bt_buf_pull_le16(struct bt_buf *buf)
{
	return bt_buf_simple_pull_le16(&buf->b);
}

/**
 * @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 16-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 16-bit value converted from big endian to host endian.
 */
static inline uint16_t bt_buf_pull_be16(struct bt_buf *buf)
{
	return bt_buf_simple_pull_be16(&buf->b);
}

/**
 * @brief Remove and convert 24 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 24-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from little endian to host endian.
 */
static inline uint32_t bt_buf_pull_le24(struct bt_buf *buf)
{
	return bt_buf_simple_pull_le24(&buf->b);
}

/**
 * @brief Remove and convert 24 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 24-bit big endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 24-bit value converted from big endian to host endian.
 */
static inline uint32_t bt_buf_pull_be24(struct bt_buf *buf)
{
	return bt_buf_simple_pull_be24(&buf->b);
}

/**
 * @brief Remove and convert 32 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 32-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 32-bit value converted from little endian to host endian.
 */
static inline uint32_t bt_buf_pull_le32(struct bt_buf *buf)
{
	return bt_buf_simple_pull_le32(&buf->b);
}

/**
 * @brief Remove and convert 32 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 32-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 32-bit value converted from big endian to host endian.
 */
static inline uint32_t bt_buf_pull_be32(struct bt_buf *buf)
{
	return bt_buf_simple_pull_be32(&buf->b);
}

/**
 * @brief Remove and convert 40 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 40-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 40-bit value converted from little endian to host endian.
 */
static inline uint64_t bt_buf_pull_le40(struct bt_buf *buf)
{
	return bt_buf_simple_pull_le40(&buf->b);
}

/**
 * @brief Remove and convert 40 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 40-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 40-bit value converted from big endian to host endian.
 */
static inline uint64_t bt_buf_pull_be40(struct bt_buf *buf)
{
	return bt_buf_simple_pull_be40(&buf->b);
}

/**
 * @brief Remove and convert 48 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 48-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 48-bit value converted from little endian to host endian.
 */
static inline uint64_t bt_buf_pull_le48(struct bt_buf *buf)
{
	return bt_buf_simple_pull_le48(&buf->b);
}

/**
 * @brief Remove and convert 48 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 48-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 48-bit value converted from big endian to host endian.
 */
static inline uint64_t bt_buf_pull_be48(struct bt_buf *buf)
{
	return bt_buf_simple_pull_be48(&buf->b);
}

/**
 * @brief Remove and convert 64 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 64-bit little endian data.
 *
 * @param buf A valid pointer on a buffer.
 *
 * @return 64-bit value converted from little endian to host endian.
 */
static inline uint64_t bt_buf_pull_le64(struct bt_buf *buf)
{
	return bt_buf_simple_pull_le64(&buf->b);
}

/**
 * @brief Remove and convert 64 bits from the beginning of the buffer.
 *
 * Same idea as with bt_buf_pull(), but a helper for operating on
 * 64-bit big endian data.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return 64-bit value converted from big endian to host endian.
 */
static inline uint64_t bt_buf_pull_be64(struct bt_buf *buf)
{
	return bt_buf_simple_pull_be64(&buf->b);
}

/**
 * @brief Check buffer tailroom.
 *
 * Check how much free space there is at the end of the buffer.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return Number of bytes available at the end of the buffer.
 */
static inline size_t bt_buf_tailroom(const struct bt_buf *buf)
{
	return bt_buf_simple_tailroom(&buf->b);
}

/**
 * @brief Check buffer headroom.
 *
 * Check how much free space there is in the beginning of the buffer.
 *
 * buf A valid pointer on a buffer
 *
 * @return Number of bytes available in the beginning of the buffer.
 */
static inline size_t bt_buf_headroom(const struct bt_buf *buf)
{
	return bt_buf_simple_headroom(&buf->b);
}

/**
 * @brief Check maximum bt_buf::len value.
 *
 * This value is depending on the number of bytes being reserved as headroom.
 *
 * @param buf A valid pointer on a buffer
 *
 * @return Number of bytes usable behind the bt_buf::data pointer.
 */
static inline uint16_t bt_buf_max_len(const struct bt_buf *buf)
{
	return bt_buf_simple_max_len(&buf->b);
}

/**
 * @brief Get the tail pointer for a buffer.
 *
 * Get a pointer to the end of the data in a buffer.
 *
 * @param buf Buffer.
 *
 * @return Tail pointer for the buffer.
 */
static inline uint8_t *bt_buf_tail(const struct bt_buf *buf)
{
	return bt_buf_simple_tail(&buf->b);
}

/**
 * @brief Find the last fragment in the fragment list.
 *
 * @return Pointer to last fragment in the list.
 */
struct bt_buf *bt_buf_frag_last(struct bt_buf *frags);

/**
 * @brief Insert a new fragment to a chain of bufs.
 *
 * Insert a new fragment into the buffer fragments list after the parent.
 *
 * Note: This function takes ownership of the fragment reference so the
 * caller is not required to unref.
 *
 * @param parent Parent buffer/fragment.
 * @param frag Fragment to insert.
 */
void bt_buf_frag_insert(struct bt_buf *parent, struct bt_buf *frag);

/**
 * @brief Add a new fragment to the end of a chain of bufs.
 *
 * Append a new fragment into the buffer fragments list.
 *
 * Note: This function takes ownership of the fragment reference so the
 * caller is not required to unref.
 *
 * @param head Head of the fragment chain.
 * @param frag Fragment to add.
 *
 * @return New head of the fragment chain. Either head (if head
 *         was non-NULL) or frag (if head was NULL).
 */
struct bt_buf *bt_buf_frag_add(struct bt_buf *head, struct bt_buf *frag);

/**
 * @brief Delete existing fragment from a chain of bufs.
 *
 * @param parent Parent buffer/fragment, or NULL if there is no parent.
 * @param frag Fragment to delete.
 *
 * @return Pointer to the buffer following the fragment, or NULL if it
 *         had no further fragments.
 */
struct bt_buf *bt_buf_frag_del(struct bt_buf *parent, struct bt_buf *frag);

/**
 * @brief Copy bytes from bt_buf chain starting at offset to linear buffer
 *
 * Copy (extract) @a len bytes from @a src bt_buf chain, starting from @a
 * offset in it, to a linear buffer @a dst. Return number of bytes actually
 * copied, which may be less than requested, if bt_buf chain doesn't have
 * enough data, or destination buffer is too small.
 *
 * @param dst Destination buffer
 * @param dst_len Destination buffer length
 * @param src Source bt_buf chain
 * @param offset Starting offset to copy from
 * @param len Number of bytes to copy
 * @return number of bytes actually copied
 */
size_t bt_buf_linearize(void *dst, size_t dst_len, const struct bt_buf *src, size_t offset,
			size_t len);

/**
 * @typedef bt_buf_allocator_cb
 * @brief Network buffer allocator callback.
 *
 * @details The allocator callback is called when bt_buf_append_bytes
 * needs to allocate a new bt_buf.
 *
 * @param timeout Affects the action taken should the net buf pool be empty.
 *        If OS_TIMEOUT_NO_WAIT, then return immediately. If OS_TIMEOUT_FOREVER, then
 *        wait as long as necessary. Otherwise, wait until the specified
 *        timeout.
 * @param user_data The user data given in bt_buf_append_bytes call.
 * @return pointer to allocated bt_buf or NULL on error.
 */
typedef struct bt_buf *__must_check (*bt_buf_allocator_cb)(int32_t timeout, void *user_data);

/**
 * @brief Append data to a list of bt_buf
 *
 * @details Append data to a bt_buf. If there is not enough space in the
 * bt_buf then more bt_buf will be added, unless there are no free bt_buf
 * and timeout occurs. If not allocator is provided it attempts to allocate from
 * the same pool as the original buffer.
 *
 * @param buf Network buffer.
 * @param len Total length of input data
 * @param value Data to be added
 * @param timeout Timeout is passed to the bt_buf allocator callback.
 * @param allocate_cb When a new bt_buf is required, use this callback.
 * @param user_data A user data pointer to be supplied to the allocate_cb.
 *        This pointer is can be anything from a mem_pool or a net_pkt, the
 *        logic is left up to the allocate_cb function.
 *
 * @return Length of data actually added. This may be less than input
 *         length if other timeout than OS_TIMEOUT_FOREVER was used, and there
 *         were no free fragments in a pool to accommodate all data.
 */
size_t bt_buf_append_bytes(struct bt_buf *buf, size_t len, const void *value, int32_t timeout,
			   bt_buf_allocator_cb allocate_cb, void *user_data);

/**
 * @brief Match data with a bt_buf's content
 *
 * @details Compare data with a content of a bt_buf. Provide information about
 * the number of bytes matching between both. If needed, traverse
 * through multiple buffer fragments.
 *
 * @param buf Network buffer
 * @param offset Starting offset to compare from
 * @param data Data buffer for comparison
 * @param len Number of bytes to compare
 *
 * @return The number of bytes compared before the first difference.
 */
size_t bt_buf_data_match(const struct bt_buf *buf, size_t offset, const void *data, size_t len);

/**
 * @brief Skip N number of bytes in a bt_buf
 *
 * @details Skip N number of bytes starting from fragment's offset. If the total
 * length of data is placed in multiple fragments, this function will skip from
 * all fragments until it reaches N number of bytes.  Any fully skipped buffers
 * are removed from the bt_buf list.
 *
 * @param buf Network buffer.
 * @param len Total length of data to be skipped.
 *
 * @return Pointer to the fragment or
 *         NULL and pos is 0 after successful skip,
 *         NULL and pos is 0xffff otherwise.
 */
static inline struct bt_buf *bt_buf_skip(struct bt_buf *buf, size_t len)
{
	while (buf && len--) {
		bt_buf_pull_u8(buf);
		if (!buf->len) {
			buf = bt_buf_frag_del(NULL, buf);
		}
	}

	return buf;
}

/**
 * @brief Calculate amount of bytes stored in fragments.
 *
 * Calculates the total amount of data stored in the given buffer and the
 * fragments linked to it.
 *
 * @param buf Buffer to start off with.
 *
 * @return Number of bytes in the buffer and its fragments.
 */
static inline size_t bt_buf_frags_len(const struct bt_buf *buf)
{
	size_t bytes = 0;

	while (buf) {
		bytes += buf->len;
		buf = buf->frags;
	}

	return bytes;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __BASE_BUF_H__ */