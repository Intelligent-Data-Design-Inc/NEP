add_library(ncnextcdf4 SHARED
    nxt4dispatch.c
    nxt4create.c
    nxt4open.c
    nxt4file.c
    nxt4meta.c
    nxt4io.c
    nxt4group.c
    nxt4type.c
)
target_include_directories(ncnextcdf4 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/include
    ${NETCDF_INCLUDE_DIRS}
    ${HDF5_INCLUDE_DIRS}
)
target_link_libraries(ncnextcdf4 PRIVATE ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES} PUBLIC nep)
set_target_properties(ncnextcdf4 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS ncnextcdf4
    EXPORT NEPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES "${CMAKE_SOURCE_DIR}/include/nextcdf4dispatch.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
