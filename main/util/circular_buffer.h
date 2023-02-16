#pragma once 

#include <stdint.h>
#include <stddef.h>

namespace Helper {
	template<bool FITS8, bool FITS16> struct Index {
		using Type = uint32_t;
	};

	template<> struct Index<false, true> {
		using Type = uint16_t;
	};

	template<> struct Index<true, true> {
		using Type = uint8_t;
	};
}

template<typename T, size_t S, typename IT = typename Helper::Index<(S <= UINT8_MAX), (S <= UINT16_MAX)>::Type> class circular_buffer {
public:
	/**
	 * The buffer capacity: read only as it cannot ever change.
	 */
	static constexpr IT capacity = static_cast<IT>(S);

	/**
	 * Aliases the index type, can be used to obtain the right index type with `decltype(buffer)::index_t`.
	 */
	using index_t = IT;

	constexpr circular_buffer();

	/**
	 * Disables copy constructor
	 */
	circular_buffer(const circular_buffer&) = delete;
	circular_buffer(circular_buffer&&) = delete;

	/**
	 * Disables assignment operator
	 */
	circular_buffer& operator=(const circular_buffer&) = delete;
	circular_buffer& operator=(circular_buffer&&) = delete;

	/**
	 * Adds an element to the beginning of buffer: the operation returns `false` if the addition caused overwriting an existing element.
	 */
	bool unshift(T value);

	/**
	 * Adds an element to the end of buffer: the operation returns `false` if the addition caused overwriting an existing element.
	 */
	bool push(T value);

	/**
	 * Removes an element from the beginning of the buffer.
	 * *WARNING* Calling this operation on an empty buffer has an unpredictable behaviour.
	 */
	T shift();

	/**
	 * Removes an element from the end of the buffer.
	 * *WARNING* Calling this operation on an empty buffer has an unpredictable behaviour.
	 */
	T pop();

	/**
	 * Returns the element at the beginning of the buffer.
	 */
	T inline first() const;

	/**
	 * Returns the element at the end of the buffer.
	 */
	T inline last() const;

	/**
	 * Array-like access to buffer.
	 * Calling this operation using and index value greater than `size - 1` returns the tail element.
	 * *WARNING* Calling this operation on an empty buffer has an unpredictable behaviour.
	 */
	T operator [] (IT index) const;

	/**
	 * Returns how many elements are actually stored in the buffer.
	 */
	IT inline size() const;

	/**
	 * Returns how many elements can be safely pushed into the buffer.
	 */
	IT inline available() const;

	/**
	 * Returns `true` if no elements can be removed from the buffer.
	 */
	bool inline isEmpty() const;

	/**
	 * Returns `true` if no elements can be added to the buffer without overwriting existing elements.
	 */
	bool inline isFull() const;

	/**
	 * Resets the buffer to a clean status, making all buffer positions available.
	 */
	void inline clear();

private:
	T buffer[S];
	T *head;
	T *tail;
	IT count;
};

template<typename T, size_t S, typename IT>
constexpr circular_buffer<T,S,IT>::circular_buffer() :
		head(buffer), tail(buffer), count(0) {
}

template<typename T, size_t S, typename IT>
bool circular_buffer<T,S,IT>::unshift(T value) {
	if (head == buffer) {
		head = buffer + capacity;
	}
	*--head = value;
	if (count == capacity) {
		if (tail-- == buffer) {
			tail = buffer + capacity - 1;
		}
		return false;
	} else {
		if (count++ == 0) {
			tail = head;
		}
		return true;
	}
}

template<typename T, size_t S, typename IT>
bool circular_buffer<T,S,IT>::push(T value) {
	if (++tail == buffer + capacity) {
		tail = buffer;
	}
	*tail = value;
	if (count == capacity) {
		if (++head == buffer + capacity) {
			head = buffer;
		}
		return false;
	} else {
		if (count++ == 0) {
			head = tail;
		}
		return true;
	}
}

template<typename T, size_t S, typename IT>
T circular_buffer<T,S,IT>::shift() {
	if (count == 0) return *head;
	T result = *head++;
	if (head >= buffer + capacity) {
		head = buffer;
	}
	count--;
	return result;
}

template<typename T, size_t S, typename IT>
T circular_buffer<T,S,IT>::pop() {
	if (count == 0) return *tail;
	T result = *tail--;
	if (tail < buffer) {
		tail = buffer + capacity - 1;
	}
	count--;
	return result;
}

template<typename T, size_t S, typename IT>
T inline circular_buffer<T,S,IT>::first() const {
	return *head;
}

template<typename T, size_t S, typename IT>
T inline circular_buffer<T,S,IT>::last() const {
	return *tail;
}

template<typename T, size_t S, typename IT>
T circular_buffer<T,S,IT>::operator [](IT index) const {
	if (index >= count) return *tail;
	return *(buffer + ((head - buffer + index) % capacity));
}

template<typename T, size_t S, typename IT>
IT inline circular_buffer<T,S,IT>::size() const {
	return count;
}

template<typename T, size_t S, typename IT>
IT inline circular_buffer<T,S,IT>::available() const {
	return capacity - count;
}

template<typename T, size_t S, typename IT>
bool inline circular_buffer<T,S,IT>::isEmpty() const {
	return count == 0;
}

template<typename T, size_t S, typename IT>
bool inline circular_buffer<T,S,IT>::isFull() const {
	return count == capacity;
}

template<typename T, size_t S, typename IT>
void inline circular_buffer<T,S,IT>::clear() {
	head = tail = buffer;
	count = 0;
}