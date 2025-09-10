cmake_minimum_required(VERSION 3.13)
project(eufyStudio-native)

add_subdirectory(build-utils)
add_subdirectory(admesh)
add_subdirectory(avrdude)
# boost/nowide
add_subdirectory(boost)
add_subdirectory(clipper)
add_subdirectory(miniz)
add_subdirectory(glu-libtess)
add_subdirectory(semver)
add_subdirectory(libigl)
add_subdirectory(hints)
add_subdirectory(qoi)
add_subdirectory(libnest2d)

# If you want to force FetchContent AnkerNet every time to ensure the latest AnkerNet code is used. 
# Please turn on the following directive, please note that this will increase the compile time each time;
set(FORCE_FETCH_ANKERNET OFF)

if(OPEN_SOURCE)
    add_definitions(-DOPEN_SOURCE_MODE)
endif()


if(OPEN_SOURCE)
    message("-------OPEN_SOURCE_MODE")
    include_directories( "slic3r/GUI/AnkerNetModule/Interface Files")
endif()

set (ankerSoftVersion "${CMAKE_CURRENT_BINARY_DIR}/libslic3r")
message("-------ankerSoftVersion=${ankerSoftVersion}")
include_directories(${ankerSoftVersion})

find_package(Qhull 7.2 REQUIRED)
add_library(qhull INTERFACE)
if(SLIC3R_STATIC)
    slic3r_remap_configs("Qhull::qhullcpp;Qhull::qhullstatic_r" RelWithDebInfo Release)
    target_link_libraries(qhull INTERFACE Qhull::qhullcpp Qhull::qhullstatic_r)
else()
    slic3r_remap_configs("Qhull::qhullcpp;Qhull::qhull_r" RelWithDebInfo Release)
    target_link_libraries(qhull INTERFACE Qhull::qhullcpp Qhull::qhull_r)
endif()

add_subdirectory(libslic3r)

if (SLIC3R_ENABLE_FORMAT_STEP)
    add_subdirectory(occt_wrapper)
endif ()

if(NOT OPEN_SOURCE)
include_directories($ENV{THIRD_PART_ROOT}/usr/local/include/sentry)
endif ()

if (SLIC3R_GUI)
    add_subdirectory(imgui)
    add_subdirectory(hidapi)
    include_directories(hidapi/include)

    if(WIN32)
        message(STATUS "WXWIN environment set to: $ENV{WXWIN}")
    elseif(UNIX)
        set(wxWidgets_USE_UNICODE ON)
        if(SLIC3R_STATIC)
            set(wxWidgets_USE_STATIC ON)
        else()
            set(wxWidgets_USE_STATIC OFF)
        endif()
    endif()

    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set (wxWidgets_CONFIG_OPTIONS "--toolkit=gtk${SLIC3R_GTK}")
        if (SLIC3R_WX_STABLE)
            find_package(wxWidgets 3.0 REQUIRED COMPONENTS base core adv html gl webview richtext net)
        else ()
            find_package(wxWidgets 3.1 QUIET COMPONENTS base core adv html gl webview richtext net)

            if (NOT wxWidgets_FOUND)
                message(FATAL_ERROR "\nCould not find wxWidgets 3.1.\n"
                    "Hint: On Linux you can set -DSLIC3R_WX_STABLE=1 to use wxWidgets 3.0\n")
            endif ()
        endif ()

        include(${wxWidgets_USE_FILE})
    else ()
        find_package(wxWidgets 3.2 CONFIG REQUIRED COMPONENTS html adv gl core base webview richtext net)
        slic3r_remap_configs(wx::wxhtml wx::wxadv wx::wxgl wx::wxcore wx::wxbase RelWithDebInfo Release)
        # find_package(wxWidgets 3.1 COMPONENTS html adv gl core base webview)
        # if (NOT wxWidgets_FOUND)
        #     message(STATUS "Trying to find wxWidgets in CONFIG mode...")
        #     find_package(wxWidgets 3.2 CONFIG REQUIRED COMPONENTS html adv gl core base webview)
        #     slic3r_remap_configs(wx::wxhtml wx::wxadv wx::wxgl wx::wxcore wx::wxbase RelWithDebInfo Release)
        # else ()
        #     include(${wxWidgets_USE_FILE})
        # endif ()
    endif ()

    if(UNIX)
        message(STATUS "wx-config path: ${wxWidgets_CONFIG_EXECUTABLE}")
    endif()

    find_package(JPEG QUIET)
    find_package(TIFF QUIET)
    find_package(NanoSVG REQUIRED)

    string(REGEX MATCH "wxpng" WX_PNG_BUILTIN ${wxWidgets_LIBRARIES})
    if (PNG_FOUND AND NOT WX_PNG_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX png)
        list(APPEND wxWidgets_LIBRARIES ${PNG_LIBRARIES})
    endif ()

    string(REGEX MATCH "wxtiff" WX_TIFF_BUILTIN ${wxWidgets_LIBRARIES})
    if (TIFF_FOUND AND NOT WX_TIFF_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX tiff)
        list(APPEND wxWidgets_LIBRARIES ${TIFF_LIBRARIES})
    endif ()

    string(REGEX MATCH "wxjpeg" WX_JPEG_BUILTIN ${wxWidgets_LIBRARIES})
    if (JPEG_FOUND AND NOT WX_JPEG_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX jpeg)
        list(APPEND wxWidgets_LIBRARIES ${JPEG_LIBRARIES})
    endif ()

    string(REGEX MATCH "wxexpat" WX_EXPAT_BUILTIN ${wxWidgets_LIBRARIES})
    if (EXPAT_FOUND AND NOT WX_EXPAT_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX expat)
        list(APPEND wxWidgets_LIBRARIES libexpat)
    endif ()
    
    # This is an issue in the new wxWidgets cmake build, doesn't deal with librt
    find_library(LIBRT rt)
    if(LIBRT)
        list(APPEND wxWidgets_LIBRARIES ${LIBRT})
    endif()

    # This fixes a OpenGL linking issue on OSX. wxWidgets cmake build includes
    # wrong libs for opengl in the link line and it does not link to it by himself.
    # libslic3r_gui will link to opengl anyway, so lets override wx
    list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX OpenGL)
    
    if (UNIX AND NOT APPLE)
        list(APPEND wxWidgets_LIBRARIES X11 wayland-client wayland-egl EGL)
    endif ()
