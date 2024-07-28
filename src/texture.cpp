///////////////////////////////////////////////////////////////////////
// A slight encapsulation of an OpenGL texture. This contains a method
// to read an image file into a texture, and methods to bind a texture
// to a shader for use, and unbind when done.
////////////////////////////////////////////////////////////////////////

#include "math.h"
#include <fstream>
#include <stdlib.h>

//#include <glbinding/gl/gl.h>
//#include <glbinding/Binding.h>
//using namespace gl;
//
//#define GLM_FORCE_CTOR_INIT
//#define GLM_FORCE_RADIANS
//#define GLM_SWIZZLE
//#include <glm/glm.hpp>
//
#include "texture.h"
#include <DirectXTex.h>
#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/WICTextureLoader.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/DirectXHelpers.h>
#include "rgbe.h"

#include "scene.h"

Texture::Texture() : m_textureID(0) {}

Texture::Texture(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device, 
    std::unique_ptr<DirectX::DescriptorPile>& _heap, 
    const std::string& path
) : m_textureID(0), m_depth(0) {
    std::wstring wpath(path.size(), L' ');
    DirectX::TexMetadata texMetadata;
    DirectX::ScratchImage scratchImage;

    size_t conversionCount = 0;
    mbstowcs_s(&conversionCount, wpath.data(), wpath.size(), path.c_str(), path.size());
    wpath.resize(conversionCount);

    HRESULT hr = DirectX::LoadFromWICFile(
        wpath.c_str(),
        DirectX::WIC_FLAGS_NONE,
        &texMetadata,
        scratchImage
    );

    if (FAILED(hr))
        throw std::runtime_error("failed to load texture");

    hr = DirectX::CreateTexture(_device.Get(), texMetadata, &m_texture);
    if (FAILED(hr))
        throw std::runtime_error("failed to create texture");

    m_textureID = _heap->Allocate();
    DirectX::CreateShaderResourceView(_device.Get(), m_texture.Get(), _heap->GetCpuHandle(m_textureID));
}


// Make a texture availabe to a shader program.  The unit parameter is
// a small integer specifying which texture unit should load the
// texture.  The name parameter is the sampler2d in the shader program
// which will provide access to the texture.
void Texture::BindTexture(
    CommandList& _cmd, 
    std::unique_ptr<DirectX::DescriptorPile>& _heap, 
    const int unit
) {
    _cmd->SetGraphicsRootDescriptorTable(unit, _heap->GetGpuHandle(m_textureID));
}

Texture Texture::LoadRGBE(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device,
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue,
    std::unique_ptr<DirectX::DescriptorPile>& _heap, 
    const std::string& filename
) {
    Texture texture{};
    FILE* file = nullptr;
    fopen_s(&file, filename.c_str(), "rb");

    if (!file)
        throw std::runtime_error("failed to open file");

    int width = 0;
    int height = 0;
    rgbe_header_info info{};
    if (RGBE_ReadHeader(file, &width, &height, &info) != RGBE_RETURN_SUCCESS) {
        fclose(file);
        throw std::runtime_error("failed to read header");
    }

    bool validProgram = (info.valid & RGBE_VALID_PROGRAMTYPE) == RGBE_VALID_PROGRAMTYPE;
    bool validGamma = (info.valid & RGBE_VALID_GAMMA) == RGBE_VALID_GAMMA;
    bool validExposure = (info.valid & RGBE_VALID_EXPOSURE) == RGBE_VALID_EXPOSURE;
    bool validOrientation = (info.valid & RGBE_VALID_ORIENTATION) == RGBE_VALID_ORIENTATION;
    
    int numPixels = width * height;
    float* pixels = new float[width * height * 4];

    if (RGBE_ReadPixels_RLE(file, pixels, width, height) != RGBE_RETURN_SUCCESS) {
        fclose(file);
        delete[] pixels;
        throw std::runtime_error("failed to read pixels");
    }

    D3D12_SUBRESOURCE_DATA subresourceData{
        .pData = pixels,
        .RowPitch = width * 4 * sizeof(float),
    };

    DirectX::ResourceUploadBatch uploadBatch(_device.Get());
    uploadBatch.Begin();
    HRESULT hr = DirectX::CreateTextureFromMemory(
        _device.Get(),
        uploadBatch,
        width,
        height,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        subresourceData,
        &texture.m_texture,
        true,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    auto mipLevels = texture.m_texture->GetDesc().MipLevels;
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create texture from memory");
    }
    uploadBatch.End(_queue.Get()).wait();
    delete[] pixels;
    fclose(file);

    texture.m_textureID = _heap->Allocate();
    DirectX::CreateShaderResourceView(
        _device.Get(),
        texture.m_texture.Get(),
        _heap->GetCpuHandle(texture.m_textureID)
    );

    return texture;
}
