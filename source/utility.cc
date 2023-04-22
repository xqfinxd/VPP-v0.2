#pragma once

#include "utility.h"
#include <algorithm>
#include <cstdarg>
#include <fstream>

std::string ReadFile(const char* fn) {
  std::string content{};
  std::ifstream file(fn);  // 打开文件
  if (!file) {
    return content;
  }
  file.seekg(0, file.end);
  size_t length = file.tellg();
  content.resize(length + 1);
  file.seekg(0, file.beg);
  file.read(&content.at(0), length);
  file.close();  // 关闭文件
  return content;
}

const char* CFmt(const char* fmt, ...) {
  static char sbuf[CFMT_MAX_LENGTH + 1] = {0};
  va_list args;
  va_start(args, fmt);
  int length = vsprintf_s(sbuf, fmt, args);
  sbuf[std::min<size_t>(length, CFMT_MAX_LENGTH)] = 0;
  va_end(args);
  return sbuf;
}