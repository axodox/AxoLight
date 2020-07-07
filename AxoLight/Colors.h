#pragma once
#include "pch.h"

namespace AxoLight::Colors
{
  struct rgb
  {
    uint8_t r, g, b;
  };

  struct hsl
  {
    float h, s, l;
  };

  hsl rgb_to_hsl(const rgb& rgb);

  rgb hsl_to_rgb(const hsl& hsl);

  void enhance(std::vector<rgb>& colors);
}