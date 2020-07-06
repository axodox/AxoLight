#include "pch.h"
#include "Graphics.h"
using namespace AxoLight::Infrastructure;

using namespace std;
using namespace winrt;

namespace AxoLight::Graphics
{
  winrt::com_ptr<IDXGIOutput2> get_default_output()
  {
    com_ptr<IDXGIFactory> dxgiFactory = nullptr;
    check_hresult(CreateDXGIFactory2(0, __uuidof(IDXGIFactory), dxgiFactory.put_void()));

    com_ptr<IDXGIAdapter> dxgiAdapter;
    for (uint32_t adapterIndex = 0u; dxgiFactory->EnumAdapters(adapterIndex, dxgiAdapter.put()) != DXGI_ERROR_NOT_FOUND; adapterIndex++)
    {
      DXGI_ADAPTER_DESC dxgiAdapterDesc;
      check_hresult(dxgiAdapter->GetDesc(&dxgiAdapterDesc));

      com_ptr<IDXGIOutput> dxgiOutput;
      for (uint32_t outputIndex = 0u; dxgiAdapter->EnumOutputs(outputIndex, dxgiOutput.put()) != DXGI_ERROR_NOT_FOUND; outputIndex++)
      {
        auto dxgiOutput2 = dxgiOutput.try_as<IDXGIOutput2>();
        if (dxgiOutput2) return dxgiOutput2;

        dxgiOutput = nullptr;
      }
      dxgiAdapter = nullptr;
    }

    return nullptr;
  }
  
  d3d11_renderer::d3d11_renderer(const tuple<com_ptr<ID3D11Device>, com_ptr<ID3D11DeviceContext>>& args) :
    device(get<0>(args)),
    context(get<1>(args))
  { }
  
  tuple<com_ptr<ID3D11Device>, com_ptr<ID3D11DeviceContext>> d3d11_renderer::args(const com_ptr<IDXGIAdapter>& dxgiAdapter)
  {
    D3D_FEATURE_LEVEL featureLevels[] =
    {
      D3D_FEATURE_LEVEL_12_1,
      D3D_FEATURE_LEVEL_12_0,
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0
    };

    uint32_t creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    tuple<com_ptr<ID3D11Device>, com_ptr<ID3D11DeviceContext>> args;
    check_hresult(D3D11CreateDevice(
      dxgiAdapter.get(),
      D3D_DRIVER_TYPE_UNKNOWN,
      nullptr,
      creationFlags,
      featureLevels,
      ARRAYSIZE(featureLevels),
      D3D11_SDK_VERSION,
      get<0>(args).put(),
      nullptr,
      get<1>(args).put()
    ));

    return args;
  }
  
  d3d11_renderer::d3d11_renderer(const com_ptr<IDXGIAdapter>& dxgiAdapter) : d3d11_renderer(args(dxgiAdapter))
  { }
  
  d3d11_resource::d3d11_resource(const com_ptr<ID3D11Resource>& resource) :
    resource(resource)
  { }
  
  void d3d11_resource::update(const com_ptr<ID3D11DeviceContext>& context, const array_view<uint8_t>& data) const
  {
    com_ptr<ID3D11Device> device;
    resource->GetDevice(device.put());

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    check_hresult(context->Map(resource.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource));
    memcpy(mappedSubresource.pData, data.data(), min(data.size(), mappedSubresource.DepthPitch));
    context->Unmap(resource.get(), 0);
  }
  
  com_ptr<ID3D11ShaderResourceView> d3d11_texture_2d::make_view(const com_ptr<ID3D11Texture2D>& texture)
  {    
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);

    if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
      com_ptr<ID3D11Device> device;
      texture->GetDevice(device.put());

