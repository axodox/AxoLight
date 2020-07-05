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

  struct AngleRange
  {
    float Min, Max;
    Side Side;
  };

  DisplaySettings DisplaySettings::FromLayout(const DisplayLightLayout& layout)
  {
    DisplaySettings settings{};
    settings.AspectRatio = layout.DisplaySize.Width / layout.DisplaySize.Height;
    settings.SampleSize = layout.SampleSize;

    auto displaySize = reinterpret_cast<const float2&>(layout.DisplaySize);
    auto sampleSize = float2(layout.SampleSize);
    auto displayCenter = displaySize / 2.f;
    auto reducedDisplaySize = displayCenter - sampleSize / 2.f;
    
    array<AngleRange, 4> angleRanges = {
      AngleRange { atan2(-reducedDisplaySize.y, reducedDisplaySize.x), atan2(reducedDisplaySize.y, reducedDisplaySize.x), Side::Right },
      AngleRange { atan2(reducedDisplaySize.y, reducedDisplaySize.x), atan2(reducedDisplaySize.y, -reducedDisplaySize.x), Side::Top },
      AngleRange { atan2(reducedDisplaySize.y, -reducedDisplaySize.x), atan2(-reducedDisplaySize.y, -reducedDisplaySize.x), Side::Left },
      AngleRange { atan2(-reducedDisplaySize.y, -reducedDisplaySize.x), atan2(-reducedDisplaySize.y, reducedDisplaySize.x), Side::Bottom }
    };

    auto startPosition = layout.StartPosition.ToTexturePosition(layout.DisplaySize);
    for (auto& segment : layout.Segments)
    {
      auto endPosition = segment.EndPosition.ToTexturePosition(layout.DisplaySize);

      auto lightDistance = (endPosition - startPosition) / max((float)segment.LightCount - 1.f, 1.f);
      for (auto i = 0u; i < segment.LightCount; i++)
      {
        auto position = startPosition + float(i) * lightDistance;

        auto vectorFromCenter = position - displayCenter;
        auto angle = atan2(vectorFromCenter.y, vectorFromCenter.x);

        for (auto angleRange : angleRanges)
        {
          if (angle >= angleRange.Min && angle <= angleRange.Max)
          {
            switch (angleRange.Side)
            {
            case Side::Right:
              position.x = reducedDisplaySize.x;
              position.y = (vectorFromCenter.y / vectorFromCenter.x) * position.x;
              break;
            case Side::Left:
              position.x = -reducedDisplaySize.x;
              position.y = (vectorFromCenter.y / vectorFromCenter.x) * position.x;
              break;
            case Side::Top:
              position.y = reducedDisplaySize.y;
              position.x = (vectorFromCenter.x / vectorFromCenter.y) * position.y;
              break;
            case Side::Bottom:
              position.y = -reducedDisplaySize.y;
              position.x = (vectorFromCenter.x / vectorFromCenter.y) * position.y;
              break;
            }
            break;
          }
        }

        position = (float2(position.x, -position.y) + displayCenter) / displaySize;
        settings.SamplePoints.push_back(position);
      }

      startPosition = endPosition;
    }

    return settings;
  }

  winrt::Windows::Foundation::Numerics::float2 DisplayPosition::ToTexturePosition(const Display::DisplaySize& size) const
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
