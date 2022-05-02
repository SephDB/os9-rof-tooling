#include <algorithm>
#include <bitset>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <binary_io/binary_io.hpp>
#include <docopt/docopt.h>
#include <spdlog/spdlog.h>

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

namespace rof {

namespace detail {
  std::string read_string(binary_io::file_istream &in)
  {
    std::string s;
    char c{};
    do {
      in.read(c);
      s.push_back(c);
    } while (c != '\0');
    s.pop_back();// pop the redundant null terminator
    return s;
  }

  template<typename Entry> std::vector<Entry> read_table(binary_io::file_istream &in)
  {
    uint16_t size{};
    in.read(size);
    std::vector<Entry> ret;
    ret.reserve(size);
    std::generate_n(std::back_inserter(ret), size, [&] { return Entry(in); });
    return ret;
  }

  std::vector<std::byte> read_bytes(binary_io::file_istream &in, uint32_t size)
  {
    std::vector<std::byte> ret(size);
    in.read_bytes(ret);
    return ret;
  }
}// namespace detail

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

enum class SymbolType { Data, InitData, Equ, Code };

struct Ref
{
  uint8_t info;
  uint8_t detail;
  uint32_t offset;
  explicit Ref(binary_io::file_istream &in) { in.read(info, detail, offset); }

  SymbolType type() const
  {
    std::bitset<8> detail_set(this->detail);
    return (detail_set.test(2) ? (detail_set.test(1) ? SymbolType::Equ : SymbolType::Code)
                               : (detail_set.test(0) ? SymbolType::InitData : SymbolType::Data));
  }
};

struct Symbol
{
  std::string name;
  Ref reference;
  explicit Symbol(binary_io::file_istream &in) : name(detail::read_string(in)), reference(in) {}
};

struct ExternRefGroup
{
  std::string name;
  std::vector<Ref> refs;
  explicit ExternRefGroup(binary_io::file_istream &in)
    : name(detail::read_string(in)), refs(detail::read_table<Ref>(in))
  {}
};

struct ROF
{
  Header header;
  std::string name;
  std::vector<Symbol> symbols;
  std::vector<std::byte> object_code;
  std::vector<std::byte> init_data;
  std::vector<std::byte> init_remote;
  std::vector<ExternRefGroup> extern_refs;
  std::vector<Ref> local_refs;

  explicit ROF(binary_io::file_istream &in)
    : header(in), name(detail::read_string(in)), symbols(detail::read_table<Symbol>(in)),
      object_code(detail::read_bytes(in, header.size_code)), init_data(detail::read_bytes(in, header.size_initialised)),
      init_remote(detail::read_bytes(in, header.size_remote_initialised)),
      extern_refs(detail::read_table<ExternRefGroup>(in)), local_refs(detail::read_table<Ref>(in))
  {}
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
        s.reference.type() == rof::SymbolType::Data
          ? "data"
          : (s.reference.type() == rof::SymbolType::InitData
               ? "idata"
               : (s.reference.type() == rof::SymbolType::Equ ? "equ" : "code")),
        s.reference.offset);
    }

  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
