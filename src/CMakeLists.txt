set(VPP_HEADERS     "Core.h"
                    "${VPP_BINARY_DIR}/src/Config.h"
                    "${VPP_SOURCE_DIR}/include/VPP/UUID.h"
                    "${VPP_SOURCE_DIR}/include/VPP/VPP.h")
set(VPP_SOURCES     Test.cc)

add_library(VPP ${VPP_SOURCES} ${VPP_HEADERS})

target_compile_definitions(VPP PRIVATE VPP_USE_CONFIG_H)
target_include_directories(VPP PUBLIC
                           "$<BUILD_INTERFACE:${VPP_SOURCE_DIR}/include>"
                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_include_directories(VPP PRIVATE
                           "${VPP_SOURCE_DIR}/src"
                           "${VPP_BINARY_DIR}/src")

if (BUILD_SHARED_LIBS)
    if (WIN32)
        if (MINGW)
            set_target_properties(VPP PROPERTIES PREFIX "")
            set_target_properties(VPP PROPERTIES IMPORT_SUFFIX "dll.a")
        else()
            set_target_properties(VPP PROPERTIES IMPORT_SUFFIX "dll.lib")
        endif()

        target_compile_definitions(VPP INTERFACE VPP_DLL)
    endif()
endif()