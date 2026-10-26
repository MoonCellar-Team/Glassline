cmake_minimum_required(VERSION 3.20)
project(medecomp_port C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
option(MEDECOMP_M32 "32-битная сборка" ON)
if(ANDROID OR APPLE OR EMSCRIPTEN)
    set(MEDECOMP_M32 OFF CACHE BOOL "32-битная сборка" FORCE)
endif()
if(EMSCRIPTEN AND MEDECOMP_WEB_SYSROOT)
    set(ENV{PKG_CONFIG_LIBDIR} "${MEDECOMP_WEB_SYSROOT}/lib/pkgconfig:$ENV{EMSDK}/upstream/emscripten/cache/sysroot/lib/pkgconfig")
endif()

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS ${ROOT}/src/*.cpp)
list(FILTER GAME_SOURCES EXCLUDE REGEX "/src/platform/")
list(REMOVE_ITEM GAME_SOURCES ${ROOT}/src/game/GameCenter.cpp)

file(GLOB_RECURSE PORT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

set(ARCH_FLAGS)
if(MEDECOMP_M32)
    set(ARCH_FLAGS -m32 -malign-double)
    set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig")
endif()

find_package(PkgConfig QUIET)
set(MEDECOMP_LINK_FLAGS)
set(MEDECOMP_INCLUDE_DIRS)
set(MEDECOMP_CFLAGS)
set(MEDECOMP_REPLACED)
set(MEDECOMP_DEPENDS)
set(MEDECOMP_RPATH)
macro(medecomp_replace path)
    list(APPEND MEDECOMP_REPLACED ${ROOT}/src/${path})
endmacro()
macro(medecomp_pkg module)
    if(NOT PKG_CONFIG_FOUND)
        message(FATAL_ERROR "Нужен pkg-config (для ${module})")
    endif()
    string(MAKE_C_IDENTIFIER "${module}" _id)
    pkg_check_modules(PKG_${_id} REQUIRED ${module})
    if(ANDROID OR EMSCRIPTEN)
        list(APPEND MEDECOMP_LINK_FLAGS ${PKG_${_id}_STATIC_LDFLAGS})
    else()
        list(APPEND MEDECOMP_LINK_FLAGS ${PKG_${_id}_LDFLAGS})
    endif()
    list(APPEND MEDECOMP_INCLUDE_DIRS ${PKG_${_id}_INCLUDE_DIRS})
    list(APPEND MEDECOMP_CFLAGS ${PKG_${_id}_CFLAGS_OTHER})
endmacro()
file(GLOB LAYER_CMAKE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*/layer.cmake)
foreach(f ${LAYER_CMAKE})
    include(${f})
endforeach()
if(MEDECOMP_REPLACED)
    list(REMOVE_ITEM GAME_SOURCES ${MEDECOMP_REPLACED})
endif()
if(WIN32)
    list(REMOVE_ITEM GAME_SOURCES ${ROOT}/src/runtime/midp/ReferenceCountedPointer.cpp)
endif()

add_library(medecomp_game OBJECT ${GAME_SOURCES} ${PORT_SOURCES})
if(MEDECOMP_DEPENDS)
    add_dependencies(medecomp_game ${MEDECOMP_DEPENDS})
endif()
set_source_files_properties(${PORT_SOURCES} PROPERTIES COMPILE_OPTIONS -O2)
set_target_properties(medecomp_game PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS ON)
target_include_directories(medecomp_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${ROOT}/include)
if(WIN32)
    target_include_directories(medecomp_game BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/win)
    target_compile_definitions(medecomp_game PRIVATE AL_LIBTYPE_STATIC)
endif()
target_include_directories(medecomp_game SYSTEM PRIVATE ${MEDECOMP_INCLUDE_DIRS})
target_compile_definitions(medecomp_game PRIVATE MEDECOMP_PORT=1)
set(MEDECOMP_VERSION "dev" CACHE STRING "Версия сборки: подпись в лаунчере")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/launcher.cpp PROPERTIES
    COMPILE_DEFINITIONS "MEDECOMP_VERSION=\"${MEDECOMP_VERSION}\"")
target_compile_options(medecomp_game PRIVATE ${ARCH_FLAGS} ${MEDECOMP_CFLAGS} -fno-rtti -fno-strict-aliasing -fwrapv -w
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include/port/prelude.h)

set(STUBS ${CMAKE_CURRENT_BINARY_DIR}/gen/stubs.cpp)
separate_arguments(EXTRA_LINK_FLAGS NATIVE_COMMAND "${CMAKE_EXE_LINKER_FLAGS}")
set(STUB_FLAGS)
foreach(f ${ARCH_FLAGS} ${EXTRA_LINK_FLAGS} ${MEDECOMP_LINK_FLAGS})
    list(APPEND STUB_FLAGS --flag=${f})
endforeach()
set(STUB_SYSTEM_FLAGS --flag=-lm --flag=-lpthread)
if(ANDROID)
    set(STUB_SYSTEM_FLAGS --flag=--target=${CMAKE_CXX_COMPILER_TARGET} --flag=-static-libstdc++ --flag=-shared
        --flag=-Wl,--no-undefined --flag=-lm --flag=-llog --flag=-landroid --flag=-lOpenSLES)
