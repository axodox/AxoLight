#include "pch.h"
#include "AdaLightController.h"
#include "Graphics.h"
#include "Infrastructure.h"
#include "SettingsImporter.h"

using namespace AxoLight::Display;
using namespace AxoLight::Graphics;
using namespace AxoLight::Infrastructure;
using namespace AxoLight::Lighting;
using namespace AxoLight::Settings;

using namespace std::filesystem;
using namespace std::chrono_literals;

using namespace winrt;
using namespace Windows::Foundation;

int main()
{
  init_apartment();

  auto root = get_root();
  auto settings = SettingsImporter::Parse(root / L"settings.json");
  auto displaySettings = DisplaySettings::FromLayout(settings.LightLayout);
  

  AdaLightController controller;
  if (!controller.IsConnected()) return 0;

  while (true)
  {
    controller.Push({
      {255, 0, 0},
      {0, 255, 0},
      {0, 0, 255}
      });

  }

  return 0;
}
