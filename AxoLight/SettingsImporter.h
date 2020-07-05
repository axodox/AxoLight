#pragma once
#include "AdaLightController.h"
#include "DisplaySettings.h"

namespace AxoLight::Settings
{
  struct Settings
  {
    Lighting::AdaLightOptions ControllerOptions;
    Display::DisplayLightLayout LightLayout;
  };

  class SettingsImporter
  {
  public:
    static Settings Parse(const std::filesystem::path& path);

  private:
    static void Parse(const winrt::Windows::Data::Json::JsonObject& json, Lighting::AdaLightOptions& options);

    static void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplaySize& displaySize);

    static void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayPosition& displayPosition);

    static void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayLightStrip& displayLightStrip);

    static void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayLightLayout& displayLightLayout);
  };
}