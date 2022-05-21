#ifndef OS9_DISASS_H
#define OS9_DISASS_H

#include <string>
#include <binary_io/span_stream.hpp>

std::string read_instruction(binary_io::span_istream&);

#endif
