StructuredBuffer<uint4> _inputTexture : register(t0);
StructuredBuffer<float4> _samplePoints : register(t1);
RWStructuredBuffer<uint4> _outputTexture : register(u0);

cbuffer constants_t: register(b0)
{
  uint2 _size;
};

static const uint _pixelGroupSize = 4;
static const uint _lightGroupSize = 16;
static const float _lightnessScale = 2;

groupshared uint4 _pixels[_pixelGroupSize][_pixelGroupSize];
groupshared uint2 _locations[_pixelGroupSize][_pixelGroupSize];
groupshared uint2 _lights[_lightGroupSize];

float falloff(float x)
{
  return 1 / pow(1 + x * 80, 2);
}

//run(pixelGroupCountX, pixelGroupCountY, lightGroupCount)
[numthreads(_pixelGroupSize, _pixelGroupSize, _lightGroupSize)]
void main(
  uint3 groupId : SV_GroupID,
  uint3 threadId : SV_GroupThreadID)
{
  uint lightIndex = threadId.z + _lightGroupSize * groupId.z;

  if (!any(threadId.xy))
  {
    _lights[threadId.z] = _samplePoints[lightIndex].xy * _size;
  }

  if (threadId.z == 0)
  {
    uint2 location = groupId.xy * _pixelGroupSize + threadId.xy;
    _locations[threadId.x][threadId.y] = location;
    _pixels[threadId.x][threadId.y] = _inputTexture[location.y * _size.x + location.x];
  }
  GroupMemoryBarrierWithGroupSync();

  {
    uint4 pixel = _pixels[threadId.x][threadId.y];

    if (pixel.a == 0) return;
    uint2 location = _locations[threadId.x][threadId.y];
    uint2 light = _lights[threadId.z];

    float distance = length(float2(light) - float2(location)) / (float)_size.x;
    
    float maxBrightness = max(max(pixel.r, pixel.g), pixel.b) / 255.0;
    float minBrightness = min(min(pixel.r, pixel.g), pixel.b) / 255.0;
    float lightness = (maxBrightness + minBrightness) / 2;
    /*float saturation = lightness > 0.5 ? (maxBrightness - minBrightness) / (2 - maxBrightness - minBrightness) : (maxBrightness - minBrightness) / (maxBrightness + minBrightness);*/

    /*float saturationFactor = (1 + saturation) / 2.0;

    uint weight = falloff(distance) * saturationFactor * 255;*/

    float lightnessFactor = max(0, 1 / (1 + pow(2 * (1 - lightness), _lightnessScale)) - 1.0 / (1+ pow(3, _lightnessScale)));
    uint weight = falloff(distance) * lightnessFactor * 255;
    //uint weight = falloff(distance) * 255;

    InterlockedAdd(_outputTexture[lightIndex].r, pixel.r * weight);
    InterlockedAdd(_outputTexture[lightIndex].g, pixel.g * weight);
    InterlockedAdd(_outputTexture[lightIndex].b, pixel.b * weight);
    InterlockedAdd(_outputTexture[lightIndex].a, weight);
  }
}