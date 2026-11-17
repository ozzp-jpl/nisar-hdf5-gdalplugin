# Standalone (out-of-tree) build of the NISAR driver as a GDAL plugin.
# Used by conda-build/nisar-gdal-recipe. The in-tree GDAL build uses
# CMakeLists.gdal.txt (copied to frmts/nisar/CMakeLists.txt in a GDAL checkout).
cmake_minimum_required(VERSION 3.16)

# The driver version lives in nisar_version.h (shared with the in-tree build).
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/nisar_version.h" NISAR_VERSION_LINE
     REGEX "^#define NISAR_DRIVER_VERSION \"[0-9.]+\"")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" NISAR_VERSION "${NISAR_VERSION_LINE}")
if(NOT NISAR_VERSION)
    message(FATAL_ERROR "Cannot read NISAR_DRIVER_VERSION from nisar_version.h")
endif()
project(gdal-driver-nisar VERSION ${NISAR_VERSION} LANGUAGES CXX)

# GDAL (3.9+) requires C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(GDAL REQUIRED)
find_package(HDF5 REQUIRED)

add_library(gdal_NISAR MODULE
    nisar.cpp
    nisardataset.cpp
    nisarrasterband.cpp
    nisarinterpolated.cpp
    nisarinterpolatedrasterband.cpp
    hdf5vfl.cpp
)

# GDAL expects "gdal_NISAR.so" / "gdal_NISAR.dylib", not "libgdal_NISAR.*".
set_target_properties(gdal_NISAR PROPERTIES PREFIX "")
if(APPLE)
    set_target_properties(gdal_NISAR PROPERTIES SUFFIX ".dylib")
endif()

# Keep the @rpath pointing at $PREFIX/lib where libgdal and libhdf5 live.
set_target_properties(gdal_NISAR PROPERTIES Build_With_Install_RPATH TRUE)

target_compile_definitions(gdal_NISAR PRIVATE
    GDAL_COMPILATION
    NISAR_STANDALONE_PLUGIN)

# Global lock around libhdf5 calls. Same switch as GDAL's frmts/hdf5: ON/OFF
# forces it; when unset, the driver asks libhdf5 at run time
# (H5is_library_threadsafe(), see nisardataset.h).
if(DEFINED GDAL_ENABLE_HDF5_GLOBAL_LOCK)
    if(GDAL_ENABLE_HDF5_GLOBAL_LOCK)
        target_compile_definitions(gdal_NISAR PRIVATE ENABLE_HDF5_GLOBAL_LOCK)
    else()
        target_compile_definitions(gdal_NISAR PRIVATE DISABLE_HDF5_GLOBAL_LOCK)
    endif()
endif()

# zlib-ng (SIMD DEFLATE) when available, standard zlib otherwise.
find_path(ZLIB_NG_INCLUDE_DIR NAMES zlib-ng.h)
find_library(ZLIB_NG_LIBRARY NAMES z-ng zlib-ng)

if(ZLIB_NG_INCLUDE_DIR AND ZLIB_NG_LIBRARY)
    message(STATUS "Found zlib-ng at ${ZLIB_NG_LIBRARY}. Enabling SIMD acceleration.")
    target_include_directories(gdal_NISAR PRIVATE ${ZLIB_NG_INCLUDE_DIR})
    target_link_libraries(gdal_NISAR PUBLIC GDAL::GDAL HDF5::HDF5 ${ZLIB_NG_LIBRARY})
    target_compile_definitions(gdal_NISAR PRIVATE USE_ZLIB_NG)
else()
    message(STATUS "zlib-ng not found, falling back to standard zlib.")
    target_link_libraries(gdal_NISAR PUBLIC GDAL::GDAL HDF5::HDF5 ZLIB::ZLIB)
endif()

install(TARGETS gdal_NISAR
        LIBRARY DESTINATION lib/gdalplugins)
