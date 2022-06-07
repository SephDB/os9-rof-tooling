#include <fmt/format.h>
#include <optional>
#include <os9rof/disass.h>

#include <os9rof/detail/ea.h>

namespace ea {
    std::string DataReg::to_string() const { return fmt::format("D{}", reg_num); }
    std::string AddressReg::to_string() const { return fmt::format("A{}", reg_num); }
    std::string IndirAddress::to_string() const { return fmt::format("(A{})", reg_num); }
    std::string IndirAddressInc::to_string() const { return fmt::format("(A{})+", reg_num); }
    std::string IndirAddressDec::to_string() const { return fmt::format("-(A{})", reg_num); }
}

template<typename Length, typename Source, typename Dest> struct TwoArgs
{
  static std::string parse(uint16_t n)
  {
    return fmt::format(".{} {},{}", to_char(Length::size(n)), Source::parse(n), Dest::parse(n));
  }
};

namespace {
template<size_t Length> struct fixed_string
{
  char _chars[Length + 1] = {};
  constexpr fixed_string(const char (&arr)[Length + 1]) { std::copy(arr, arr + Length, _chars); }
  constexpr operator std::string_view() const { return std::string_view{ _chars, _chars + Length }; }
};

template<size_t N> fixed_string(const char (&arr)[N]) -> fixed_string<N - 1>;
}// namespace

template<fixed_string Name, uint16_t Indicator, uint16_t fixed, uint16_t mask, class ParsingDetails> struct InsDesc
{
  constexpr static std::string_view Name = Name;
  constexpr static uint16_t Indicator = Indicator << 12 | fixed;
  constexpr static uint16_t Mask = 0xf000 | mask;
  using Parser = ParsingDetails;
};

template<typename... Ins> struct InsList
{
  static std::string find_ins(binary_io::span_istream &in)
  {
    const uint16_t operation = std::get<0>(in.read<uint16_t>());
    std::string res{};
    auto test = [&]<typename I>(I) -> bool {
      if ((operation & I::Mask) == I::Indicator) {
        res = fmt::format("{}{}",I::Name,I::Parser::parse(operation));
        return true;
      }
      return false;
    };
    ((test(Ins{})) || ...);
    return res;
  }
};

using Instructions = InsList<InsDesc<"ADDA", 0b1101, 00300, 00300, TwoArgs<BitWL<8>, ea::DefaultEAReader<ea::AllEAs>, ea::EAReader<ea::Fixed<1>,ea::Read<9,3>,ea::AddressOnly>>>>;

std::string read_instruction(binary_io::span_istream &s) { return Instructions::find_ins(s); }