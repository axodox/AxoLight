#pragma once
#include "pch.h"

namespace AxoLight::Colors
{
  struct rgb
  {
    uint8_t r, g, b;

    rgb() = default;
    rgb(uint8_t r, uint8_t g, uint8_t b);
    rgb(const winrt::Windows::Foundation::Numerics::float3& color);
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