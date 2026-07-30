#include <boost/test/unit_test.hpp>

#include <webasio/coro/detail/basic_detached.hpp>

#include <cstdlib>
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

webasio::coro::detail::basic_detached throw_now()
{
    throw std::runtime_error("basic_detached failure");
    co_return;
}

} // namespace

BOOST_AUTO_TEST_SUITE(coro_basic_detached)

BOOST_AUTO_TEST_CASE(test_exception_terminates)
{
    if (terminate_probe_enabled()) {
        throw_now();
        BOOST_FAIL("basic_detached exception probe unexpectedly returned");
    }

    auto const rc = run_terminate_probe("coro_basic_detached/test_exception_terminates");
    BOOST_TEST(rc != 0);
}

BOOST_AUTO_TEST_SUITE_END()
