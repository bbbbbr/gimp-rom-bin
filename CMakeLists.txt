
cmake_minimum_required(VERSION 3.20)

project(gimp-rom-bin)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

include(FindPkgConfig)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GTK REQUIRED gtk+-2.0)
pkg_search_module(GIMP REQUIRED gimp-2.0)
pkg_search_module(GIMPUI REQUIRED gimpui-2.0)

include_directories(
	"${PROJECT_SOURCE_DIR}/src"
	${GTK_INCLUDE_DIRS}
	${GIMP_INCLUDE_DIRS}
)

# detect all source files dynamically
file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(file-rom-bin ${SRC_FILES})
target_link_libraries(
	file-rom-bin PRIVATE
	${GLIB_LIBRARIES}
	${GTK_LIBRARIES}
	${GIMP_LIBRARIES}
	${GIMPUI_LIBRARIES}
)

if(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_custom_command(
		TARGET file-rom-bin POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E echo "Stripping runtime of debugging symbols ..."
		COMMAND ${CMAKE_STRIP} --strip-debug "$<TARGET_FILE:file-rom-bin>"
	)
endif()

# parse GIMP's major & minor versions
string(REGEX MATCH "^[0-9]+\\.[0-9]+" GIMP_API_VERSION "${GIMP_VERSION}")
if(WIN32)
	set(GIMP_PLUGINS_DIR "$ENV{APPDATA}/GIMP/${GIMP_API_VERSION}/plug-ins" CACHE PATH "Directory where GIMP's plugins are located")
else()
	set(GIMP_PLUGINS_DIR "$ENV{HOME}/.config/GIMP/${GIMP_API_VERSION}/plug-ins" CACHE PATH "Directory where GIMP's plugins are located")
endif()

# execute `cmake --install <build_path>` to install to GIMP plugin-ins directory
install(TARGETS file-rom-bin RUNTIME DESTINATION "${GIMP_PLUGINS_DIR}")
