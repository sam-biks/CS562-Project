///////////////////////////////////////////////////////////////////////
// A slight encapsulation of an OpenGL texture. This contains a method
// to read an image file into a texture, and methods to bind a texture
// to a shader for use, and unbind when done.
////////////////////////////////////////////////////////////////////////

#ifndef _TEXTURE_
#define _TEXTURE_


// This class reads an image from a file, stores it on the graphics
// card as a texture, and stores the (small integer) texture id which
// identifies it.  It also supplies two methods for binding and
// unbinding the texture to/from a shader.

#include <d3d12.h>
#include <wrl.h>
#include <directxtk12/DescriptorHeap.h>
#include <memory>
struct CommandList;
class Texture
{
 public:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture = nullptr;
    size_t m_textureID = 0;
    int m_width = 0, m_height = 0, m_depth = 0;
    unsigned char* image = nullptr;
    Texture();
    Texture(
        Microsoft::WRL::ComPtr<ID3D12Device>& _device,
        std::unique_ptr<DirectX::DescriptorPile>& _heap,
        const std::string &filename
    );

    void BindTexture(
        CommandList& _cmd, 
        std::unique_ptr<DirectX::DescriptorPile>& _heap, 
        const int unit
    );

    static Texture LoadRGBE(
        Microsoft::WRL::ComPtr<ID3D12Device>& _device,
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue,
        std::unique_ptr<DirectX::DescriptorPile>& _heap,
        const std::string& filename
    );

    operator bool() {
        return m_texture != nullptr;
    }
};


#endif
