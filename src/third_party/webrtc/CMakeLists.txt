PROJECT(rtc_base)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

file(GLOB_RECURSE rtc_base 
  "${CMAKE_CURRENT_SOURCE_DIR}/rtc_base/*.cc" 
  "${CMAKE_CURRENT_SOURCE_DIR}/rtc_base/*.mm"
  )
list(FILTER rtc_base EXCLUDE REGEX ".*test")
list(FILTER rtc_base EXCLUDE REGEX ".*win")
list(FILTER rtc_base EXCLUDE REGEX "helpers.cc")
list(FILTER rtc_base EXCLUDE REGEX ".*json")
list(FILTER rtc_base EXCLUDE REGEX ".*ssl")
list(FILTER rtc_base EXCLUDE REGEX ".*messagedigest.cc")
list(FILTER rtc_base EXCLUDE REGEX ".*rate_limiter")
list(FILTER rtc_base EXCLUDE REGEX ".*_libevent.cc")

add_definitions(-DWEBRTC_POSIX -DWEBRTC_MAC)

add_library(rtc_base STATIC ${rtc_base} 
  ${CMAKE_CURRENT_SOURCE_DIR}/rtc_base/nethelpers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/rtc_base/thread_darwin.mm
)

target_compile_options(
	rtc_base PRIVATE -std=gnu++11
)




