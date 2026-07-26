#pragma once

#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/dispatch.hpp>

#include <memory>


namespace webasio::asio {
namespace detail {

template<class Executor>
struct cancellation_proxy_handler {
    Executor ex_;
    std::weak_ptr<boost::asio::cancellation_signal> signal;

    void operator()(boost::asio::cancellation_type_t type)
    {
        boost::asio::dispatch(ex_, [weak_signal = signal, type]() {
            if (auto signal = weak_signal.lock())
                signal->emit(type);
        });
    }
};

} // namespace detail

class cancellation_proxy {
public:
    template<class Executor, class CompletionToken>
    cancellation_proxy(Executor const& ex, CompletionToken& token)
        : parent_slot_ { boost::asio::get_associated_cancellation_slot(token) }
    {
        if (parent_slot_.is_connected()) {
            signal_ = std::make_shared<boost::asio::cancellation_signal>();
            parent_slot_.emplace<detail::cancellation_proxy_handler<Executor>>(ex, signal_);
        }
    }

    ~cancellation_proxy()
    {
        reset();
    }

    cancellation_proxy(cancellation_proxy const&) = delete;
    cancellation_proxy& operator=(cancellation_proxy const&) = delete;
    cancellation_proxy(cancellation_proxy&&) = default;
    cancellation_proxy& operator=(cancellation_proxy&&) = default;

    void reset() noexcept
    {
        signal_.reset();
        parent_slot_.clear();
    }

    boost::asio::cancellation_slot slot()
    {
        return signal_ ? signal_->slot() : boost::asio::cancellation_slot {};
    }

    template<class CompletionToken>
    decltype(auto) bind(CompletionToken&& token)
    {
        return boost::asio::bind_cancellation_slot(slot(), std::forward<CompletionToken>(token));
    }

private:
    boost::asio::cancellation_slot parent_slot_;
    std::shared_ptr<boost::asio::cancellation_signal> signal_;
};

} // webasio::asio