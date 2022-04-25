find_package(fmt CONFIG)
find_package(spdlog CONFIG)
find_package(docopt CONFIG)
find_package(binary_io REQUIRED CONFIG) #https://github.com/Ryan-rsm-McKenzie/binary_io

# Generic test that uses conan libs
add_executable(rdump main.cpp)
target_link_libraries(
  rdump
  PRIVATE project_options
          project_warnings
          docopt::docopt
          fmt::fmt
          spdlog::spdlog
          binary_io::binary_io)

target_include_directories(rdump PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")

