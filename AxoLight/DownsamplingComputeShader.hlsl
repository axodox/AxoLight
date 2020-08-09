Texture2D _inputTexture : register(t0);
RWStructuredBuffer<uint4> _outputTexture : register(u0);

static const uint _subKernelSize = 4;
static const uint _superKernelSize = 16;
static const uint _totalKernelSize = _subKernelSize * _superKernelSize;

groupshared uint4 _kernelSamples;

[numthreads(_superKernelSize, _superKernelSize, 1)]
void main(
  uint3 groupId : SV_GroupID,
  uint3 threadId : SV_GroupThreadID)
{
  bool isLeader = !any(threadId);
  if (isLeader)
  {
    _kernelSamples = 0;
  }
  GroupMemoryBarrierWithGroupSync();

  uint2 inputSize;
  _inputTexture.GetDimensions(inputSize.x, inputSize.y);

  {
    uint2 origin = _totalKernelSize * groupId.xy + _subKernelSize * threadId.xy;

    uint3 sum = 0;
    uint count = 0;
    for (uint x = 0; x < _subKernelSize; x++)
    {
      for (uint y = 0; y < _subKernelSize; y++)
      {
        uint2 position = origin + uint2(x, y);
        
        if (position.x < inputSize.x && position.y < inputSize.y)
        {
          uint3 color = _inputTexture.Load(uint3(position.x, position.y, 0)).rgb * 255;
          //if (any(color))
          {
            sum += color;
            count++;
          }
        }
      }
    }

    InterlockedAdd(_kernelSamples.x, sum.x);
    InterlockedAdd(_kernelSamples.y, sum.y);
    InterlockedAdd(_kernelSamples.z, sum.z);
    InterlockedAdd(_kernelSamples.w, count);
  }

  GroupMemoryBarrierWithGroupSync();
  if (!any(threadId))
  {
    uint2 outputSize = inputSize / _totalKernelSize;

    uint3 average = _kernelSamples.w > 0 ? _kernelSamples.xyz / _kernelSamples.w : 0;
    _outputTexture[outputSize.x * groupId.y + groupId.x] = uint4(average, _kernelSamples.w);
  }
}