#    list(REMOVE_ITEM wxWidgets_LIBRARIES oleacc)
    message(STATUS "wx libs: ${wxWidgets_LIBRARIES}")

    add_subdirectory(slic3r)
endif()


# Create a slic3r executable
# Process mainfests for various platforms.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/msw/AnkerStudio.rc.in ${CMAKE_CURRENT_BINARY_DIR}/AnkerStudio.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/msw/AnkerStudio-gcodeviewer.rc.in ${CMAKE_CURRENT_BINARY_DIR}/AnkerStudio-gcodeviewer.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/msw/AnkerStudio.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/AnkerStudio.manifest @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/osx/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)
if (WIN32)
    add_library(eufyStudio SHARED AnkerStudio.cpp AnkerStudio.hpp AnkerComFunction.hpp AnkerComFunction.cpp exception_handle/AnkerStackWalker.hpp)
else ()
    add_executable(eufyStudio AnkerStudio.cpp AnkerStudio.hpp AnkerComFunction.hpp AnkerComFunction.cpp)
endif ()

if(FORCE_FETCH_ANKERNET)
add_custom_command(
    TARGET eufyStudio
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Force get the latest code for AnkerNet"
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/CMakeLists.txt
)
endif()

if (MSVC)
    message("--------------------anker net copy file , path = ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
    add_custom_command(TARGET eufyStudio POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/libcrypto-1_1-x64.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/libssl-1_1-x64.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/libcrypto-3-x64.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/paho-mqtt3cs.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/PPCS_API.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/GD_CA.crt ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/make-us.crt ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/make-us-qa.crt ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/avcodec-59.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/avformat-59.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/avutil-57.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/swresample-4.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_PREFIX_PATH}/bin/swscale-6.dll ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        VERBATIM
    )
endif()

if (MINGW)
    target_link_options(eufyStudio PUBLIC "-Wl,-allow-multiple-definition")
    set_target_properties(eufyStudio PROPERTIES PREFIX "")
endif (MINGW)

if (NOT WIN32 AND NOT APPLE)
    # Binary name on unix like systems (Linux, Unix)
    set_target_properties(eufyStudio PROPERTIES OUTPUT_NAME "eufymake studio")
endif ()

target_link_libraries(eufyStudio libslic3r libcereal)


