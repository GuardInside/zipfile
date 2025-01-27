project(src)

file(GLOB_RECURSE SRC_FILES *.cpp *.c *.h)

if(NOT WIN32)
    list(FILTER SRC_FILES EXCLUDE REGEX "iowin32")
endif()

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE ZIP_FILE_LIBRARY)

find_package(Qt5 COMPONENTS Core REQUIRED)

target_link_libraries(${PROJECT_NAME} Qt5::Core z)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
