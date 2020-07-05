#pragma once
#include "pch.h"

namespace AxoLight::Infrastructure
{
  std::filesystem::path get_root();

  std::vector<uint8_t> load_file(const std::filesystem::path& path);

  LRESULT CALLBACK debug_message_handler(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

  winrt::handle create_debug_window();
}