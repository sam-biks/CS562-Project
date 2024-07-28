
////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.

#include "object.h"
#include <pix3.h>

#define COMPUTE
#define AO
#include "../ShaderData.h"
#include "scene.h"
#include "shader.h"
#include <cmath>
#include <combaseapi.h>
#include <cstdint>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dx12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/GraphicsMemory.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/SimpleMath.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_6.h>
#include <dxgiformat.h>
#include <dxgitype.h>
#include <intsafe.h>
#include <memory>
#include <minwindef.h>
#include <stdexcept>
#include <synchapi.h>
#include <vector>
#include <WinBase.h>
#include <winerror.h>
#include <winnt.h>
#include <fstream>


#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_glfw.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


const bool fullPolyCount = true; // Use false when emulating the graphics pipeline in software

const float PI = 3.14159f;
const float rad = PI / 180.0f;    // Convert degrees to radians

const float grndSize = 100.0f;    // Island radius;  Minimum about 20;  Maximum 1000 or so
const float grndOctaves = 4.0f;  // Number of levels of detail to compute
const float grndFreq = 0.03f;    // Number of hills per (approx) 50m
const float grndPersistence = 0.03f; // Terrain roughness: Slight:0.01  rough:0.05
const float grndLow = -3.0f;         // Lowest extent below sea level
const float grndHigh = 5.0f;        // Highest extent above sea level

DirectX::SimpleMath::Matrix PerspectiveReverseZ(const float _fovy, const float _aspect, const float _near, const float _far) {
    const float e = 1.0f / std::tan(_fovy * 0.5f);
    return { e / _aspect, 0.0f, 0.0f, 0.0f,
            0.0f, e, 0.0f, 0.0f,
            0.0f, 0.0f, _far / (_near - _far), -1.f,
            0.0f, 0.0f, (_far * _near) / (_near - _far), 0.0f };
}

