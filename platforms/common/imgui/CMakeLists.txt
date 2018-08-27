add_library(imgui
  GL/gl3w.c  
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
)

target_include_directories(imgui PUBLIC .)