      com_ptr<ID3D11ShaderResourceView> view;
      winrt::check_hresult(device->CreateShaderResourceView(texture.get(), nullptr, view.put()));
      return view;
    }
    else
    {
      return nullptr;
    }
  }
  
  d3d11_texture_2d::d3d11_texture_2d(const com_ptr<ID3D11Texture2D>& texture) :
    d3d11_resource(texture),
    texture(texture),
    view(make_view(texture))
  { }
  
  void d3d11_texture_2d::set(const com_ptr<ID3D11DeviceContext>& context, d3d11_shader_stage stage, uint32_t slot) const
  {
    const array<ID3D11ShaderResourceView*, 1> views = { view.get() };
    switch (stage)
    {
    case d3d11_shader_stage::cs:
      context->CSSetShaderResources(slot, 1, views.data());
      break;
    case d3d11_shader_stage::vs:
      context->VSSetShaderResources(slot, 1, views.data());
      break;
    case d3d11_shader_stage::ps:
      context->PSSetShaderResources(slot, 1, views.data());
      break;
    default:
      throw out_of_range("Invalid shader stage for texture!");
    }
  }
  
  com_ptr<ID3D11RenderTargetView> d3d11_render_target_2d::get_view(const com_ptr<ID3D11Texture2D>& texture)
  {
    com_ptr<ID3D11Device> device;
    texture->GetDevice(device.put());

    com_ptr<ID3D11RenderTargetView> view;
    check_hresult(device->CreateRenderTargetView(texture.get(), nullptr, view.put()));

    return view;
  }
  
  D3D11_VIEWPORT d3d11_render_target_2d::get_view_port(const com_ptr<ID3D11Texture2D>& texture)
  {
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    return { 0, 0, (float)desc.Width, (float)desc.Height, D3D11_MIN_DEPTH, D3D11_MAX_DEPTH };
  }
  
  d3d11_render_target_2d::d3d11_render_target_2d(const com_ptr<ID3D11Texture2D>& texture) :
    d3d11_texture_2d(texture),
    view(get_view(texture)),
    view_port(get_view_port(texture))
  { }
  
  void d3d11_render_target_2d::set(const com_ptr<ID3D11DeviceContext>& context) const
  {
    array<ID3D11RenderTargetView*, 1> targets = { view.get() };
    context->OMSetRenderTargets(1, targets.data(), nullptr);
    context->RSSetViewports(1, &view_port);
  }
  
  void d3d11_render_target_2d::clear(const com_ptr<ID3D11DeviceContext>& context, const array<float, 4>& color) const
  {
    context->ClearRenderTargetView(view.get(), color.data());
  }

  d3d11_renderer_with_swap_chain::d3d11_renderer_with_swap_chain(const args_t& args) :
    d3d11_renderer({ get<1>(args), get<2>(args) }),
    swap_chain(get<0>(args))
  { }

  d3d11_renderer_with_swap_chain::args_t d3d11_renderer_with_swap_chain::args(const com_ptr<IDXGIAdapter>& dxgiAdapter, const handle& windowHandle)
  {
    D3D_FEATURE_LEVEL featureLevels[] =
    {
      D3D_FEATURE_LEVEL_12_1,
      D3D_FEATURE_LEVEL_12_0,
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0
    };

    uint32_t creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = (HWND)windowHandle.get();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    args_t args;
    check_hresult(D3D11CreateDeviceAndSwapChain(
      dxgiAdapter.get(),
      D3D_DRIVER_TYPE_UNKNOWN,
      nullptr,
      creationFlags,
      featureLevels,
      ARRAYSIZE(featureLevels),
      D3D11_SDK_VERSION,
      &swapChainDesc,
      get<0>(args).put(),
      get<1>(args).put(),
      nullptr,
      get<2>(args).put()
    ));
    return args;
  }

  d3d11_renderer_with_swap_chain::d3d11_renderer_with_swap_chain(const com_ptr<IDXGIAdapter>& dxgiAdapter, const handle& windowHandle) : d3d11_renderer_with_swap_chain(args(dxgiAdapter, windowHandle))
  { }

  const d3d11_render_target_2d& d3d11_renderer_with_swap_chain::render_target()
  {
    com_ptr<ID3D11Texture2D> texture;
    check_hresult(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), texture.put_void()));

    auto& render_target = _buffers[texture.get()];
    if (!render_target)
    {
      render_target = make_unique<d3d11_render_target_2d>(texture);
    }

    return *render_target.get();
  }
  
  d3d11_buffer::d3d11_buffer(const com_ptr<ID3D11Buffer>& buffer) :
    d3d11_resource(buffer),
    buffer(buffer)
  { }
  
  com_ptr<ID3D11VertexShader> d3d11_vertex_shader::load_shader(const com_ptr<ID3D11Device>& device, const vector<uint8_t>& source)
  {
    com_ptr<ID3D11VertexShader> result;
    check_hresult(device->CreateVertexShader(source.data(), source.size(), nullptr, result.put()));
    return result;
  }
  
  d3d11_vertex_shader::d3d11_vertex_shader(const com_ptr<ID3D11Device>& device, const filesystem::path& path) :
    _source(load_file(path)),
    shader(load_shader(device, _source))
  { }
  
  void d3d11_vertex_shader::set_input_layout(const array_view<D3D11_INPUT_ELEMENT_DESC> & inputDesc)
  {
    if (_currentLayout.first == inputDesc.data()) return;

    _currentLayout.first = inputDesc.data();

    auto& layout = _inputLayouts[inputDesc.data()];
    if (!layout)
    {
      com_ptr<ID3D11Device> device;
      shader->GetDevice(device.put());

      check_hresult(device->CreateInputLayout(
        inputDesc.data(),
        inputDesc.size(),
        _source.data(), _source.size(), layout.put()));
    }
    _currentLayout.second = layout.get();
  }
  
  void d3d11_vertex_shader::set(const com_ptr<ID3D11DeviceContext>& context) const
  {
    context->VSSetShader(shader.get(), 0, 0);
    context->IASetInputLayout(_currentLayout.second);
  }
  
  com_ptr<ID3D11PixelShader> d3d11_pixel_shader::load_shader(const com_ptr<ID3D11Device>& device, const vector<uint8_t>& source)
  {
    com_ptr<ID3D11PixelShader> result;
    check_hresult(device->CreatePixelShader(source.data(), source.size(), nullptr, result.put()));
    return result;
  }
  
  d3d11_pixel_shader::d3d11_pixel_shader(const com_ptr<ID3D11Device>& device, const filesystem::path& path) :
    shader(load_shader(device, load_file(path)))
  { }
  
  void d3d11_pixel_shader::set(const com_ptr<ID3D11DeviceContext> & context) const
  {
    context->PSSetShader(shader.get(), 0, 0);
  }
  
  com_ptr<ID3D11ComputeShader> d3d11_compute_shader::load_shader(const com_ptr<ID3D11Device>& device, const vector<uint8_t>& source)
  {
    com_ptr<ID3D11ComputeShader> result;
    check_hresult(device->CreateComputeShader(source.data(), source.size(), nullptr, result.put()));
    return result;
  }
  
  d3d11_compute_shader::d3d11_compute_shader(const com_ptr<ID3D11Device>& device, const filesystem::path& path) :
    shader(load_shader(device, load_file(path)))
  { }
  
  void d3d11_compute_shader::run(const com_ptr<ID3D11DeviceContext> & context, uint32_t x, uint32_t y, uint32_t z) const
  {
    context->CSSetShader(shader.get(), 0, 0);
    context->Dispatch(x, y, z);
  }
  
  com_ptr<ID3D11RasterizerState> d3d11_rasterizer_state::make_state(const com_ptr<ID3D11Device>& device, d3d11_rasterizer_type type)
  {
    CD3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
    switch (type)
    {
    case d3d11_rasterizer_type::cull_none:
      rasterizerDesc.CullMode = D3D11_CULL_NONE;
      break;
    case d3d11_rasterizer_type::cull_clockwise:
      rasterizerDesc.CullMode = D3D11_CULL_FRONT;
      break;
    case d3d11_rasterizer_type::cull_counter_clockwise:
      rasterizerDesc.CullMode = D3D11_CULL_BACK;
      break;
    case d3d11_rasterizer_type::wireframe:
      rasterizerDesc.CullMode = D3D11_CULL_NONE;
      rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
      break;
    }

    com_ptr<ID3D11RasterizerState> result;
    check_hresult(device->CreateRasterizerState(&rasterizerDesc, result.put()));
    return result;
  }
  
  d3d11_rasterizer_state::d3d11_rasterizer_state(const com_ptr<ID3D11Device>& device, d3d11_rasterizer_type type) :
    state(make_state(device, type))
  { }
  
  void d3d11_rasterizer_state::set(const com_ptr<ID3D11DeviceContext> & context) const
  {
    context->RSSetState(state.get());
  }
  
  com_ptr<ID3D11BlendState> d3d11_blend_state::make_state(const com_ptr<ID3D11Device>& device, d3d11_blend_type type)
  {
    D3D11_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
    renderTargetBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    switch (type)
    {
    case d3d11_blend_type::opaque:
      renderTargetBlendDesc.BlendEnable = false;
      renderTargetBlendDesc.SrcBlend = D3D11_BLEND_ONE;
      renderTargetBlendDesc.DestBlend = D3D11_BLEND_ZERO;
      renderTargetBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
      renderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
      renderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
      renderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
      break;
    case d3d11_blend_type::additive:
    case d3d11_blend_type::subtractive:
      renderTargetBlendDesc.BlendEnable = true;
      renderTargetBlendDesc.SrcBlend = D3D11_BLEND_ONE;
      renderTargetBlendDesc.DestBlend = D3D11_BLEND_ONE;
      renderTargetBlendDesc.BlendOp = renderTargetBlendDesc.BlendOpAlpha = (type == d3d11_blend_type::additive ? D3D11_BLEND_OP_ADD : D3D11_BLEND_OP_REV_SUBTRACT);
      renderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
      renderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND_ONE;
      renderTargetBlendDesc.BlendOpAlpha = renderTargetBlendDesc.BlendOp;
      break;
    case d3d11_blend_type::alpha_blend:
      renderTargetBlendDesc.BlendEnable = true;
      renderTargetBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
      renderTargetBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
      renderTargetBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
      renderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
      renderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
      renderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
      break;
    }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0] = renderTargetBlendDesc;

    com_ptr<ID3D11BlendState> result;
    winrt::check_hresult(device->CreateBlendState(&blendDesc, result.put()));
    return result;
  }
  
  d3d11_blend_state::d3d11_blend_state(const com_ptr<ID3D11Device>& device, d3d11_blend_type type) :
    state(make_state(device, type))
  { }
  
  void d3d11_blend_state::set(const com_ptr<ID3D11DeviceContext> & context) const
  {
    context->OMSetBlendState(state.get(), nullptr, 0xffffffff);
  }
  
  com_ptr<ID3D11SamplerState> d3d11_sampler_state::make_state(const com_ptr<ID3D11Device>& device, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
  {
    float borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
    CD3D11_SAMPLER_DESC sd(
      filter,
      addressMode,
      addressMode,
      addressMode,
      0.f,
      0,
      D3D11_COMPARISON_NEVER,
      borderColor,
      0.f,
      D3D11_FLOAT32_MAX
    );

    com_ptr<ID3D11SamplerState> state;
    check_hresult(device->CreateSamplerState(&sd, state.put()));
    return state;
  }
  
  d3d11_sampler_state::d3d11_sampler_state(const com_ptr<ID3D11Device>& device, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode) :
    state(make_state(device, filter, addressMode))
  { }
  
  void d3d11_sampler_state::set(const com_ptr<ID3D11DeviceContext> & context, d3d11_shader_stage stage, int slot) const
  {
    const array<ID3D11SamplerState*, 1> samplerStates = { state.get() };

    switch (stage)
    {
    case d3d11_shader_stage::cs:
      context->CSSetSamplers(slot, 1, samplerStates.data());
      break;
    case d3d11_shader_stage::vs:
      context->VSSetSamplers(slot, 1, samplerStates.data());
      break;
    case d3d11_shader_stage::ps:
      context->PSSetSamplers(slot, 1, samplerStates.data());
      break;
    default:
      throw out_of_range("Invalid shader stage for texture!");
    }
  }

  namespace Primitives
  {
    d3d11_simple_mesh<d3d11_vertex_position_texture> make_quad(const com_ptr<ID3D11Device>& device, float size)
    {
      size = size / 2.f;
      array<d3d11_vertex_position_texture, 4> vertices = {
        d3d11_vertex_position_texture{ { -size, size, 0.f },{ 0.f, 0.f } },
        d3d11_vertex_position_texture{ { -size, -size, 0.f },{ 0.f, 1.f } },
        d3d11_vertex_position_texture{ { size, size, 0.f },{ 1.f, 0.f } },
        d3d11_vertex_position_texture{ { size, -size, 0.f },{ 1.f, 1.f } }
      };

      return d3d11_simple_mesh<d3d11_vertex_position_texture>(
        d3d11_vertex_buffer<d3d11_vertex_position_texture>::make_immutable(device, vertices),
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    }
  }
  
  d3d11_desktop_duplication::d3d11_desktop_duplication(const com_ptr<ID3D11Device>& device, const com_ptr<IDXGIOutput2>& output) :
    device(device),
    output(output)
  { }
  
  d3d11_texture_2d& d3d11_desktop_duplication::lock_frame(uint16_t timeout, std::function<void()> timeoutCallback)
  {
    com_ptr<IDXGIResource> resource;
    do
    {
      if (_outputDuplication == nullptr)
      {
        output->DuplicateOutput(device.get(), _outputDuplication.put());
      }

      if (_outputDuplication != nullptr)
      {
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        auto result = _outputDuplication->AcquireNextFrame(timeout, &frameInfo, resource.put());
        if (result == DXGI_ERROR_WAIT_TIMEOUT)
        {
          if (timeoutCallback) timeoutCallback();
        }
        else if (result != ERROR_SUCCESS)
        {
          _outputDuplication = nullptr;
        }
      }
      else
      { 
        this_thread::sleep_for(100ms);
      }
    } while (!resource);

    auto texture = resource.as<ID3D11Texture2D>();
    if (!_texture || _texture->texture != texture)
    {
      _texture = make_unique<d3d11_texture_2d>(texture);
    }

    return *_texture;
  }
  
  void d3d11_desktop_duplication::unlock_frame()
  {
    _outputDuplication->ReleaseFrame();
  }
}