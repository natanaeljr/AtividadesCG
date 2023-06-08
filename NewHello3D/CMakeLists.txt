find_package(spdlog REQUIRED)

add_executable(NewHello3D
    main.cpp
    core.cpp
)
target_link_libraries(NewHello3D
    glad
    glfw
    spdlog::spdlog
)
