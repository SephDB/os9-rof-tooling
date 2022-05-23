#include <catch2/catch.hpp>
#include <os9rof/disass.h>

TEST_CASE("ADDA", "[instructions]")
{
  const uint16_t base = 0b1101 << 12;
  const uint16_t a1_w_d2 = base | 01302;
  const std::array ins = { a1_w_d2 };
  auto is = binary_io::span_istream(std::as_bytes(std::span{ ins }));
  REQUIRE(read_instruction(is) == "ADDA.W D2,A1");
}
