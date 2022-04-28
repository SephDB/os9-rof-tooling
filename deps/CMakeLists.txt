include(FetchContent)

FetchContent_Declare(binary_io
    GIT_REPOSITORY https://github.com/Ryan-rsm-McKenzie/binary_io.git
    GIT_TAG        2.0.4
)

if(NOT binary_io_POPULATED)
    FetchContent_Populate(binary_io)

    option(BUILD_TESTING OFF)
    
    add_subdirectory(${binary_io_SOURCE_DIR} ${binary_io_BINARY_DIR})
endif()