using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Devices.SerialCommunication;
using Windows.Storage.Streams;

namespace AxoLightCalibrator
{
  class AdaLightController
  {
    private readonly DataWriter _dataWriter;

    private AdaLightController(DataWriter dataWriter)
    {
      _dataWriter = dataWriter;
    }

    public static async Task<AdaLightController> Create()
    {
      var deviceSelector = SerialDevice.GetDeviceSelectorFromUsbVidPid(0x1A86, 0x7523);
      var deviceInformations = await DeviceInformation.FindAllAsync(deviceSelector);
      if (deviceInformations.Count == 0) return null;

      var deviceInformation = deviceInformations.FirstOrDefault();
      var serialDevice = await SerialDevice.FromIdAsync(deviceInformation.Id);
      serialDevice.BaudRate = 1000000;

      var dataWriter = new DataWriter(serialDevice.OutputStream);
      return new AdaLightController(dataWriter);
    }

    public async Task Push(IList<MainPage.RGB> colors)
    {
      var messageLength = 6 + colors.Count * 3;

      var stream = new MemoryStream(messageLength);
      stream.WriteByte(0x41);
      stream.WriteByte(0x64);
      stream.WriteByte(0x61);

      var hi = (byte)((colors.Count - 1) >> 8);
      var lo = (byte)((colors.Count - 1) & 0xff);
      var checksum = (byte)(hi ^ lo ^ 0x55);
      stream.WriteByte(hi);
      stream.WriteByte(lo);
      stream.WriteByte(checksum);
      foreach(var color in colors)
      {
        stream.WriteByte(ToneMap(color.R));
        stream.WriteByte(ToneMap(color.G));
        stream.WriteByte(ToneMap(color.B));
      }

      _dataWriter.WriteBytes(stream.ToArray());
      await _dataWriter.StoreAsync();
    }

    static byte[] _gamma8 = new byte[] {
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

    private static byte ToneMap(byte p)
    {
      //return (byte)(Math.Pow(2, p / 255.0 * 8) - 1);
      return _gamma8[p];
    }
  }
}