// Create an RGB color from human friendly parameters: hue, saturation, value
DirectX::SimpleMath::Vector3 HSV2RGB(const float h, const float s, const float v) {
    if (s == 0.0f)
        return DirectX::SimpleMath::Vector3(v, v, v);

    int i = (int)(h * 6.0f) % 6;
    float f = (h * 6.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    if (i == 0)     return DirectX::SimpleMath::Vector3(v, t, p);
    else if (i == 1)     return DirectX::SimpleMath::Vector3(q, v, p);
    else if (i == 2)     return DirectX::SimpleMath::Vector3(p, v, t);
    else if (i == 3)     return DirectX::SimpleMath::Vector3(p, q, v);
    else if (i == 4)     return DirectX::SimpleMath::Vector3(t, p, v);
    else   /*i == 5*/    return DirectX::SimpleMath::Vector3(v, p, q);
}

////////////////////////////////////////////////////////////////////////
// Constructs a hemisphere of spheres of varying hues
std::shared_ptr<Object> SphereOfSpheres(std::shared_ptr<DirectX::GeometricPrimitive>& SpherePolygons) {
    std::shared_ptr<Object> ob = std::make_shared<Object>(nullptr, nullId);

    using namespace DirectX::SimpleMath;
    for (float angle = 0.0; angle < 360.0; angle += 18.0)
        for (float row = 0.075f; row < PI / 2.0f; row += PI / 2.0f / 6.0f) {
            Vector3 hue = HSV2RGB(angle / 360.0f, 1.0f - 2.0f * row / PI, 1.0f);

            std::shared_ptr<Object> sp = std::make_shared<Object>(
                SpherePolygons,
                spheresId,
                hue,
                Vector3(1.0, 1.0, 1.0),
                120.0
            );
            float s = sin(row);
            float c = cos(row);
            ob->add(sp,
                Matrix::CreateScale(0.075f * c, 0.075f * c, 0.075f * c)
                * Matrix::CreateTranslation(c, 0, s)
                * Matrix::CreateRotationZ(DirectX::XMConvertToRadians(angle))
            );
        }
    return ob;
}

////////////////////////////////////////////////////////////////////////
// Constructs a -1...+1  quad (canvas) framed by four (elongated) boxes
std::shared_ptr<Object> FramedPicture(const DirectX::SimpleMath::Matrix& modelTr, const int objectId,
    std::shared_ptr<DirectX::GeometricPrimitive> BoxPolygons,
    std::shared_ptr<DirectX::GeometricPrimitive> QuadPolygons) {
    using namespace DirectX::SimpleMath;
    // This draws the frame as four (elongated) boxes of size +-1.0
    float w = 0.05f;             // Width of frame boards.

    std::shared_ptr<Object> frame = std::make_shared<Object>(nullptr, nullId);
    std::shared_ptr<Object> ob;

    Vector3 woodColor(87.0f / 255.0f, 51.0f / 255.0f, 35.0f / 255.0f);
    ob = std::make_shared<Object>(BoxPolygons, frameId, woodColor, Vector3(0.2f, 0.2f, 0.2f), 10.0f);
    frame->add(ob, Matrix::CreateScale(1.0f, w, w) * Matrix::CreateTranslation(0.0f, 0.0f, 1.0f + w));
    frame->add(ob, Matrix::CreateScale(1.0f, w, w) * Matrix::CreateTranslation(0.0f, 0.0f, -1.0f - w));
    frame->add(ob, Matrix::CreateScale(w, w, 1.0f + 2.f * w) * Matrix::CreateTranslation(1.0f + w, 0.0f, 0.0f));
    frame->add(ob, Matrix::CreateScale(w, w, 1.0f + 2.f * w) * Matrix::CreateTranslation(-1.0f - w, 0.0f, 0.0f));

    ob = std::make_shared<Object>(QuadPolygons, objectId, woodColor, Vector3(1.0f, 1.0f, 1.0f), 10.0f);
    frame->add(ob, Matrix::CreateRotationX(DirectX::XMConvertToRadians(-90)));

    return frame;
}

////////////////////////////////////////////////////////////////////////
// InitializeScene is called once during setup to create all the
// textures, shape VAOs, and shader programs as well as setting a
// number of other parameters.
void Scene::InitializeScene() {
    using namespace DirectX::SimpleMath;
    // @@ Initialize interactive viewing variables here. (spin, tilt, ry, front back, ...)
    //{
    //    ComPtr<ID3D12Debug6> debug;
    //    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    //    if (FAILED(hr)) {
    //        throw std::runtime_error("failed to create debug");
    //    }
    //    debug->EnableDebugLayer();
    //    debug->SetEnableAutoName(true);
    //    debug->SetEnableGPUBasedValidation(true);
    //    debug->SetEnableSynchronizedCommandQueueValidation(true);
    //}
    printf("hi\n");
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory7> factory;
    HRESULT hr = CreateDXGIFactory2(
        DXGI_CREATE_FACTORY_DEBUG,
        IID_PPV_ARGS(&factory)
    );
    if (FAILED(hr))
        throw std::runtime_error("failed to create factory");

    UINT i = 0;
    while (factory->EnumAdapterByGpuPreference(i,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND) {
        hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
        if (SUCCEEDED(hr))
            break;
        i++;
    }

    if (FAILED(hr))
        throw std::runtime_error("Failed to create device");

    m_graphicsMemory = std::make_unique<DirectX::GraphicsMemory>(m_device.Get());
    m_rtvHeap = std::make_unique<DirectX::DescriptorPile>(
        m_device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        FrameCount * 6
    );
    m_dsvHeap = std::make_unique<DirectX::DescriptorPile>(
        m_device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        FrameCount
    );
    m_descHeap = std::make_unique<DirectX::DescriptorPile>(m_device.Get(), 1024);

    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0,
    };
    hr = m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_queue));
    if (FAILED(hr))
        throw std::runtime_error("Failed to create command queue");

    m_descHeap->Allocate();
    if (!ImGui_ImplDX12_Init(
        m_device.Get(),
        FrameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        m_descHeap->Heap(),
        m_descHeap->GetCpuHandle(0),
        m_descHeap->GetGpuHandle(0)
    )) {
        throw std::runtime_error("Failed to Init ImGui");
    }

    glfwGetWindowSize(window, &m_width, &m_height);
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc{
        .Width = static_cast<UINT>(m_width),
        .Height = static_cast<UINT>(m_height),
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .Stereo = false,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = FrameCount,
        .Scaling = DXGI_SCALING_NONE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
    };

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchainFullscreenDesc{
        .RefreshRate = {
            .Numerator = 60,
            .Denominator = 1
        },
        .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
        .Windowed = true
    };
    ComPtr<IDXGISwapChain1> swapchain;
    hr = factory->CreateSwapChainForHwnd(
        m_queue.Get(),
        glfwGetWin32Window(window),
        &swapchainDesc,
        &swapchainFullscreenDesc,
        nullptr,
        &swapchain
    );
    if (FAILED(hr))
        throw std::runtime_error("failed to create swapchain");

    hr = swapchain->QueryInterface(m_swapchain.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to query for swapchain 4");
    for (auto& cmd : m_geometryCmds) {
        cmd.Init(m_device);
    }
    for (auto& cmd : m_lightingCmds) {
        cmd.Init(m_device);
    }
    for (auto& cmd : m_shadowCmds) {
        cmd.Init(m_device);
    }

    for (uint32_t i = 0; i < FrameCount; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        m_rtvHeap->Allocate();
        DirectX::CreateRenderTargetView(m_device.Get(), backBuffer.Get(), m_rtvHeap->GetCpuHandle(i));

        CD3DX12_RESOURCE_DESC dsvDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            m_width, m_height
        );
        dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_CLEAR_VALUE clearValue{
            .Format = DXGI_FORMAT_D32_FLOAT,
            .DepthStencil = {
                .Depth = DepthClearValue,
                .Stencil = 0
            },
        };
        hr = m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &dsvDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_depthTextures[i])
        );
        if (FAILED(hr))
            throw std::runtime_error("Failed to create depth buffer");
        m_device->CreateDepthStencilView(m_depthTextures[i].Get(), nullptr, m_dsvHeap->GetCpuHandle(i));
        m_depthTextureID = m_descHeap->Allocate() - i;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
            .Format = DXGI_FORMAT_R32_FLOAT,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = 1
            },
        };
        m_device->CreateShaderResourceView(
            m_depthTextures[i].Get(),
            &srvDesc,
            m_descHeap->GetCpuHandle(m_depthTextureID + i)
        );
    }

    // Shadow Texture
    {
        CD3DX12_RESOURCE_DESC dsvDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16G16B16A16_UNORM,
            1024, 1024
        );
        dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_CLEAR_VALUE clearValue{
            .Format = DXGI_FORMAT_R16G16B16A16_UNORM,
            .Color = { 0, 0, 0, 0 }
        };

        hr = m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &dsvDesc,
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            &clearValue,
            IID_PPV_ARGS(&m_shadowTexture)
        );
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create shadow texture");
        }

        m_device->CreateRenderTargetView(
            m_shadowTexture.Get(),
            nullptr,
            m_rtvHeap->GetCpuHandle(FrameCount * 5)
        );
        m_shadowTextureID = m_descHeap->Allocate();

        DirectX::CreateShaderResourceView(
            m_device.Get(),
            m_shadowTexture.Get(),
            m_descHeap->GetCpuHandle(m_shadowTextureID)
        );
    }

    m_AOMapID = m_descHeap->Allocate();
    m_AOMapSrvID = m_descHeap->Allocate();
    m_descHeap->Allocate();
    //for (int i = 0; i < 2; i++)
    {
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_FLOAT,
            m_width,
            m_height
        );
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        hr = m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_AOMap[0])
        );
        DirectX::CreateUnorderedAccessView(
            m_device.Get(),
            m_AOMap[0].Get(),
            m_descHeap->GetCpuHandle(m_AOMapID)
        );
        DirectX::CreateShaderResourceView(
            m_device.Get(),
            m_AOMap[0].Get(),
            m_descHeap->GetCpuHandle(m_AOMapSrvID)
        );
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        hr = m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            nullptr,
            IID_PPV_ARGS(&m_AOMap[1])
        );
        DirectX::CreateShaderResourceView(
            m_device.Get(),
            m_AOMap[1].Get(),
            m_descHeap->GetCpuHandle(m_AOMapSrvID + 1)
        );
        DirectX::CreateRenderTargetView(
            m_device.Get(),
            m_AOMap[1].Get(),
            m_rtvHeap->GetCpuHandle(FrameCount * 5 + 1)
        );
    }

    m_blurMapID = m_descHeap->Allocate();
    m_descHeap->Allocate();
    m_blurMapSrvID = m_descHeap->Allocate();
    m_descHeap->Allocate();
    for (int i = 0; i < 2; i++) {
        CD3DX12_RESOURCE_DESC dsvDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16G16B16A16_UNORM,
            1024, 1024
        );
        dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        hr = m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &dsvDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_blurMap[i])
        );


        DirectX::CreateUnorderedAccessView(
            m_device.Get(),
            m_blurMap[i].Get(),
            m_descHeap->GetCpuHandle(m_blurMapID + i)
        );

        DirectX::CreateShaderResourceView(
            m_device.Get(),
            m_blurMap[i].Get(),
            m_descHeap->GetCpuHandle(m_blurMapSrvID + i)
        );
    }

    for (uint32_t i = 0; i < FrameCount; i++) {
        for (uint32_t j = 0; j < static_cast<uint32_t>(FBOIndex::Count); j++) {
            m_fbos[j + i * 4].CreateFBO(
                m_device,
                m_rtvHeap,
                m_descHeap,
                m_width,
                m_height
            );
            switch (static_cast<FBOIndex>(j)) {
            case FBOIndex::WorldPosition:
                m_fbos[j + i * 4].m_texture->SetName(L"WorldPosition");
                break;
            case FBOIndex::Normal:
                m_fbos[j + i * 4].m_texture->SetName(L"Normal");
                break;
            case FBOIndex::Diffuse:
                m_fbos[j + i * 4].m_texture->SetName(L"Diffuse");
                break;
            case FBOIndex::SpecularAlpha:
                m_fbos[j + i * 4].m_texture->SetName(L"SpecularAlpha");
                break;
            }
        }
    }

    nav = false;
    w_down = s_down = a_down = d_down = false;
    Yaw = 0.0f;
    Pitch = 0.0f;
    eye = Vector3(0.0f, -20.0f, 0.0f);
    speed = 300.0f / 30.0f;
    last_time = static_cast<float>(glfwGetTime());
    cameraPos = Vector3(0.0f, 8.0f, 8.0f);

    ry = 0.4f;
    front = 0.1f;
    back = 5000.0f;

    objectRoot = std::make_unique<Object>(nullptr, nullId);

    cameraForward = { 0, -.75f, -.66f };
    right = { 1, 0, 0 };
    up = { 0, 1, 0 };

    // Create the lighting shader program from source code files.
    // @@ Initialize additional shaders if necessary
    LoadShaders();

    // Create all the Polygon shapes
    //proceduralground = new Shapes::ProceduralGround(m_queue, grndSize, 400,
    //    grndOctaves, grndFreq, grndPersistence,
    //    grndLow, grndHigh);

    std::shared_ptr<DirectX::GeometricPrimitive> TeapotPolygons = DirectX::GeometricPrimitive::CreateTeapot();
    std::shared_ptr<DirectX::GeometricPrimitive> BoxPolygons = DirectX::GeometricPrimitive::CreateBox({ 1, 1, 1 });
    std::shared_ptr<DirectX::GeometricPrimitive> SpherePolygons = DirectX::GeometricPrimitive::CreateSphere(
    1.f, 32Ui64);
    std::shared_ptr<DirectX::GeometricPrimitive> InvSpherePolygons = DirectX::GeometricPrimitive::CreateSphere(
        1.f, 16Ui64, false, true
    );
    // Shape* RoomPolygons = new Ply(m_queue, "room.ply");
    // std::shared_ptr<DirectX::GeometricPrimitive> FloorPolygons = DirectX::GeometricPrimitive::Create new Shapes::Plane(10.0, 10, m_queue);
    // std::shared_ptr<DirectX::GeometricPrimitive> QuadPolygons = new Quad(m_queue);
    // std::shared_ptr<DirectX::GeometricPrimitive> SeaPolygons = new Shapes::Plane(2000.0, 50, m_queue);
    // std::shared_ptr<DirectX::GeometricPrimitive> GroundPolygons = proceduralground;

    DirectX::GeometricPrimitive::VertexCollection vertices;
    vertices.push_back({ DirectX::XMFLOAT3{ -1, -1, 0 }, DirectX::XMFLOAT3{ 0, 0, 1 }, DirectX::XMFLOAT2{ 0, 1 } });
    vertices.push_back({ DirectX::XMFLOAT3{ -1, 1, 0 }, DirectX::XMFLOAT3{ 0, 0, 1 }, DirectX::XMFLOAT2{ 0, 0 } });
    vertices.push_back({ DirectX::XMFLOAT3{ 1, 1, 0 }, DirectX::XMFLOAT3{ 0, 0, 1 }, DirectX::XMFLOAT2{ 1, 0 } });
    vertices.push_back({ DirectX::XMFLOAT3{ 1, -1, 0 }, DirectX::XMFLOAT3{ 0, 0, 1 }, DirectX::XMFLOAT2{ 1, 1 } });
    DirectX::GeometricPrimitive::IndexCollection indices;
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);
    std::shared_ptr<DirectX::GeometricPrimitive> QuadPolygons = DirectX::GeometricPrimitive::CreateCustom(
        vertices, indices
    );


    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options{};
    m_device->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS12, &options, sizeof(options));
    DirectX::ResourceUploadBatch uploadBatch(m_device.Get());
    uploadBatch.Begin();
    BoxPolygons->LoadStaticBuffers(m_device.Get(), uploadBatch);
    SpherePolygons->LoadStaticBuffers(m_device.Get(), uploadBatch);
    QuadPolygons->LoadStaticBuffers(m_device.Get(), uploadBatch);
    InvSpherePolygons->LoadStaticBuffers(m_device.Get(), uploadBatch);
    uploadBatch.End(m_queue.Get()).wait();

    // Various colors used in the subsequent models
    Vector3 woodColor(87.0f / 255.0f, 51.0f / 255.0f, 35.0f / 255.0f);
    Vector3 brickColor(134.0f / 255.0f, 60.0f / 255.0f, 56.0f / 255.0f);
    Vector3 floorColor(6 * 16 / 255.0f, 5.5f * 16 / 255.0f, 3 * 16 / 255.0f);
    Vector3 brassColor(0.5f, 0.5f, 0.1f);
    Vector3 grassColor(62.0f / 255.0f, 102.0f / 255.0f, 38.0f / 255.0f);
    Vector3 waterColor(0.3f, 0.3f, 1.0f);

    Vector3 black(0.0f, 0.0f, 0.0f);
    Vector3 brightSpec(0.5f, 0.5f, 0.5f);
    Vector3 polishedSpec(0.3f, 0.3f, 0.3f);

    // Creates all the models from which the scene is composed.  Each
    // is created with a polygon shape (possibly NULL), a
    // transformation, and the surface lighting parameters Kd, Ks, and
    // alpha.

    // @@ This is where you could read in all the textures and
    // associate them with the various objects being created in the
    // next dozen lines of code.

    // @@ To change an object's surface parameters (Kd, Ks, or alpha),
    // modify the following lines.

    central = std::make_shared<Object>(nullptr, nullId);
    anim = std::make_shared<Object>(nullptr, nullId);
    //room       = new Object(RoomPolygons, roomId, brickColor, black, 1);
    //floor      = new Object(FloorPolygons, floorId, floorColor, black, 1);
    teapot = std::make_shared<Object>(TeapotPolygons, teapotId, Vector3(1.0f, 1.0f, 1.0f), brightSpec, 0.1f);
    podium = std::make_shared<Object>(BoxPolygons, boxId, Vector3(woodColor), Vector3(0.01f, 0.01f, 0.01f), 1.0f);
    sky = std::make_shared<Object>(InvSpherePolygons, skyId, black, black, 0);
    //ground     = new Object(GroundPolygons, groundId, grassColor, black, 1);
    //sea        = new Object(SeaPolygons, seaId, waterColor, brightSpec, 120);
    leftFrame = std::shared_ptr<Object>(FramedPicture(Matrix::Identity, lPicId, BoxPolygons, QuadPolygons));
    rightFrame = std::shared_ptr<Object>(FramedPicture(Matrix::Identity, rPicId, BoxPolygons, QuadPolygons));
    spheres = std::make_shared<Object>(SpherePolygons, 14, Vector3(0.3f, 0.3f, 0.3f), Vector3(0.3f, 0.3f, 0.3f), 0.1f);
    //std::shared_ptr<Object>(SphereOfSpheres(SpherePolygons));
    frame = std::make_shared<Object>(QuadPolygons, 12, Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    light = std::make_shared<Object>(SpherePolygons, 13, Vector3(1, 1, 1), Vector3(0, 0, 0), 1);
#ifdef REFL
    spheres->drawMe = true;
#else
    spheres->m_drawMe = false;
#endif


    // @@ To change the scene hierarchy, examine the hierarchy created
    // by the following object->add() calls and adjust as you wish.
    // The objects being manipulated and their polygon shapes are
    // created above here.
    //objectRoot->add(teapot);

    // Scene is composed of sky, ground, sea, room and some central models
    if (fullPolyCount) {
        objectRoot->add(sky, Matrix::CreateScale(2000.0f, 2000.0f, 2000.0f));
        //objectRoot->add(sea);
        //objectRoot->add(ground);
    }
    objectRoot->add(central);
#ifndef REFL
    //objectRoot->add(room);
#endif
    //objectRoot->add(floor, Matrix::CreateTranslation(0, 0.02f, 0));

    // Central model has a rudimentary animation (constant rotation on Z)
    //animated.push_back(anim.get());

    // Central contains a teapot on a podium and an external sphere of spheres
    central->add(podium, Matrix::CreateScale(24.f, 0.5f, 24.f) * Matrix::CreateTranslation(0, -1.5f, 0));
    central->add(anim, Matrix::CreateTranslation(0, 0.0f, 0));
    //for (int i = 0; i < 8; i++) {
    //    auto object = std::make_shared<Object>(
    //        SpherePolygons, 14, Vector3(0.5f, 0.5f, 0.5f), Vector3(0.5f, 0.5f, 0.5f), i * (1.f / 8.f) + .01f
    //    );
    //    central->add(
    //        object,
    //        Matrix::CreateScale(2) * Matrix::CreateTranslation({0, 0, 5}) * Matrix::CreateRotationY(i * (2.f * PI) / 8.f)
    //    );
    //}
    anim->add(teapot, Matrix::CreateScale(2.0f, 2.0f, 2.0f) * Matrix::CreateTranslation(0.f, 0.25f, 0));

    if (fullPolyCount)
        anim->add(spheres, Matrix::CreateScale(16, 16, 16) * Matrix::CreateTranslation(0.0f, 0.0f, 0.0f));

    // Room contains two framed pictures
    if (fullPolyCount) {
        //room->add(leftFrame, Matrix::CreateScale(0.8f, 0.8f, 0.8f) * Matrix::CreateTranslation(-1.5f, 9.85f, 1.f));
        //room->add(rightFrame, Matrix::CreateScale(0.8f, 0.8f, 0.8f) * Matrix::CreateTranslation(1.5f, 9.85f, 1.f));
    }

    // Options menu stuff
    show_demo_window = false;

    m_waitableObject = m_swapchain->GetFrameLatencyWaitableObject();

    hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_idleFence));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create fence");
    }

    Texture texture = Texture::LoadRGBE(m_device, m_queue, m_descHeap, "skys/Newport_Loft_Ref.hdr");
    m_irradianceMap = Texture::LoadRGBE(m_device, m_queue, m_descHeap, "skys/Newport_Loft_Ref.irr.hdr");
    sky->m_texture = texture;
    m_states = std::make_unique<DirectX::CommonStates>(m_device.Get());

    m_lights.push_back({
        .ShadowView = ShadowView,
        .ShadowProj = ShadowProj,
        .lightPos = m_lightPos,
        .ShadowMin = m_shadowMin - m_lightPos.Length(),
        .lightColor = ShaderData::float3(1, 1, 1),
        .ShadowMax = m_shadowMax + m_lightPos.Length(),
        .useShadows = 1,
        .range = 10
    });
    for (int i = 0; i < 1024; i++) {

        ShaderData::Light light{
            .lightPos = ShaderData::float3((i * 2) % 64 - 32, 0, (i * 2) / 32 - 32),
            .lightColor = { (float)rand() / (float)RAND_MAX, 
            (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX},
            .useShadows = 0,
            .range = 20
        };
        m_lights.push_back(light);
    }

    m_computeData.cwidth = 4;
    float weights[104];
    const float e = 2.718281828f;
    float sum = 0;
    for (int i = 0; i < 2 * m_computeData.cwidth + 1; i++) {
        float exp = (i - m_computeData.cwidth) / (m_computeData.cwidth / 2.f);
        weights[i] = powf(e, -0.5f * exp * exp);
        sum += weights[i];
    }

    for (int i = 0; i < 2 * m_computeData.cwidth + 1; i++) {
        weights[i] /= sum;
    }
    memcpy(m_computeData.weights, weights, sizeof(weights));
    m_lightDataID = m_descHeap->Allocate();
    m_descHeap->Allocate();
}

