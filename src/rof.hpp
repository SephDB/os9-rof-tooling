#ifndef ROF_H
#define ROF_H

#include <vector>
#include <string>
#include <bitset>
#include <algorithm>
#include <filesystem>

namespace rof {

struct Header
{
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
};

enum class Section { Data = 0, InitData = 1, RemoteData = 2, RemoteInitData = 3, Equ, Code };

struct Ref
{
  uint8_t info;
  uint8_t detail;
  uint32_t offset;

  Section target() const
  {
    std::bitset<8> detail_set(this->detail);
    bool data = !detail_set.test(2);
    if (data) {
      bool initialised = detail_set.test(0);
      bool remote = detail_set.test(1);
      return static_cast<Section>(remote * 2 + initialised);
    } else {
      return detail_set.test(1) ? Section::Equ : Section::Code;
    }
  }
};

struct ReplacementInfo
{
  Section target;
  uint32_t offset;
  enum class Size { Byte = 0b1, Word = 0b10, Long = 0b11 } size;
  bool relative;
  bool negative;
  explicit ReplacementInfo(Ref r) : offset(r.offset)
  {
    std::bitset<8> detail(r.detail);
    size = static_cast<Size>(detail.test(4) * 2 + detail.test(3));
    relative = detail.test(7); //For some reason these two bits are swapped vs the documentation?
    negative = detail.test(6);
    target = (r.info & 0x2) ? Section::RemoteInitData : (detail.test(5) ? Section::Code : Section::InitData);
  }
};

struct Symbol
{
  std::string name;
  Ref reference;
};

struct ExternRefGroup
{
  std::string name;
  std::vector<Ref> refs;
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
};

std::vector<ROF> read_rof(const std::filesystem::path& file);

}// namespace rof

#endif