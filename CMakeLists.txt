cmake_minimum_required(VERSION 3.10)
project(FrameGUILayout LANGUAGES CXX)

# 设置为 Windows 应用程序（关键修改：添加 WIN32 标识）
set(CMAKE_WIN32_EXECUTABLE ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源文件
file(GLOB_RECURSE IMGUI_SOURCES 
    "imgui/*.cpp"
    "imgui/backends/imgui_impl_win32.cpp"
    "imgui/backends/imgui_impl_dx11.cpp"
)
set(SOURCES 
    main.cpp
    ${IMGUI_SOURCES}
)

# 添加可执行文件（关键修改：显式声明 WIN32 应用程序）
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# 包含目录（头文件路径）
target_include_directories(${PROJECT_NAME} PRIVATE
    imgui
    imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}  # 更规范的当前目录引用
)

# 链接 Windows 系统库（添加 d3dcompiler.lib）
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    d3dcompiler.lib  # 添加 HLSL 编译支持
    dxgi.lib
    user32.lib
    gdi32.lib       # 添加 GDI 支持（某些 ImGui 功能需要）
    imm32.lib       # 输入法支持
)

# 启用 Unicode 字符集
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS  # 禁用安全警告
)

# 调试模式配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _DEBUG
        IMGUI_DEBUG
    )
    # 调试符号配置
    target_compile_options(${PROJECT_NAME} PRIVATE /Zi)
    target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
endif()

# Release 模式配置
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /O2     # 优化级别
        /Oi     # 启用内建函数
        /GL     # 全程序优化
    )
    target_link_options(${PROJECT_NAME} PRIVATE /LTCG)  # 链接时代码生成
endif()

# 安装后处理（可选）
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    CONFIGURATIONS Release
)