void Scene::DrawMenu() {
    //ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplDX12_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        // This menu demonstrates how to provide the user a list of toggleable settings.
        if (ImGui::BeginMenu("Objects")) {
            if (ImGui::MenuItem("Draw spheres", "", spheres->m_drawMe)) { spheres->m_drawMe ^= true; }
            if (ImGui::MenuItem("Draw walls", "", room->m_drawMe)) { room->m_drawMe ^= true; }
            if (ImGui::MenuItem("Draw ground/sea", "", ground->m_drawMe)) {
                ground->m_drawMe ^= true;
                sea->m_drawMe = ground->m_drawMe;
            }
            ImGui::EndMenu();
        }

        // This menu demonstrates how to provide the user a choice
        // among a set of choices.  The current choice is stored in a
        // variable named "mode" in the application, and sent to the
        // shader to be used as you wish.
        if (ImGui::BeginMenu("Menu ")) {
            if (ImGui::MenuItem("<sample menu of choices>", "", false, false)) {}
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Framebuffers")) {
            if (ImGui::MenuItem("Default", "", frameBufferMode == 0)) {
                frameBufferMode = 0;
            }

            if (ImGui::MenuItem("WordlPosition", "", frameBufferMode == 1)) {
                frameBufferMode = 1;
            }

            if (ImGui::MenuItem("Normal", "", frameBufferMode == 2)) {
                frameBufferMode = 2;
            }

            if (ImGui::MenuItem("Diffuse", "", frameBufferMode == 3)) {
                frameBufferMode = 3;
            }

            if (ImGui::MenuItem("Specular", "", frameBufferMode == 4)) {
                frameBufferMode = 4;
            }

            if (ImGui::MenuItem("AOMap", "", frameBufferMode == 5)) {
                frameBufferMode = 5;
            }


            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::MenuItem("Reset", "")) {
                m_reset = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::Begin("Time")) {
        ImGui::Text("Frame Time %f", m_frameTime);
        ImGui::Text("fps %f", m_fps);
    }
    ImGui::End();

    if (ImGui::Begin("Scene")) {
        if (ImGui::TreeNode("Light")) {
            ImGui::DragFloat3("Light Position", &m_lightPos.x, 1, -25.f, 25.f);
            ImGui::DragFloat("Moment Bias", &m_momentBias, 0.000001f, 0.0f, 0.05f, "%.7f");
            ImGui::DragFloat("Depth Bias", &m_depthBias, 0.000001f, 0.0f, 0.05f, "%.7f");
            ImGui::DragFloat("Light Far", &m_lightFar, 1.f, 0.0f, 10000.f, "%.3f");
            ImGui::DragFloat("Light Near", &m_lightNear, 0.1f, 0.0f, 10000.f, "%.3f");
            ImGui::DragFloat("ShadowMin", &m_shadowMin, 0.5f, -100.f, 100.f);
            ImGui::DragFloat("ShadowMax", &m_shadowMax, 0.5f, -100.f, 100.f);
            if (ImGui::SliderInt("Blur Width", &m_computeData.cwidth, 0, WIDTH)) {
                float weights[104];
                const float e = 2.718281828f;
                float sum = 0;
                for (int i = 0; i < 2 * m_computeData.cwidth + 1; i++) {
                    float exp = (i - m_computeData.cwidth) / (m_computeData.cwidth / 2.f);
                    weights[i] = powf(e, -0.5f * exp * exp);
                    sum += weights[i];
                }

                for (int i = 0; i < 2 * m_computeData.cwidth + 1; i++) {
                    weights[i] /= sum;
                }
                memcpy(m_computeData.weights, weights, sizeof(weights));
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Object")) {
            ImGui::SliderFloat("Roughness", &teapot->m_roughness, 0.001f, 1, "%.5f");
            ImGui::ColorEdit3("Diffuse", &teapot->m_diffuseColor.x);
            ImGui::ColorEdit3("Specular", &teapot->m_specularColor.x);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Podium")) {
            static DirectX::SimpleMath::Vector3 pos = { 0, -1.5f, 0 };
            ImGui::DragFloat3("Position", &pos.x);
            central->m_instances[0].second = 
                DirectX::SimpleMath::Matrix::CreateScale(200, .5f, 200) * DirectX::SimpleMath::Matrix::CreateTranslation(pos);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Ambient Occlusion")) {
            ImGui::SliderFloat("R", &m_aoData.R, 0.1f, 10.f);
            ImGui::SliderFloat("n", &m_aoData.n, 10, 20);
            ImGui::SliderFloat("s", &m_aoData.s, 0.01f, 1.f);
            ImGui::SliderFloat("k", &m_aoData.k, 0, 100);
            ImGui::TreePop();
        }
    }
        ImGui::End();


    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *m_lightingCmds[m_frameIndex]);
    //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::BuildTransforms() {

    // Work out the eye position as the user move it with the WASD keys.
    using namespace DirectX::SimpleMath;
    using namespace DirectX;
    float now = static_cast<float>(glfwGetTime());
    float dist = (now - last_time) * speed;
    last_time = now;

    if (w_down)
        cameraPos += dist * cameraForward;
    if (s_down)
        cameraPos -= dist * cameraForward;
    if (d_down)
        cameraPos += dist * right;
    if (a_down)
        cameraPos -= dist * right;

    WorldView = DirectX::XMMatrixLookToRH(cameraPos, cameraForward, up);
    WorldProj = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(60.0f),
        static_cast<float>(m_width) / static_cast<float>(m_height),
        front,
        back
    );
    WorldView.Invert(WorldInverse);

    ShadowView = Matrix::CreateLookAt(
        m_lightPos,
        { 0, 0, 0 },
        { 0, 1, 0 }
    );

    ShadowProj = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(90),
        1024.f / 1024.f,
        m_lightNear,
        m_lightFar
    );
    m_lights[0] = {
        .ShadowView = ShadowView,
        .ShadowProj = ShadowProj,
        .lightPos = m_lightPos,
        .ShadowMin = m_shadowMin - m_lightPos.Length(),
        .lightColor = ShaderData::float3(1, 1, 1),
        .ShadowMax = m_shadowMax + m_lightPos.Length(),
        .useShadows = 1,
        .range = 1000
    };
}

