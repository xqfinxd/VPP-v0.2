set(VPP_HEADERS     "Core.h"
					"UUID.h"
					"GameObject.h"
					"Scene.h"
                    "${VPP_BINARY_DIR}/src/Config.h"
                    "${VPP_SOURCE_DIR}/include/VPP/VPP.h")
set(VPP_SOURCES     "Core.cc"
					"UUID.cc"
					"GameObject.cc"
					"Scene.cc")

add_library(VPP ${VPP_SOURCES} ${VPP_HEADERS})

target_compile_definitions(VPP PRIVATE VPP_USE_CONFIG_H)

target_include_directories(VPP PUBLIC
                           "$<BUILD_INTERFACE:${VPP_SOURCE_DIR}/include>")
						   
target_include_directories(VPP PRIVATE
                           "${VPP_SOURCE_DIR}/src"
                           "${VPP_BINARY_DIR}/src")
						   
target_include_directories(VPP PRIVATE
                           "${VPP_SOURCE_DIR}/third/entt"
                           "${VPP_SOURCE_DIR}/third/stb_image"
						   "${VPP_SOURCE_DIR}/third/glm")

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