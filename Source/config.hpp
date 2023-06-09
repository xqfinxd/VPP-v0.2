#pragma once

#include <set>
#include <string>

struct Config {
  int         width = 1280;
  int         height = 900;
  int         fps = 60;
  std::string title = "VPP";

  std::set<std::string> layers{"VK_LAYER_KHRONOS_validation"};

  uint32_t frame_lag = 2;
};

inline const Config& DefaultConfig() {
  static Config s_default_config;
  return s_default_config;
}