////////////////////////////////////////////////////////////////////////
// Procedure DrawScene is called whenever the scene needs to be
// drawn. (Which is often: 30 to 60 times per second are the common
// goals.)
void Scene::DrawScene() {
    m_frameStart = static_cast<float>(glfwGetTime());
    PIXScopedEvent(PIX_COLOR(0, 255, 0), "DrawScene");
    using namespace DirectX::SimpleMath;
    // Set the viewport
    

    // Update position of any continuously animating objects
    float atime = 360.0f * static_cast<float>(glfwGetTime()) / 36.f;
    for (std::vector<Object*>::iterator m = animated.begin(); m < animated.end(); m++)
        (*m)->m_animTr = Matrix::CreateRotationY(DirectX::XMConvertToRadians(atime));

    BuildTransforms();

    // The lighting algorithm needs the inverse of the WorldView matrix
    WorldView.Invert(WorldInverse);

    if (m_reset) {
        LoadShaders();
        m_reset = false;
    }
    //DrawShadow();
    DrawGeometry();
    //DrawAO();
    DrawLighting();
}

void Scene::EndFrame() {
    PIXScopedEvent(PIX_COLOR(0, 255, 0), "EndFrame");
    auto& cmd = m_lightingCmds[m_frameIndex];
    ComPtr<ID3D12Resource> backBuffer;
    m_swapchain->GetBuffer(m_frameIndex, IID_PPV_ARGS(&backBuffer));
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    cmd->ResourceBarrier(1, &barrier);
    cmd.Execute(m_queue);
    m_swapchain->Present(1, 0);
    m_graphicsMemory->Commit(m_queue.Get());

    m_idleValue++;
    m_queue->Signal(m_idleFence.Get(), m_idleValue);
    m_frameIndex++;
    m_frameIndex %= FrameCount;

    auto stats = m_graphicsMemory->GetStatistics();
    static auto gigsCommittedMemory = 0;
    static auto gigsTotalMemory = 0;
    if (stats.committedMemory / (1024 * 4) > gigsCommittedMemory) {
        gigsCommittedMemory = stats.committedMemory / (1024 * 4);
        printf_s("Committed Memory: %u\n", gigsCommittedMemory);
    }
    if (stats.totalMemory / (1024 * 4) > gigsTotalMemory) {
        gigsTotalMemory = stats.totalMemory / (1024 * 4);
        printf_s("Total Memory: %u\n", gigsTotalMemory);
    }
    PIXEndEvent(m_queue.Get());
    m_frameEnd = static_cast<float>(glfwGetTime());
    m_frameTime = m_frameEnd - m_frameStart;
    m_totalTime += m_frameTime;
    if (m_totalTime > 1) {
        m_fps = m_numFrames / m_totalTime;
        m_totalTime = 0;
        m_numFrames = 0;
    }
    m_numFrames++;
}

