add_library(imgui
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_stl.cpp
)

target_include_directories(imgui PUBLIC .)
