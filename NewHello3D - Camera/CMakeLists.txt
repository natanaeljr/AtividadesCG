find_package(spdlog REQUIRED)

add_executable(NewHello3DCamera
    main.cpp
    sgl.cpp
)
target_include_directories(NewHello3DCamera PRIVATE
    ../Common/include
)
target_link_libraries(NewHello3DCamera
    glad
    glfw
    spdlog::spdlog
)
target_compile_definitions(NewHello3DCamera PRIVATE
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)
