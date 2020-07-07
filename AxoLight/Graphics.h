#pragma once
#include "pch.h"
#include "Infrastructure.h"

namespace AxoLight::Graphics
{
  winrt::com_ptr<IDXGIOutput2> get_default_output();

  struct d3d11_renderer
  {
  protected:
    d3d11_renderer(const std::tuple<winrt::com_ptr<ID3D11Device>, winrt::com_ptr<ID3D11DeviceContext>>& args);

  private:
    static std::tuple<winrt::com_ptr<ID3D11Device>, winrt::com_ptr<ID3D11DeviceContext>> args(const winrt::com_ptr<IDXGIAdapter>& dxgiAdapter);

  public:
    const winrt::com_ptr<ID3D11Device> device;
    const winrt::com_ptr<ID3D11DeviceContext> context;

    d3d11_renderer(const winrt::com_ptr<IDXGIAdapter>& dxgiAdapter);
  };

  struct d3d11_resource
  {
    const winrt::com_ptr<ID3D11Resource> resource;

    d3d11_resource(const winrt::com_ptr<ID3D11Resource>& resource);

    void update(const winrt::com_ptr<ID3D11DeviceContext>& context, const winrt::array_view<uint8_t>& data) const;
  };

  enum class d3d11_shader_stage
  {
    cs,
    vs,
    ps
  };

  struct d3d11_texture_2d : d3d11_resource
  {
  private:
    static winrt::com_ptr<ID3D11ShaderResourceView> make_view(const winrt::com_ptr<ID3D11Texture2D>& texture);

  public:
    const winrt::com_ptr<ID3D11Texture2D> texture;
    const winrt::com_ptr<ID3D11ShaderResourceView> view;

    d3d11_texture_2d(const winrt::com_ptr<ID3D11Texture2D>& texture);

