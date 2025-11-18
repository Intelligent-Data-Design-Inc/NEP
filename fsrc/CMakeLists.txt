# CMake build file for the fsrc directory (Fortran wrappers)
# Edward Hartnett, integrated via v1.1.0 Fortran wrappers sprint

add_library(ncsqueezef SHARED ncsqueeze.F90)

# Fortran wrappers depend on the core ncsqueeze C library
target_link_libraries(ncsqueezef PRIVATE ncsqueeze)

target_compile_definitions(ncsqueezef PRIVATE
    BUILD_LZ4=$<IF:$<BOOL:${BUILD_LZ4}>,1,0>
    BUILD_BZIP2=$<IF:$<BOOL:${BUILD_BZIP2}>,1,0>
)

set_target_properties(ncsqueezef PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

install(TARGETS ncsqueezef
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