Scene::~Scene() {
    for (auto& cmd : m_lightingCmds) {
        cmd.Wait();
    }
    WaitForSingleObject(m_waitableObject, 1000);
    m_graphicsMemory->GarbageCollect();

    if (m_idleFence->GetCompletedValue() < m_idleValue) {
        m_idleFence->SetEventOnCompletion(m_idleValue, m_waitableObject);
        WaitForSingleObject(m_waitableObject, 1000);
    }
}

void Scene::DrawShadow() {
    PIXScopedEvent(PIX_COLOR(0, 255, 0), "DrawShadow");
    PIXBeginEvent(m_queue.Get(), PIX_COLOR(255, 0, 0), "Shadow Pass");
    using namespace DirectX::SimpleMath;
    CommandList& cmd = m_shadowCmds[m_frameIndex];
    cmd.Wait();
    cmd.Reset();

    D3D12_VIEWPORT vp{
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = static_cast<FLOAT>(1024),
        .Height = static_cast<FLOAT>(1024),
        .MinDepth = 0,
        .MaxDepth = 1,
    };

    D3D12_RECT scissor{
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(1024),
        .bottom = static_cast<LONG>(1024),
    };

    m_shadowProgram->UseShader(cmd.cmd);

    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);
    ID3D12DescriptorHeap* heaps[] = {
        m_descHeap->Heap()
    };
    cmd->SetDescriptorHeaps(_countof(heaps), heaps);



    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_shadowTexture.Get(),
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmd->ResourceBarrier(1, &barrier);

    auto rtvHandle = m_rtvHeap->GetCpuHandle(FrameCount * 5);
    auto dsvHandle = m_dsvHeap->GetCpuHandle(m_frameIndex);
    cmd->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, DepthClearValue, 0, 0, nullptr);
    cmd->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ShaderData::Constants constants{
        .WorldView = WorldView,
        .WorldProj = WorldProj,
    };

    float L = m_lightPos.Length();

    auto constantsMemory = m_graphicsMemory->AllocateConstant(constants);
    auto lightMemory = m_graphicsMemory->AllocateConstant(m_lights[0]);
    cmd->SetGraphicsRootConstantBufferView(0, constantsMemory.GpuAddress());
    cmd->SetGraphicsRootConstantBufferView(1, lightMemory.GpuAddress());

    central->Draw(cmd, m_shadowProgram, m_descHeap, Matrix::Identity);
    //cmd.Execute(m_queue);
    //cmd.Wait();
    //
    //cmd.Reset();
    //
    //cmd->SetDescriptorHeaps(_countof(heaps), heaps);
    m_copyProgram->UseShader(cmd.cmd);
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_shadowTexture.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
    );
    cmd->ResourceBarrier(1, &barrier);
    cmd->SetComputeRootDescriptorTable(0, m_descHeap->GetGpuHandle(m_shadowTextureID));
    cmd->SetComputeRootDescriptorTable(1, m_descHeap->GetGpuHandle(m_blurMapID));

    cmd->Dispatch(1024 / 16, 1024 / 16, 1);
    


    barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_blurMap[0].Get());
    cmd->ResourceBarrier(1, &barrier);

    m_computeProgram->UseShader(cmd.cmd);
    auto mem = m_graphicsMemory->AllocateConstant(m_computeData);
    cmd->SetComputeRootConstantBufferView(2, mem.GpuAddress());
    cmd->SetComputeRootDescriptorTable(0, m_descHeap->GetGpuHandle(m_blurMapID));
    cmd->SetComputeRootDescriptorTable(1, m_descHeap->GetGpuHandle(m_blurMapID + 1));
    cmd->Dispatch(1024 / 128, 1024, 1);
    cmd->ResourceBarrier(1, &barrier);
    //}
    m_computeVertical->UseShader(cmd.cmd);
    cmd->SetComputeRootDescriptorTable(0, m_descHeap->GetGpuHandle(m_blurMapID + 1));
    cmd->SetComputeRootDescriptorTable(1, m_descHeap->GetGpuHandle(m_blurMapID));
    cmd->Dispatch(1024 / 128, 1024, 1);
    cmd->ResourceBarrier(1, &barrier);
    cmd.Execute(m_queue);
    m_graphicsMemory->Commit(m_queue.Get());
    PIXEndEvent(m_queue.Get());
    //cmd.Wait();
}

