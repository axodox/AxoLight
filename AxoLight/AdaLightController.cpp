#include "pch.h"
#include "AdaLightController.h"

using namespace std;
using namespace std::chrono;

using namespace winrt;
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

  void AdaLightController::Push(const std::vector<RGB>& colors)
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

    for (auto& color : colors)
    {
      messsage.push_back(color.R);
      messsage.push_back(color.G);
      messsage.push_back(color.B);
    }

    _serialWriter.WriteBytes(messsage);
    _serialWriter.StoreAsync().get();
  }
}