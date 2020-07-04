using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Devices.SerialCommunication;
using Windows.Storage.Streams;

namespace AdaLightTest
{
  class Program
  {
    struct RGBA
    {
      public byte R, G, B, A;

      public RGBA(byte r, byte g, byte b, byte a)
      {
        R = r;
        G = g;
        B = b;
        A = a;
      }

      static byte Scale(double x)
      {
        return (byte)Math.Round(x * 255d);
      }

      public RGBA(double r, double g, double b, double a)
      {
        R = Scale(r);
        G = Scale(g);
        B = Scale(b);
        A = Scale(a);
      }

      public RGBA(double i)
      {
        R = G = B = Scale(i);
        A = 255;
      }
    }

    static RGBA HSLToRGB(double h, double s, double l, double a)
    {
      double v;
      double r = 1d, g = 1d, b = 1d;
      if (h >= 1) h = 0.999999;
      v = (l <= 0.5) ? (l * (1d + s)) : (l + s - l * s);
      if (v >= 0d)
      {
        double m;
        double sv;
        double fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m) / v;
        h *= 6d;
        fract = h - Math.Truncate(h);
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        if (h < 1d)
        {
          r = v;
          g = mid1;
          b = m;
        }
        else if (h < 2d)
        {
          r = mid2;
          g = v;
          b = m;
        }
        else if (h < 3d)
        {
          r = m;
          g = v;
          b = mid1;
        }
        else if (h < 4d)
        {
          r = m;
          g = mid2;
          b = v;
        }
        else if (h < 5d)
        {
          r = mid1;
          g = m;
          b = v;
        }
        else
        {
          r = v;
          g = m;
          b = mid2;
        }
      }
      return new RGBA(r, g, b, a);
    }

    static async Task Main(string[] args)
    {
      var deviceSelector = SerialDevice.GetDeviceSelectorFromUsbVidPid(0x1A86, 0x7523);
      var deviceInformations = await DeviceInformation.FindAllAsync(deviceSelector);
      if (deviceInformations.Count == 0) return;

      var deviceInformation = deviceInformations.FirstOrDefault();
      var serialDevice = await SerialDevice.FromIdAsync(deviceInformation.Id);
      serialDevice.BaudRate = 1000000;
            
      var dataWriter = new DataWriter(serialDevice.OutputStream);

      var watch = new Stopwatch();
      var frame = 0;
      while (true)
      {
        watch.Start();
        ushort count = 156;
        //ushort count = 50;
        var messageLength = 6 + count * 3;

        var stream = new MemoryStream(messageLength);
        stream.WriteByte(0x41);
        stream.WriteByte(0x64);
        stream.WriteByte(0x61);
                
        var hi = (byte)((count - 1) >> 8);
        var lo = (byte)((count - 1) & 0xff);
        var checksum = (byte)(hi ^ lo ^ 0x55);
        stream.WriteByte(hi);
        stream.WriteByte(lo);
        stream.WriteByte(checksum);
        for (var i = 0; i < count; i++)
        {
          //var p = Math.Sin(Math.PI * ((i + frame / 100.0) % (count - 1)) / (count - 1)) * 255;
          //var s = ((double)(i + frame / 25.0) % (count - 1)) / (count - 1);
          var s = ((double)(i * 8 + frame) % (count - 1)) / (count - 1);
          //var color = HSLToRGB(s, 1, (frame % 2 > 0) ? 0.5 : 0, 1);
          var color = HSLToRGB(s, 1, 0.20, 1);
          //var p = ((count - 1 - i) / (count - 1.0)) * 255;
          //var q = ToneMap(p);
          //var x = (byte)(q);
          //Console.WriteLine(x);
          //var color = new RGBA(100, 100, 100, 0);
          //var y = (frame % 100) / 100.0 > 0.5 ? 0 : 1;
          //var x = (byte)(i % 2 == y ? 64 : 255);
          stream.WriteByte(ToneMap(color.R));
          //stream.WriteByte((byte)(frame % 255));
          //stream.WriteByte(1);
          stream.WriteByte(ToneMap(color.G));
          stream.WriteByte(ToneMap(color.B));
        }

        dataWriter.WriteBytes(stream.ToArray());
        await dataWriter.StoreAsync();
        Thread.Sleep(6);

        watch.Stop();
        var time = watch.Elapsed;
        watch.Reset();
        frame++;
        if (frame % 10 == 0) Console.WriteLine(1 / time.TotalSeconds);
      }

      var dataReader = new DataReader(serialDevice.InputStream);
      await dataReader.LoadAsync(3);
      var text = dataReader.ReadString(3);
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
