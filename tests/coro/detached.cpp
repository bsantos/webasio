#include <boost/test/unit_test.hpp>

#include <webasio/coro/detached.hpp>
#include <webasio/coro/promise.hpp>

#include <memory>

namespace {
struct coro_test {
    bool completed = false;

    webasio::coro::detached set_complete()
    {
        completed = co_await get_true();
    }

    webasio::coro::promise<bool> get_true()
    {
        co_return true;
    }

    webasio::coro::detached throw_exception()
    {
        if (co_await get_true())
            throw std::runtime_error("get_true returned true");
    }
};
}

BOOST_AUTO_TEST_SUITE(coro_detached)

BOOST_AUTO_TEST_CASE(test_detached)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.set_complete());
    BOOST_TEST(c.completed);
}

BOOST_AUTO_TEST_CASE(test_exception)
{
    coro_test c;

    BOOST_CHECK_THROW(c.throw_exception(), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
