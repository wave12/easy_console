cmake_minimum_required(VERSION 3.15)

# 定义项目名称、版本和语言
project(easy_console VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------
# 1. 全局设置
# ---------------------------------------------------------

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置默认构建类型为 Release（如果用户没有指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
endif()

# 如果是 Windows 下使用 MSVC 编译器，添加 /utf-8 选项防止中文乱码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ---------------------------------------------------------
# 2. 目录设置
# ---------------------------------------------------------

# 头文件搜索路径
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# ---------------------------------------------------------
# 3. 查找依赖库
# ---------------------------------------------------------

# 查找多线程库（跨平台必备）
find_package(Threads REQUIRED)

# 如果以后使用 httplib 等非 Header-Only 的库，可以在这里 find_package
# find_package(CURL REQUIRED) 

# 查找 Paho MQTT C++ 库 (确保系统已正确安装)
find_package(PahoMqttCpp REQUIRED)

# ---------------------------------------------------------
# 4. 定义源文件
# ---------------------------------------------------------

# 基础源文件（所有平台共用）
set(COMMON_SOURCES
    src/main.cpp
    src/sender/HttpSender.cpp
    src/sender/MqttSender.cpp
)

# 平台相关的源文件与库
set(PLATFORM_SOURCES "")
set(PLATFORM_LIBS "")

if (WIN32)
    message(STATUS "Configuring for Windows platform")
    list(APPEND PLATFORM_SOURCES 
        src/collectors/WindowsCollector.cpp
    )
    # Windows 下的性能/进程查询库
    list(APPEND PLATFORM_LIBS psapi) 
    
elseif (UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux platform")
    list(APPEND PLATFORM_SOURCES 
        src/collectors/LinuxCollector.cpp
    )
    # Linux 下的线程库
    list(APPEND PLATFORM_LIBS pthread)
    
elseif (APPLE)
    message(STATUS "Configuring for macOS platform (Not fully implemented)")
    # 预留 macOS 的实现
    list(APPEND PLATFORM_SOURCES 
        src/collectors/MacCollector.cpp
    )
endif()

# 合并所有源文件
set(ALL_SOURCES 
    ${COMMON_SOURCES} 
    ${PLATFORM_SOURCES}
)

# ---------------------------------------------------------
# 5. 创建可执行文件并链接库
# ---------------------------------------------------------

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# 链接多线程库及平台特定库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads 
    ${PLATFORM_LIBS}
    PahoMqttCpp::paho-mqttpp3
    OpenSSL::SSL      
    OpenSSL::Crypto   
)

# ---------------------------------------------------------
# 6. 打印构建信息（便于排查问题）
# ---------------------------------------------------------

message(STATUS "--------------------------------------------------")
message(STATUS "Project Name:  ${PROJECT_NAME}")
message(STATUS "Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler:  ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "--------------------------------------------------")

# =========================================================
# 7. 拷贝配置文件到构建目录
# =========================================================
# 源码根目录下的 config.json
set(CONFIG_SOURCE ${CMAKE_SOURCE_DIR}/config.json)

# 构建目录（例如 build/）下的 config.json
set(CONFIG_DEST ${CMAKE_BINARY_DIR}/config.json)

if(EXISTS ${CONFIG_SOURCE})
    # 以配置模式拷贝，如果 config.json 内容需要替换 CMake 变量，用 configure_file 更好
    configure_file(${CONFIG_SOURCE} ${CONFIG_DEST} COPYONLY)
    message(STATUS "Config file copied to ${CONFIG_DEST}")
else()
    message(WARNING "config.json not found in source directory!")
endif()