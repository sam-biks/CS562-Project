///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a Frame Buffer Object (i'e' Render
// Target) and its associated texture.  When the FBO is "Bound", the
// output of the graphics pipeline is captured into the texture.  When
// it is "Unbound", the texture is available for use as any normal
// texture.
////////////////////////////////////////////////////////////////////////

#include <d3d12.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/DescriptorHeap.h>
#include <memory>
#include <wrl/client.h>
class FBO {
public:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
    unsigned int m_fboID;
    unsigned int m_textureID;
    int m_width, m_height;  // Size of the texture.

    void CreateFBO(
        Microsoft::WRL::ComPtr<ID3D12Device>& _device,
        std::unique_ptr<DirectX::DescriptorPile>& _rtvHeap,
        std::unique_ptr<DirectX::DescriptorPile>& _srvHeap,
        const int w, 
        const int h
    );
};