if(NOT OPEN_SOURCE)
    add_dependencies(eufyStudio AnkerNet)
endif()

if (APPLE)
#    add_compile_options(-stdlib=libc++)
#    add_definitions(-DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE)
    # -liconv: boost links to libiconv by default
    target_link_libraries(eufyStudio "-liconv -framework IOKit" "-framework CoreFoundation" -lc++)
elseif (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
else ()
    # Boost on Raspberry-Pi does not link to pthreads explicitely.
    target_link_libraries(eufyStudio ${CMAKE_DL_LIBS} -lstdc++ Threads::Threads)
endif ()


# Add the Slic3r GUI library, libcurl, OpenGL and GLU libraries.
if (SLIC3R_GUI)
    target_link_libraries(eufyStudio libslic3r_gui)
    if (MSVC)
        # Generate debug symbols even in release mode.
        target_link_options(eufyStudio PUBLIC "$<$<CONFIG:RELEASE>:/DEBUG>")
        target_link_libraries(eufyStudio user32.lib Setupapi.lib)
    elseif (MINGW)
        target_link_libraries(eufyStudio ws2_32 uxtheme setupapi)
    elseif (APPLE)
        target_link_libraries(eufyStudio "-framework OpenGL")
    else ()
        target_link_libraries(eufyStudio -ldl)
    endif ()
    if (WIN32)
        find_library(PSAPI_LIB NAMES Psapi)
		if(PSAPI_LIB_FOUND)
            target_link_libraries(eufyStudio ${PSAPI_LIB})
		endif()
    endif ()
endif ()

# On Windows, a shim application is required to produce a console / non console version of the Slic3r application.
# Also the shim may load the Mesa software OpenGL renderer if the default renderer does not support OpenGL 2.0 and higher.
if (WIN32)
    if (MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    endif()

    add_executable(eufyStudio_app_gui WIN32 AnkerStudio_app_msvc.cpp AnkerComFunction.hpp AnkerComFunction.cpp ${CMAKE_CURRENT_BINARY_DIR}/AnkerStudio.rc)
    # Generate debug symbols even in release mode.
    if(MSVC)
        target_link_options(eufyStudio_app_gui PUBLIC "$<$<CONFIG:RELEASE>:/DEBUG>")
    endif()
    target_compile_definitions(eufyStudio_app_gui PRIVATE -DSLIC3R_WRAPPER_NOCONSOLE)
    add_dependencies(eufyStudio_app_gui eufyStudio)
    set_target_properties(eufyStudio_app_gui PROPERTIES OUTPUT_NAME "eufymake studio")
    target_link_libraries(eufyStudio_app_gui PRIVATE boost_headeronly)
	
if(NOT OPEN_SOURCE)
	target_link_libraries(eufyStudio_app_gui PUBLIC $ENV{THIRD_PART_ROOT}/usr/local/lib/sentry/win/sentry.lib)
endif ()
	
    add_executable(eufyStudio_app_console AnkerStudio_app_msvc.cpp AnkerComFunction.hpp AnkerComFunction.cpp ${CMAKE_CURRENT_BINARY_DIR}/AnkerStudio.rc)
    # Generate debug symbols even in release mode.
    if (MSVC)
        target_link_options(eufyStudio_app_console PUBLIC "$<$<CONFIG:RELEASE>:/DEBUG>")
    endif ()
    target_compile_definitions(eufyStudio_app_console PRIVATE -DSLIC3R_WRAPPER_CONSOLE)
    add_dependencies(eufyStudio_app_console eufyStudio)
    set_target_properties(eufyStudio_app_console PROPERTIES OUTPUT_NAME "eufymake studio-console")
if(NOT OPEN_SOURCE)	
	target_link_libraries(eufyStudio_app_console PUBLIC $ENV{THIRD_PART_ROOT}/usr/local/lib/sentry/win/sentry.lib)
endif ()	
    target_link_libraries(eufyStudio_app_console PRIVATE boost_headeronly)

    add_executable(eufyStudio_app_gcodeviewer WIN32 AnkerStudio_app_msvc.cpp AnkerComFunction.hpp AnkerComFunction.cpp ${CMAKE_CURRENT_BINARY_DIR}/AnkerStudio-gcodeviewer.rc)
    # Generate debug symbols even in release mode.
    if (MSVC)
        target_link_options(eufyStudio_app_gcodeviewer PUBLIC "$<$<CONFIG:RELEASE>:/DEBUG>")
    endif ()
    target_compile_definitions(eufyStudio_app_gcodeviewer PRIVATE -DSLIC3R_WRAPPER_NOCONSOLE -DSLIC3R_WRAPPER_GCODEVIEWER)
    add_dependencies(eufyStudio_app_gcodeviewer eufyStudio)
    set_target_properties(eufyStudio_app_gcodeviewer PROPERTIES OUTPUT_NAME "anker-gcodeviewer")
    target_link_libraries(eufyStudio_app_gcodeviewer PRIVATE boost_headeronly)
if(NOT OPEN_SOURCE)		
	target_link_libraries(eufyStudio_app_gcodeviewer PUBLIC $ENV{THIRD_PART_ROOT}/usr/local/lib/sentry/win/sentry.lib)
endif ()	
endif ()

# Link the resources dir to where Slic3r GUI expects it
if (WIN32)
    if (CMAKE_CONFIGURATION_TYPES)
        foreach (CONF ${CMAKE_CONFIGURATION_TYPES})
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CONF}" WIN_CONF_OUTPUT_DIR)
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CONF}/resources" WIN_RESOURCES_SYMLINK)
            add_custom_command(TARGET eufyStudio POST_BUILD
                COMMAND if exist "${WIN_CONF_OUTPUT_DIR}" "("
                        if not exist "${WIN_RESOURCES_SYMLINK}" "("
                            mklink /J "${WIN_RESOURCES_SYMLINK}" "${SLIC3R_RESOURCES_DIR_WIN}"
                        ")"
                    ")"
                COMMENT "Symlinking the resources directory into the build tree"
                VERBATIM
            )
        endforeach ()
    else ()
        file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/resources" WIN_RESOURCES_SYMLINK)
        add_custom_command(TARGET eufyStudio POST_BUILD
            COMMAND if not exist "${WIN_RESOURCES_SYMLINK}" "(" mklink /J "${WIN_RESOURCES_SYMLINK}" "${SLIC3R_RESOURCES_DIR_WIN}" ")"
            COMMENT "Symlinking the resources directory into the build tree"
            VERBATIM
        )
    endif ()

    # This has to be a separate target due to the windows command line lenght limits
    add_custom_target(AnkerStudioDllsCopy)
    AnkerStudio_copy_dlls(AnkerStudioDllsCopy)
    add_dependencies(eufyStudio AnkerStudioDllsCopy)

