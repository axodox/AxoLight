#include "pch.h"
#include "AdaLightController.h"

using namespace std;
using namespace std::chrono;

using namespace winrt;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Devices::SerialCommunication;
using namespace winrt::Windows::Storage::Streams;

namespace AxoLight::Lighting
{
  AdaLightController::AdaLightController(const AdaLightOptions& options)
  {
    auto deviceSelector = SerialDevice::GetDeviceSelectorFromUsbVidPid(options.UsbVendorId, options.UsbProductId);
    auto deviceInformations = DeviceInformation::FindAllAsync(deviceSelector).get();
    if (deviceInformations.Size() == 0) return;

    auto deviceInformation = deviceInformations.GetAt(0);
    auto serialDevice = SerialDevice::FromIdAsync(deviceInformation.Id()).get();
    serialDevice.BaudRate(options.BaudRate);
    _serialWriter = DataWriter(serialDevice.OutputStream());

    _ledSyncDuration = options.LedSyncDuration;
  }

  bool AdaLightController::IsConnected()
  {
    return _serialWriter != nullptr;
  }

  std::array<float, 256> make_gamma(float gamma)
  {
    std::array<float, 256> values;
    for (auto i = 0u; i < 256; i++)
    {
      values[i] = pow(i / 255.f, gamma) * 255.f;
    }
    return values;
  }

  std::array<float, 256> _gammaMapping = make_gamma(1.6f);

  void AdaLightController::Push(const std::vector<Colors::rgb>& colors)
  {
    if (!_serialWriter) throw hresult_illegal_method_call(L"Cannot push colors if no device is connected");

    auto now = steady_clock::now();
    auto timeSyncLastUpdate = now - _lastUpdate;
    if (timeSyncLastUpdate < _ledSyncDuration)
    {
      this_thread::sleep_for(_ledSyncDuration - timeSyncLastUpdate);
      _lastUpdate = steady_clock::now();
    }
    else
    {
      _lastUpdate = now;
    }

    auto length = 6 + colors.size() * 3;

    vector<uint8_t> messsage;
    messsage.reserve(length);

    messsage.push_back(0x41);
    messsage.push_back(0x64);
    messsage.push_back(0x61);

    auto adjustedLedCount = colors.size() - 1;
    auto highCount = (uint8_t)(adjustedLedCount >> 8);
    auto lowCount = (uint8_t)(adjustedLedCount & 0xff);
    auto checksumCount = (uint8_t)(highCount ^ lowCount ^ 0x55);
    
    messsage.push_back(highCount);
    messsage.push_back(lowCount);
    messsage.push_back(checksumCount);

    float3 target{}, error{}, actual{}, correction;
    for (auto& color : colors)
    {
      target = {
        _gammaMapping[color.r],
        _gammaMapping[color.g],
        _gammaMapping[color.b]
      };

      actual = {
        round(target.x),
        round(target.y),
        round(target.z),
      };

      error += target - actual;

      correction = {
        round(error.x),
        round(error.y),
        round(error.z)
      };

      actual += correction;
      error -= correction;

      messsage.push_back((uint8_t)actual.x);
      messsage.push_back((uint8_t)actual.y);
      messsage.push_back((uint8_t)actual.z);
    }

    _serialWriter.WriteBytes(messsage);
    _serialWriter.StoreAsync().get();
  }
}