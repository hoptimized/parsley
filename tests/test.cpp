#include <catch2/catch_test_macros.hpp>
#include <parsley.h>

TEST_CASE("magic number tests")
{
    REQUIRE(parsley::get_magic_number() == 42);
}
