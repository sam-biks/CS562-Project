#pragma once
////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * Viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.

#include "shapes.h"
#include "object.h"
#include "texture.h"
#include "fbo.h"
#include <memory>

enum ObjectIds {
    nullId = 0,
    skyId = 1,
    seaId = 2,
    groundId = 3,
    roomId = 4,
    boxId = 5,
    frameId = 6,
    lPicId = 7,
    rPicId = 8,
    teapotId = 9,
    spheresId = 10,
    floorId = 11
};

class Shader;
class ShaderProgram;
constexpr float DepthClearValue = 1.0f;

#include <wrl.h>
#include <array>
#include <dxgi1_6.h>
#include <directxtk12/GraphicsMemory.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/CommonStates.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define AO
#define COMPUTE
#include "../ShaderData.h"
struct CommandList {
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> cmd = nullptr;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
    HANDLE fenceEvent = nullptr;
    uint32_t fenceEventValue = 0;

    ID3D12GraphicsCommandList7* operator->() {
        return cmd.Get();
    }

    ID3D12GraphicsCommandList7* operator*() {
        return cmd.Get();
    }

    void Wait() {
        if (fence) {

            if (fence->GetCompletedValue() < fenceEventValue) {
                fence->SetEventOnCompletion(fenceEventValue, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }
        }
    }

    void Reset() {
        allocator->Reset();
        cmd->Reset(allocator.Get(), nullptr);
    }

    void Execute(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue) {
        cmd->Close();
        _queue->ExecuteCommandLists(
            1, reinterpret_cast<ID3D12CommandList**>(cmd.GetAddressOf())
        );
        _queue->Signal(fence.Get(), ++fenceEventValue);
    }

    void Init(Microsoft::WRL::ComPtr<ID3D12Device>& _device) {
        HRESULT hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
        if (FAILED(hr))
            throw std::runtime_error("Failed to create command allocator");

        hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            allocator.Get(), nullptr, IID_PPV_ARGS(&cmd));
        if (FAILED(hr))
            throw std::runtime_error("Failed to create command list");
        cmd->Close();

        hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        if (FAILED(hr))
            throw std::runtime_error("Failed to create fence");

        fenceEvent = CreateEventA(nullptr, false, false, nullptr);
    }
};

class Scene {
public:
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    static constexpr uint32_t FrameCount = 2;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<IDXGISwapChain4> m_swapchain;

    ComPtr<ID3D12Fence1> m_idleFence;
    uint32_t m_idleValue = 0;

    std::array<CommandList, FrameCount> m_shadowCmds;
    std::array<CommandList, FrameCount> m_geometryCmds;
    std::array<CommandList, FrameCount> m_lightingCmds;
    std::array<ComPtr<ID3D12Resource>, FrameCount> m_depthTextures;
    uint32_t m_depthTextureID = 0;
    ComPtr<ID3D12Resource> m_shadowTexture;
    uint32_t m_shadowMapID = 0;
    std::array<ComPtr<ID3D12Resource>, 2> m_blurMap;
    uint32_t m_blurMapID = 0;
    uint32_t m_blurMapSrvID = 0;
    uint32_t m_shadowTextureID = 0;
    uint32_t m_lightDataID = 0;
    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile> m_rtvHeap;
    std::unique_ptr<DirectX::DescriptorPile> m_dsvHeap;
    std::unique_ptr<DirectX::DescriptorPile> m_descHeap;

    std::unique_ptr<DirectX::CommonStates> m_states;

    std::array<ComPtr<ID3D12Resource>, 2> m_AOMap;
    uint32_t m_AOMapID = 0;
    uint32_t m_AOMapSrvID = 0;

    enum class FBOIndex {
        WorldPosition,
        Normal,
        Diffuse,
        SpecularAlpha,
        Count
    };
    std::array<FBO, FrameCount * 4> m_fbos;

    uint32_t m_frameIndex = 0;

    GLFWwindow* window;

    // @@ Declare interactive viewing variables here. (spin, tilt, ry, front back, ...)

    // Light parameters
    //float lightSpin, lightTilt, lightDist;
    //DirectX::SimpleMath::Vector3 lightPos;
    DirectX::SimpleMath::Vector3 m_lightPos{ 0, 2.f, -9.f };
    // @@ Perhaps declare additional scene lighting values here. (lightVal, lightAmb)

    HANDLE m_waitableObject;

    bool drawReflective;
    bool m_reset = false;
    int frameBufferMode = 0;
    bool nav;
    bool w_down, s_down, a_down, d_down;
    float Yaw, Pitch, speed, ry, front, back;
    DirectX::SimpleMath::Vector3 eye, cameraPos;
    DirectX::SimpleMath::Vector3 cameraForward;
    DirectX::SimpleMath::Vector3 right;
    DirectX::SimpleMath::Vector3 up;
    float m_momentBias = .003f;
    float m_lightFar = 10000.f;
    float m_lightNear = 0.1f;
    float m_depthBias = 0.005f;
    float m_shadowMin = -24;
    float m_shadowMax = 24;
    float last_time;
    float m_frameStart = 0;
    float m_frameEnd = 0;
    float m_frameTime = 0;
    float m_totalTime = 0;
    int m_numFrames = 0;
    float m_fps = 0;
    enum class ShadowMode {
        On,
        Off,
        Debug
    } m_shadowMode;

    enum class ReflectionMode {
        On,
        Off,
        Debug
    } m_reflectionMode;

    enum class BRDFMode {
        On,
        Off,
        Debug
    } m_brdfMode;

    enum class TextureMode {

    } m_textureMode;

    enum class IBLMode {

    } m_iblMode;

    bool m_flatshade;

    // Viewport
    int m_width, m_height;

    // Transformations
    DirectX::SimpleMath::Matrix WorldProj, WorldView, WorldInverse, ShadowView, ShadowProj;

    // All objects in the scene are children of this single root object.
    std::shared_ptr<Object> objectRoot;
    std::shared_ptr<Object> central, anim, room, floor, teapot, podium, sky,
        ground, sea, spheres, leftFrame, rightFrame;
    std::shared_ptr<Object> frame;
    std::shared_ptr<Object> light;

    std::vector<Object*> animated;
    Shapes::ProceduralGround* proceduralground;

    // Shader programs
    std::unique_ptr<ShaderProgram> m_lightingProgram;
    std::unique_ptr<ShaderProgram> m_geometryProgram;
    std::unique_ptr<ShaderProgram> m_shadowProgram;
    std::unique_ptr<ShaderProgram> m_copyProgram;
    std::unique_ptr<ShaderProgram> m_computeProgram;
    std::unique_ptr<ShaderProgram> m_computeVertical;

    std::unique_ptr<ShaderProgram> m_AOH;
    std::unique_ptr<ShaderProgram> m_AOV;

    // Ambient Occlusion
    std::unique_ptr<ShaderProgram> m_AOProgram;
    // @@ Declare additional shaders if necessary

    Texture m_irradianceMap;
    
    std::vector<ShaderData::Light> m_lights{};
    ShaderData::AoData m_aoData{
        .R = 1,
        .n = 10,
        .s = 0.5f,
        .k = 1,
    };
    ShaderData::ComputeData m_computeData{};

    // Options menu stuff
    bool show_demo_window;

    void InitializeScene();
    void BuildTransforms();
    void DrawMenu();
    void DrawScene();
    void EndFrame();

    void DrawShadow();
    void DrawGeometry();
    void DrawLighting();
    void DrawAO();

    void LoadShaders();

    ~Scene();
};
