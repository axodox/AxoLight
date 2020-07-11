using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Numerics;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Storage;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace AxoLightCalibrator
{
  /// <summary>
  /// An empty page that can be used on its own or navigated to within a Frame.
  /// </summary>
  public sealed partial class MainPage : Page, INotifyPropertyChanged
  {
    public event PropertyChangedEventHandler PropertyChanged;

    public struct HSL
    {
      public float H, S, L;
    }

    public struct RGB
    {
      public byte R, G, B;
    }

    public SolidColorBrush ReferenceBrush { get; set; }

    private HSL _referenceHSL;
    private RGB _referenceRGB;

    public RGB TargetRGB => _referenceRGB;

    public float Hue
    {
      get => _referenceHSL.H;
      set
      {
        _referenceHSL.H = value;
        UpdateReferenceColor();
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Hue)));
      }
    }

    public float Saturation
    {
      get => _referenceHSL.S * 255f;
      set
      {
        _referenceHSL.S = value / 255f;
        UpdateReferenceColor();
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Saturation)));
      }
    }

    public float Lightness
    {
      get => _referenceHSL.L * 255f;
      set
      {
        _referenceHSL.L = value / 255f;
        UpdateReferenceColor();
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Lightness)));
      }
    }

    static HSL RgbToHsl(RGB rgb)
    {
      HSL result;

      var floatRGB = new Vector3(rgb.R, rgb.G, rgb.B) / 255f;

      var max = Math.Max(Math.Max(floatRGB.X, floatRGB.Y), floatRGB.Z);
      var min = Math.Min(Math.Min(floatRGB.X, floatRGB.Y), floatRGB.Z);

      var diff = max - min;
      result.L = (max + min) / 2f;
      if (Math.Abs(diff) < 0.00001f)
      {
        result.S = 0f;
        result.H = 0f;
      }
      else
      {
        if (result.L <= 0.5f)
        {
          result.S = diff / (max + min);
        }
        else
        {
          result.S = diff / (2f - max - min);
        }

        var dist = (new Vector3(max) - floatRGB) / diff;

        if (floatRGB.X == max)
        {
          result.H = dist.Z - dist.Y;
        }
        else if (floatRGB.Y == max)
        {
          result.H = 2f + dist.X - dist.Z;
        }
        else
        {
          result.H = 4f + dist.Y - dist.X;
        }

        result.H = result.H * 60f;
        if (result.H < 0f) result.H += 360f;
      }

      return result;
    }

    float QqhToRgb(float q1, float q2, float hue)
    {
      if (hue > 360f) hue -= 360f;
      else if (hue < 0f) hue += 360f;

      if (hue < 60f) return q1 + (q2 - q1) * hue / 60f;
      if (hue < 180f) return q2;
      if (hue < 240f) return q1 + (q2 - q1) * (240f - hue) / 60f;
      return q1;
    }

    RGB HslToRgb(HSL hsl)
    {
      float p2;
      if (hsl.L <= 0.5f) p2 = hsl.L * (1 + hsl.S);
      else p2 = hsl.L + hsl.S - hsl.L * hsl.S;

      float p1 = 2f * hsl.L - p2;
      Vector3 floatRGB;
      if (hsl.S == 0)
      {
        floatRGB = new Vector3(hsl.L);
      }
      else
      {
        floatRGB = new Vector3(
          QqhToRgb(p1, p2, hsl.H + 120f),
          QqhToRgb(p1, p2, hsl.H),
          QqhToRgb(p1, p2, hsl.H - 120f)
        );
      }

      floatRGB *= 255f;
      return new RGB
      {
        R = (byte)floatRGB.X,
        G = (byte)floatRGB.Y,
        B = (byte)floatRGB.Z
      };
    }

    private void UpdateReferenceColor()
    {
      _referenceRGB = HslToRgb(_referenceHSL);
      ReferenceBrush = new SolidColorBrush(Color.FromArgb(255, _referenceRGB.R, _referenceRGB.G, _referenceRGB.B));
      PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ReferenceBrush)));
      PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(TargetRGB)));
    }

    public RGB LedColor;

    public byte Red
    {
      get => LedColor.R;
      set
      {
        LedColor.R = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(LedColor)));
      }
    }

    public byte Green
    {
      get => LedColor.G;
      set
      {
        LedColor.G = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(LedColor)));
      }
    }

    public byte Blue
    {
      get => LedColor.B;
      set
      {
        LedColor.B = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(LedColor)));
      }
    }

    private AdaLightController _controller;
    private DispatcherTimer _timer;

    public MainPage()
    {
      InitializeComponent();
      Initialize();
    }

    private async void Initialize()
    {
      _controller = await AdaLightController.Create();
      _timer = new DispatcherTimer();
      _timer.Tick += OnTimerTick;
      _timer.Interval = TimeSpan.FromMilliseconds(16);
      _timer.Start();
    }

    private async void SaveDataPoint()
    {
      var folder = KnownFolders.DocumentsLibrary;

      {
        var rgbMapping = $"{TargetRGB.R},{TargetRGB.G},{TargetRGB.B},{LedColor.R},{LedColor.G},{LedColor.B}\r\n";
        var rgbFile = await folder.CreateFileAsync("rgbMapping.csv", CreationCollisionOption.OpenIfExists);
        await FileIO.AppendTextAsync(rgbFile, rgbMapping);
      }

      {
        var hsl = RgbToHsl(LedColor);
        var hslMapping = $"{_referenceHSL.H}, {_referenceHSL.S}, {_referenceHSL.L},{hsl.H},{hsl.S},{hsl.L}\r\n";
        var hslFile = await folder.CreateFileAsync("hslMapping.csv", CreationCollisionOption.OpenIfExists);
        await FileIO.AppendTextAsync(hslFile, hslMapping);
      }      
    }

    private async void OnTimerTick(object sender, object e)
    {
      var leds = new RGB[156];
      for(var i = 0; i < leds.Length; i++)
      {
        leds[i] = LedColor;
      }
      await _controller.Push(leds);
    }
  }
}
