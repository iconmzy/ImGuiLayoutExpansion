cmake_minimum_required(VERSION 3.10)
project(FrameGUILayout LANGUAGES CXX)


set(CMAKE_WIN32_EXECUTABLE ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE IMGUI_SOURCES 
    "imgui/*.cpp"
    "imgui/backends/imgui_impl_win32.cpp"
    "imgui/backends/imgui_impl_dx11.cpp"
)
set(SOURCES 
    main.cpp
    FrameGUILayout.cpp
    ${IMGUI_SOURCES}
)


add_executable(${PROJECT_NAME} WIN32 ${SOURCES})


target_include_directories(${PROJECT_NAME} PRIVATE
    imgui
    imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}  
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    d3dcompiler.lib  
    dxgi.lib
    user32.lib
    gdi32.lib       
    imm32.lib       
)



install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    CONFIGURATIONS Release
)