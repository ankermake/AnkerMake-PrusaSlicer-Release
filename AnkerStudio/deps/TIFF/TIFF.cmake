find_package(OpenGL QUIET REQUIRED)

prusaslicer_add_cmake_project(TIFF
    URL https://gitlab.com/libtiff/libtiff/-/archive/v4.1.0/libtiff-v4.1.0.zip
    URL_HASH SHA256=17a3e875acece9be40b093361cfef47385d4ef22c995ffbf36b2871f5785f9b8
    DEPENDS ${ZLIB_PKG} ${PNG_PKG} dep_JPEG
    CMAKE_ARGS
        -Dlzma:BOOL=OFF
        -Dwebp:BOOL=OFF
        -Djbig:BOOL=OFF
        -Dzstd:BOOL=OFF
        -Dpixarlog:BOOL=OFF
)