else ()
    if (APPLE)
        # On OSX, the name of the binary matches the name of the Application.
        add_custom_command(TARGET eufyStudio POST_BUILD
            COMMAND ln -sf eufyStudio "eufymake studio"
            COMMAND ln -sf eufyStudio anker-gcodeviewer
            COMMAND ln -sf eufyStudio AnkerGCodeViewer
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:eufyStudio>"
            COMMENT "Symlinking the G-code viewer to eufyStudio, symlinking to eufymake studio and anker-gcodeviewer"
            VERBATIM)
    else ()
        add_custom_command(TARGET eufyStudio POST_BUILD
            COMMAND ln -sf "eufymake studio" anker-gcodeviewer
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:eufyStudio>"
            COMMENT "Symlinking the G-code viewer to eufyStudio"
            VERBATIM)
    endif ()
    if (XCODE)
        # Because of Debug/Release/etc. configurations (similar to MSVC) the slic3r binary is located in an extra level
        set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources")
    else ()
        set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/../resources")
    endif ()
    add_custom_command(TARGET eufyStudio POST_BUILD
        COMMAND ln -sfn "${SLIC3R_RESOURCES_DIR}" "${BIN_RESOURCES_DIR}"
        COMMENT "Symlinking the resources directory into the build tree"
        VERBATIM)
endif ()

# Slic3r binary install target
if (WIN32)
    install(TARGETS eufyStudio RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
    if (MSVC)
        install(TARGETS eufyStudio_app_gui RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
        install(TARGETS eufyStudio_app_console RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
        install(TARGETS eufyStudio_app_gcodeviewer RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif ()
else ()
    install(TARGETS eufyStudio RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

    # Install the symlink for gcodeviewer
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink eufymake studio anker-gcodeviewer WORKING_DIRECTORY \$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})")
endif ()


