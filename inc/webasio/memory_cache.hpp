//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#ifdef WEBASIO_MEMCACHE_USAGE_DEBUG
#   include <webasio/logger.hpp>
#   define WEBASIO_MEMCACHE_INC_STAT(name) do { ++name; } while (false)
#else
#   define WEBASIO_MEMCACHE_INC_STAT(name) do {} while (false)
#endif

#include <array>

namespace webasio {

class memory_cache {
public:
    static constexpr size_t cache_count = 128;
    static constexpr size_t entry_count = 16;
    static constexpr size_t alignment = 64;

    class tls;

    static constexpr size_t aligned_size(size_t size)
    {
        return (size % alignment == 0) ? size : size + (alignment - size % alignment);
    }

    static constexpr size_t cache_index(size_t size)
    {
        return aligned_size(size) / alignment - 1;
    }

    constexpr memory_cache() noexcept
    {
        for (auto& entries : cache_)
            for (auto& entry : entries)
                entry = nullptr;
    }

    ~memory_cache() noexcept
    {
#ifdef WEBASIO_MEMCACHE_USAGE_DEBUG
        auto print_bucket_usage = [this]() {
            std::string res;
            res += '[';
            for (size_t count : usages_) {
                res += std::to_string(count);
                res += ',';
            }
            res.back() = ']';
            return res;
        };

        logger { "memory-cache" }.info("memory_cache: total=", total_, ", misses=", misses_, ", oversize=", oversize_, ", overcache=", overcache_, ", hit-rate=", std::round(double(total_ - misses_ - oversize_) / total_ * 100), '%', ", bucket-stats=", print_bucket_usage());
#endif

        for (auto&& entries : cache_)
            for (auto entry : entries)
                ::operator delete(entry);
    }

    void* alloc(size_t const size)
    {
        size_t const idx = cache_index(size);

        WEBASIO_MEMCACHE_INC_STAT(total_);

        if (idx < cache_.size()) {
            auto& entries = cache_[idx];

            WEBASIO_MEMCACHE_INC_STAT(usages_[idx]);

            for (auto& entry : entries) {
                if (entry)
                    return std::exchange(entry, nullptr);
            }

            WEBASIO_MEMCACHE_INC_STAT(misses_);
            return ::operator new(aligned_size(size));
        }

        WEBASIO_MEMCACHE_INC_STAT(oversize_);
        return ::operator new(size);
    }

    void free(void* ptr, size_t const size)
    {
        size_t const idx = cache_index(size);

        if (idx < cache_.size()) {
            auto& entries = cache_[idx];

            for (auto& entry : entries) {
                if (!entry) {
                    entry = ptr;
                    return;
                }
            }

            WEBASIO_MEMCACHE_INC_STAT(overcache_);
        }

        ::operator delete(ptr);
    }

private:
    std::array<std::array<void*, entry_count>, cache_count> cache_;

#ifdef WEBASIO_MEMCACHE_USAGE_DEBUG
    std::array<size_t, entry_count> usages_;
    size_t total_ = 0;
    size_t misses_ = 0;
    size_t oversize_ = 0;
    size_t overcache_ = 0;
#endif
};

class memory_cache::tls {
    static thread_local memory_cache cache_;

public:
    class deleter {
    public:
        deleter(size_t size)
            : size_ { size }
        {}

        void operator()(void* p) const
        {
            cache_.free(p, size_);
        }

    private:
        size_t size_;
    };

    static void* alloc(size_t size)
    {
        return cache_.alloc(size);
    }

    static void free(void* ptr, size_t size)
    {
        cache_.free(ptr, size);
    }
};

template<class T = void>
class cached_tls_allocator {
public:
    using value_type = T;

    template<class U>
    struct rebind {
        using other = cached_tls_allocator<U>;
    };

    constexpr cached_tls_allocator()
    {}

    template<class U>
    cached_tls_allocator(cached_tls_allocator<U> const&)
    {}

    T* allocate(size_t n)
    {
        return static_cast<T*>(memory_cache::tls::alloc(sizeof(T) * n));
    }

    void deallocate(T* p, size_t n)
    {
        memory_cache::tls::free(p, sizeof(T) * n);
    }
};

template<>
class cached_tls_allocator<void> {
public:
    using value_type = void;

    template<class U>
    struct rebind {
        using other = cached_tls_allocator<U>;
    };

    constexpr cached_tls_allocator()
    {}

    template<class U>
    cached_tls_allocator(cached_tls_allocator<U> const&)
    {}
};

template< class T1, class T2>
constexpr bool operator==(cached_tls_allocator<T1> const&, cached_tls_allocator<T2> const&) noexcept
{
    return true;
}

} // namespace webasio
