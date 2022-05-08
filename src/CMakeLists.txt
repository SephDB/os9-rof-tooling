find_package(fmt CONFIG)
find_package(spdlog CONFIG)
find_package(docopt CONFIG)


add_executable(rdump main.cpp rof.hpp rof.cpp)
target_link_libraries(
  rdump
  PRIVATE project_options
          project_warnings
          docopt::docopt
          fmt::fmt
          binary_io::binary_io)

target_include_directories(rdump PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")

