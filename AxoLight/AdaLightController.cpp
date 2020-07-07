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

  std::array<uint8_t, 256> _gamma8 = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

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

    for (auto& color : colors)
    {
      messsage.push_back(_gamma8[color.g]);
      messsage.push_back(_gamma8[color.r]);
      messsage.push_back(_gamma8[color.b]);
    }

    _serialWriter.WriteBytes(messsage);
    _serialWriter.StoreAsync().get();
  }
}