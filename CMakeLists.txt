cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/ext/local/share/opencv")

 
#find_package(Qt4 REQUIRED)
#include(${QT_USE_FILE})


find_package(Boost COMPONENTS date_time filesystem iostreams math_tr1 serialization signals system thread program_options)

include_directories(${Boost_INCLUDE_DIR})
include_directories("${CMAKE_SOURCE_DIR}")
#include_directories("include")
#include_directories("lib")

FIND_PACKAGE(OpenCV REQUIRED )


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -march=i686 -msse -msse2")




#add_library(track image_source.cpp)
#add_executable(lkdemo lkdemo.c)
add_executable(track_bgsub track_bgsub.cpp image_source.cpp)
#add_executable(track_pf track_pf.cpp)

#target_link_libraries(track ${OpenCV_LIBS} ${Boost_FILESYSTEM_LIBRARIES} ${Boost_SYSTEM_LIBRARIES})
target_link_libraries(track_bgsub ${OpenCV_LIBS} ${Boost_FILESYSTEM_LIBRARIES} ${Boost_SYSTEM_LIBRARIES} ${Boost_PROGRAM_OPTIONS_LIBRARY})
#target_link_libraries(track_pf ${OpenCV_LIBS} ${Boost_FILESYSTEM_LIBRARIES} ${Boost_SYSTEM_LIBRARIES})
#target_link_libraries(lkdemo ${OpenCV_LIBS} ${Boost_FILESYSTEM_LIBRARIES} ${Boost_SYSTEM_LIBRARIES})
