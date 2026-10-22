medecomp_pkg(sdl2)
medecomp_pkg(zlib)
set_source_files_properties(${ROOT}/src/m3g/GZipStreamBasic.cpp PROPERTIES COMPILE_DEFINITIONS
    "inflate=inflate_port;inflateInit2_=inflateInit2__port;inflateEnd=inflateEnd_port")

medecomp_replace(game/AppEngineThread.cpp)

set(MEDECOMP_IMGUI ${CMAKE_CURRENT_LIST_DIR}/../../third_party/imgui)
list(APPEND PORT_SOURCES ${MEDECOMP_IMGUI}/imgui.cpp ${MEDECOMP_IMGUI}/imgui_draw.cpp
    ${MEDECOMP_IMGUI}/imgui_tables.cpp ${MEDECOMP_IMGUI}/imgui_widgets.cpp
    ${MEDECOMP_IMGUI}/backends/imgui_impl_sdl2.cpp ${MEDECOMP_IMGUI}/backends/imgui_impl_sdlrenderer2.cpp)
list(APPEND MEDECOMP_INCLUDE_DIRS ${MEDECOMP_IMGUI})
set(MEDECOMP_MCUI ${CMAKE_CURRENT_LIST_DIR}/../../third_party/mooncellar-ui)
list(APPEND PORT_SOURCES ${MEDECOMP_MCUI}/mcui.cpp ${MEDECOMP_MCUI}/mcui_icon.cpp ${MEDECOMP_MCUI}/mcui_widgets.cpp)
list(APPEND MEDECOMP_INCLUDE_DIRS ${MEDECOMP_MCUI})
foreach(f fonts/ApercuPro-Regular.ttf fonts/ApercuPro-Medium.ttf fonts/ApercuPro-Bold.ttf logo-icon.png logo-text.png
          logo-full.png app-icon-256.png)
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../branding/${f} OR NOT f MATCHES "^fonts/")
        configure_file(${CMAKE_CURRENT_LIST_DIR}/../../branding/${f} ${CMAKE_BINARY_DIR}/branding/${f} COPYONLY)
    endif()
endforeach()
medecomp_pkg(fontconfig)
set(MEDECOMP_MONOCYPHER ${CMAKE_CURRENT_LIST_DIR}/../../third_party/monocypher)
list(APPEND PORT_SOURCES ${MEDECOMP_MONOCYPHER}/monocypher.c ${MEDECOMP_MONOCYPHER}/monocypher-ed25519.c)
list(APPEND MEDECOMP_INCLUDE_DIRS ${MEDECOMP_MONOCYPHER})
if(WIN32)
    list(APPEND MEDECOMP_LINK_FLAGS -lole32 -luuid)
endif()
