#include <functional>
#include <iostream>
#include <string>

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

void read_string(std::string &s, binary_io::file_istream &in) { 
    s.clear();
    char c;
    do {
        in.read(c);
        s.push_back(c);
    } while (c != '\0');
    s.pop_back();// pop the redundant null terminator
}

struct rof_header
{
  uint32_t sync;
  uint16_t type_language;
  uint16_t attribute_rev;
  uint16_t asm_valid;
  uint16_t asm_version;
  uint16_t date1;
  uint16_t date2;
  uint16_t date3;
  uint16_t os9_edition;
  uint32_t size_static;
  uint32_t size_initialised;
  uint32_t size_code;
  uint32_t size_stack;
  uint32_t entry_point_offset;
  uint32_t trap_entry_point_offset;
  uint32_t size_remote_static;
  uint32_t size_remote_initialised;
  uint32_t size_debug;

  rof_header(binary_io::file_istream& in) { 
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

    rof_header header(in);
    std::string name;
    read_string(name, in);
    fmt::print("{}\n", name);

  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
