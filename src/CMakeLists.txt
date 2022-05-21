find_package(fmt CONFIG)
find_package(spdlog CONFIG)
find_package(docopt CONFIG)

add_library(os9_core INTERFACE)
target_link_libraries(os9_core
  INTERFACE
    project_options
    project_warnings)
target_include_directories(os9_core
  INTERFACE
    "${CMAKE_SOURCE_DIR}/include"
)

add_library(os9_disass STATIC)
target_sources(os9_disass
  PRIVATE
    disass.cpp
)
target_link_libraries(os9_disass
  PUBLIC os9_core binary_io::binary_io
  PRIVATE fmt::fmt
)

add_executable(rdump main.cpp rof.hpp rof.cpp)
target_link_libraries(
  rdump
  PRIVATE os9_disass
          docopt::docopt
          fmt::fmt
          binary_io::binary_io)

target_include_directories(rdump PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")

