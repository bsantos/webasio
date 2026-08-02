//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/immediate.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/prepend.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/basic_stream.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace webasio::net {

/**
 * @brief A dual-mode TCP stream that can be upgraded to TLS in place.
 *
 * @details
 * Wraps a Beast `basic_stream` (plain TCP with a configurable rate policy and
 * per-operation timeouts) and an @em optional `boost::asio::ssl::stream`
 * layered on top of it. The stream starts in plain TCP mode; @ref upgrade
 * engages a TLS layer over the existing connection without reconnecting, and
 * @ref downgrade removes it again.
 *
 * Every async operation transparently targets the correct layer: while TLS is
 * engaged, reads and writes go through the encrypted stream, otherwise they go
 * straight to the plain TCP stream. @ref async_handshake and @ref async_shutdown
 * complete immediately with no error when no TLS layer is present, so callers
 * can drive the same handshake/shutdown sequence regardless of mode. The type
 * models both the `AsyncReadStream` and `AsyncWriteStream` concepts.
 *
 * The TLS layer holds a reference to the owned TCP stream rather than a copy,
 * so upgrading and downgrading never move the underlying socket. As a
 * consequence the stream is pinned once constructed: it is neither copyable
 * nor movable.
 *
 * @tparam Executor   Executor bound to the underlying I/O objects.
 * @tparam RatePolicy Beast rate policy governing the plain TCP stream.
 */
template<class Executor, class RatePolicy>
class basic_stream {
    struct init_async_handshake;
    struct init_async_shutdown;
    struct init_async_write;
    struct init_async_read;

public:
    using tcp_stream = boost::beast::basic_stream<boost::asio::ip::tcp, Executor, RatePolicy>;
    using tls_stream = boost::asio::ssl::stream<tcp_stream&>;
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

    /// Constructs a plain TCP stream, forwarding @p args to the Beast stream.
    template<class... Args>
    basic_stream(Args&&... args)
        : tcp_stream_ { std::forward<Args>(args)... }
    {}

    /// Constructs the stream already upgraded to TLS using context @p ctx.
    template<class... Args>
    basic_stream(context_type& ctx, Args&&... args)
        : tcp_stream_ { std::forward<Args>(args)... }
        , tls_stream_ { tcp_stream_, ctx }
    {}

    ~basic_stream() = default;

    /// Engages a TLS layer over the current TCP connection using context @p ctx.
    void upgrade(context_type& ctx)
    {
        tls_stream_.emplace(tcp_stream_, ctx);
    }

    /// Removes the TLS layer, reverting to plain TCP.
    void downgrade()
    {
        tls_stream_.reset();
    }

    /// Returns the active TLS stream, or `nullptr` when in plain TCP mode.
    tls_stream* tls() noexcept { return tls_stream_ ? std::addressof(*tls_stream_) : nullptr; }
    /// @copydoc tls()
    tls_stream const* tls() const noexcept { return tls_stream_ ? std::addressof(*tls_stream_) : nullptr; }

    /// Returns the underlying plain TCP stream (the next layer below TLS).
    tcp_stream& next_layer() noexcept
    {
        return tcp_stream_;
    }

    /// @copydoc next_layer()
    tcp_stream const& next_layer() const noexcept
    {
        return tcp_stream_;
    }

    /// Returns the executor associated with the stream.
    executor_type get_executor() noexcept
    {
        return next_layer().get_executor();
    }

    /// Returns the underlying socket.
    socket_type& socket() noexcept
    {
        return next_layer().socket();
    }

    /// @copydoc socket()
    socket_type const& socket() const noexcept
    {
        return next_layer().socket();
    }

    /// Sets the timeout for subsequent operations to @p expiry_time from now.
    void expires_after(boost::asio::steady_timer::duration expiry_time)
    {
        next_layer().expires_after(expiry_time);
    }

    /// Sets the absolute timeout for subsequent operations to @p expiry_time.
    void expires_at(boost::asio::steady_timer::time_point expiry_time)
    {
        next_layer().expires_at(expiry_time);
    }

    /// Disables the timeout for subsequent operations.
    void expires_never()
    {
        next_layer().expires_never();
    }

    /**
     * @brief Initiates a TLS handshake as @p type.
     * @details Completes immediately with no error when the stream is in plain
     * TCP mode. Completion signature: `void(error_code)`.
     */
    template<class HandshakeToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_handshake(handshake_type type, HandshakeToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<HandshakeToken, void(boost::system::error_code)>(init_async_handshake { this }, token, type);
    }

    /**
     * @brief Initiates a TLS shutdown.
     * @details Completes immediately with no error when the stream is in plain
     * TCP mode. Completion signature: `void(error_code)`.
     */
    template<class ShutdownToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_shutdown(ShutdownToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<ShutdownToken, void(boost::system::error_code)>(init_async_shutdown { this }, token);
    }

    /**
     * @brief Writes some data from @p buffers, through TLS when engaged.
     * @details Completion signature: `void(error_code, size_t)`.
     */
    template<class ConstBufferSequence, class WriteToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_write_some(ConstBufferSequence const& buffers, WriteToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<WriteToken, void(boost::system::error_code, size_t)>(init_async_write { this }, token, buffers);
    }

    /**
     * @brief Reads some data into @p buffers, through TLS when engaged.
     * @details Completion signature: `void(error_code, size_t)`.
     */
    template<class MutableBufferSequence, class ReadToken = boost::asio::default_completion_token_t<executor_type>>
    decltype(auto) async_read_some(MutableBufferSequence const& buffers, ReadToken&& token = boost::asio::default_completion_token_t<executor_type>())
    {
        return boost::asio::async_initiate<ReadToken, void(boost::system::error_code, size_t)>(init_async_read { this }, token, buffers);
    }

private:
    /// @internal Initiation for @ref async_handshake.
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
            if (auto* stream = self->tls()) {
                stream->async_handshake(type, std::forward<HandshakeHandler>(h));
                return;
            }

            boost::asio::async_immediate(self->get_executor(), boost::asio::prepend(std::forward<HandshakeHandler>(h), boost::system::error_code {}));
        }
    };

    /// @internal Initiation for @ref async_shutdown.
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
            if (auto* stream = self->tls()) {
                stream->async_shutdown(std::forward<ShutdownHandler>(h));
                return;
            }

            boost::asio::async_immediate(self->get_executor(), boost::asio::prepend(std::forward<ShutdownHandler>(h), boost::system::error_code {}));
        }
    };

    /// @internal Initiation for @ref async_write_some.
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
            if (auto* stream = self->tls()) {
                stream->async_write_some(buffers, std::forward<WriteHandler>(h));
                return;
            }

            self->next_layer().async_write_some(buffers, std::forward<WriteHandler>(h));
        }
    };

    /// @internal Initiation for @ref async_read_some.
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
            if (auto* stream = self->tls()) {
                stream->async_read_some(buffers, std::forward<ReadHandler>(h));
                return;
            }

            self->next_layer().async_read_some(buffers, std::forward<ReadHandler>(h));
        }
    };

    tcp_stream                tcp_stream_;
    std::optional<tls_stream> tls_stream_;
};

/// Convenience alias: a type-erased @ref basic_stream with no rate limiting.
using stream = basic_stream<boost::asio::any_io_executor, boost::beast::unlimited_rate_policy>;

} // namespace webasio::net
