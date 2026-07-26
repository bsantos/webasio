#include <boost/test/unit_test.hpp>

#include <webasio/coro/await.hpp>
#include <webasio/coro/promise.hpp>


namespace {
struct coro_test {
    std::string result;

    webasio::coro::promise<std::string> foo()
    {
        co_return "foo return value";
    }

    webasio::coro::promise<std::string> bar(std::string const& value)
    {
        if (!value.empty())
            throw std::runtime_error(value);

        co_return "bar return value";
    }

    void test_foo()
    {
        webasio::coro::await(foo(), [this](webasio::coro::outcome<std::string>&& res) {
            result = res.get();
        });
    }

    void test_bar(std::string const& throw_value)
    {
        webasio::coro::await(std::bind(&coro_test::bar, this, throw_value), [this](webasio::coro::outcome<std::string>&& res) {
            try {
                result = res.get();
            }
            catch (std::exception const& ex) {
                result = ex.what();
            }
        });
    }
};
}

BOOST_AUTO_TEST_SUITE(coro_await)

BOOST_AUTO_TEST_CASE(test_foo)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test_foo());
    BOOST_TEST(c.result == "foo return value");
}

BOOST_AUTO_TEST_CASE(test_bar_throw)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test_bar("bar throw up"));
    BOOST_TEST(c.result == "bar throw up");
}

BOOST_AUTO_TEST_CASE(test_bar_no_throw)
{
    coro_test c;

	BOOST_CHECK_NO_THROW(c.test_bar(""));
    BOOST_TEST(c.result == "bar return value");
}

BOOST_AUTO_TEST_SUITE_END()
