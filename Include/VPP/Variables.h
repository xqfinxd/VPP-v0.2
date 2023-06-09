#pragma once

#include <set>
#include <string>

struct {
  int         width = 1280;
  int         height = 900;
  int         fps = 60;
  std::string title = "VPP";

  std::set<std::string> layers{"VK_LAYER_KHRONOS_validation"};

  uint32_t frame_lag = 2;
} g_Vars{};
