#include <boost/test/unit_test.hpp>

#include <webasio/coro/detached.hpp>
#include <webasio/coro/dispatch.hpp>
#include <webasio/coro/post.hpp>
#include <webasio/coro/promise.hpp>

#include <boost/asio/io_context.hpp>

#include <memory>
#include <thread>

namespace {
struct coro_test {
    webasio::coro::unique_handle<> resume;
    boost::asio::io_context ioc1;
    boost::asio::io_context ioc2;
    std::thread::id tid1;
    std::thread::id tid2;
    bool completed = false;

    webasio::co_promise<std::thread::id> get_thread_id()
    {
        co_return std::this_thread::get_id();
    }

    webasio::co_detached dispatch1()
    {
        co_await webasio::coro::dispatch(ioc1.get_executor());
        tid1 = co_await get_thread_id();
        completed = true;
    }

    webasio::co_detached dispatch2()
    {
        co_await webasio::coro::dispatch(ioc2.get_executor());
        tid2 = co_await get_thread_id();
        completed = true;
    }

    webasio::co_detached dispatch3()
    {
        dispatch1();
        dispatch2();
        co_return;
    }

    webasio::co_promise<void> foo1()
    {
        co_await webasio::coro::dispatch(ioc1.get_executor());
        tid1 = co_await get_thread_id();
    }

    webasio::co_detached dispatch4()
    {
        auto guard1 = boost::asio::make_work_guard(ioc1);
        auto guard2 = boost::asio::make_work_guard(ioc2);

        co_await webasio::coro::post(ioc1.get_executor());
        co_await webasio::coro::dispatch(ioc2.get_executor());
        co_await foo1();
        tid2 = co_await get_thread_id();
        completed = true;
    }
};
}

BOOST_AUTO_TEST_SUITE(coro_executor)

BOOST_AUTO_TEST_CASE(test_executor_t1)
{
    coro_test c;
    std::thread t1;
    std::thread t2;
    std::thread::id tid1;
    std::thread::id tid2;

	BOOST_CHECK_NO_THROW(c.dispatch1());

    t1 = std::thread([&]() { c.ioc1.run(); });
    tid1 = t1.get_id();
    t2 = std::thread([&]() { c.ioc2.run(); });
    tid2 = t2.get_id();

    t1.join();
    t2.join();

    BOOST_TEST(c.completed);
    BOOST_TEST(c.tid1 == tid1);
    BOOST_TEST(c.tid2 == std::thread::id());
}

BOOST_AUTO_TEST_CASE(test_executor_t2)
{
    coro_test c;
    std::thread t1;
    std::thread t2;
    std::thread::id tid1;
    std::thread::id tid2;

	BOOST_CHECK_NO_THROW(c.dispatch2());

    t1 = std::thread([&]() { c.ioc1.run(); });
    tid1 = t1.get_id();
    t2 = std::thread([&]() { c.ioc2.run(); });
    tid2 = t2.get_id();

    t2.join();
    t1.join();

    BOOST_TEST(c.completed);
    BOOST_TEST(c.tid1 == std::thread::id());
    BOOST_TEST(c.tid2 == tid2);
}

BOOST_AUTO_TEST_CASE(test_executor_all)
{
    coro_test c;
    std::thread t1;
    std::thread t2;
    std::thread::id tid1;
    std::thread::id tid2;

	BOOST_CHECK_NO_THROW(c.dispatch3());

    t1 = std::thread([&]() { c.ioc1.run(); });
    tid1 = t1.get_id();
    t2 = std::thread([&]() { c.ioc2.run(); });
    tid2 = t2.get_id();

    t1.join();
    t2.join();

    BOOST_TEST(c.completed);
    BOOST_TEST(c.tid1 == tid1);
    BOOST_TEST(c.tid2 == tid2);
}

BOOST_AUTO_TEST_CASE(test_executor_caller_callee)
{
    coro_test c;
    std::thread t1;
    std::thread t2;
    std::thread::id tid1;
    std::thread::id tid2;

	BOOST_CHECK_NO_THROW(c.dispatch4());

    t1 = std::thread([&]() { c.ioc1.run(); });
    tid1 = t1.get_id();
    t2 = std::thread([&]() { c.ioc2.run(); });
    tid2 = t2.get_id();

    t1.join();
    t2.join();

    BOOST_TEST(c.completed);
    BOOST_TEST(c.tid1 == tid1);
    BOOST_TEST(c.tid2 == tid2);
}

BOOST_AUTO_TEST_SUITE_END()
