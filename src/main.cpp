#include <algorithm>
#include <functional>
#include <filesystem>
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
      fmt::print("{}\n", current.name);
      fmt::print("Symbols: {}\n", current.symbols.size());
      for (auto &r : current.local_refs) {
        auto target = rof::ReplacementInfo(r);
        fmt::print("{}@{:08X}->{}\n", target.target, target.offset, r.target());
      }
    }
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
