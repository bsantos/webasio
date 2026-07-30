#include <boost/test/unit_test.hpp>

#include <webasio/coro/detached.hpp>
#include <webasio/coro/promise.hpp>

#include <cstdlib>
#include <memory>
#include <sstream>
#include <string_view>

namespace {
bool terminate_probe_enabled()
{
    auto* var = std::getenv("WEBASIO_TERMINATE_PROBE");
    return var && std::string_view { var } == "1";
}

int run_terminate_probe(char const* test_case)
{
    auto const& mts = boost::unit_test::framework::master_test_suite();
    std::ostringstream cmd;

    cmd << "WEBASIO_TERMINATE_PROBE=1 \"" << mts.argv[0] << "\" --run_test=" << test_case
        << " --report_level=no > /dev/null 2>&1";
    return std::system(cmd.str().c_str());
}

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

BOOST_AUTO_TEST_CASE(test_exception_terminates)
{
    if (terminate_probe_enabled()) {
        coro_test c;
        c.throw_exception();
        BOOST_FAIL("detached exception probe unexpectedly returned");
    }

    auto const rc = run_terminate_probe("coro_detached/test_exception_terminates");
    BOOST_TEST(rc != 0);
}

BOOST_AUTO_TEST_SUITE_END()
