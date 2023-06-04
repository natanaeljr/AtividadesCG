add_executable(Hello3DOBJTex
  Exericio8/Origem.cpp
)
target_include_directories(Hello3DOBJTex PRIVATE
    ../Common/include
)
target_link_libraries(Hello3DOBJTex
  glad
  glfw
)
