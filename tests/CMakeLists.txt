project(tests)

file(GLOB SRC_FILES *.cpp *.h)

add_executable(${PROJECT_NAME} ${SRC_FILES})

find_package(Qt5 COMPONENTS Core Test REQUIRED)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Test zipfile)
