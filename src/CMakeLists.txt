# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources main.cpp)

idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "."
    REQUIRES manager ring_lights light_sensor)
