cmake_minimum_required(VERSION 3.0)

project(raytracer11)

if(MINGW)
  message("MINGW")
  add_definitions("-DMINGW")
elseif(WIN32)
  include_directories($ENV{HOME}/Source/share/include)
endif()

include_directories("${PROJECT_SOURCE_DIR}/inc")
file(GLOB SOURCE src/*.cpp inc/*.h)
add_executable(raytracer11 ${SOURCE})
set_property(TARGET raytracer11 PROPERTY CXX_STANDARD 14)
if(UNIX)
  target_link_libraries(raytracer11 pthread)
endif()