void Scene::LoadShaders() {
    for (auto& it : m_shadowCmds) {
        it.Wait();
    }

    for (auto& it : m_geometryCmds) {
        it.Wait();
    }

    for (auto& it : m_lightingCmds) {
        it.Wait();
    }
    m_computeVertical = std::make_unique<ShaderProgram>();
    m_computeVertical->AddShader("ComputeShader.hlsl", ShaderProgram::Type::Compute, L"main", {L"V"});
    m_computeVertical->LinkProgram(m_device);

    m_computeProgram = std::make_unique<ShaderProgram>();
    m_computeProgram->AddShader("ComputeShader.hlsl", ShaderProgram::Type::Compute);
    m_computeProgram->LinkProgram(m_device);


    m_lightingProgram = std::make_unique<ShaderProgram>();
    m_lightingProgram->AddShader("lightingPhongVert.hlsl", ShaderProgram::Type::Vertex);
    m_lightingProgram->AddShader("lightingPhongPixel.hlsl", ShaderProgram::Type::Pixel);
    m_lightingProgram->LinkProgram(m_device, D3D12_CULL_MODE_BACK, DirectX::CommonStates::DepthDefault,
        DirectX::CommonStates::Additive);

    m_geometryProgram = std::make_unique<ShaderProgram>();
    m_geometryProgram->AddShader("geometryPhongVert.hlsl", ShaderProgram::Type::Vertex);
    m_geometryProgram->AddShader("geometryPhongPixel.hlsl", ShaderProgram::Type::Pixel);
    m_geometryProgram->LinkProgram(m_device);

    m_shadowProgram = std::make_unique<ShaderProgram>();
    m_shadowProgram->AddShader("shadowVert.hlsl", ShaderProgram::Type::Vertex);
    m_shadowProgram->AddShader("shadowVert.hlsl", ShaderProgram::Type::Pixel, L"PSmain");
    m_shadowProgram->SetRenderTargetFormat(0, DXGI_FORMAT_R16G16B16A16_UNORM);
    m_shadowProgram->LinkProgram(m_device);

    m_copyProgram = std::make_unique<ShaderProgram>();
    m_copyProgram->AddShader("DepthCopyShader.hlsl", ShaderProgram::Type::Compute);
    m_copyProgram->LinkProgram(m_device);

    m_AOProgram = std::make_unique<ShaderProgram>();
    m_AOProgram->AddShader("AOShader.hlsl", ShaderProgram::Type::Vertex, L"VSMain");
    m_AOProgram->AddShader("AOShader.hlsl", ShaderProgram::Type::Pixel, L"PSMain");
    m_AOProgram->SetRenderTargetFormat(0, DXGI_FORMAT_R32_FLOAT);
    m_AOProgram->LinkProgram(m_device, D3D12_CULL_MODE_NONE, DirectX::CommonStates::DepthNone);

    m_AOH = std::make_unique<ShaderProgram>();
    m_AOH->AddShader("AOShader.hlsl", ShaderProgram::Type::Compute, L"AOBlurmain");
    m_AOH->LinkProgram(m_device);

    m_AOV = std::make_unique<ShaderProgram>();
    m_AOV->AddShader("AOShader.hlsl", ShaderProgram::Type::Compute, L"AOBlurmainV", {L"V"});
    m_AOV->LinkProgram(m_device);

}

