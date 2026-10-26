set(MEDECOMP_FFMPEG_VERSION 9.0.2)
set(MEDECOMP_FFMPEG_SHA256 8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e)
set(MEDECOMP_FFMPEG_LIBS avformat.63 avcodec.63 swresample.7 avutil.61)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ext_dir.cmake)
set(MEDECOMP_FFMPEG_PREFIX ${MEDECOMP_EXT_DIR}/ffmpeg)

include(ExternalProject)
cmake_host_system_information(RESULT _jobs QUERY NUMBER_OF_LOGICAL_CORES)
set(_timestamp)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    set(_timestamp DOWNLOAD_EXTRACT_TIMESTAMP ON)
endif()
medecomp_nospace(_cc ${CMAKE_C_COMPILER})
medecomp_nospace(_cxx ${CMAKE_CXX_COMPILER})
set(_target --cc=${_cc} --cxx=${_cxx} --enable-pic)
set(_linkage --enable-shared --disable-static)
if(WIN32)
    get_filename_component(_mingw_bin ${_cc} DIRECTORY)
    set(_target --enable-cross-compile --target-os=mingw32 --arch=x86_64
                --cross-prefix=${_mingw_bin}/x86_64-w64-mingw32- --cc=${_cc} --cxx=${_cxx})
elseif(ANDROID)
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(_arch aarch64)
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(_arch arm)
    else()
        set(_arch x86_64)
    endif()
    medecomp_nospace(_ar ${CMAKE_AR})
    medecomp_nospace(_ranlib ${CMAKE_RANLIB})
    medecomp_nospace(_nm ${CMAKE_NM})
    medecomp_nospace(_strip ${CMAKE_STRIP})
    set(_target --enable-cross-compile --target-os=android --arch=${_arch} --cc=${_cc}
                --cxx=${_cxx} --ar=${_ar} --ranlib=${_ranlib} --nm=${_nm}
                --strip=${_strip} --extra-cflags=--target=${CMAKE_C_COMPILER_TARGET}
                --extra-ldflags=--target=${CMAKE_C_COMPILER_TARGET} --enable-pic --disable-asm)
elseif(EMSCRIPTEN)
    medecomp_nospace(_ar ${CMAKE_AR})
    medecomp_nospace(_ranlib ${CMAKE_RANLIB})
    medecomp_nospace(_nm ${CMAKE_NM})
    set(_target --enable-cross-compile --target-os=none --arch=x86_32 --cpu=generic --cc=${_cc} --cxx=${_cxx}
                --ar=${_ar} --ranlib=${_ranlib} --nm=${_nm} --disable-asm --disable-inline-asm --disable-stripping
                --disable-pthreads --disable-runtime-cpudetect --extra-cflags=-pthread --extra-cxxflags=-pthread
                --extra-ldflags=-pthread)
    set(_linkage --enable-static --disable-shared)
endif()
set(_demuxers caf mov mp3 ogg flac wav aac asf ape wv matroska aiff)
set(_decoders aac aac_latm alac mp3float mp3 mp2float flac vorbis opus wmav1 wmav2 wmapro ape wavpack
              pcm_s16le pcm_s16be pcm_s24le pcm_s24be pcm_s32le pcm_f32le pcm_u8 adpcm_ima_qt adpcm_ima_wav)
set(_parsers aac mpegaudio flac vorbis opus)
set(_enable)
foreach(_d ${_demuxers})
    list(APPEND _enable --enable-demuxer=${_d})
endforeach()
foreach(_d ${_decoders})
    list(APPEND _enable --enable-decoder=${_d})
endforeach()
foreach(_p ${_parsers})
    list(APPEND _enable --enable-parser=${_p})
endforeach()
set(_byproducts)
foreach(_lib ${MEDECOMP_FFMPEG_LIBS})
    string(REGEX REPLACE "\\..*" "" _name ${_lib})
    if(WIN32)
        list(APPEND _byproducts ${MEDECOMP_FFMPEG_PREFIX}/lib/lib${_name}.dll.a)
    elseif(EMSCRIPTEN)
        list(APPEND _byproducts ${MEDECOMP_FFMPEG_PREFIX}/lib/lib${_name}.a)
    else()
        list(APPEND _byproducts ${MEDECOMP_FFMPEG_PREFIX}/lib/lib${_name}.so)
    endif()
