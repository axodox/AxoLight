#include "pch.h"
#include "SettingsImporter.h"

using namespace std;
using namespace std::chrono;

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Data::Json;

namespace AxoLight::Settings
{
  Settings SettingsImporter::Parse(const std::filesystem::path& path)
  {
    try
    {
      auto file = StorageFile::GetFileFromPathAsync(path.c_str()).get();
      auto text = FileIO::ReadTextAsync(file).get();
      auto json = JsonObject::Parse(text);

      Settings settings = {};
      for (const auto& property : json)
      {
        try
        {
          if (property.Key() == L"controllerOptions")
          {
            Parse(property.Value().GetObject(), settings.ControllerOptions);
          }
          else if (property.Key() == L"lightLayout")
          {
            Parse(property.Value().GetObject(), settings.LightLayout);
          }
        }
        catch (...)
        {
          wprintf(L"Failed to parse setting %s.", property.Key().c_str());
        }
      }

      return settings;
    }
    catch (...)
    {
      wprintf(L"Failed to parse settings from %s.", path.c_str());
      return {};
    }
  }

  void SettingsImporter::Parse(const JsonObject& json, Lighting::AdaLightOptions& options)
  {
    for (const auto& property : json)
    {
      try
      {
        if (property.Key() == L"usbVendorId")
        {
          options.UsbVendorId = (uint16_t)property.Value().GetNumber();
        }
        else if (property.Key() == L"usbProductId")
        {
          options.UsbProductId = (uint16_t)property.Value().GetNumber();
        }
        else if (property.Key() == L"baudRate")
        {
          options.BaudRate = (uint32_t)property.Value().GetNumber();
        }
        else if (property.Key() == L"ledSyncDuration")
        {
          options.LedSyncDuration = milliseconds((uint64_t)property.Value().GetNumber());
        }
      }
      catch (...)
      {
        wprintf(L"Failed to parse setting %s.", property.Key().c_str());
      }
    }
  }

  void SettingsImporter::Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplaySize& displaySize)
  {
    for (const auto& property : json)
    {
      try
      {
        if (property.Key() == L"width")
        {
          displaySize.Width = (float)property.Value().GetNumber();
        }
        else if (property.Key() == L"height")
        {
          displaySize.Height = (float)property.Value().GetNumber();
        }
      }
      catch (...)
      {
        wprintf(L"Failed to parse setting %s.", property.Key().c_str());
      }
    }
  }

  const unordered_map<wstring, Display::DisplayPositionReference> _displayPositionReferenceValues = {
    { L"BottomLeft", Display::DisplayPositionReference::BottomLeft },
    { L"BottomRight", Display::DisplayPositionReference::BottomRight },
    { L"TopLeft", Display::DisplayPositionReference::TopLeft },
    { L"TopRight", Display::DisplayPositionReference::TopRight }
  };

  void SettingsImporter::Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayPosition& displayPosition)
  {
    for (const auto& property : json)
    {
      try
      {
        if (property.Key() == L"reference")
        {
          displayPosition.Reference = _displayPositionReferenceValues.at(wstring(property.Value().GetString()));
        }
        else if (property.Key() == L"x")
        {
          displayPosition.X = (float)property.Value().GetNumber();
        }
        else if (property.Key() == L"y")
        {
          displayPosition.Y = (float)property.Value().GetNumber();
        }
      }
      catch (...)
      {
        wprintf(L"Failed to parse setting %s.", property.Key().c_str());
      }
    }
  }

  void SettingsImporter::Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayLightStrip& displayLightStrip)
  {
    for (const auto& property : json)
    {
      try
      {
        if (property.Key() == L"endPosition")
        {
          Parse(property.Value().GetObject(), displayLightStrip.EndPosition);
        }
        else if (property.Key() == L"lightCount")
        {
          displayLightStrip.LightCount = (uint16_t)property.Value().GetNumber();
        }
      }
      catch (...)
      {
        wprintf(L"Failed to parse setting %s.", property.Key().c_str());
      }
    }
  }

  void SettingsImporter::Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayLightLayout& displayLightLayout)
  {
    for (const auto& property : json)
    {
      try
      {
        if (property.Key() == L"displaySize")
        {
          Parse(property.Value().GetObject(), displayLightLayout.DisplaySize);
        }
        else if (property.Key() == L"startPosition")
        {
          Parse(property.Value().GetObject(), displayLightLayout.StartPosition);
        }
        else if (property.Key() == L"segments")
        {
          const auto& items = property.Value().GetArray();
          for (const auto& item : items)
          {
            Display::DisplayLightStrip strip{};
            Parse(item.GetObject(), strip);
            displayLightLayout.Segments.push_back(strip);
          }          
        }
        else if (property.Key() == L"sampleSize")
        {
          displayLightLayout.SampleSize = (float)property.Value().GetNumber();
        }
      }
      catch (...)
      {
        wprintf(L"Failed to parse setting %s.", property.Key().c_str());
      }
    }
  }
}