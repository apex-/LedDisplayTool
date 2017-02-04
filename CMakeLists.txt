cmake_minimum_required (VERSION 2.6)

# Program major and minor version number
#set (LedDisplayTool_VERSION_MAJOR 0)
#set (LedDisplayTool_VERSION_MINOR 1)
include_directories("ws2811")
add_subdirectory(ws2811)

project (LedDisplayTool C)
add_executable(LedDisplayTool main.c)
target_link_libraries (LedDisplayTool ws2811)

