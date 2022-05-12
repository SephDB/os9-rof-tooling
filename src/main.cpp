#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <docopt/docopt.h>
#include <fmt/format.h>

#include "rof.hpp"

// This file will be generated automatically when you run the CMake configuration step.
// It creates a namespace called `os9_rof_tooling`.
// You can modify the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>

std::string_view to_string(rof::Section section)
{
  switch (section) {
  case rof::Section::Data:
    return "data";
  case rof::Section::InitData:
    return "idata";
  case rof::Section::RemoteData:
    return "rdata";
  case rof::Section::RemoteInitData:
    return "irdata";
  case rof::Section::Code:
    return "code";
  case rof::Section::Equ:
    return "equ";
  }
  return "UnkownSection!";
}

char to_char(rof::ReplacementInfo::Size s)
{
  switch (s) {
  case rof::ReplacementInfo::Size::Byte:
    return 'b';
  case rof::ReplacementInfo::Size::Word:
    return 'w';
  default:
    return 'l';
  }
}

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

int main(int argc, const char **argv)
{
  try {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
      { std::next(argv), std::next(argv, argc) },
      true,// show help if requested
      fmt::format("{} {}",
        os9_rof_tooling::cmake::project_name,
        os9_rof_tooling::cmake::project_version));// version string, acquired from config.hpp via CMake

    auto segments = rof::read_rof(args.at("FILE").asString());

    for (auto &current : segments) {
      fmt::print("{}: code={} idata={} data={} rinit={} rdata={} debug={}\n",
        current.name,
        current.object_code.size(),
        current.init_data.size(),
        current.header.size_static,
        current.init_remote.size(),
        current.header.size_remote_static,
        current.header.size_debug);
      if (current.symbols.size()) {
        fmt::print("SYMBOLS:\n");
        for (auto &s : current.symbols) {
          fmt::print("{:<48}{:<8}${:08X}\n", s.name, to_string(s.reference.target()), s.reference.offset);
        }
      }
      if (current.extern_refs.size()) {
        fmt::print("\nEXT_REFS:\n");
        for (auto &r : current.extern_refs) {
          fmt::print("{}\n", r.name);
          for (auto &details : r.refs) {
            rof::ReplacementInfo replacement(details);
            fmt::print("{:>9}@${:08X}.{}{}{}\n",
              to_string(replacement.target),
              replacement.offset,
              to_char(replacement.size),
              replacement.negative ? " neg" : "",
              replacement.relative ? " rel" : "");
          }
        }
      }
      if (current.local_refs.size()) {
        fmt::print("\nLOCAL:\n");
        for (auto &r : current.local_refs) {
          rof::ReplacementInfo replacement(r);
          fmt::print("{:>9}@${:08X}.{}->{}{}{}\n",
            to_string(replacement.target),
            replacement.offset,
            to_char(replacement.size),
            to_string(r.target()),
            replacement.negative ? " neg" : "",
            replacement.relative ? " rel" : "");
        }
      }
    }
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
