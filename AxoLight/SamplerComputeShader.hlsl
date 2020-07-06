SamplerState _sampler : register(s0);
Texture2D _texture : register(t0);
StructuredBuffer<float2> _samplePoints : register(t1);
RWStructuredBuffer<uint4> _sumTexture : register(u0);

cbuffer constants_t: register(b0)
{
  float2 SampleOffset;
  float2 SampleStep;
};

groupshared float2 _samplePoint;
groupshared uint4 _sum = uint4(0, 0, 0, 0);

[numthreads(8, 8, 1)]
void main( 
  uint3 groupId : SV_GroupID,
  uint3 threadId : SV_GroupThreadID)
{
  bool isLeader = !any(threadId);
  if (isLeader)
  {
    _samplePoint = _samplePoints[groupId.x];
  }
  GroupMemoryBarrierWithGroupSync();

  float2 offsetPoint = _samplePoint + SampleOffset + SampleStep * threadId.xy;

  if (offsetPoint.x >= 0 && offsetPoint.y >= 0 && offsetPoint.x <= 1 && offsetPoint.y <= 1)
  {
    float4 color = _texture.SampleLevel(_sampler, offsetPoint, 0);

    uint4 value = uint4(255 * color.r, 255 * color.g, 255 * color.b, 1);
    InterlockedAdd(_sum.x, value.x);
    InterlockedAdd(_sum.y, value.y);
    InterlockedAdd(_sum.z, value.z);
    InterlockedAdd(_sum.w, 1);
  }

  GroupMemoryBarrierWithGroupSync();
  if (isLeader)
  {
    uint4 value = _sum.xyzw / _sum.w;
    _sumTexture[groupId.x] = value;
  }
}