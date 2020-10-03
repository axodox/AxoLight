#pragma once
#include "pch.h"

namespace AxoLight::Colors
{
  struct rgb
  {
    uint8_t r, g, b;

    void apply_gamma(float value);
    static float gamma(float value, float gamma);
  };

  struct hsl
  {
    float h, s, l;
  };

  hsl rgb_to_hsl(const rgb& rgb);

  rgb hsl_to_rgb(const hsl& hsl);

  void enhance(std::vector<rgb>& colors);

  rgb lerp(const rgb& a, const rgb& b, float factor);

  
}