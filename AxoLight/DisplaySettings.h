#pragma once
#include "pch.h"

namespace AxoLight::Display
{
  struct DisplaySize
  {
    float Width, Height;
  };

  enum class DisplayPositionReference
  {
    BottomLeft,
    BottomRight,
    TopLeft,
    TopRight
  };

  struct DisplayPosition
  {
    DisplayPositionReference Reference;
    float X, Y;

    winrt::Windows::Foundation::Numerics::float2 ToTexturePosition(const DisplaySize& size) const;
  };

  struct DisplayLightStrip
  {
    DisplayPosition EndPosition;
    uint16_t LightCount;
  };

  struct DisplayLightLayout
  {
    DisplaySize DisplaySize;
    DisplayPosition StartPosition;
    std::vector<DisplayLightStrip> Segments;
    float SampleSize;
  };

  struct DisplayProbe
  {
    winrt::Windows::Foundation::Numerics::float2 TopLeft;
    winrt::Windows::Foundation::Numerics::float2 Size;
  };

  struct DisplaySettings
  {
    float AspectRatio;
    float SampleSize;
    std::vector<winrt::Windows::Foundation::Numerics::float2> SamplePoints;

    static DisplaySettings FromLayout(const DisplayLightLayout& layout);
  };
}