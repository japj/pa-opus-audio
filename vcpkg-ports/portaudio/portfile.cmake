vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://git.assembla.com/portaudio.git
    REF 799a6834a58592eadc5712cba73b35956dc51579
    PATCHES
        fix-library-can-not-be-found.patch
        fix-include.patch
)

# NOTE: building with PA_BUILD_SHARED=OFF for pa-opus-audio
# NOTE: ONLY_STATIC_CRT due to linking to Node (only needed on Windows)

if (WIN32)
    set(ONLY_STATIC_CRT "ONLY_STATIC_CRT")
else()
    set(ONLY_STATIC_CRT)
endif()

vcpkg_check_linkage(
    ONLY_STATIC_LIBRARY
    ${ONLY_STATIC_CRT}
)

# NOTE: the ASIO backend will be built automatically if the ASIO-SDK is provided
# in a sibling folder of the portaudio source in vcpkg/buildtrees/portaudio/src
vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
    	-DPA_USE_DS=ON
        -DPA_USE_WASAPI=ON
        -DPA_USE_WDMKS=ON
        -DPA_USE_WMME=ON
        -DPA_USE_ALSA=ON
        -DPA_LIBNAME_ADD_SUFFIX=OFF
        -DPA_BUILD_SHARED=OFF
    OPTIONS_DEBUG
        -DPA_ENABLE_DEBUG_OUTPUT:BOOL=ON
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/${PORT})
vcpkg_copy_pdbs()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)

if(VCPKG_LIBRARY_LINKAGE STREQUAL static)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin ${CURRENT_PACKAGES_DIR}/debug/bin)
endif()

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/portaudio RENAME copyright)