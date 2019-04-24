PROJECT(reechat_base)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libevent2/include
	${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

file(GLOB_RECURSE reechat_base "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_definitions(-DWEBRTC_POSIX -DWEBRTC_MAC)

add_library(reechat_base STATIC ${reechat_base})

target_compile_options(
	reechat_base PRIVATE -std=gnu++11
)




