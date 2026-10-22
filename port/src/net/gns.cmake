include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ext_dir.cmake)
option(MEDECOMP_GNS_BUNDLE "GameNetworkingSockets из исходников рядом с игрой (G8)" ON)
set(MEDECOMP_GNS_PREFIX "${CMAKE_BINARY_DIR}/gns" CACHE PATH "Куда ставить собранные OpenSSL, protobuf и GameNetworkingSockets")

set(MEDECOMP_GNS_VERSION 1.6.1)
set(MEDECOMP_GNS_SHA256 4091ddcb5285134d5d0e97385ae4792b828e5b103c8ae2c79652dc7d177366d9)
set(MEDECOMP_PROTOBUF_VERSION 21.12)
set(MEDECOMP_PROTOBUF_SHA256 4eab9b524aa5913c6fffb20b2a8abf5ef7f95a80bc0701f3a6dbb4c607f73460)
set(MEDECOMP_OPENSSL_VERSION 3.5.9)
set(MEDECOMP_OPENSSL_SHA256 603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a)
file(SHA256 ${CMAKE_CURRENT_LIST_FILE} MEDECOMP_GNS_RECIPE)
string(SUBSTRING ${MEDECOMP_GNS_RECIPE} 0 12 MEDECOMP_GNS_RECIPE)

set(MEDECOMP_HOST_CXX_FLAGS "" CACHE STRING "Флаги компилятора машины сборки для protoc (сборка под Windows и Android)")

function(medecomp_gns_windows prefix done timestamp jobs)
    set(host ${MEDECOMP_EXT_DIR}/gns-host)
    set(caseDir ${MEDECOMP_EXT_DIR}/gns-build/win-case)
    file(WRITE ${caseDir}/Windows.h "#include <windows.h>\n")
    file(CREATE_LINK ${MEDECOMP_MINGW}/x86_64-w64-mingw32/lib/libiphlpapi.a ${caseDir}/libIphlpapi.a SYMBOLIC)
    set(win -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DMEDECOMP_MINGW=${MEDECOMP_MINGW}
            -DMEDECOMP_WIN_SYSROOT=${MEDECOMP_WIN_SYSROOT} -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_INSTALL_PREFIX=${prefix} -DCMAKE_INSTALL_LIBDIR=lib)
    if(EXISTS ${done})
        add_custom_target(medecomp_gns_build)
    else()
        ExternalProject_Add(medecomp_protoc_host
            URL https://github.com/protocolbuffers/protobuf/releases/download/v${MEDECOMP_PROTOBUF_VERSION}/protobuf-cpp-3.${MEDECOMP_PROTOBUF_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_PROTOBUF_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/protoc-host
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cc -DCMAKE_CXX_COMPILER=c++
                "-DCMAKE_CXX_FLAGS=${MEDECOMP_HOST_CXX_FLAGS}"
                -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_WITH_ZLIB=OFF
            BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target protoc -j${jobs}
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <BINARY_DIR>/protoc ${host}/bin/protoc)
        ExternalProject_Add(medecomp_protobuf
            URL https://github.com/protocolbuffers/protobuf/releases/download/v${MEDECOMP_PROTOBUF_VERSION}/protobuf-cpp-3.${MEDECOMP_PROTOBUF_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_PROTOBUF_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/protobuf
            CMAKE_ARGS ${win} -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_WITH_ZLIB=OFF
                -Dprotobuf_BUILD_PROTOC_BINARIES=OFF -Dprotobuf_BUILD_LIBPROTOC=OFF
            INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${prefix}/licenses/protobuf.txt)
        ExternalProject_Add(medecomp_gns_build
            DEPENDS medecomp_protoc_host medecomp_protobuf
            URL https://github.com/ValveSoftware/GameNetworkingSockets/archive/refs/tags/v${MEDECOMP_GNS_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_GNS_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/gns
            CMAKE_ARGS ${win}
                -DMEDECOMP_EXTRA_FIND_ROOT=${prefix}
                -DProtobuf_DIR=${prefix}/lib/cmake/protobuf
                -Dprotobuf_DIR=${prefix}/lib/cmake/protobuf
                -DProtobuf_PROTOC_EXECUTABLE=${host}/bin/protoc
                -DBUILD_SHARED_LIB=ON
                -DBUILD_STATIC_LIB=OFF
                -DProtobuf_USE_STATIC_LIBS=ON
                -DUSE_CRYPTO=BCrypt
                "-DCMAKE_CXX_FLAGS=-I${caseDir}"
                "-DCMAKE_SHARED_LINKER_FLAGS=-s -L${caseDir}"
            INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${prefix}/licenses/GameNetworkingSockets.txt
                COMMAND ${CMAKE_COMMAND} -E touch ${done})
    endif()
    set(out ${CMAKE_BINARY_DIR}/lib)
    add_custom_target(medecomp_gns ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${out}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/bin/libGameNetworkingSockets.dll ${out}/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/GameNetworkingSockets.txt
                ${out}/GameNetworkingSockets-LICENSE.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/protobuf.txt ${out}/protobuf-LICENSE.txt
        VERBATIM)
    add_dependencies(medecomp_gns medecomp_gns_build)
