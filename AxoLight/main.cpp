#include "pch.h"
#include "AdaLightController.h"
#include "Graphics.h"
#include "Infrastructure.h"
#include "SettingsImporter.h"
#include "Colors.h"

using namespace AxoLight::Display;
using namespace AxoLight::Colors;
using namespace AxoLight::Graphics;
using namespace AxoLight::Infrastructure;
using namespace AxoLight::Lighting;
using namespace AxoLight::Settings;

using namespace std;
using namespace std::filesystem;
using namespace std::chrono_literals;

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Numerics;

struct constants_t
{
  float2 SampleOffset;
  float2 SampleStep;
};

union rect
{
  struct {
    float left, top, right, bottom;
  };
  struct {
    float2 top_left, bottom_right;
  };

  rect(float top, float left, float bottom, float right) :
    left(left),
    top(top),
    right(right),
    bottom(bottom)
  { }

  rect(float2 topLeft, float2 bottomRight) :
    left(topLeft.x),
    top(topLeft.y),
    right(bottomRight.x),
    bottom(bottomRight.y)
  { }

  bool intersects(const rect& other) const
  {
    return (left < other.right && right > other.left && top > other.bottom && bottom < other.top);
  }

  float2 center() const
  {
    return (top_left + bottom_right) / 2.f;
  }
};

struct SamplingDescription
{
  std::vector<rect> Rects;
  vector<vector<pair<uint16_t, float>>> RectFactors;

  static SamplingDescription Create(const DisplaySettings& settings, size_t verticalDivisions = 32)
  {
    auto horizontalDivisions = size_t(verticalDivisions * settings.AspectRatio);

    //
    vector<rect> lightRects;
    lightRects.reserve(settings.SamplePoints.size());
    auto sampleOffset = settings.SampleSize / float2(2.f, -2.f);
    for (auto& samplePoint : settings.SamplePoints)
    {
      lightRects.push_back({ samplePoint - sampleOffset, samplePoint + sampleOffset });
    }

    //
    vector<vector<pair<uint16_t, float>>> lightsDisplayRectFactors(settings.SamplePoints.size());

    vector<rect> displayRects;
    displayRects.reserve(verticalDivisions * horizontalDivisions);

    float2 step{ 1.f / horizontalDivisions, 1.f / verticalDivisions };
    rect displayRect{ step.y, 0, 0, step.x };
    for (size_t y = 0u; y < verticalDivisions; y++)
    {
      displayRect.left = 0;
      displayRect.right = step.x;

      for (size_t x = 0u; x < horizontalDivisions; x++)
      {
        uint16_t lightIndex = 0u;
        bool isUsed = false;
        for (auto& sampleRect : lightRects)
        {
          if (sampleRect.intersects(displayRect))
          {
            isUsed = true;

            auto weight = 1.f - length((displayRect.center() - sampleRect.center()) / settings.SampleSize);
            lightsDisplayRectFactors[lightIndex].push_back({ displayRects.size(), weight });
          }

          lightIndex++;
        }

        if (isUsed)
        {
          displayRects.push_back(displayRect);
        }

        displayRect.left += step.x;
        displayRect.right += step.x;
      }

      displayRect.top += step.y;
      displayRect.bottom += step.y;
    }

    //
    for (auto& lightDisplayRectFactors : lightsDisplayRectFactors)
    {
      auto totalWeight = 0.f;
      for (auto& displayRectFactor : lightDisplayRectFactors)
      {
        totalWeight += displayRectFactor.second;
      }

      for (auto& displayRectFactor : lightDisplayRectFactors)
      {
        displayRectFactor.second /= totalWeight;
      }
    }

    return { displayRects, lightsDisplayRectFactors };
  }
};



