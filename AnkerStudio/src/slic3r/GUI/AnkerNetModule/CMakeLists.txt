# Native, open-source replacement for the proprietary libAnkerNet plugin.
# Built as its own shared library so it satisfies AnkerNetModuleManager's
# boost::dll plugin-loading contract exactly like the real dylib would.

add_library(AnkerNetNative SHARED
    AnkerNetNative.hpp
    AnkerNetNative.cpp
    DeviceObjectNative.hpp
    AnkerMqtt.hpp
    AnkerMqtt.cpp
    AnkerPppp.hpp
    AnkerPppp.cpp
    AnkerH264Decoder.hpp
    AnkerH264Decoder.mm
)

# libcurl (HTTP device list), boost (JSON/filesystem), OpenSSL (TLS + AES for MQTT).
target_link_libraries(AnkerNetNative PRIVATE libcurl boost_libs libopenssl)

# macOS frameworks for hardware H.264 decode of the camera stream.
if (APPLE)
    target_link_libraries(AnkerNetNative PRIVATE
        "-framework VideoToolbox"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework CoreFoundation")
endif ()

set_target_properties(AnkerNetNative PROPERTIES
    OUTPUT_NAME "AnkerNet"
    PREFIX "lib"
    SUFFIX ".dylib"
    # Debug-mode AnkerNetModuleManager::GetDefaultDir() resolves to the directory
    # containing the running executable (boost::dll::program_location().parent_path()).
    # The eufyStudio target (defined in src/CMakeLists.txt) lands at
    # ${CMAKE_BINARY_DIR}/src/<Config>/ under the Xcode generator's default
    # per-source-directory output layout; match it explicitly here since this
    # target lives in a nested subdirectory and would otherwise default to its
    # own (wrong) nested output path.
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
)
