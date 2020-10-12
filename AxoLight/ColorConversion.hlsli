
float3 RemoveREC2084Curve(float3 N)
{
  float m1 = 2610.0 / 4096.0 / 4;
  float m2 = 2523.0 / 4096.0 * 128;
  float c1 = 3424.0 / 4096.0;
  float c2 = 2413.0 / 4096.0 * 32;
  float c3 = 2392.0 / 4096.0 * 32;
  float3 Np = pow(N, 1 / m2);
  return pow(max(Np - c1, 0) / (c2 - c3 * Np), 1 / m1);
}

float3 ApplyREC2084Curve(float3 L)
{
  float m1 = 2610.0 / 4096.0 / 4;
  float m2 = 2523.0 / 4096.0 * 128;
  float c1 = 3424.0 / 4096.0;
  float c2 = 2413.0 / 4096.0 * 32;
  float c3 = 2392.0 / 4096.0 * 32;
  float3 Lp = pow(L, m1);
  return pow((c1 + c2 * Lp) / (1 + c3 * Lp), m2);
}

float3 ApplyREC709Curve(float3 x)
{
  return x < 0.0181 ? 4.5 * x : 1.0993 * pow(x, 0.45) - 0.0993;
}

float3 RemoveREC709Curve(float3 x)
{
  return x < 0.08145 ? x / 4.5 : pow((x + 0.0993) / 1.0993, 1.0 / 0.45);
}

inline float3 ApplysRGBCurve(float3 x)
{
  return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

float3 RemoveSRGBCurve(float3 x)
{
  // Approximately pow(x, 2.2)
  return x < 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
}

float3 REC2020toREC709(float3 RGB2020)
{
  static const float3x3 ConvMat =
  {
      1.660496, -0.587656, -0.072840,
      -0.124547, 1.132895, -0.008348,
      -0.018154, -0.100597, 1.118751
  };
  return mul(ConvMat, RGB2020);
}

float3 REC709toREC2020(float3 RGB709)
{
  static const float3x3 ConvMat =
  {
      0.627402, 0.329292, 0.043306,
      0.069095, 0.919544, 0.011360,
      0.016394, 0.088028, 0.895578
  };
  return mul(ConvMat, RGB709);
}

float3 MapColors(float3 color)
{
  static const float3x3 ConvMat =
  {
      1, 0, 0,
      0, 1, 0,
      0, 0, 1
  };
  return mul(ConvMat, color);
}