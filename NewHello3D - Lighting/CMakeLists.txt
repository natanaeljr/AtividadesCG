find_package(spdlog REQUIRED)

add_executable(NewHello3DLighting
    main.cpp
    sgl.cpp
)
target_include_directories(NewHello3DLighting PRIVATE
    ../Common/include
)
target_link_libraries(NewHello3DLighting
    glad
    glfw
    spdlog::spdlog
)
target_compile_definitions(NewHello3DLighting PRIVATE
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)
