# 设置项目名称
project(hdrd_app)

if(MSVC)
    add_compile_options(/utf-8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
endif()

# C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6
    REQUIRED
    Core
    Gui
    Widgets
    Network
    OpenGL
    OpenGLWidgets
    PrintSupport
    Multimedia
    MultimediaWidgets
    WebEngineWidgets      
)

# 寻找 海康MVS
get_filename_component(MVS_DIR ../extern/MVS ABSOLUTE)
include_directories(${MVS_DIR}/Includes)
file(GLOB MVS_LIB_FILE ${MVS_DIR}/Libraries/win64/*.lib)
message(STATUS "MVS LIB: " ${MVS_LIB_FILE})

# 添加可执行文件
add_executable(${PROJECT_NAME} WIN32)

# 添加源文件
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.h" "src/*.hpp")

# 将源文件添加到可执行文件中
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

# 设置资源文件
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/res/app.qrc
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    oclero::qlementine
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::PrintSupport
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::WebEngineWidgets     
    ${MVS_LIB_FILE}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERNAL_CONSOLE OFF
    EXCLUDE_FROM_ALL OFF
    FOLDER qtgui
    AUTOMOC ON
    AUTORCC ON
)
