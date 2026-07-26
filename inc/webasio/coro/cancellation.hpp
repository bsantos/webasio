//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/assert.hpp>
#include <webasio/memory_cache.hpp>

#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/dispatch.hpp>


namespace webasio::coro {

using cancellation_slot = boost::asio::cancellation_slot;
using cancellation_type = boost::asio::cancellation_type;

class cancellation_signal : public boost::asio::cancellation_signal {
    using base = boost::asio::cancellation_signal;

    struct emit_signal {
        cancellation_signal* signal_;
        cancellation_type type_;

        using allocator_type = cached_tls_allocator<>;

        allocator_type get_allocator() const noexcept
        {
            return {};
        }

        void operator()() const
        {
            signal_->emit(type_);
        }
    };

public:
    void emit(cancellation_type type = cancellation_type::terminal)
    {
        base::emit(type);
    }

    template<class Executor>
    void emit(Executor&& ex, cancellation_type type = cancellation_type::terminal)
    {
        boost::asio::dispatch(std::forward<Executor>(ex), emit_signal { this, type });
    }
};

class multicast_cancellation {
    auto get()
    {
        if (!signals_)
            signals_ = std::make_shared<std::vector<signal*>>();

        return signals_;
    }

public:
    class signal : public cancellation_signal {
    public:
        signal(multicast_cancellation& multicast)
            : signals_ { multicast.get() }
        {
            signals_->push_back(this);
        }

        ~signal()
        {
            std::erase(*signals_, this);
        }

    private:
        std::shared_ptr<std::vector<signal*>> signals_;
    };

private:
    struct emit_signal {
        cancellation_type type_;
        std::shared_ptr<std::vector<signal*>> signals_;

        using allocator_type = cached_tls_allocator<>;

        allocator_type get_allocator() const noexcept
        {
            return {};
        }

        void operator()() const
        {
            if (!signals_)
                return;

            for (auto& sig : *signals_)
                sig->emit(type_);
        }
    };

public:
    void emit(cancellation_type type = cancellation_type::terminal)
    {
        if (!signals_)
            return;

        for (auto& sig : *signals_)
            sig->emit(type);
    }

    template<class Executor>
    void emit(Executor&& ex, cancellation_type type = cancellation_type::terminal)
    {
        boost::asio::dispatch(std::forward<Executor>(ex), emit_signal { type, signals_ });
    }

private:
    std::shared_ptr<std::vector<signal*>> signals_;
};

using multicast_cancellation_signal = multicast_cancellation::signal;

} // namespace webasio::coro
