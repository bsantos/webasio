#include <boost/test/unit_test.hpp>

#include <webasio/coro/detached.hpp>
#include <webasio/coro/promise.hpp>

#include <memory>

namespace {
struct coro_test {
    std::string result;

    webasio::coro::promise<std::string> foo()
    {
        co_return "foo return value";
    }

    webasio::coro::promise<std::string> bar(bool throw_up)
    {
        if (throw_up)
            throw std::runtime_error("bar throw up");

        co_return "bar return value";
    }

    webasio::coro::detached test_foo()
    {
        result = co_await foo();
    }

    webasio::coro::detached test_bar(bool throw_up)
    {
        try {
            result = co_await bar(throw_up);
        }
        catch (std::exception const& ex) {
            result = ex.what();
        }
    }
};
}

BOOST_AUTO_TEST_SUITE(coro_promise)

BOOST_AUTO_TEST_CASE(test_foo)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test_foo());
    BOOST_TEST(c.result == "foo return value");
}

BOOST_AUTO_TEST_CASE(test_bar_throw)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test_bar(true));
    BOOST_TEST(c.result == "bar throw up");
}

BOOST_AUTO_TEST_CASE(test_bar_no_throw)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test_bar(false));
    BOOST_TEST(c.result == "bar return value");
}

BOOST_AUTO_TEST_SUITE_END()