void Scene::DrawAO() {
    PIXScopedEvent(PIX_COLOR(0, 255, 0), "DrawAO");
    PIXBeginEvent(m_queue.Get(), PIX_COLOR(255, 0, 0), "Ambient Occlusion");
    using namespace DirectX::SimpleMath;
    CommandList& cmd = m_shadowCmds[m_frameIndex];
    cmd.Wait();
    cmd.Reset();

    ID3D12DescriptorHeap* heaps[] = {
        m_descHeap->Heap()
    };
    cmd->SetDescriptorHeaps(_countof(heaps), heaps);
   
    float weights[2 * WIDTH + 1];
    const float e = 2.718281828f;
    float sum = 0;
    for (int i = 0; i < 2 * gwidth + 1; i++) {
        float exp = (i - gwidth) / (gwidth / 2);
        weights[i] = powf(e, -0.5f * exp * exp);
        sum += weights[i];
    }

    for (int i = 0; i < 2 * gwidth + 1; i++) {
        weights[i] /= sum;
    }
    PIXBeginEvent(cmd.cmd.Get(), PIX_COLOR(255, 0, 0), "AO Pass");
    D3D12_VIEWPORT vp{
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = static_cast<FLOAT>(m_width),
        .Height = static_cast<FLOAT>(m_height),
        .MinDepth = 0,
        .MaxDepth = 1,
    };

    D3D12_RECT scissor{
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(m_width),
        .bottom = static_cast<LONG>(m_height),
    };

    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);

    memcpy(m_aoData.weights, weights, sizeof(weights));
    m_AOProgram->UseShader(cmd.cmd);
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_AOMap[1].Get(),
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmd->ResourceBarrier(1, &barrier);
    auto mem = m_graphicsMemory->AllocateConstant(m_aoData);
    auto rtvHandle = m_rtvHeap->GetCpuHandle(FrameCount * 5 + 1);
    cmd->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    cmd->SetGraphicsRootConstantBufferView(0, mem.GpuAddress());
    cmd->SetGraphicsRootDescriptorTable(1, m_descHeap->GetGpuHandle(m_fbos[4 * m_frameIndex + 0].m_textureID));
    cmd->SetGraphicsRootDescriptorTable(2, m_descHeap->GetGpuHandle(m_fbos[4 * m_frameIndex + 1].m_textureID));
    //frame->Draw(cmd, m_AOProgram, m_descHeap, Matrix::Identity);
    frame->m_shape->Draw(cmd.cmd.Get());
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_AOMap[1].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
    );
    cmd->ResourceBarrier(1, &barrier);
    PIXEndEvent(cmd.cmd.Get());
    

    PIXBeginEvent(cmd.cmd.Get(), PIX_COLOR(255, 0, 0), "Horrizontal Blur");
    m_AOH->UseShader(cmd.cmd);
    cmd->SetComputeRootConstantBufferView(0, mem.GpuAddress());
    cmd->SetComputeRootDescriptorTable(1, m_descHeap->GetGpuHandle(m_fbos[4 * m_frameIndex + 0].m_textureID));
    cmd->SetComputeRootDescriptorTable(2, m_descHeap->GetGpuHandle(m_fbos[4 * m_frameIndex + 1].m_textureID));
    cmd->SetComputeRootDescriptorTable(3, m_descHeap->GetGpuHandle(m_AOMapSrvID + 1));
    cmd->SetComputeRootDescriptorTable(4, m_descHeap->GetGpuHandle(m_AOMapID));
    cmd->Dispatch(m_width / 128, m_height, 1);
    PIXEndEvent(cmd.cmd.Get());
    
    PIXBeginEvent(cmd.cmd.Get(), PIX_COLOR(255, 0, 0), "Vertical Blur");
    m_AOV->UseShader(cmd.cmd);
    cmd->SetComputeRootDescriptorTable(1, m_descHeap->GetGpuHandle(m_fbos[4 * m_frameIndex + 0].m_textureID));
    cmd->SetComputeRootDescriptorTable(2, m_descHeap->GetGpuHandle(m_fbos[4 * m_frameIndex + 1].m_textureID));
    cmd->SetComputeRootDescriptorTable(3, m_descHeap->GetGpuHandle(m_AOMapSrvID));
    cmd->SetComputeRootDescriptorTable(4, m_descHeap->GetGpuHandle(m_AOMapID));
    cmd->Dispatch(m_width, m_height / 128 + 1, 1);
    PIXEndEvent(cmd.cmd.Get());

    cmd.Execute(m_queue);
    PIXEndEvent(m_queue.Get());
    //cmd.Wait();
}

void Scene::DrawGeometry() {
    PIXScopedEvent(PIX_COLOR(0, 255, 0), "DrawGeometry");
    using namespace DirectX::SimpleMath;
    CommandList& cmd = m_geometryCmds[m_frameIndex];
    cmd.Wait();
    PIXBeginEvent(m_queue.Get(), PIX_COLOR(255, 0, 0), "Geometry");
    cmd.Reset();
    WorldView.Invert(WorldInverse);

    D3D12_VIEWPORT vp{
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = static_cast<FLOAT>(m_width),
        .Height = static_cast<FLOAT>(m_height),
        .MinDepth = 0,
        .MaxDepth = 1,
    };

    D3D12_RECT scissor{
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(m_width),
        .bottom = static_cast<LONG>(m_height),
    };

    m_geometryProgram->UseShader(cmd.cmd);

    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);
    ID3D12DescriptorHeap* heaps[] = {
        m_descHeap->Heap()
    };
    cmd->SetDescriptorHeaps(_countof(heaps), heaps);

    std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
    auto dsvHandle = m_dsvHeap->GetCpuHandle(m_frameIndex);
    for (uint32_t i = 0; i < static_cast<uint32_t>(FBOIndex::Count); i++) {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_fbos[4 * m_frameIndex + i].m_texture.Get(),
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        barriers.push_back(barrier);
        rtvHandles.push_back(m_rtvHeap->GetCpuHandle(m_fbos[4 * m_frameIndex + i].m_fboID));
    }
    cmd->ResourceBarrier(barriers.size(), barriers.data());
    cmd->OMSetRenderTargets(rtvHandles.size(), rtvHandles.data(), false, &dsvHandle);
    cmd->ClearDepthStencilView(
        m_dsvHeap->GetCpuHandle(m_frameIndex),
        D3D12_CLEAR_FLAG_DEPTH,
        DepthClearValue, 0,
        0,
        nullptr
    );
    constexpr FLOAT color[] = { 0, 0, 0, 1 };
    for (auto& rtv : rtvHandles) {
        cmd->ClearRenderTargetView(rtv, color, 0, nullptr);
    }

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // @@ The scene specific parameters (uniform variables) used by
    // the shader are set here.  Object specific parameters are set in
    // the Draw procedure in object.cpp
    ShaderData::Constants constants{
        .WorldView = WorldView,
        .WorldInverse = WorldInverse,
        .WorldProj = WorldProj,
    };
    auto constantsMemory = m_graphicsMemory->AllocateConstant(constants);
    auto lightMemory = m_graphicsMemory->AllocateConstant(m_lights[0]);
    cmd->SetGraphicsRootConstantBufferView(0, constantsMemory.GpuAddress());
    cmd->SetGraphicsRootConstantBufferView(1, lightMemory.GpuAddress());

    objectRoot->Draw(cmd, m_geometryProgram, m_descHeap, Matrix::Identity);
    for (auto& ligh : m_lights) {
        Vector4 l = Vector4::Transform(Vector4::Transform(Vector4(ligh.lightPos.x, ligh.lightPos.y, ligh.lightPos.z, 1), WorldView), WorldProj);
        if (abs(l.x / l.w) < 1 && abs(l.y / l.w) < 1)
            light->Draw(cmd, m_geometryProgram, m_descHeap, Matrix::CreateTranslation(ligh.lightPos));
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(FBOIndex::Count); i++) {
        FBO& fbo = m_fbos[4 * m_frameIndex + i];
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            fbo.m_texture.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
        );
        cmd->ResourceBarrier(1, &barrier);
    }

    cmd.Execute(m_queue);
    m_graphicsMemory->Commit(m_queue.Get());
    PIXEndEvent(m_queue.Get());
    //m_queue->Wait(cmd.fence.Get(), cmd.fenceEventValue);
}

