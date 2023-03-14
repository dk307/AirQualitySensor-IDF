#pragma once

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

template <typename T, size_t S, typename IT = size_t>
    requires std::is_trivially_copyable_v<T>
class circular_buffer
{
  public:
    /**
     * The buffer capacity: read only as it cannot ever change.
     */
    static constexpr IT capacity = static_cast<IT>(S);

    /**
     * Aliases the index type, can be used to obtain the right index type with
     * `decltype(buffer)::index_t`.
     */
    using index_t = IT;

    constexpr circular_buffer();

    /**
     * Disables copy constructor
     */
    circular_buffer(const circular_buffer &) = delete;
    circular_buffer(circular_buffer &&) = delete;

    /**
     * Disables assignment operator
     */
    circular_buffer &operator=(const circular_buffer &) = delete;
    circular_buffer &operator=(circular_buffer &&) = delete;

    /**
     * Adds an element to the beginning of buffer: the operation returns `false`
     * if the addition caused overwriting an existing element.
     */
    bool unshift(T value);

    /**
     * Adds an element to the end of buffer: the operation returns `false` if the
     * addition caused overwriting an existing element.
     */
    bool push(T value);

    /**
     * Removes an element from the beginning of the buffer.
     * *WARNING* Calling this operation on an empty buffer has an unpredictable
     * behaviour.
     */
    T shift();

    /**
     * Removes an element from the end of the buffer.
     * *WARNING* Calling this operation on an empty buffer has an unpredictable
     * behaviour.
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
     * Calling this operation using and index value greater than `size - 1`
     * returns the tail element. *WARNING* Calling this operation on an empty
     * buffer has an unpredictable behaviour.
     */
    T operator[](IT index) const;

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
     * Returns `true` if no elements can be added to the buffer without
     * overwriting existing elements.
     */
    bool inline is_full() const;

    /**
     * Resets the buffer to a clean status, making all buffer positions available.
     */
    void inline clear();

  private:
    T buffer_[S]{};
    T *head_{};
    T *tail_{};
    IT count_{};
};

template <typename T, size_t S, typename IT> constexpr circular_buffer<T, S, IT>::circular_buffer() : head_(buffer_), tail_(buffer_), count_(0)
{
}

template <typename T, size_t S, typename IT> bool circular_buffer<T, S, IT>::unshift(T value)
{
    if (head_ == buffer_)
    {
        head_ = buffer_ + capacity;
    }
    *--head_ = value;
    if (count_ == capacity)
    {
        if (tail_-- == buffer_)
        {
            tail_ = buffer_ + capacity - 1;
        }
        return false;
    }
    else
    {
        if (count_++ == 0)
        {
            tail_ = head_;
        }
        return true;
    }
}

template <typename T, size_t S, typename IT> bool circular_buffer<T, S, IT>::push(T value)
{
    if (++tail_ == buffer_ + capacity)
    {
        tail_ = buffer_;
    }
    *tail_ = value;
    if (count_ == capacity)
    {
        if (++head_ == buffer_ + capacity)
        {
            head_ = buffer_;
        }
        return false;
    }
    else
    {
        if (count_++ == 0)
        {
            head_ = tail_;
        }
        return true;
    }
}

template <typename T, size_t S, typename IT> T circular_buffer<T, S, IT>::shift()
{
    if (count_ == 0)
        return *head_;
    T result = *head_++;
    if (head_ >= buffer_ + capacity)
    {
        head_ = buffer_;
    }
    count_--;
    return result;
}

template <typename T, size_t S, typename IT> T circular_buffer<T, S, IT>::pop()
{
    if (count_ == 0)
        return *tail_;
    T result = *tail_--;
    if (tail_ < buffer_)
    {
        tail_ = buffer_ + capacity - 1;
    }
    count_--;
    return result;
}

template <typename T, size_t S, typename IT> T inline circular_buffer<T, S, IT>::first() const
{
    return *head_;
}

template <typename T, size_t S, typename IT> T inline circular_buffer<T, S, IT>::last() const
{
    return *tail_;
}

template <typename T, size_t S, typename IT> T circular_buffer<T, S, IT>::operator[](IT index) const
{
    if (index >= count_)
        return *tail_;
    return *(buffer_ + ((head_ - buffer_ + index) % capacity));
}

template <typename T, size_t S, typename IT> IT inline circular_buffer<T, S, IT>::size() const
{
    return count_;
}

template <typename T, size_t S, typename IT> IT inline circular_buffer<T, S, IT>::available() const
{
    return capacity - count_;
}

template <typename T, size_t S, typename IT> bool inline circular_buffer<T, S, IT>::isEmpty() const
{
    return count_ == 0;
}

template <typename T, size_t S, typename IT> bool inline circular_buffer<T, S, IT>::is_full() const
{
    return count_ == capacity;
}

template <typename T, size_t S, typename IT> void inline circular_buffer<T, S, IT>::clear()
{
    head_ = tail_ = buffer_;
    count_ = 0;
}