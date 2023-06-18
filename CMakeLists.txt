cmake_minimum_required(VERSION 3.15 FATAL_ERROR) 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(usb_event_tracer)

add_executable(tracer_executable usbtest/test.cpp)

add_library(usb_event_tracer my_usb_tracer/monitor.cpp 
		 my_usb_tracer/usb_monitor.cpp
		 my_usb_tracer/usb_monitor.h
		 my_usb_tracer/monitor.h)

include_directories(my_usb_tracer)
target_link_libraries(tracer_executable usb_event_tracer)

if (UNIX)
	add_subdirectory(efsw-master)
	target_link_libraries(usb_event_tracer efsw)
elseif (WIN32)
	target_link_libraries(usb_event_tracer SetupAPI)
endif()
