SamplerState _sampler : register(s0);
Texture2D _texture : register(t0);
StructuredBuffer<float4> _sampleRects : register(t1);
RWStructuredBuffer<uint4> _sumTexture : register(u0);

groupshared float4 _sampleRect;
groupshared float2 _sampleStep;
groupshared uint4 _sum = uint4(0, 0, 0, 0);

cbuffer constants : register(b0)
{
  bool is_hdr;
}
            
float3 rgb_to_hsl(float3 rgb)
{
  float3 result;

  float maximum = max(max(rgb.x, rgb.y), rgb.z);
  float minimum = min(min(rgb.x, rgb.y), rgb.z);

  float diff = maximum - minimum;
  result.z = (maximum + minimum) / 2.f;
  if (abs(diff) < 0.00001f)
  {
    result.y = 0.f;
    result.x = 0.f;
  }
  else
  {
    if (result.z <= 0.5f)
    {
      result.y = diff / (maximum + minimum);
    }
    else
    {
      result.y = diff / (2.f - maximum - minimum);
    }

    float3 dist = (maximum - rgb) / diff;

    if (rgb.x == maximum)
    {
      result.x = dist.z - dist.y;
    }
    else if (rgb.y == maximum)
    {
      result.x = 2.f + dist.x - dist.z;
    }
    else
    {
      result.x = 4.f + dist.y - dist.x;
    }

    result.x *= 60.f;
    if (result.x < 0.f) result.x += 360.f;
  }

  return result;
}

float qqh_to_rgb(float q1, float q2, float hue)
{
  if (hue > 360.f) hue -= 360.f;
  else if (hue < 0.f) hue += 360.f;

  if (hue < 60.f) return q1 + (q2 - q1) * hue / 60.f;
  if (hue < 180.f) return q2;
  if (hue < 240.f) return q1 + (q2 - q1) * (240.f - hue) / 60.f;
  return q1;
}

float3 hsl_to_rgb(float3 hsl)
{
  float p2;
  if (hsl.z <= 0.5f) p2 = hsl.z * (1 + hsl.y);
  else p2 = hsl.z + hsl.y - hsl.z * hsl.y;

  float p1 = 2.f * hsl.z - p2;
  float3 rgb;
  if (hsl.y == 0)
  {
    rgb = hsl.z;
  }
  else
  {
    rgb = float3(
      qqh_to_rgb(p1, p2, hsl.x + 120.f),
      qqh_to_rgb(p1, p2, hsl.x),
      qqh_to_rgb(p1, p2, hsl.x - 120.f)
      );
  }

  return rgb;
}
#define PI 3.141592653589793

float ease(float t, float p0, float p1, float max = 1)
{
  if (t < p0) return 0;
  if (t > p1) return max;

  float x = 2 * ((t - p0) / (p1 - p0) - 0.5);
  return 0.5 * (sin(x * 2 / PI) + 1) * max;
}

#define SAMPLE_POINTS 32

[numthreads(SAMPLE_POINTS, SAMPLE_POINTS, 1)]
void main(
  uint3 groupId : SV_GroupID,
  uint3 threadId : SV_GroupThreadID)
{
  bool isLeader = !any(threadId);
  if (isLeader)
  {
    _sampleRect = _sampleRects[groupId.x];
    _sampleStep = (_sampleRect.zy - _sampleRect.xw) / SAMPLE_POINTS;
  }
  GroupMemoryBarrierWithGroupSync();

  float2 samplePoint = float2(0, 1) + float2(1, -1) * (_sampleRect.xw + _sampleStep * threadId.xy);

  float4 color = _texture.SampleLevel(_sampler, samplePoint, 0);
  if (any(color.rgb))
  {
    if (is_hdr)
    {
      color.rgb /= 2;
    }
    color.rgb = min(color.rgb, float3(1, 1, 1));

    float3 hsl = rgb_to_hsl(color.rgb);

    float factor = 255 * ease(hsl.z, 0.05, 0.5);// * (0.5 + 0.5 * hsl.y);
    hsl.y = 1 - pow(hsl.y - 1, 2);
    hsl.z = 1 - pow(hsl.z - 1, 2);//ease(hsl.z, 0, 0.7);
    
    color.rgb = hsl_to_rgb(hsl);
    
    uint4 value = uint4(255 * color.r, 255 * color.g, 255 * color.b, 1) * factor;
    InterlockedAdd(_sum.x, value.x);
    InterlockedAdd(_sum.y, value.y);
    InterlockedAdd(_sum.z, value.z);
    InterlockedAdd(_sum.w, value.w);
  }

  GroupMemoryBarrierWithGroupSync();
  if (isLeader)
  {
    uint4 value = _sum.w > 0 ? _sum.xyzw / _sum.w : 0;
    _sumTexture[groupId.x] = value;
  }
}