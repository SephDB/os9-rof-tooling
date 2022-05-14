#include <filesystem>

#include <binary_io/binary_io.hpp>
#include <fmt/format.h>

#include "rof.hpp"

namespace {
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

std::vector<std::byte> read_bytes(binary_io::file_istream &in, uint32_t size)
{
  std::vector<std::byte> ret(size);
  in.read_bytes(ret);
  return ret;
}
}// namespace

namespace rof {
namespace {
  struct Reader
  {
    binary_io::file_istream in;
    void read(Header &h)
    {
      in.read(h.type_language,
        h.attribute_rev,
        h.asm_valid,
        h.asm_version,
        h.date1,
        h.date2,
        h.date3,
        h.os9_edition,
        h.size_static,
        h.size_initialised,
        h.size_code,
        h.size_stack,
        h.entry_point_offset,
        h.trap_entry_point_offset,
        h.size_remote_static,
        h.size_remote_initialised,
        h.size_debug);
    }
    void read(Ref &r) { in.read(r.info, r.detail, r.offset); }
    void read(Symbol &s)
    {
      s.name = read_string(in);
      read(s.reference);
    }

    void read(ExternRefGroup &g)
    {
      g.name = read_string(in);
      g.refs = read_table<Ref>();
    }

    ROF get_segment()
    {
      Header h;
      read(h);

      ROF ret{ h,
        read_string(in),
        read_table<Symbol>(),
        read_bytes(in, h.size_code),
        read_bytes(in, h.size_initialised),
        read_bytes(in, h.size_remote_initialised),
        read_bytes(in, h.size_debug),
        read_table<ExternRefGroup>(),
        read_table<Ref>() };
      in.seek_relative(16);// 16 bytes of padding after each ROF

      return ret;
    }

    template<typename Entry> std::vector<Entry> read_table()
    {
      uint16_t size{};
      in.read(size);
      std::vector<Entry> ret;
      ret.reserve(size);
      std::generate_n(std::back_inserter(ret), size, [this] {
        Entry e{};
        read(e);
        return e;
      });
      return ret;
    }
  };
}// namespace

std::vector<ROF> read_rof(const std::filesystem::path &file)
{
  Reader reader{ file };
  reader.in.endian(std::endian::big);

  std::vector<rof::ROF> segments;
  try {
    constexpr uint32_t magic = 0xdeadface;
    uint32_t sync{};
    reader.in.read(sync);
    for (; sync == magic; reader.in.read(sync)) { segments.emplace_back(reader.get_segment()); }
    fmt::print("Wrong sync bytes: {:08X}\n", sync);
    return {};
  } catch (const binary_io::buffer_exhausted &) {
  }
  return segments;
}
}// namespace rof