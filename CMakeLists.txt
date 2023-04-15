cmake_minimum_required (VERSION 3.8)
project ("RandomAnime")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Wno-narrowing")

find_package(Threads REQUIRED)

find_package(CURL)
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

if(WIN32)
    add_definitions("-DCURL_STATICLIB")
endif()

add_executable (RandomAnime "src/main.cpp" "src/parser.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RandomAnime PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(RandomAnime PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(RandomAnime ${CMAKE_THREAD_LIBS_INIT} ${CURL_LIBRARIES})