void Scene::DrawLighting() {
    //WaitForSingleObjectEx(m_waitableObject, 1000, true);
    PIXScopedEvent(PIX_COLOR(0, 255, 0), "DrawLighting");
    using namespace DirectX::SimpleMath;
    CommandList& cmd = m_lightingCmds[m_frameIndex];
    cmd.Wait();
    PIXBeginEvent(m_queue.Get(), PIX_COLOR(255, 0, 0), "Lighting");
    cmd.Reset();

    D3D12_VIEWPORT vp{
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = static_cast<FLOAT>(m_width),
        .Height = static_cast<FLOAT>(m_height),
        .MinDepth = 0,
        .MaxDepth = 1,
    };

    D3D12_RECT scissor{
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(m_width),
        .bottom = static_cast<LONG>(m_height),
    };
    m_lightingProgram->UseShader(cmd.cmd);
    //programId = lightingProgram->programId;

    // Set the viewport, and clear the screen

    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);
    ID3D12DescriptorHeap* heaps[] = {
        m_descHeap->Heap()
    };
    cmd->SetDescriptorHeaps(_countof(heaps), heaps);
    auto rtvHandle = m_rtvHeap->GetCpuHandle(m_frameIndex);
    auto dsvHandle = m_dsvHeap->GetCpuHandle(m_frameIndex);
    ComPtr<ID3D12Resource> backBuffer;
    m_swapchain->GetBuffer(m_frameIndex, IID_PPV_ARGS(&backBuffer));
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmd->ResourceBarrier(1, &barrier);


    for (uint32_t i = 0; i < static_cast<uint32_t>(FBOIndex::Count); i++) {
        FBO& fbo = m_fbos[4 * m_frameIndex + i];
        cmd->SetGraphicsRootDescriptorTable(2 + i, m_descHeap->GetGpuHandle(fbo.m_textureID));
    }
    cmd->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    cmd->ClearDepthStencilView(
        m_dsvHeap->GetCpuHandle(m_frameIndex),
        D3D12_CLEAR_FLAG_DEPTH,
        DepthClearValue, 0,
        0,
        nullptr
    );
    constexpr FLOAT color[] = { 0, 0, 0, 1 };
    cmd->ClearRenderTargetView(m_rtvHeap->GetCpuHandle(m_frameIndex), color, 0, nullptr);


    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //cmd->SetGraphicsRootDescriptorTable(7, m_descHeap->GetGpuHandle(m_blurMapSrvID + 0));
    //m_irradianceMap.BindTexture(cmd, m_descHeap, 8);
    //sky->m_texture.BindTexture(cmd, m_descHeap, 9);
    //cmd->SetGraphicsRootDescriptorTable(10, m_descHeap->GetGpuHandle(m_AOMapSrvID));

    //float hammersley[80] = { 0 };
    //int pos = 0;
    //int kk = 0;
    //float p = 0;
    //float u = 0;
    //for (int k = 0; k < 40; k++) {
    //    for (p = 0.5f, kk = k, u = 0.0f; kk; p *= 0.5f, kk >>= 1) {
    //        if (kk & 1) {
    //            u += p;
    //        }
    //    }
    //    float v = (k + 0.5f) / 40;
    //    hammersley[pos++] = u;
    //    hammersley[pos++] = v;
    //}

    // @@ The scene specific parameters (uniform variables) used by
    // the shader are set here.  Object specific parameters are set in
    // the Draw procedure in object.cpp
    ShaderData::Constants constants{
        .WorldView = WorldView,
        .WorldInverse = WorldInverse,
        .WorldProj = WorldProj,
        .CameraPos = cameraPos,
        .shaderMode = frameBufferMode,
        .momentBias = m_momentBias,
        .depthBias = m_depthBias,
    };
    //for (int i = 0; i < 80; i++) {
    //    constants.hammersley[i].x = hammersley[i];
    //}
    //memcpy(constants.hammersley, hammersley, sizeof(hammersley));

    auto constantsMemory = m_graphicsMemory->AllocateConstant(constants);

    auto lightMemory = m_graphicsMemory->Allocate(m_lights.size() * sizeof(m_lights[0]));
    memcpy(lightMemory.Memory(), m_lights.data(), m_lights.size() * sizeof(m_lights[0]));
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = lightMemory.ResourceOffset() / sizeof(m_lights[0]),
            .NumElements = static_cast<UINT>(m_lights.size()),
            .StructureByteStride = sizeof(m_lights[0]),
        },
    };
    m_device->CreateShaderResourceView(
        lightMemory.Resource(),
        &srvDesc,
        m_descHeap->GetCpuHandle(m_lightDataID + m_frameIndex)
    );
    //for (auto& light : m_lights) {
    //
    //    auto lightMemory = m_graphicsMemory->AllocateConstant(light);
    cmd->SetGraphicsRootConstantBufferView(0, constantsMemory.GpuAddress());
    cmd->SetGraphicsRootDescriptorTable(1, m_descHeap->GetGpuHandle(m_lightDataID + m_frameIndex));
    frame->m_shape->DrawInstanced(*cmd, 1, 0);
    //
    //    //frame->Draw(cmd, m_lightingProgram, m_descHeap, Matrix::Identity);
    //}
}
