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
    Settings Parse(const winrt::hstring& path);

  private:
    void Parse(const winrt::Windows::Data::Json::JsonObject& json, Lighting::AdaLightOptions& options);

    void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplaySize& displaySize);

    void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayPosition& displayPosition);

    void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayLightStrip& displayLightStrip);

    void Parse(const winrt::Windows::Data::Json::JsonObject& json, Display::DisplayLightLayout& displayLightLayout);
  };
}