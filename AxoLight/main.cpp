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

struct light_constants_t
{
  array<uint32_t, 2> size;
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

  MONITORINFOEX x;
  x.cbSize = sizeof(MONITORINFOEX);

  GetMonitorInfo(desc.Monitor, &x);

  DISPLAY_DEVICE dev = {};
  dev.cb = sizeof(dev);
  EnumDisplayDevices(desc.DeviceName, 0, &dev, 0);

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

  constants_t constants{
    displaySettings.SampleSize / -2,
    displaySettings.SampleSize / 16
  };
  auto constantBuffer = d3d11_constant_buffer<constants_t>::make_dynamic(renderer.device);
  constantBuffer.update(renderer.context, constants);

  auto lightnessConstantsBuffer = d3d11_constant_buffer<light_constants_t>::make_dynamic(renderer.device);

  vector<float4> points;
  for (auto& point : displaySettings.SamplePoints)
  {
    points.push_back({ point.x, point.y, 0, 0 });
  }

  auto samplePoints = d3d11_structured_buffer<float4>::make_immutable(renderer.device, points);
  auto ledColorSums = d3d11_structured_buffer<array<uint32_t, 4>>::make_writeable(renderer.device, (uint32_t)displaySettings.SamplePoints.size());
  //auto samplerShader = d3d11_compute_shader(renderer.device, root / L"SamplerComputeShader.cso");
  auto downsamplingShader = d3d11_compute_shader(renderer.device, root / L"DownsamplingComputeShader.cso");
  auto lightnessShader = d3d11_compute_shader(renderer.device, root / L"LightnessComputeShader.cso");
  auto ledColorStage = d3d11_structured_buffer<array<uint32_t, 4>>::make_staging(renderer.device, (uint32_t)displaySettings.SamplePoints.size());

 /* vector<uint32_t> pixels(16);
  memset(pixels.data(), 127, pixels.size() * 4);
  auto texture = d3d11_texture_2d::make_immutable<uint32_t>(renderer.device, DXGI_FORMAT_B8G8R8A8_UNORM, 4, 4, pixels);*/

  std::unique_ptr<d3d11_structured_buffer<array<uint32_t, 4>>> downsampledScreen;

  std::vector<rgb> lights;
  lights.reserve(displaySettings.SamplePoints.size());
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
    D3D11_TEXTURE2D_DESC inputDesc;
    texture.texture->GetDesc(&inputDesc);

    auto sampleSize = (inputDesc.Width * inputDesc.Height) / 64 / 64;

    if (!downsampledScreen || downsampledScreen->capacity != sampleSize)
    {
      downsampledScreen = make_unique<d3d11_structured_buffer<array<uint32_t, 4>>>(d3d11_structured_buffer<array<uint32_t, 4>>::make_writeable(renderer.device, sampleSize));

      light_constants_t lightnessConstants = { { inputDesc.Width / 64, inputDesc.Height / 64 } };
      lightnessConstantsBuffer.update(renderer.context, lightnessConstants);
    }

    texture.set(renderer.context, d3d11_shader_stage::cs);
    downsampledScreen->set_writeable(renderer.context);

    array<uint32_t, 2> downsampledSize = { 
      (uint32_t)ceil(inputDesc.Width / 64.0), 
      (uint32_t)ceil(inputDesc.Height / 64.0) 
    };
    downsamplingShader.run(renderer.context, downsampledSize[0], downsampledSize[1]);

    lightnessConstantsBuffer.set(renderer.context, d3d11_shader_stage::cs);    
    samplePoints.set_readonly(renderer.context, 1);
    ledColorSums.set_writeable(renderer.context);
    downsampledScreen->set_readonly(renderer.context);

    array<UINT, 4> clearValue = {};
    renderer.context->ClearUnorderedAccessViewUint(ledColorSums.uav.get(), clearValue.data());
    lightnessShader.run(renderer.context,
      (uint32_t)ceil(downsampledSize[0] / 4.0),
      (uint32_t)ceil(downsampledSize[1] / 4.0),
      (uint32_t)ceil(displaySettings.SamplePoints.size() / 16.0));

    //
    /*sampler.set(renderer.context, d3d11_shader_stage::cs);
    texture.set(renderer.context, d3d11_shader_stage::cs);
    samplePoints.set_readonly(renderer.context, 1);
    ledColorSums.set_writeable(renderer.context);
    constantBuffer.set(renderer.context, d3d11_shader_stage::cs);
    samplerShader.run(renderer.context, (uint32_t)displaySettings.SamplePoints.size());*/
    ledColorSums.copy_to(renderer.context, ledColorStage);
    auto data = ledColorStage.get_data(renderer.context);

    lights.clear();
    for (auto& light : data)
    {
      auto count = max(light[3], 1u);
      lights.push_back({ uint8_t(light[0] / count), uint8_t(light[1] / count), uint8_t(light[2] / count) });
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