endfunction()

function(medecomp_gns_android prefix done timestamp jobs)
    set(host ${MEDECOMP_EXT_DIR}/gns-host)
    set(ndk -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DANDROID_ABI=${ANDROID_ABI}
            -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_INSTALL_PREFIX=${prefix} -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_FIND_ROOT_PATH=${prefix})
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(target android-arm64)
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(target android-arm)
    else()
        set(target android-x86_64)
    endif()
    medecomp_nospace(ndkRoot ${ANDROID_NDK})
    medecomp_nospace(toolchainRoot ${ANDROID_TOOLCHAIN_ROOT})
    set(sslenv ${CMAKE_COMMAND} -E env ANDROID_NDK_ROOT=${ndkRoot} "PATH=${toolchainRoot}/bin:$ENV{PATH}")
    if(EXISTS ${done})
        add_custom_target(medecomp_gns_build)
    else()
        ExternalProject_Add(medecomp_openssl
            URL https://github.com/openssl/openssl/releases/download/openssl-${MEDECOMP_OPENSSL_VERSION}/openssl-${MEDECOMP_OPENSSL_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_OPENSSL_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/openssl
            BUILD_IN_SOURCE ON
            CONFIGURE_COMMAND ${sslenv} perl <SOURCE_DIR>/Configure ${target} -D__ANDROID_API__=${ANDROID_PLATFORM_LEVEL}
                no-shared no-tests no-apps no-docs no-module no-legacy -fPIC
                --prefix=${prefix} --libdir=lib --openssldir=/nonexistent/medecomp-openssl
            BUILD_COMMAND ${sslenv} make -j${jobs} build_libs
            INSTALL_COMMAND make install_dev
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE.txt ${prefix}/licenses/OpenSSL.txt)
        ExternalProject_Add(medecomp_protoc_host
            URL https://github.com/protocolbuffers/protobuf/releases/download/v${MEDECOMP_PROTOBUF_VERSION}/protobuf-cpp-3.${MEDECOMP_PROTOBUF_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_PROTOBUF_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/protoc-host
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cc -DCMAKE_CXX_COMPILER=c++
                "-DCMAKE_CXX_FLAGS=${MEDECOMP_HOST_CXX_FLAGS}"
                -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_WITH_ZLIB=OFF
            BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target protoc -j${jobs}
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <BINARY_DIR>/protoc ${host}/bin/protoc)
        ExternalProject_Add(medecomp_protobuf
            URL https://github.com/protocolbuffers/protobuf/releases/download/v${MEDECOMP_PROTOBUF_VERSION}/protobuf-cpp-3.${MEDECOMP_PROTOBUF_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_PROTOBUF_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/protobuf
            CMAKE_ARGS ${ndk} -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_WITH_ZLIB=OFF
                -Dprotobuf_BUILD_PROTOC_BINARIES=OFF -Dprotobuf_BUILD_LIBPROTOC=OFF
            INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${prefix}/licenses/protobuf.txt)
        ExternalProject_Add(medecomp_gns_build
            DEPENDS medecomp_openssl medecomp_protoc_host medecomp_protobuf
            URL https://github.com/ValveSoftware/GameNetworkingSockets/archive/refs/tags/v${MEDECOMP_GNS_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_GNS_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/gns
            PATCH_COMMAND perl -pi -e "s/#if !IsAndroid\\(\\)[ \\t]*$/#if !IsAndroid() || __ANDROID_API__ >= 24/"
                    <SOURCE_DIR>/src/public/tier0/platform_sockets.h
                COMMAND perl -pi -e "s/ && !IsAndroid\\(\\) && !IsNintendoSwitch\\(\\)/ && ( !IsAndroid() || __ANDROID_API__ >= 24 ) && !IsNintendoSwitch()/"
                    <SOURCE_DIR>/src/steamnetworkingsockets/clientlib/steamnetworkingsockets_socketthread.cpp
            CMAKE_ARGS ${ndk}
                -DProtobuf_DIR=${prefix}/lib/cmake/protobuf
                -Dprotobuf_DIR=${prefix}/lib/cmake/protobuf
                -DProtobuf_PROTOC_EXECUTABLE=${host}/bin/protoc
                -DBUILD_SHARED_LIB=ON
                -DBUILD_STATIC_LIB=OFF
                -DProtobuf_USE_STATIC_LIBS=ON
                -DUSE_CRYPTO=OpenSSL
                -DOPENSSL_ROOT_DIR=${prefix}
                -DOPENSSL_USE_STATIC_LIBS=TRUE
                "-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--exclude-libs,ALL -s"
            INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${prefix}/licenses/GameNetworkingSockets.txt
                COMMAND ${CMAKE_COMMAND} -E touch ${done})
    endif()
    set(out ${CMAKE_BINARY_DIR}/lib)
    add_custom_target(medecomp_gns ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${out}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/lib/libGameNetworkingSockets.so ${out}/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/GameNetworkingSockets.txt
                ${out}/GameNetworkingSockets-LICENSE.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/protobuf.txt ${out}/protobuf-LICENSE.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/OpenSSL.txt ${out}/OpenSSL-LICENSE.txt
        VERBATIM)
    add_dependencies(medecomp_gns medecomp_gns_build)