elseif(EMSCRIPTEN)
    set(STUB_SYSTEM_FLAGS --flag=-pthread --flag=-sWASMFS --flag=-sFULL_ES2 --flag=-sMAX_WEBGL_VERSION=2
        --flag=-fexceptions --flag=-sFETCH)
endif()
add_custom_command(
    OUTPUT ${STUBS}
    COMMAND ${CMAKE_COMMAND} -E env NM=${CMAKE_NM}
            ${Python3_EXECUTABLE} -I ${ROOT}/tools/port_stubs.py --out ${STUBS} --cxx ${CMAKE_CXX_COMPILER}
            --status ${CMAKE_CURRENT_BINARY_DIR}/gen/port-status.json
            ${STUB_FLAGS} ${STUB_SYSTEM_FLAGS}
            $<TARGET_OBJECTS:medecomp_game>
    DEPENDS $<TARGET_OBJECTS:medecomp_game> ${ROOT}/tools/port_stubs.py
    COMMAND_EXPAND_LISTS
    VERBATIM)

if(ANDROID)
    add_library(medecomp SHARED $<TARGET_OBJECTS:medecomp_game> ${STUBS})
    set_target_properties(medecomp PROPERTIES OUTPUT_NAME main)
else()
    add_executable(medecomp $<TARGET_OBJECTS:medecomp_game> ${STUBS})
endif()
if(EMSCRIPTEN)
    set(_web_link_flags)
    foreach(flag IN LISTS MEDECOMP_LINK_FLAGS)
        if(flag MATCHES "^-L(.+)$")
            target_link_directories(medecomp PRIVATE "${CMAKE_MATCH_1}")
        else()
            list(APPEND _web_link_flags "${flag}")
        endif()
    endforeach()
    set(MEDECOMP_LINK_FLAGS ${_web_link_flags})
    set_target_properties(medecomp PROPERTIES SUFFIX ".html")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/web/manifest.webmanifest ${CMAKE_BINARY_DIR}/manifest.webmanifest COPYONLY)
    foreach(icon app-icon-256.png app-icon-512.png)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/branding/${icon} ${CMAKE_BINARY_DIR}/icons/${icon} COPYONLY)
    endforeach()
    target_link_options(medecomp PRIVATE -pthread -fexceptions -sPROXY_TO_PTHREAD -sWASMFS -sALLOW_MEMORY_GROWTH
        -sMAXIMUM_MEMORY=2GB -sSTACK_SIZE=8MB -sDEFAULT_PTHREAD_STACK_SIZE=2MB -sPTHREAD_POOL_SIZE=8
        -sFULL_ES2 -sMAX_WEBGL_VERSION=2 -sOFFSCREENCANVAS_SUPPORT -sGL_ENABLE_GET_PROC_ADDRESS -sINVOKE_RUN=0
        -O2 --profiling-funcs
        -sFETCH
        -sASYNCIFY "-sASYNCIFY_ONLY=@${CMAKE_CURRENT_SOURCE_DIR}/web/asyncify.json"
        -sEXPORTED_RUNTIME_METHODS=callMain,ENV,UTF8ToString,stringToUTF8,lengthBytesUTF8
        "SHELL:--shell-file \"${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html\""
        "SHELL:--pre-js \"${CMAKE_CURRENT_SOURCE_DIR}/web/pre.js\""
        "SHELL:--preload-file \"${MEDECOMP_WEB_SYSROOT}/share/fonts@/fonts\""
        "SHELL:--preload-file \"${ROOT}/config/string_overrides.tsv@/config/string_overrides.tsv\""
        "SHELL:--preload-file \"${ROOT}/config/strings_ru.tsv@/config/strings_ru.tsv\""
        "SHELL:--preload-file \"${ROOT}/config/achievements@/config/achievements\""
        "SHELL:--preload-file \"${CMAKE_BINARY_DIR}/branding@/branding\"")
    set_property(TARGET medecomp APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html
        ${CMAKE_CURRENT_SOURCE_DIR}/web/pre.js ${CMAKE_CURRENT_SOURCE_DIR}/web/asyncify.json)
endif()
if(WIN32)
    enable_language(RC)
    target_sources(medecomp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win/medecomp.rc)
endif()
target_compile_options(medecomp PRIVATE ${ARCH_FLAGS} -w)
target_link_options(medecomp PRIVATE ${ARCH_FLAGS})
if(ANDROID)
    target_link_libraries(medecomp PRIVATE m log android OpenSLES ${MEDECOMP_LINK_FLAGS})
    target_link_options(medecomp PRIVATE -Wl,--no-undefined)
else()
    target_link_libraries(medecomp PRIVATE m pthread ${MEDECOMP_LINK_FLAGS})
endif()
if(MEDECOMP_RPATH)
    set_target_properties(medecomp PROPERTIES BUILD_RPATH "${MEDECOMP_RPATH}")
endif()