int main()
{
  init_apartment();

  auto root = get_root();
  auto settings = SettingsImporter::Parse(root / L"settings.json");
  auto displaySettings = DisplaySettings::FromLayout(settings.LightLayout);

  AdaLightController controller{ settings.ControllerOptions };
  if (!controller.IsConnected()) return 0;

  auto output = get_default_output();

  DXGI_OUTPUT_DESC1 desc;
  output.as<IDXGIOutput6>()->GetDesc1(&desc);

  com_ptr<IDXGIAdapter> adapter;
  check_hresult(output->GetParent(__uuidof(IDXGIAdapter), adapter.put_void()));

  /*MONITORINFOEX x;
  x.cbSize = sizeof(MONITORINFOEX);

  GetMonitorInfo(desc.Monitor, &x);

  DISPLAY_DEVICE dev = {};
  dev.cb = sizeof(dev);
  EnumDisplayDevices(desc.DeviceName, 0, &dev, 0);*/

#ifndef NDEBUG
  auto window = create_debug_window();

  d3d11_renderer_with_swap_chain renderer(adapter, window);

  auto vertexShader = d3d11_vertex_shader(renderer.device, root / L"SimpleVertexShader.cso");
  auto pixelShader = d3d11_pixel_shader(renderer.device, root / L"SimplePixelShader.cso");

  auto quad = Primitives::make_quad(renderer.device);
  vertexShader.set_input_layout(quad.vertex_buffer.input_desc());

  auto blendState = d3d11_blend_state(renderer.device, d3d11_blend_type::opaque);
  auto rasterizerState = d3d11_rasterizer_state(renderer.device, d3d11_rasterizer_type::cull_none);
#else
  d3d11_renderer renderer(adapter);
#endif // NDEBUG  

  auto duplication = d3d11_desktop_duplication(renderer.device, output);

  auto sampler = d3d11_sampler_state(renderer.device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);

  /*constants_t constants{
    displaySettings.SampleSize / -2,
    displaySettings.SampleSize / 16
  };
  auto constantBuffer = d3d11_constant_buffer<constants_t>::make_dynamic(renderer.device);
  constantBuffer.update(renderer.context, constants);*/

  auto samplingDescription = SamplingDescription::Create(displaySettings);

  auto samplePoints = d3d11_structured_buffer<rect>::make_immutable(renderer.device, samplingDescription.Rects);
  auto ledColorSums = d3d11_structured_buffer<array<uint32_t, 4>>::make_writeable(renderer.device, samplingDescription.Rects.size());
  auto samplerShader = d3d11_compute_shader(renderer.device, root / L"SamplerComputeShader.cso");
  auto ledColorStage = d3d11_structured_buffer<array<uint32_t, 4>>::make_staging(renderer.device, samplingDescription.Rects.size());

  /* vector<uint32_t> pixels(16);
   memset(pixels.data(), 127, pixels.size() * 4);
   auto texture = d3d11_texture_2d::make_immutable<uint32_t>(renderer.device, DXGI_FORMAT_B8G8R8A8_UNORM, 4, 4, pixels);*/

  std::vector<rgb> lights(displaySettings.SamplePoints.size());
  while (true)
  {
    auto& texture = duplication.lock_frame(1000u, [&] {
      controller.Push(lights);
      });

#ifndef NDEBUG
    auto& target = renderer.render_target();
    target.set(renderer.context);
    target.clear(renderer.context, { 1.f, 0.f, 0.f, 1.f });

    vertexShader.set(renderer.context);
    pixelShader.set(renderer.context);
    texture.set(renderer.context, d3d11_shader_stage::ps);
    sampler.set(renderer.context, d3d11_shader_stage::ps);
    blendState.set(renderer.context);
    rasterizerState.set(renderer.context);
    quad.draw(renderer.context);
#endif

    //
    sampler.set(renderer.context, d3d11_shader_stage::cs);
    texture.set(renderer.context, d3d11_shader_stage::cs);
    samplePoints.set_readonly(renderer.context, 1);
    ledColorSums.set_writeable(renderer.context);
    //constantBuffer.set(renderer.context, d3d11_shader_stage::cs);
    samplerShader.run(renderer.context, (uint32_t)samplingDescription.Rects.size());
    ledColorSums.copy_to(renderer.context, ledColorStage);
    auto data = ledColorStage.get_data(renderer.context);

    auto it = lights.begin();
    for (auto rectFactors : samplingDescription.RectFactors)
    {
      float4 color{};
      for (auto rectFactor : rectFactors)
      {
        auto& sample = data[rectFactor.first];

        color += float4(sample[0], sample[1], sample[2], sample[3]) * rectFactor.second;
      }

      rgb newColor{ uint8_t(color.x), uint8_t(color.y), uint8_t(color.z) };
      //newColor.apply_gamma(3.f);
      *it++ = lerp(*it, newColor, 0.5f);
    }
        
    enhance(lights);
    controller.Push(lights);

#ifndef NDEBUG
    renderer.swap_chain->Present(1, 0);
#endif

    duplication.unlock_frame();
  }

  return 0;
}
