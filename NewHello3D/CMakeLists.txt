find_package(spdlog REQUIRED)

add_executable(NewHello3D
    main.cpp
    core.cpp
)
target_include_directories(NewHello3D PRIVATE
    ../Common/include
)
target_link_libraries(NewHello3D
    glad
    glfw
    spdlog::spdlog
)
