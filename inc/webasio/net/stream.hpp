//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/basic_stream.hpp>

#include <memory>
#include <variant>
#include <utility>

namespace webasio::net {

template<class Executor, class RatePolicy>
class basic_stream {
public:
    using tcp_stream = boost::beast::basic_stream<boost::asio::ip::tcp, Executor, RatePolicy>;
    using tls_stream = boost::asio::ssl::stream<tcp_stream>;
    using socket_type = tcp_stream::socket_type;
    using executor_type = tcp_stream::executor_type;
    using protocol_type = tcp_stream::protocol_type;
    using endpoint_type = tcp_stream::endpoint_type;
    using context_type = boost::asio::ssl::context;
    using handshake_type = tls_stream::handshake_type;

    template<class OtherExecutor>
    struct rebind_executor {
        using other = basic_stream<OtherExecutor, RatePolicy>;
    };

    template<class... Args>
    basic_stream(Args&&... args)
        : stream_ { std::in_place_type<tcp_stream>, std::forward<Args>(args)... }
    {}

    template<class... Args>
    basic_stream(context_type& ctx, Args&&... args)
        : stream_ { std::in_place_type<tls_stream>, std::forward<Args>(args)..., ctx }
    {}

    basic_stream(basic_stream&& other)
        : stream_ { std::move(other) }
    {}

    void upgrade(context_type& ctx)
    {
        tcp_stream tmp { std::move(next_layer()) };
        stream_.emplace(std::in_place_type<tls_stream>, std::move(tmp), ctx);
    }

    void downgrade()
    {
        tcp_stream tmp = std::move(next_layer());
        stream_.emplace(std::in_place_type<tcp_stream>, std::move(tmp));
    }

    tls_stream* tls() noexcept { return std::get_if<tls_stream>(&stream_); }
    tls_stream const* tls() const noexcept { return std::get_if<tls_stream>(&stream_); }

    tcp_stream& next_layer() noexcept
    {
        return std::visit([](auto& stream) -> tcp_stream& {
            if constexpr (std::is_same_v<tls_stream&, decltype(stream)>)
                return stream.next_layer();
            else
                return stream;
        }, stream_);
    }

    tcp_stream const& next_layer() const noexcept
    {
        return std::visit([](auto& stream) -> tcp_stream const& {
            if constexpr (std::is_same_v<tls_stream const&, decltype(stream)>)
                return stream.next_layer();
            else
                return stream;
        }, stream_);
    }

    executor_type get_executor() noexcept
    {
        return next_layer().get_executor();
    }

    socket_type& socket() noexcept
    {
        return next_layer().socket();
    }

    socket_type const& socket() const noexcept
    {
        return next_layer().socket();
    }

    void expires_after(boost::asio::steady_timer::duration expiry_time)
    {
        next_layer().expires_after(expiry_time);
    }

    void expires_at(boost::asio::steady_timer::time_point expiry_time)
    {
        next_layer().expires_at(expiry_time);
    }

    void expires_never()
    {
        next_layer().expires_never();
    }

    struct init_async_handshake {
        basic_stream* self;

        using executor_type = basic_stream::executor_type;

        executor_type get_executor() const noexcept
        {
            return self->get_executor();
        }

        template<class HandshakeHandler>
        void operator()(HandshakeHandler&& h, handshake_type type) const
        {
            if (auto stream = self->tls()) {
                stream->async_handshake(type, std::forward<HandshakeHandler>(h));
                return;
            }

            boost::asio::dispatch(boost::asio::get_associated_executor(h, self->get_executor()), [h = std::forward<HandshakeHandler>(h)]() mutable {
                std::move(h)(boost::system::error_code {});
            });
        }
    };

    struct init_async_shutdown {
        basic_stream* self;

        using executor_type = basic_stream::executor_type;

        executor_type get_executor() const noexcept
        {
            return self->get_executor();
        }

        template<class ShutdownHandler>
        void operator()(ShutdownHandler&& h) const
        {
            if (auto stream = self->tls()) {
                stream->async_shutdown(std::forward<ShutdownHandler>(h));
                return;
            }

            boost::asio::dispatch(boost::asio::get_associated_executor(h, self->get_executor()), [h = std::forward<ShutdownHandler>(h)]() mutable {
                std::move(h)(boost::system::error_code {});
            });
        }
    };

    struct init_async_write {
        basic_stream* self;

        using executor_type = basic_stream::executor_type;

        executor_type get_executor() const noexcept
        {
            return self->get_executor();
        }

        template<class WriteHandler, class ConstBufferSequence>
        void operator()(WriteHandler&& h, ConstBufferSequence const& buffers) const
        {
            std::visit([h = std::forward<WriteHandler>(h), &buffers](auto& stream) mutable {
                stream.async_write_some(buffers, std::move(h));
            }, self->stream_);
        }
    };

    struct init_async_read {
        basic_stream* self;

        using executor_type = basic_stream::executor_type;

        executor_type get_executor() const noexcept
        {
            return self->get_executor();
        }

        template<class ReadHandler, class MutableBufferSequence>
        void operator()(ReadHandler&& h, MutableBufferSequence const& buffers) const
        {
            std::visit([h = std::forward<ReadHandler>(h), &buffers](auto& stream) mutable {
                stream.async_read_some(buffers, std::move(h));
            }, self->stream_);
        }
    };

public:
    template<class HandshakeToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_handshake(handshake_type type, HandshakeToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<HandshakeToken, void(boost::system::error_code)>(init_async_handshake { this }, token, type);
    }

    template<class ShutdownToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_shutdown(ShutdownToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<ShutdownToken, void(boost::system::error_code)>(init_async_shutdown { this }, token);
    }

    template<class ConstBufferSequence, class WriteToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_write_some(ConstBufferSequence const& buffers, WriteToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<WriteToken, void(boost::system::error_code, size_t)>(init_async_write { this }, token, buffers);
    }

    template<class MutableBufferSequence, class ReadToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_read_some(MutableBufferSequence const& buffers, ReadToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<ReadToken, void(boost::system::error_code, size_t)>(init_async_read { this }, token, buffers);
    }

private:
    std::variant<tcp_stream, tls_stream> stream_;
};

using stream = basic_stream<boost::asio::any_io_executor, boost::beast::unlimited_rate_policy>;

} // namespace webasio::net
