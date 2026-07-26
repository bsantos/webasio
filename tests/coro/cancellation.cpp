
#include <boost/test/unit_test.hpp>

#include <webasio/coro/cancellation.hpp>
#include <webasio/coro/detached.hpp>
#include <webasio/coro/promise.hpp>
#include <webasio/scoped_final.hpp>
#include <webasio/logger.hpp>

#include <boost/asio/cancel_after.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <memory>

namespace {
struct coro_test {
    boost::asio::io_context ioc;
    bool aborted = false;
    bool completed = false;
    boost::asio::cancellation_type cancelled = boost::asio::cancellation_type::total;
    boost::asio::cancellation_signal cancel;
    webasio::coro::multicast_cancellation multi_cancel;

    bool is(boost::asio::cancellation_type c) const { return c == cancelled; }

    webasio::co_promise<bool> foo(std::chrono::seconds duration)
    {
        co_return co_await webasio::this_coro::sleep_for(duration) == std::chrono::steady_clock::time_point::min();
    }

    webasio::co_promise<bool> bar()
    {
        boost::asio::ip::udp::socket socket { ioc, boost::asio::ip::udp::v4() };
        char buffer[256];

        auto [ec, _] = co_await socket.async_receive(boost::asio::buffer(buffer));
        co_return ec == boost::asio::error::operation_aborted;
    }

    webasio::co_detached test1(std::chrono::seconds duration)
    {
        co_await webasio::this_coro::reset_cancellation_state(cancel.slot());
        co_await webasio::this_coro::dispatch(ioc.get_executor());

        aborted = co_await foo(duration);
        cancelled = co_await webasio::this_coro::cancelled;
        completed = true;
    }

    webasio::co_detached test2()
    {
        co_await webasio::this_coro::reset_cancellation_state(cancel.slot());

        aborted = co_await bar();
        cancelled = co_await webasio::this_coro::cancelled;
        completed = true;
    }

    webasio::co_detached test3()
    {
        co_await webasio::this_coro::reset_cancellation_state(cancel.slot());
        boost::asio::ip::udp::socket socket { ioc, boost::asio::ip::udp::v4() };
        char buffer[256];

        auto [ec, _] = co_await socket.async_receive(boost::asio::buffer(buffer), boost::asio::cancel_after(std::chrono::seconds(1)));
        aborted = ec == boost::asio::error::operation_aborted;
        cancelled = co_await webasio::this_coro::cancelled;
        completed = true;
    }

    webasio::co_detached foo4()
    {
        webasio::coro::multicast_cancellation_signal signal { multi_cancel };
        co_await webasio::this_coro::reset_cancellation_state(signal.slot());
        co_await webasio::this_coro::dispatch(ioc.get_executor());
        aborted = co_await foo(std::chrono::seconds(3600));
        cancelled = co_await webasio::this_coro::cancelled;
    }

    webasio::co_detached bar4()
    {
        webasio::coro::multicast_cancellation_signal signal { multi_cancel };
        co_await webasio::this_coro::reset_cancellation_state(signal.slot());
        aborted = co_await bar();
        completed = co_await webasio::this_coro::cancelled == boost::asio::cancellation_type::terminal;
    }

    void test4()
    {
        foo4();
        bar4();
    }
};
}

BOOST_AUTO_TEST_SUITE(coro_cancellation)

BOOST_AUTO_TEST_CASE(test_sleep_no_cancel)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test1(std::chrono::seconds(1)));
    BOOST_TEST(!c.aborted);
    BOOST_TEST(!c.completed);
    BOOST_CHECK_NO_THROW(c.ioc.run());
    BOOST_TEST(!c.aborted);
    BOOST_TEST(c.completed);
    BOOST_TEST(c.is(boost::asio::cancellation_type::none));
}

BOOST_AUTO_TEST_CASE(test_sleep_cancel)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test1(std::chrono::seconds(3600)));
    BOOST_TEST(!c.aborted);
    BOOST_TEST(!c.completed);
    boost::asio::post(c.ioc, [&]() { c.cancel.emit(boost::asio::cancellation_type::terminal); });
    BOOST_CHECK_NO_THROW(c.ioc.run());
    BOOST_TEST(c.aborted);
    BOOST_TEST(c.completed);
    BOOST_TEST(c.is(boost::asio::cancellation_type::terminal));
}

BOOST_AUTO_TEST_CASE(test_receive_cancel)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test2());
    BOOST_TEST(!c.aborted);
    BOOST_TEST(!c.completed);
    boost::asio::post(c.ioc, [&]() { c.cancel.emit(boost::asio::cancellation_type::terminal); });
    BOOST_CHECK_NO_THROW(c.ioc.run());
    BOOST_TEST(c.aborted);
    BOOST_TEST(c.completed);
    BOOST_TEST(c.is(boost::asio::cancellation_type::terminal));
}

BOOST_AUTO_TEST_CASE(test_receive_cancel_after)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test3());
    BOOST_TEST(!c.aborted);
    BOOST_TEST(!c.completed);
    BOOST_CHECK_NO_THROW(c.ioc.run());
    BOOST_TEST(c.aborted);
    BOOST_TEST(c.completed);
    BOOST_TEST(c.is(boost::asio::cancellation_type::none));
}

BOOST_AUTO_TEST_CASE(test_receive_multi_cancel)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test4());
    BOOST_TEST(!c.aborted);
    BOOST_TEST(!c.completed);
    c.multi_cancel.emit(c.ioc);
    BOOST_CHECK_NO_THROW(c.ioc.run());
    BOOST_TEST(c.aborted);
    BOOST_TEST(c.completed);
    BOOST_TEST(c.is(boost::asio::cancellation_type::terminal));
}

BOOST_AUTO_TEST_SUITE_END()
