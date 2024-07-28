///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a Frame Buffer Object (i'e' Render
// Target) and its associated texture.  When the FBO is "Bound", the
// output of the graphics pipeline is captured into the texture.  When
// it is "Unbound", the texture is available for use as any normal
// texture.
////////////////////////////////////////////////////////////////////////

#include "fbo.h"
#include <directxtk12/DirectXHelpers.h>
#include <d3dx12.h>

void FBO::CreateFBO(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device,
    std::unique_ptr<DirectX::DescriptorPile>& _rtvHeap,
    std::unique_ptr<DirectX::DescriptorPile>& _srvHeap,
    const int w,
    const int h
) {
    m_width = w;
    m_height = h;

    CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        w, h
    );
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_CLEAR_VALUE clearValue{
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .Color = { 0, 0, 0, 1 },
    };
    HRESULT hr = _device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(&m_texture)
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create FBO texture");
    }

    m_fboID = _rtvHeap->Allocate();
    m_textureID = _srvHeap->Allocate();

    DirectX::CreateRenderTargetView(
        _device.Get(),
        m_texture.Get(),
        _rtvHeap->GetCpuHandle(m_fboID)
    );

    DirectX::CreateShaderResourceView(
        _device.Get(),
        m_texture.Get(),
        _srvHeap->GetCpuHandle(m_textureID)
    );
}
