# CMakeLists.txt for NetCDF-4 C examples
# Edward Hartnett 1/25/26

# Get NetCDF compiler and linker flags from nc-config
execute_process(COMMAND nc-config --cflags OUTPUT_VARIABLE NC_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND nc-config --libs OUTPUT_VARIABLE NC_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

# Parse flags into lists for CMake (separate by spaces)
separate_arguments(NC_CFLAGS_LIST UNIX_COMMAND "${NC_CFLAGS}")
separate_arguments(NC_LIBS_LIST UNIX_COMMAND "${NC_LIBS}")

# List of NetCDF-4 example programs
set(NETCDF4_EXAMPLES
    simple_nc4
    compression
    chunking_performance
    multi_unlimited
    user_types
)

# Build each example
foreach(example ${NETCDF4_EXAMPLES})
    add_executable(${example} ${example}.c)
    target_compile_options(${example} PRIVATE ${NC_CFLAGS_LIST})
    target_link_libraries(${example} PRIVATE ${NC_LIBS_LIST} m)
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()
