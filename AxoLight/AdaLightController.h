#pragma once
#include "Colors.h"

namespace AxoLight::Lighting
{
  struct AdaLightOptions
  {
    uint16_t UsbVendorId = 0x1A86;
    uint16_t UsbProductId = 0x7523;
    uint32_t BaudRate = 1000000;
    std::chrono::milliseconds LedSyncDuration = std::chrono::milliseconds(8);
  };

  class AdaLightController
  {
  public:
    AdaLightController(const AdaLightOptions& options = {});

    bool IsConnected();
    void Push(const std::vector<Colors::rgb>& colors);

  private:
    winrt::Windows::Storage::Streams::DataWriter _serialWriter = nullptr;
    std::chrono::steady_clock::duration _ledSyncDuration;
    std::chrono::steady_clock::time_point _lastUpdate;
  };
}