endforeach()
ExternalProject_Add(medecomp_ffmpeg
    URL https://ffmpeg.org/releases/ffmpeg-${MEDECOMP_FFMPEG_VERSION}.tar.xz
    URL_HASH SHA256=${MEDECOMP_FFMPEG_SHA256}
    ${_timestamp}
    PREFIX ${MEDECOMP_EXT_DIR}/ffmpeg-build
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${MEDECOMP_FFMPEG_PREFIX} --libdir=${MEDECOMP_FFMPEG_PREFIX}/lib
        ${_target} --pkg-config=false
        ${_linkage} --disable-programs --disable-doc --disable-debug
        --disable-autodetect --disable-everything --disable-network --disable-x86asm
        --disable-avdevice --disable-avfilter --disable-swscale
        --enable-protocol=file --enable-swresample ${_enable}
    BUILD_COMMAND make -j${_jobs}
    INSTALL_COMMAND make install
        COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/COPYING.LGPLv2.1 ${MEDECOMP_FFMPEG_PREFIX}/LICENSE.txt
    BUILD_BYPRODUCTS ${_byproducts})
list(APPEND MEDECOMP_DEPENDS medecomp_ffmpeg)
if(NOT MEDECOMP_EXT_DIR STREQUAL CMAKE_BINARY_DIR)
    add_custom_target(medecomp_ffmpeg_copy ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEDECOMP_FFMPEG_PREFIX} ${CMAKE_BINARY_DIR}/ffmpeg VERBATIM)
    add_dependencies(medecomp_ffmpeg_copy medecomp_ffmpeg)
endif()
list(APPEND MEDECOMP_INCLUDE_DIRS ${MEDECOMP_FFMPEG_PREFIX}/include)
set(_link -L${MEDECOMP_FFMPEG_PREFIX}/lib)
foreach(_lib ${MEDECOMP_FFMPEG_LIBS})
    string(REGEX REPLACE "\\..*" "" _name ${_lib})
    string(REGEX REPLACE "\\." ".so." _file ${_lib})
    if(WIN32 OR ANDROID OR EMSCRIPTEN)
        list(APPEND _link -l${_name})
    else()
        list(APPEND _link -l:lib${_file})
    endif()
endforeach()
list(APPEND MEDECOMP_LINK_FLAGS ${_link})
if(ANDROID)
    set(_copy)
    foreach(_lib ${MEDECOMP_FFMPEG_LIBS})
        string(REGEX REPLACE "\\..*" "" _name ${_lib})
        list(APPEND _copy COMMAND ${CMAKE_COMMAND} -E copy_if_different
             ${MEDECOMP_FFMPEG_PREFIX}/lib/lib${_name}.so ${CMAKE_BINARY_DIR}/lib/lib${_name}.so)
    endforeach()
    add_custom_target(medecomp_ffmpeg_libs ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib ${_copy}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MEDECOMP_FFMPEG_PREFIX}/LICENSE.txt
                ${CMAKE_BINARY_DIR}/lib/FFmpeg-LICENSE.txt
        VERBATIM)
    add_dependencies(medecomp_ffmpeg_libs medecomp_ffmpeg)
elseif(NOT WIN32 AND NOT EMSCRIPTEN)
    set(_copy)
    foreach(_lib ${MEDECOMP_FFMPEG_LIBS})
        string(REGEX REPLACE "\\." ".so." _file ${_lib})
        list(APPEND _copy COMMAND ${CMAKE_COMMAND} -E copy_if_different
             ${MEDECOMP_FFMPEG_PREFIX}/lib/lib${_file} ${CMAKE_BINARY_DIR}/lib/lib${_file})
    endforeach()
    add_custom_target(medecomp_ffmpeg_libs ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib ${_copy}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MEDECOMP_FFMPEG_PREFIX}/LICENSE.txt
                ${CMAKE_BINARY_DIR}/lib/FFmpeg-LICENSE.txt
        VERBATIM)
    add_dependencies(medecomp_ffmpeg_libs medecomp_ffmpeg)
    set(MEDECOMP_RPATH "\$ORIGIN/lib")
endif()
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/decode.cpp ${CMAKE_CURRENT_LIST_DIR}/music.cpp
                            PROPERTIES COMPILE_DEFINITIONS MEDECOMP_FFMPEG=1)
