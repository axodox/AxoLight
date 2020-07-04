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
      serialDevice.BaudRate = 115200;
            
      var dataWriter = new DataWriter(serialDevice.OutputStream);

      var watch = new Stopwatch();
      var frame = 0;
      while (true)
      {
        watch.Start();
        ushort count = 20;
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
          var s = ((double)(i + frame / 25.0) % (count - 1)) / (count - 1);
          var color = HSLToRGB(s, 1, 0.25, 1);
          //var p = ((count - 1 - i) / (count - 1.0)) * 255;
          //var q = ToneMap(p);
          //var x = (byte)(q);
          //Console.WriteLine(x);

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
        watch.Stop();
        var time = watch.Elapsed;
        watch.Reset();
        frame++;
        if(frame % 10 == 0) Console.WriteLine(1 / time.TotalSeconds);
      }

      var dataReader = new DataReader(serialDevice.InputStream);
      await dataReader.LoadAsync(3);
      var text = dataReader.ReadString(3);
    }

    private static byte ToneMap(byte p)
    {
      return (byte)(Math.Pow(2, p / 255.0 * 8) - 1);
    }
  }
}
