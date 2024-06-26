cmake_minimum_required(VERSION 3.28)
project(Qt-Mandelbrot CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

#=======================================================================================================================
# Qt config.
#=======================================================================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_VERSION 5)
set(REQUIRED_LIBS Core Gui Widgets)
set(REQUIRED_LIBS_QUALIFIED Qt5::Core Qt5::Gui Qt5::Widgets)

if (NOT CMAKE_PREFIX_PATH)
    message(WARNING "CMAKE_PREFIX_PATH is not defined, you may need to set it "
            "(-DCMAKE_PREFIX_PATH=\"path/to/Qt/lib/cmake\" or -DCMAKE_PREFIX_PATH=/usr/include/{host}/qt{version}/ on Ubuntu)")
endif ()

find_package(Qt${QT_VERSION} COMPONENTS ${REQUIRED_LIBS} REQUIRED)

#=======================================================================================================================
# Output Directories.
#=======================================================================================================================
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output-${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output-${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output-${CMAKE_BUILD_TYPE})



#=======================================================================================================================
# Executables.
#=======================================================================================================================
set(SRC main.cpp
        mainwindow.cpp
        mainwindow.ui
        mainwindow.h
        tile.h
        tile.cpp
        tile_update.h
        tilestorage.h
        tilestorage.cpp
        timer.h)

if (CMAKE_BUILD_TYPE MATCHES Release)
    if (WIN32)
        SET(GUI_TYPE WIN32)
    elseif (APPLE)
        SET(GUI_TYPE MACOSX_BUNDLE)
    endif ()
endif ()

add_executable(${PROJECT_NAME} ${GUI_TYPE} ${SRC} tile_update.cpp)

target_link_libraries(${PROJECT_NAME} ${REQUIRED_LIBS_QUALIFIED})


#CUDA
set(SRC-CUDA main.cpp
        mainwindow.cpp
        mainwindow.ui
        mainwindow.h
        tile.h
        tile.cpp
        tile_update.h
        tile_update.cu
        tilestorage.h
        tilestorage.cpp
        timer.h)

add_executable(${PROJECT_NAME}-Cuda ${GUI_TYPE} ${SRC-CUDA})
target_compile_definitions(${PROJECT_NAME}-Cuda PRIVATE CUDA)

target_link_libraries(${PROJECT_NAME}-Cuda ${REQUIRED_LIBS_QUALIFIED})

set_target_properties(${PROJECT_NAME}-Cuda PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)
