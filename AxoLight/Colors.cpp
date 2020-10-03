#include "pch.h"
#include "Colors.h"

using namespace std;
using namespace winrt::Windows::Foundation::Numerics;

namespace AxoLight::Colors
{
  hsl rgb_to_hsl(const rgb& rgb)
  {
    hsl result;

    auto floatRGB = float3(rgb.r, rgb.g, rgb.b) / 255.f;

    auto max = std::max({ floatRGB.x, floatRGB.y, floatRGB.z });
    auto min = std::min({ floatRGB.x, floatRGB.y, floatRGB.z });

    auto diff = max - min;
    result.l = (max + min) / 2.f;
    if (abs(diff) < 0.00001f)
    {
      result.s = 0.f;
      result.h = 0.f;
    }
    else
    {
      if (result.l <= 0.5f)
      {
        result.s = diff / (max + min);
      }
      else
      {
        result.s = diff / (2.f - max - min);
      }

      auto dist = (float3(max) - floatRGB) / diff;
      
      if (floatRGB.x == max)
      {
        result.h = dist.z - dist.y;
      }
      else if (floatRGB.y == max)
      {
        result.h = 2.f + dist.x - dist.z;
      }
      else
      {
        result.h = 4.f + dist.y - dist.x;
      }

      result.h = result.h * 60.f;
      if (result.h < 0.f) result.h += 360.f;
    }

    return result;
  }

  float qqh_to_rgb(float q1, float q2, float hue)
  {
    if (hue > 360.f) hue -= 360.f;
    else if (hue < 0.f) hue += 360.f;

    if (hue < 60.f) return q1 + (q2 - q1) * hue / 60.f;
    if (hue < 180.f) return q2;
    if (hue < 240.f) return q1 + (q2 - q1) * (240.f - hue) / 60.f;
    return q1;
  }

  rgb hsl_to_rgb(const hsl& hsl)
  {
    float p2;
    if (hsl.l <= 0.5f) p2 = hsl.l * (1 + hsl.s);
    else p2 = hsl.l + hsl.s - hsl.l * hsl.s;

    float p1 = 2.f * hsl.l - p2;
    float3 floatRGB;
    if (hsl.s == 0)
    {
      floatRGB = float3(hsl.l);
    }
    else
    {
      floatRGB = float3(
        qqh_to_rgb(p1, p2, hsl.h + 120.f),
        qqh_to_rgb(p1, p2, hsl.h),
        qqh_to_rgb(p1, p2, hsl.h - 120.f)
      );
    }

    floatRGB *= 255.f;
    return rgb{
      (uint8_t)floatRGB.x,
      (uint8_t)floatRGB.y,
      (uint8_t)floatRGB.z
    };
  }

  struct linear_transfer_function
  {
    std::vector<float2> points;

    linear_transfer_function(initializer_list<float2> values)
    {
      points = values;
    }

    float operator()(float value)
    {
      value = clamp(value, 0.f, 1.f);

      float2 p1 = { 0.f, 0.f };
      for (auto& p2 : points)
      {
        if (value < p2.x)
        {
          return (value - p1.x) / (p2.x - p1.x) * (p2.y - p1.y) + p1.y;
        }
        p1 = p2;
      }

      return (value - p1.x) / (1 - p1.x) * (1 - p1.y) + p1.y;
    }
  };

  linear_transfer_function brightness_tf = {
    {0.4f, 0.6f},
    {0.6f, 0.7f}
  };

  linear_transfer_function saturation_tf = {
    {0.3f, 0.3f},
    {0.4f, 0.6f},
    {0.6f, 0.8f}
  };

  const float _maxLightness = 0.7f;

  void enhance(std::vector<rgb>& colors)
  {
    uint32_t sumLightness = 0u;

    for (auto& color : colors)
    {
      /*rgb corrected;
      corrected.r = min(255.0, max(0.0, 0.8005918860435486 * color.r + 0.23673559725284576 * color.g + 0.054503846913576126 * color.b + 0.07293142378330231 * 255));
      corrected.g = min(255.0, max(0.0, 0.018977604806423187 * color.r + 0.9645366072654724 * color.g + 0.06882637739181519 * color.b + 0.04205838590860367 * 255));
      corrected.b = min(255.0, max(0.0, 0.0500728040933609 * color.r + -0.060099828988313675 * color.g + 1.0020443201065063 * color.b + 0.01855257712304592 * 255));
      color = corrected;*/      

      auto hsl = rgb_to_hsl(color);
      hsl.s = saturation_tf(hsl.s);
      hsl.l = brightness_tf(hsl.l);
      color = hsl_to_rgb(hsl);

      sumLightness += color.r + color.g + color.b;
    }

    auto avgLightness = sumLightness / 255.f / 3.f / colors.size();
    if (avgLightness > _maxLightness)
    {
      auto factor = _maxLightness / avgLightness;
      for (auto& rgb : colors)
      {
        rgb.r *= factor;
        rgb.g *= factor;
        rgb.b *= factor;
      }
    }
  }
  
  rgb lerp(const rgb& a, const rgb& b, float factor)
  {
    auto invFactor = 1 - factor;
    return {
      uint8_t(a.r * invFactor + b.r * factor),
      uint8_t(a.g * invFactor + b.g * factor),
      uint8_t(a.b * invFactor + b.b * factor)
    };
  }
  
  void rgb::apply_gamma(float value)
  {
    r = uint8_t(gamma(r / 255.f, value) * 255.f);
    g = uint8_t(gamma(g / 255.f, value) * 255.f);
    b = uint8_t(gamma(b / 255.f, value) * 255.f);
  }

  float rgb::gamma(float value, float gamma)
  {
    return pow(value, gamma);
  }
}