endfunction()

function(medecomp_gns_bundle)
    if(MEDECOMP_M32 OR NOT MEDECOMP_GNS_BUNDLE OR NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32 OR ANDROID))
        return()
    endif()
    include(ExternalProject)
    set(prefix ${MEDECOMP_GNS_PREFIX})
    if(prefix MATCHES "[ \t]")
        set(prefix ${MEDECOMP_EXT_DIR}/gns)
    endif()
    set(done ${prefix}/medecomp-gns-${MEDECOMP_GNS_RECIPE}.done)
    set(common
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=${prefix}
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
        "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
    set(timestamp)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        set(timestamp DOWNLOAD_EXTRACT_TIMESTAMP ON)
    endif()
    cmake_host_system_information(RESULT jobs QUERY NUMBER_OF_LOGICAL_CORES)
    if(WIN32)
        medecomp_gns_windows(${prefix} ${done} "${timestamp}" ${jobs})
        return()
    endif()
    if(ANDROID)
        medecomp_gns_android(${prefix} ${done} "${timestamp}" ${jobs})
        return()
    endif()
    medecomp_nospace(cc ${CMAKE_C_COMPILER})
    if(EXISTS ${done})
        add_custom_target(medecomp_gns_build)
    else()
        ExternalProject_Add(medecomp_openssl
            URL https://github.com/openssl/openssl/releases/download/openssl-${MEDECOMP_OPENSSL_VERSION}/openssl-${MEDECOMP_OPENSSL_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_OPENSSL_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/openssl
            BUILD_IN_SOURCE ON
            CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env CC=${cc}
                perl <SOURCE_DIR>/Configure no-shared no-tests no-apps no-docs no-module no-legacy -fPIC
                --prefix=${prefix} --libdir=lib --openssldir=/nonexistent/medecomp-openssl
            BUILD_COMMAND make -j${jobs} build_libs
            INSTALL_COMMAND make install_dev
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE.txt ${prefix}/licenses/OpenSSL.txt)
        ExternalProject_Add(medecomp_protobuf
            URL https://github.com/protocolbuffers/protobuf/releases/download/v${MEDECOMP_PROTOBUF_VERSION}/protobuf-cpp-3.${MEDECOMP_PROTOBUF_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_PROTOBUF_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/protobuf
            CMAKE_ARGS ${common} -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_WITH_ZLIB=OFF
            INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${prefix}/licenses/protobuf.txt)
        ExternalProject_Add(medecomp_gns_build
            DEPENDS medecomp_openssl medecomp_protobuf
            URL https://github.com/ValveSoftware/GameNetworkingSockets/archive/refs/tags/v${MEDECOMP_GNS_VERSION}.tar.gz
            URL_HASH SHA256=${MEDECOMP_GNS_SHA256}
            ${timestamp}
            PREFIX ${MEDECOMP_EXT_DIR}/gns-build/gns
            CMAKE_ARGS ${common}
                -DCMAKE_PREFIX_PATH=${prefix}
                -DBUILD_SHARED_LIB=ON
                -DBUILD_STATIC_LIB=OFF
                -DProtobuf_USE_STATIC_LIBS=ON
                -DUSE_CRYPTO=OpenSSL
                -DOPENSSL_ROOT_DIR=${prefix}
                -DOPENSSL_USE_STATIC_LIBS=TRUE
                "-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL -s"
            INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${prefix}/licenses/GameNetworkingSockets.txt
                COMMAND ${CMAKE_COMMAND} -E touch ${done})
    endif()
    set(out ${CMAKE_BINARY_DIR}/lib)
    add_custom_target(medecomp_gns ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${out}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/lib/libGameNetworkingSockets.so ${out}/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/GameNetworkingSockets.txt
                ${out}/GameNetworkingSockets-LICENSE.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/protobuf.txt ${out}/protobuf-LICENSE.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${prefix}/licenses/OpenSSL.txt ${out}/OpenSSL-LICENSE.txt
        VERBATIM)
    add_dependencies(medecomp_gns medecomp_gns_build)
endfunction()
medecomp_gns_bundle()
