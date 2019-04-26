PROJECT(reechat_base)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

macro(ADD_FRAMEWORK fwname appname)
find_library(FRAMEWORK_${fwname}
    NAMES ${fwname}
    PATHS ${CMAKE_OSX_SYSROOT}/System/Library
    PATH_SUFFIXES Frameworks
    NO_DEFAULT_PATH)
if( ${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
    MESSAGE(ERROR ": Framework ${fwname} not found")
else()
    TARGET_LINK_LIBRARIES(${appname} ${FRAMEWORK_${fwname}})
    MESSAGE(STATUS "Framework ${fwname} found at ${FRAMEWORK_${fwname}}")
endif()
endmacro(ADD_FRAMEWORK)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libevent2/include
	${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

file(GLOB_RECURSE reechat_base "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_definitions(-DWEBRTC_POSIX -DWEBRTC_MAC)

add_library(reechat_base STATIC ${reechat_base})

ADD_SUBDIRECTORY(third_party/libevent2)
ADD_SUBDIRECTORY(third_party/webrtc)

target_compile_options(
	reechat_base PRIVATE -std=gnu++11
)

file(GLOB_RECURSE test_src "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp")
add_executable(reechat_base_test ${test_src})
target_compile_options(reechat_base_test PRIVATE -std=gnu++11)
target_link_libraries(reechat_base_test reechat_base rtc_base event)
add_framework(Foundation reechat_base_test)




