#include "pch.h"
#include "DisplaySettings.h"

using namespace std;
using namespace winrt::Windows::Foundation::Numerics;

namespace AxoLight::Display
{
  enum class Side
  {
    Left,
    Right,
    Top,
    Bottom
  };

  DisplaySettings DisplaySettings::FromLayout(const DisplayLightLayout& layout)
  {
    DisplaySettings settings{};
    settings.AspectRatio = layout.DisplaySize.Width / layout.DisplaySize.Height;

    auto displaySize = reinterpret_cast<const float2&>(layout.DisplaySize);
    settings.SampleSize = float2(layout.SampleSize) / displaySize;
    
    auto sampleSize = float2(layout.SampleSize);
    auto displayCenter = displaySize / 2.f;    
    
    auto startPosition = layout.StartPosition.ToAbsolutePosition(layout.DisplaySize);
    for (auto& segment : layout.Segments)
    {
      auto endPosition = segment.EndPosition.ToAbsolutePosition(layout.DisplaySize);

      auto lightDistance = (endPosition - startPosition) / max((float)segment.LightCount - 1.f, 1.f);
      for (auto i = 0u; i < segment.LightCount; i++)
      {
        auto position = startPosition + float(i) * lightDistance;
        settings.SamplePoints.push_back(position / displaySize);
      }

      startPosition = endPosition;
    }

    return settings;
  }

  winrt::Windows::Foundation::Numerics::float2 DisplayPosition::ToAbsolutePosition(const Display::DisplaySize& size) const
  {
    float2 position;
    switch (Reference)
    {
    case DisplayPositionReference::BottomLeft:
      position = { X, Y };
      break;
    case DisplayPositionReference::BottomRight:
      position = { size.Width - X, Y };
      break;
    case DisplayPositionReference::TopLeft:
      position = { X, size.Height - Y };
      break;
    case DisplayPositionReference::TopRight:
      position = { size.Width - X, size.Height - Y };
      break;
    default:
      position = {};
      break;
    }

    return position;
  }
}
