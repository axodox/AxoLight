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
  //if (any(color.rgb))
  {
    if (is_hdr)
    {
      color.rgb /= 2;
    }
    color.rgb = min(color.rgb, float3(1, 1, 1));

    float minimum = min(color.r, min(color.g, color.b));
    float maximum = max(color.r, max(color.g, color.b));
    float brightness = (minimum + maximum) / 2.f;

    float weight = 1;// brightness * 255;

    uint4 value = uint4(255 * color.r * weight, 255 * color.g * weight, 255 * color.b * weight, weight);
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