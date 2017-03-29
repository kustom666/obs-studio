project(enc-openh264)

#find_package(Libx264 REQUIRED)
#include_directories(${LIBX264_INCLUDE_DIRS})
#add_definitions(${LIBX264_DEFINITIONS})
LINK_DIRECTORIES(res/)

set(enc-openh264_SOURCES
	enc-openh264.c)

add_library(enc-openh264 MODULE
	${enc-openh264_SOURCES})
target_link_libraries(enc-openh264
	libobs
	openh264)

install_obs_plugin_with_data(enc-openh264 data)
