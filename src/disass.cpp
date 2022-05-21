#include <fmt/format.h>
#include <optional>
#include <os9rof/disass.h>

namespace {
template<typename T, T def> struct opt
{
  T value = def;
  constexpr opt(T v) : value(v) {}
  constexpr bool empty() const { return value == def; }
};
}// namespace

enum class InstructionSize { Byte, Word, Long };

namespace ea {
namespace ExtensionHelpers {
  template<int N> constexpr auto const_num = [](InstructionSize) { return N; };

  constexpr auto eq_ins_size = [](InstructionSize s) {
    switch (s) {
    case InstructionSize::Long:
      return 2;
    default:
      return 1;
    }
  };
}// namespace ExtensionHelpers

template<uint8_t Mode, opt<uint8_t, 0xff> Reg = 0xff, auto Extension = ExtensionHelpers::const_num<0>> struct EA
{
  constexpr static uint8_t mode = Mode;
  constexpr static std::optional<uint8_t> reg = Reg.empty() ? std::nullopt : std::optional{ Reg.value };
  constexpr static auto num_extensions = Extension;
};

struct DataReg : EA<0>
{
  uint8_t reg_num;
  DataReg(uint8_t r) : reg_num(r) {}
};
}// namespace ea

namespace {
template<size_t Length> struct fixed_string
{
  char _chars[Length + 1] = {};
  constexpr fixed_string(const char(&arr)[Length + 1]) {
      std::copy(arr,arr+Length,_chars);
  }
  constexpr operator std::string_view() const { return std::string_view{_chars,_chars+Length}; }
};

template<size_t N> fixed_string(const char (&arr)[N]) -> fixed_string<N - 1>;
}// namespace

template<fixed_string Name, uint16_t Indicator, uint16_t fixed, uint16_t mask, class ParsingDetails> struct InsDesc
{
  constexpr static std::string_view Name = Name;
  constexpr static uint16_t Indicator = Indicator << 12 | fixed;
  constexpr static uint16_t Mask = 0xf000 | mask;
};

template<typename... Ins> struct InsList {
    static std::string_view find_ins(binary_io::span_istream& in) {
        const uint16_t operation = std::get<0>(in.read<uint16_t>());
        std::string_view res{};
        auto test = [&]<typename I>(I) -> bool {
            if ((operation & I::Mask) == I::Indicator) {
                res = I::Name;
                return true;
            }
            return false;
        };
        ((test(Ins{})) || ...);
        return res;
    }
};

using Instructions = InsList<InsDesc<"ADDA", 0b1101, 00300, 00300, void>>;

std::string read_instruction(binary_io::span_istream &s) { return fmt::format("{}",Instructions::find_ins(s)); }