#include <functional>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <bitset>

#include <docopt/docopt.h>
#include <spdlog/spdlog.h>
#include <binary_io/binary_io.hpp>

// This file will be generated automatically when you run the CMake configuration step.
// It creates a namespace called `os9_rof_tooling`.
// You can modify the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>

static constexpr auto USAGE =
  R"(ROF Parser.

    Usage:
          rdump FILE
          rdump -h
          rdump --version
 Options:
          -h --help     Show this screen.
          --version     Show version.
)";

std::string read_string(binary_io::file_istream &in) { 
    std::string s;
    char c{};
    do {
        in.read(c);
        s.push_back(c);
    } while (c != '\0');
    s.pop_back();// pop the redundant null terminator
    return s;
}

namespace rof {

struct Header
{
  uint32_t sync{};
  uint16_t type_language{};
  uint16_t attribute_rev{};
  uint16_t asm_valid{};
  uint16_t asm_version{};
  uint16_t date1{};
  uint16_t date2{};
  uint16_t date3{};
  uint16_t os9_edition{};
  uint32_t size_static{};
  uint32_t size_initialised{};
  uint32_t size_code{};
  uint32_t size_stack{};
  uint32_t entry_point_offset{};
  uint32_t trap_entry_point_offset{};
  uint32_t size_remote_static{};
  uint32_t size_remote_initialised{};
  uint32_t size_debug{};

  explicit Header(binary_io::file_istream &in)
  {
    in.read(sync,
      type_language,
      attribute_rev,
      asm_valid,
      asm_version,
      date1,
      date2,
      date3,
      os9_edition,
      size_static,
      size_initialised,
      size_code,
      size_stack,
      entry_point_offset,
      trap_entry_point_offset,
      size_remote_static,
      size_remote_initialised,
      size_debug);
  }
};

enum class SymbolType {
    Data,
    InitData,
    Equ,
    Code
};

struct Symbol
{
  std::string name;
  uint8_t type_common;
  uint8_t type_detail;
  uint32_t value;
  explicit Symbol(binary_io::file_istream &in) : name(read_string(in)) {
      in.read(type_common, type_detail, value);
  }



  bool is_common() const { return type_common == 0; }
  SymbolType type() const {
    std::bitset<8> detail(type_detail);
    return (detail.test(2) ? (detail.test(1) ? SymbolType::Equ : SymbolType::Code) : (detail.test(0) ? SymbolType::InitData : SymbolType::Data));
  }

};

namespace detail {
    std::vector<Symbol> read_symbols(binary_io::file_istream& in) {
      uint16_t size{};
      in.read(size);
      std::vector<Symbol> ret;
      ret.reserve(size);
      std::generate_n(std::back_inserter(ret), size, [&] { return Symbol(in); });
      return ret;
    }
}

struct ROF
{
  Header header;
  std::string name;
  std::vector<Symbol> symbols;

  explicit ROF(binary_io::file_istream &in) : header(in), name(read_string(in)), symbols(detail::read_symbols(in)) {}
};

}// namespace rof

int main(int argc, const char **argv)
{
  try {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
      { std::next(argv), std::next(argv, argc) },
      true,// show help if requested
      fmt::format("{} {}",
        os9_rof_tooling::cmake::project_name,
        os9_rof_tooling::cmake::project_version));// version string, acquired from config.hpp via CMake

    binary_io::file_istream in(args.at("FILE").asString());
    in.endian(std::endian::big);

    rof::ROF current(in);
    fmt::print("{}\n", current.name);
    fmt::print("Symbols: {}\n", current.symbols.size());
    for (auto &s : current.symbols) {
      fmt::print("{:<20}{:<7}${:08X}\n",
        s.name,
        s.type() == rof::SymbolType::Data ? "data"
                                          : (s.type() == rof::SymbolType::InitData ? "idata"
                                             : (s.type() == rof::SymbolType::Equ ? "equ" : "code")),
        s.value);
    }

  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
