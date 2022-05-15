#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include <binary_io/span_stream.hpp>
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

std::string resolve_symbol(rof::Section section, uint32_t offset, const rof::ROF &rof)
{
  auto viable_symbols = rof.symbols | std::views::filter([&](auto &r) { return r.reference.target() == section; })
                        | std::views::filter([&](auto &r) { return r.reference.offset <= offset; });

  if (viable_symbols.begin() == viable_symbols.end()) { return fmt::format("${:08X}", offset); }

  const rof::Symbol &closest = std::ranges::max(viable_symbols, {}, [](auto &r) { return r.reference.offset; });

  if (closest.reference.offset == offset) { return closest.name; }

  return fmt::format("{}+${:X}", closest.name, offset - closest.reference.offset);
}

std::string resolve_local_reference(rof::Ref local, const rof::ROF &rof)
{
  const rof::ReplacementInfo replacement(local);
  const auto section = [&]() -> std::span<const std::byte> {
    switch (replacement.target) {
    case rof::Section::Code:
      return rof.object_code;
    case rof::Section::InitData:
      return rof.init_data;
    case rof::Section::RemoteInitData:
      return rof.init_remote;
    }
    return {};
  }();

  const auto location = [&]() -> uint32_t {
    binary_io::span_istream section_read(section);
    section_read.endian(std::endian::big);
    section_read.seek_absolute(replacement.offset);

    switch (replacement.size) {
    case rof::ReplacementInfo::Size::Byte:
      return std::get<0>(section_read.read<uint8_t>());
    case rof::ReplacementInfo::Size::Word:
      return std::get<0>(section_read.read<uint16_t>());
    case rof::ReplacementInfo::Size::Long:
      return std::get<0>(section_read.read<uint32_t>());
    }
    return 0;
  }();

  return resolve_symbol(local.target(), location, rof);
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
            fmt::print("{:>9}({}).{}{}{}\n",
              to_string(replacement.target),
              resolve_symbol(replacement.target, replacement.offset, current),
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
          fmt::print("{:>9}({}).{}->{}({}){}{}\n",
            to_string(replacement.target),
            resolve_symbol(replacement.target, replacement.offset, current),
            to_char(replacement.size),
            to_string(r.target()),
            resolve_local_reference(r, current),
            replacement.negative ? " neg" : "",
            replacement.relative ? " rel" : "");
        }
      }
    }
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
