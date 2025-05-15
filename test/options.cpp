
#include "util/options.hpp"

#include <catch2/catch_all.hpp>
namespace vb::opt {
using namespace std::literals;
using namespace literals;

TEST_CASE("Test options parsing", "[options]")
{
    constexpr auto option = "-t, --test Testing the options"_opt;
    REQUIRE(option.shortkey() == 't');
    REQUIRE(option == "-t");
    REQUIRE(option.key() == "test"sv);
}

} // namespace vb::opt