    template<typename TItem>
    static d3d11_texture_2d make_immutable(const winrt::com_ptr<ID3D11Device>& device, DXGI_FORMAT format, uint32_t width, uint32_t height, const winrt::array_view<TItem>& pixels)
    {
      CD3D11_TEXTURE2D_DESC desc(format, width, height);
      desc.Usage = D3D11_USAGE_IMMUTABLE;
      desc.ArraySize = 1;
      desc.MipLevels = 1;

      D3D11_SUBRESOURCE_DATA data = {};
      data.pSysMem = pixels.data();
      data.SysMemPitch = width * sizeof(TItem);

      std::vector<D3D11_SUBRESOURCE_DATA> resources;
      resources.push_back(data);

      winrt::com_ptr<ID3D11Texture2D> texture;
      winrt::check_hresult(device->CreateTexture2D(&desc, resources.data(), texture.put()));

      return d3d11_texture_2d(texture);
    }

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context, d3d11_shader_stage stage, uint32_t slot = 0u) const;
  };

  struct d3d11_render_target_2d : public d3d11_texture_2d
  {
  private:
    static winrt::com_ptr<ID3D11RenderTargetView> get_view(const winrt::com_ptr<ID3D11Texture2D>& texture);

    static D3D11_VIEWPORT get_view_port(const winrt::com_ptr<ID3D11Texture2D>& texture);

  public:
    const winrt::com_ptr<ID3D11RenderTargetView> view;
    const D3D11_VIEWPORT view_port;

    d3d11_render_target_2d(const winrt::com_ptr<ID3D11Texture2D>& texture);

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context) const;

    void clear(const winrt::com_ptr<ID3D11DeviceContext>& context, const std::array<float, 4>& color = {}) const;
  };

  struct d3d11_renderer_with_swap_chain : public d3d11_renderer
  {
  private:
    std::unordered_map<void*, std::unique_ptr<d3d11_render_target_2d>> _buffers;

    typedef std::tuple<winrt::com_ptr<IDXGISwapChain>, winrt::com_ptr<ID3D11Device>, winrt::com_ptr<ID3D11DeviceContext>> args_t;

    d3d11_renderer_with_swap_chain(const args_t& args);

    static args_t args(const winrt::com_ptr<IDXGIAdapter>& dxgiAdapter, const winrt::handle& windowHandle);

  public:
    const winrt::com_ptr<IDXGISwapChain> swap_chain;

    d3d11_renderer_with_swap_chain(const winrt::com_ptr<IDXGIAdapter>& dxgiAdapter, const winrt::handle& windowHandle);

    const d3d11_render_target_2d& render_target();
  };

  struct d3d11_buffer : public d3d11_resource
  {
    const winrt::com_ptr<ID3D11Buffer> buffer;

    d3d11_buffer(const winrt::com_ptr<ID3D11Buffer>& buffer);
  };

  struct d3d11_vertex_position
  {
    std::array<float, 3> position;

    static inline std::array<D3D11_INPUT_ELEMENT_DESC, 1> input_desc = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
  };

  struct d3d11_vertex_position_texture : public d3d11_vertex_position
  {
    std::array<float, 2> texture;

    static inline std::array<D3D11_INPUT_ELEMENT_DESC, 2> input_desc = {
      d3d11_vertex_position::input_desc[0],
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
  };

  template<typename TVertex>
  struct d3d11_vertex_buffer : public d3d11_buffer
  {
  private:
    d3d11_vertex_buffer(const winrt::com_ptr<ID3D11Buffer>& buffer, uint32_t capacity) :
      d3d11_buffer(buffer),
      capacity(capacity)
    { }

    uint32_t _size = 0;

  public:
    typedef TVertex vertex_t;
    const uint32_t capacity;

    const winrt::array_view<D3D11_INPUT_ELEMENT_DESC> input_desc() const
    {
      return vertex_t::input_desc;
    }

    uint32_t size() const
    {
      return _size;
    }

    static d3d11_vertex_buffer make_immutable(const winrt::com_ptr<ID3D11Device>& device, const winrt::array_view<vertex_t>& vertices)
    {
      CD3D11_BUFFER_DESC desc(
        sizeof(vertex_t) * vertices.size(),
        D3D11_BIND_VERTEX_BUFFER,
        D3D11_USAGE_IMMUTABLE
      );

      D3D11_SUBRESOURCE_DATA data = {};
      data.pSysMem = vertices.data();

      winrt::com_ptr<ID3D11Buffer> buffer;
      winrt::check_hresult(device->CreateBuffer(&desc, &data, buffer.put()));

      d3d11_vertex_buffer result(buffer, vertices.size());
      result._size = vertices.size();
      return result;
    }

    static d3d11_vertex_buffer make_dynamic(const winrt::com_ptr<ID3D11Device>& device, uint32_t capacity)
    {
      CD3D11_BUFFER_DESC desc(
        sizeof(vertex_t) * capacity,
        D3D11_BIND_VERTEX_BUFFER,
        D3D11_USAGE_DYNAMIC,
        D3D11_CPU_ACCESS_WRITE
      );

      winrt::com_ptr<ID3D11Buffer> buffer;
      check_hresult(device->CreateBuffer(&desc, nullptr, buffer.put()));

      return d3d11_vertex_buffer(buffer, capacity);
    }

    void update(const winrt::com_ptr<ID3D11DeviceContext>& context, const winrt::array_view<vertex_t>& vertices) const
    {
      _size = min(vertices.size() * sizeof(vertex_t), capacity);
      d3d11_buffer::update(context, { vertices.data(), _size });
    }

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context, uint32_t slot = 0) const
    {
      uint32_t stride = sizeof(vertex_t);
      uint32_t offset = 0;
      std::array<ID3D11Buffer*, 1> buffers = { buffer.get() };
      context->IASetVertexBuffers(slot, 1u, buffers.data(), &stride, &offset);
    }
  };

  template<typename TConstant>
  struct d3d11_constant_buffer : public d3d11_buffer
  {
  private:
    d3d11_constant_buffer(const winrt::com_ptr<ID3D11Buffer>& buffer, uint32_t capacity) :
      d3d11_buffer(buffer),
      capacity(capacity)
    { }

  public:
    typedef TConstant constant_t;
    const uint32_t capacity;

    static d3d11_constant_buffer make_dynamic(const winrt::com_ptr<ID3D11Device>& device)
    {
      auto capacity = (uint32_t)sizeof(constant_t);
      CD3D11_BUFFER_DESC desc(
        (capacity % 16 == 0 ? capacity : capacity + 16 - capacity % 16),
        D3D11_BIND_CONSTANT_BUFFER,
        D3D11_USAGE_DYNAMIC,
        D3D11_CPU_ACCESS_WRITE
      );

      winrt::com_ptr<ID3D11Buffer> buffer;
      winrt::check_hresult(device->CreateBuffer(&desc, nullptr, buffer.put()));

      return d3d11_constant_buffer(buffer, capacity);
    }

    void update(const winrt::com_ptr<ID3D11DeviceContext>& context, const constant_t& value) const
    {
      d3d11_buffer::update(context, winrt::array_view{ (uint8_t*)&value, capacity });
    }

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context, d3d11_shader_stage stage, uint32_t slot = 0) const
    {
      std::array<ID3D11Buffer*, 1> buffers = { buffer.get() };
      switch (stage)
      {
      case d3d11_shader_stage::cs:
        context->CSSetConstantBuffers(slot, 1, buffers.data());
        break;
      case d3d11_shader_stage::vs:
        context->VSSetConstantBuffers(slot, 1, buffers.data());
        break;
      case d3d11_shader_stage::ps:
        context->PSSetConstantBuffers(slot, 1, buffers.data());
        break;
      default:
        throw std::out_of_range("Invalid shader stage for constant buffer!");
      }
    }
  };

  template<typename TItem>
  struct d3d11_structured_buffer : public d3d11_buffer
  {
  private:
    d3d11_structured_buffer(
      const winrt::com_ptr<ID3D11Buffer>& buffer, 
      uint32_t capacity,
      const winrt::com_ptr<ID3D11ShaderResourceView>& view,
      const winrt::com_ptr<ID3D11UnorderedAccessView> uav) :
      d3d11_buffer(buffer),
      view(view),
      uav(uav),
      capacity(capacity)
    { }

  public:
    typedef TItem item_t;
    const winrt::com_ptr<ID3D11ShaderResourceView> view;
    const winrt::com_ptr<ID3D11UnorderedAccessView> uav;
    uint32_t capacity;

    static d3d11_structured_buffer make_immutable(const winrt::com_ptr<ID3D11Device>& device, const std::vector<item_t>& items)
    {
      CD3D11_BUFFER_DESC desc(
        (uint32_t)(sizeof(item_t) * items.size()),
        D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_IMMUTABLE,
        0,
        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        sizeof(item_t)
      );

      D3D11_SUBRESOURCE_DATA data{ items.data() };
      winrt::com_ptr<ID3D11Buffer> buffer;
      winrt::check_hresult(device->CreateBuffer(&desc, &data, buffer.put()));

      D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
      viewDesc.Format = DXGI_FORMAT_UNKNOWN;
      viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      viewDesc.Buffer.NumElements = (uint32_t)items.size();

      winrt::com_ptr<ID3D11ShaderResourceView> view;
      winrt::check_hresult(device->CreateShaderResourceView(buffer.get(), &viewDesc, view.put()));

      return d3d11_structured_buffer(buffer, (uint32_t)items.size(), view, nullptr);
    }

    static d3d11_structured_buffer make_writeable(const winrt::com_ptr<ID3D11Device>& device, uint32_t capacity)
    {
      CD3D11_BUFFER_DESC desc(
        sizeof(item_t) * capacity,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
        D3D11_USAGE_DEFAULT,
        0,
        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        sizeof(item_t)
      );

      winrt::com_ptr<ID3D11Buffer> buffer;
      winrt::check_hresult(device->CreateBuffer(&desc, nullptr, buffer.put()));

      D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
      viewDesc.Format = DXGI_FORMAT_UNKNOWN;
      viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      viewDesc.Buffer.NumElements = capacity;

      winrt::com_ptr<ID3D11ShaderResourceView> view;
      winrt::check_hresult(device->CreateShaderResourceView(buffer.get(), &viewDesc, view.put()));

      D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
      uavDesc.Format = DXGI_FORMAT_UNKNOWN;
      uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
      uavDesc.Buffer.NumElements = capacity;

      winrt::com_ptr<ID3D11UnorderedAccessView> uav;
      winrt::check_hresult(device->CreateUnorderedAccessView(buffer.get(), &uavDesc, uav.put()));

      return d3d11_structured_buffer(buffer, capacity, view, uav);
    }

    static d3d11_structured_buffer make_staging(const winrt::com_ptr<ID3D11Device>& device, uint32_t capacity)
    {
      CD3D11_BUFFER_DESC desc(
        sizeof(item_t) * capacity,
        0,
        D3D11_USAGE_STAGING,
        D3D11_CPU_ACCESS_READ
      );

      winrt::com_ptr<ID3D11Buffer> buffer;
      winrt::check_hresult(device->CreateBuffer(&desc, nullptr, buffer.put()));

      return d3d11_structured_buffer(buffer, capacity, nullptr, nullptr);
    }

    void copy_to(const winrt::com_ptr<ID3D11DeviceContext>& context, const d3d11_structured_buffer& target)
    {
      context->CopyResource(target.resource.get(), resource.get());
    }

    std::vector<item_t> get_data(const winrt::com_ptr<ID3D11DeviceContext>& context)
    {
      D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};

      winrt::check_hresult(context->Map(buffer.get(), 0, D3D11_MAP_READ, 0, &mappedSubresource));

      std::vector<item_t> items(capacity);
      memcpy(items.data(), mappedSubresource.pData, capacity * sizeof(item_t));

      context->Unmap(buffer.get(), 0);
      return items;
    }

    void set_readonly(const winrt::com_ptr<ID3D11DeviceContext>& context, uint32_t slot = 0) const
    {
      std::array<ID3D11ShaderResourceView*, 1> views = { view.get() };
      context->CSSetShaderResources(slot, 1, views.data());
    }

    void set_writeable(const winrt::com_ptr<ID3D11DeviceContext>& context, uint32_t slot = 0) const
    {
      auto uavInitialCount = 0u;
      std::array<ID3D11UnorderedAccessView*, 1> views = { uav.get() };
      context->CSSetUnorderedAccessViews(slot, 1, views.data(), &uavInitialCount);
    }
  };

  template<typename TVertex>
  struct d3d11_simple_mesh
  {
    const d3d11_vertex_buffer<TVertex> vertex_buffer;
    const D3D11_PRIMITIVE_TOPOLOGY topology;

    d3d11_simple_mesh(const d3d11_vertex_buffer<TVertex>& vertex_buffer, D3D11_PRIMITIVE_TOPOLOGY topology) :
      vertex_buffer(vertex_buffer),
      topology(topology)
    { }

    void draw(const winrt::com_ptr<ID3D11DeviceContext>& context) const
    {
      vertex_buffer.set(context);
      context->IASetPrimitiveTopology(topology);
      context->Draw(vertex_buffer.size(), 0u);
    }
  };

  namespace Primitives
  {
    d3d11_simple_mesh<d3d11_vertex_position_texture> make_quad(const winrt::com_ptr<ID3D11Device>& device, float size = 2.f);
  }

  struct d3d11_vertex_shader
  {
  private:
    std::pair<const D3D11_INPUT_ELEMENT_DESC*, ID3D11InputLayout*> _currentLayout = {};
    std::map<const D3D11_INPUT_ELEMENT_DESC*, winrt::com_ptr<ID3D11InputLayout>> _inputLayouts;
    const std::vector<uint8_t> _source;

    static winrt::com_ptr<ID3D11VertexShader> load_shader(const winrt::com_ptr<ID3D11Device>& device, const std::vector<uint8_t>& source);

  public:
    const winrt::com_ptr<ID3D11VertexShader> shader;

    d3d11_vertex_shader(const winrt::com_ptr<ID3D11Device>& device, const std::filesystem::path& path);

    void set_input_layout(const winrt::array_view<D3D11_INPUT_ELEMENT_DESC>& inputDesc);

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context) const;
  };

  struct d3d11_pixel_shader
  {
  private:
    static winrt::com_ptr<ID3D11PixelShader> load_shader(const winrt::com_ptr<ID3D11Device>& device, const std::vector<uint8_t>& source);

  public:
    const winrt::com_ptr<ID3D11PixelShader> shader;

    d3d11_pixel_shader(const winrt::com_ptr<ID3D11Device>& device, const std::filesystem::path& path);

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context) const;
  };

  struct d3d11_compute_shader
  {
  private:
    static winrt::com_ptr<ID3D11ComputeShader> load_shader(const winrt::com_ptr<ID3D11Device>& device, const std::vector<uint8_t>& source);

  public:
    const winrt::com_ptr<ID3D11ComputeShader> shader;

    d3d11_compute_shader(const winrt::com_ptr<ID3D11Device>& device, const std::filesystem::path& path);

    void run(const winrt::com_ptr<ID3D11DeviceContext>& context, uint32_t x, uint32_t y = 1, uint32_t z = 1) const;
  };

  enum class d3d11_rasterizer_type
  {
    cull_none,
    cull_clockwise,
    cull_counter_clockwise,
    wireframe
  };

  struct d3d11_rasterizer_state
  {
  private:
    static winrt::com_ptr<ID3D11RasterizerState> make_state(const winrt::com_ptr<ID3D11Device>& device, d3d11_rasterizer_type type);

  public:
    const winrt::com_ptr<ID3D11RasterizerState> state;

    d3d11_rasterizer_state(const winrt::com_ptr<ID3D11Device>& device, d3d11_rasterizer_type type);

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context) const;
  };

  enum class d3d11_blend_type
  {
    opaque,
    additive,
    subtractive,
    alpha_blend
  };

  struct d3d11_blend_state
  {
  private:
    static winrt::com_ptr<ID3D11BlendState> make_state(const winrt::com_ptr<ID3D11Device>& device, d3d11_blend_type type);

  public:
    const winrt::com_ptr<ID3D11BlendState> state;

    d3d11_blend_state(const winrt::com_ptr<ID3D11Device>& device, d3d11_blend_type type);

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context) const;
  };

  struct d3d11_sampler_state
  {
  private:
    static winrt::com_ptr<ID3D11SamplerState> make_state(const winrt::com_ptr<ID3D11Device>& device, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode);

  public:
    const winrt::com_ptr<ID3D11SamplerState> state;

    d3d11_sampler_state(const winrt::com_ptr<ID3D11Device>& device, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode);

    void set(const winrt::com_ptr<ID3D11DeviceContext>& context, d3d11_shader_stage stage, int slot = 0) const;
  };

  struct d3d11_desktop_duplication
  {
  private:
    winrt::com_ptr<IDXGIOutputDuplication> _outputDuplication;
    std::unique_ptr<d3d11_texture_2d> _texture;

  public:
    const winrt::com_ptr<ID3D11Device> device;
    const winrt::com_ptr<IDXGIOutput2> output;

    d3d11_desktop_duplication(const winrt::com_ptr<ID3D11Device>& device, const winrt::com_ptr<IDXGIOutput2>& output);

    d3d11_texture_2d& lock_frame(uint16_t timeout = 1000u, std::function<void()> timeoutCallback = nullptr);

    void unlock_frame();
  };
}