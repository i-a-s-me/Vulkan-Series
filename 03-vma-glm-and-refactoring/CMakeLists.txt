cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(I_A_S_ME_VulkanSeries)

add_executable(App 
    main.cpp
    sample.cpp
    pipeline.cpp
)

target_link_libraries(App PRIVATE 
    glfw
    vulkan
)
