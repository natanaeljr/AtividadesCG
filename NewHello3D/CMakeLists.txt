find_package(spdlog REQUIRED)

add_executable(NewHello3D
    main.cpp
    sgl.cpp
)
target_include_directories(NewHello3D PRIVATE
    ../Common/include
)
target_link_libraries(NewHello3D
    glad
    glfw
    spdlog::spdlog
)
target_compile_definitions(NewHello3D PRIVATE
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)
