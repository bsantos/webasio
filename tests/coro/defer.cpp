#include <boost/test/unit_test.hpp>

#include <webasio/coro/defer.hpp>
#include <webasio/coro/detached.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <thread>
#include <vector>

namespace {
struct defer_test {
    boost::asio::io_context ioc;
    std::thread::id ran_on;
    std::vector<int> order;
    bool completed = false;

    webasio::co_detached switch_executor()
    {
        co_await webasio::coro::defer(ioc.get_executor());
        ran_on = std::this_thread::get_id();
        completed = true;
    }

    webasio::co_detached ordered()
    {
        order.push_back(1);
        co_await webasio::coro::defer(ioc.get_executor());
        order.push_back(3);
        completed = true;
    }
};
}

BOOST_AUTO_TEST_SUITE(coro_defer)

BOOST_AUTO_TEST_CASE(test_defer_switches_executor)
{
    defer_test c;

    BOOST_CHECK_NO_THROW(c.switch_executor());
    // defer queues the resume, so nothing past the await runs until the io_context does
    BOOST_TEST(!c.completed);

    std::thread t([&]() { c.ioc.run(); });
    auto tid = t.get_id();
    t.join();

    BOOST_TEST(c.completed);
    BOOST_TEST(c.ran_on == tid);
}

BOOST_AUTO_TEST_CASE(test_defer_never_resumes_inline)
{
    defer_test c;

    // start the coroutine from within the io_context, already on the target
    // executor: defer must still queue the resume behind the current handler
    boost::asio::post(c.ioc, [&]() {
        c.ordered();
        c.order.push_back(2);
    });

    c.ioc.run();

    BOOST_TEST(c.completed);
    BOOST_TEST((c.order == std::vector<int> { 1, 2, 3 }));
}

BOOST_AUTO_TEST_SUITE_END()
