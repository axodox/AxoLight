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
using Windows.Foundation.Numerics;
using System.Numerics;
using Windows.Media.Devices;

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

      Vector3 error = new Vector3(), target;
      foreach (var color in colors)
      {
        target = new Vector3(
          ToneMap(color.R),
          ToneMap(color.G),
          ToneMap(color.B));
        var actual = new Vector3(
          (float)Math.Round((float)color.R),
          (float)Math.Round((float)color.G),
          (float)Math.Round((float)color.B));

        error += target - actual;

        var correctionX = (float)Math.Round(error.X);
        actual.X += correctionX;
        error.X -= correctionX;

        var correctionY = (float)Math.Round(error.Y);
        actual.Y += correctionY;
        error.Y -= correctionY;

        var correctionZ = (float)Math.Round(error.Z);
        actual.Z += correctionZ;
        error.Z -= correctionZ;

        stream.WriteByte((byte)actual.X);
        stream.WriteByte((byte)actual.Y);
        stream.WriteByte((byte)actual.Z);
      }

      _dataWriter.WriteBytes(stream.ToArray());
      await _dataWriter.StoreAsync();
    }

    static float[] MakeGamma(float gamma)
    {
      var values = new float[256]; 
      for (var i = 0; i <= 255; i++)
      {
        values[i] = (float)Math.Pow(i / 255f, gamma) * 255f;
      }
      return values;
    }

    static float[] _gammaF = MakeGamma(1.6f);

    private static float ToneMap(byte p)
    {
      return _gammaF[p];
    }
  }
}
