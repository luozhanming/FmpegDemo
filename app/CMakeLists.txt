cmake_minimum_required(VERSION 3.4.2)

include_directories(src/main/jni/include)
include_directories(src/main/jni/ffmpeg/include)

set(CPATH "src/main/jni")
set(FFMPEG_LIB_DIR "${CMAKE_SOURCE_DIR}/src/main/jni/ffmpeg")

file(GLOB_RECURSE myCFile "${CPATH}/*")

add_library(test-lib
        SHARED
        ${myCFile})
#find_library定义的库名称调用时需要添加${}
find_library(log-lib
        log)
#add_library的库名称不需要调用时不需要添加${}
add_library(ffmpeg-lib
        SHARED
        IMPORTED)
set_target_properties(ffmpeg-lib
        PROPERTIES IMPORTED_LOCATION
        ${FFMPEG_LIB_DIR}/${ANDROID_ABI}/libijkffmpeg.so)

#set(ffmpeg-lib
#        ${FFMPEG_LIB_DIR}/${ANDROID_ABI}/libijkffmpeg.so)


target_link_libraries( # Specifies the target library.
        test-lib
        ffmpeg-lib
        ${log-lib}
       )