///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a shader program. This contains methods
// to build a shader program from multiples files containing vertex
// and pixel shader code, and a method to link the result.  When
// loaded (method "Use"), its vertex shader and pixel shader will be
// invoked for all geometry passing through the graphics pipeline.
// When done, unload it with method "Unuse".
////////////////////////////////////////////////////////////////////////

#include <d3d12.h>
#include <directx-dxc/dxcapi.h>
#include <wrl.h>
#include <map>
#include <directxtk12/RenderTargetState.h>
#include <directxtk12/CommonStates.h>
#include <string>
#include <vector>
class ShaderProgram
{
public:
    enum class Type {
        Vertex,
        Pixel,
        Compute
    };
    
    ShaderProgram();
    void AddShader(const char* fileName, const Type type,
        std::wstring _entry = L"main", std::vector<std::wstring> _defines = {});
    void LinkProgram(
        Microsoft::WRL::ComPtr<ID3D12Device>& _device,
        D3D12_CULL_MODE _cullMode = D3D12_CULL_MODE_BACK,
        D3D12_DEPTH_STENCIL_DESC _depthStencil = DirectX::CommonStates::DepthDefault,
        D3D12_BLEND_DESC _blendDesc = DirectX::CommonStates::Opaque
    );
    void UseShader(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7>& _cmd);
    void SetRenderTargetFormat(uint32_t _index, DXGI_FORMAT _format);
private:
    void LinkComputeProgram(
        Microsoft::WRL::ComPtr<ID3D12Device>& _device
    );
    void LinkGraphicsProgram(
        Microsoft::WRL::ComPtr<ID3D12Device>& _device,
        D3D12_CULL_MODE _cullMode,
        D3D12_DEPTH_STENCIL_DESC _depthStencil,
        D3D12_BLEND_DESC _blendDesc
    );
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    bool m_compute = false;

    std::map<Type, Microsoft::WRL::ComPtr<IDxcBlob>> m_shaderData;

    DirectX::RenderTargetState m_renderTargetState;
};
