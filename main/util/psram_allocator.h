#pragma once

#include <esp_heap_caps.h>
#include <memory>

namespace esp32
{
namespace psram
{
template <typename T> class allocator
{
  public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T value_type;

    allocator()
    {
    }
    ~allocator()
    {
    }

    template <class U> struct rebind
    {
        typedef allocator<U> other;
    };
    template <class U> allocator(const allocator<U> &)
    {
    }

    pointer address(reference x) const
    {
        return &x;
    }
    const_pointer address(const_reference x) const
    {
        return &x;
    }
    size_type max_size() const throw()
    {
        return size_t(-1) / sizeof(value_type);
    }

    pointer allocate(size_type n, const void *hint = 0)
    {
        return static_cast<pointer>(heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM));
    }

    void deallocate(pointer p, size_type n)
    {
        free(p);
    }

    template <class U, class... Args> void construct(U *p, Args &&...args)
    {
        ::new ((void *)p) U(std::forward<Args>(args)...);
    }

    void destroy(pointer p)
    {
        p->~T();
    }
};

struct deleter
{
    void operator()(void *p) const
    {
        free(p);
    }
};

template <class T, class... Args> std::unique_ptr<T, deleter> make_unique(Args &&...args)
{
    auto p = heap_caps_malloc(sizeof(T), MALLOC_CAP_SPIRAM);
    return std::unique_ptr<T, deleter>(::new (p) T(std::forward<Args>(args)...), deleter());
}

struct json_allocator
{
    void *allocate(size_t size)
    {
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    }

    void deallocate(void *pointer)
    {
        free(pointer);
    }

    void *reallocate(void *ptr, size_t new_size)
    {
        return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
    }
};

using string = std::basic_string<char, std::char_traits<char>, esp32::psram::json_allocator>;
} // namespace psram
} // namespace esp32
