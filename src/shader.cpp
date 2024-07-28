///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a shader program. This contains methods
// to build a shader program from multiples files containing vertex
// and pixel shader code, and a method to link the result.  When
// loaded (method "Use"), its vertex shader and pixel shader will be
// invoked for all geometry passing through the graphics pipeline.
// When done, unload it with method "Unuse".
////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <vector>

#include "shader.h"
#include <d3dcompiler.h>
#include <string>

#include <directxtk12/EffectPipelineStateDescription.h>
#include <directxtk12/CommonStates.h>
#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/RenderTargetState.h>
#include <array>

#include <directx-dxc/dxcapi.h>

// Reads a specified file into a string and returns the string.  The
// file is examined first to determine the needed string size.
std::vector<char> ReadFile(const char* name) {
    std::ifstream f;
    f.open(name, std::ios_base::binary); // Open
    f.seekg(0, std::ios_base::end); // Position at end
    int length = static_cast<int>(f.tellg()); //   to get the length

    std::vector<char> content(length + 1); // Create buffer of needed length
    f.seekg(0, std::ios_base::beg); // Position at beginning
    f.read(content.data(), length); //   to read complete file
    f.close(); // Close

    content[length] = char(0); // Finish with a NULL
    return content;
}

// Creates an empty shader program.
ShaderProgram::ShaderProgram() {
    using namespace DirectX;

    m_renderTargetState = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT };
}

// Use a shader program
void ShaderProgram::UseShader(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7>& _cmd) {
    if (m_compute) {
        _cmd->SetComputeRootSignature(m_rootSignature.Get());
    }
    else {
        _cmd->SetGraphicsRootSignature(m_rootSignature.Get());
    }
    _cmd->SetPipelineState(m_pipeline.Get());
}

void ShaderProgram::SetRenderTargetFormat(uint32_t _index, DXGI_FORMAT _format) {
    m_renderTargetState.rtvFormats[_index] = _format;
}

// Read, send to OpenGL, and compile a single file into a shader
// program.  In case of an error, retrieve and print the error log
// string.
void ShaderProgram::AddShader(
    const char* _fileName, 
    const Type _type,
    std::wstring _entry,
    std::vector<std::wstring> _defines
) {
    // Read the source from the named file
    std::vector<char> src = ReadFile(_fileName);

    // Create a shader and attach, hand it the source, and compile it.
    std::wstring target = L"";
    switch (_type) {
    case Type::Vertex:
        target = L"vs_6_6";
        break;

    case Type::Pixel:
        target = L"ps_6_6";
        break;

    case Type::Compute:
        target = L"cs_6_6";
        break;
    }

    std::vector<LPCWSTR> compilationArguments{
        L"-E",
        _entry.c_str(),
        L"-T",
        target.data(),
        DXC_ARG_WARNINGS_ARE_ERRORS
    };
    for (auto& define : _defines) {
        compilationArguments.push_back(L"-D");
        compilationArguments.push_back(define.c_str());
    }

    //if constexpr (_DEBUG) {
    //    compilationArguments.push_back(DXC_ARG_DEBUG);
    //}
    //else {
    //}
        compilationArguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);

    using namespace Microsoft::WRL;
    ComPtr<IDxcUtils> utils;
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create dxc utils");
    }

    ComPtr<IDxcCompiler3> compiler;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create compiler");
    }

    ComPtr<IDxcIncludeHandler> includeHandler;
    hr = utils->CreateDefaultIncludeHandler(&includeHandler);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create include handler");
    }

    int convertCount = MultiByteToWideChar(CP_UTF8, 0, _fileName, strlen(_fileName), nullptr, 0);
    if (convertCount == 0)
        throw std::runtime_error("failed to convert UTF-8 to UTF-16");
    std::vector<wchar_t> buffer(convertCount);
    convertCount = MultiByteToWideChar(CP_UTF8, 0, _fileName, strlen(_fileName), buffer.data(), buffer.size());
    if (convertCount == 0)
        throw std::runtime_error("failed to convert UTF-8 TO UTF-16");
    std::wstring wpath(buffer.data(), convertCount);

    ComPtr<IDxcBlobEncoding> sourceBlob{};
    hr = utils->LoadFile(wpath.c_str(), nullptr, &sourceBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to load file");
    }

    DxcBuffer sourceBuffer{
        .Ptr = sourceBlob->GetBufferPointer(),
        .Size = sourceBlob->GetBufferSize(),
        .Encoding = 0u
    };

    ComPtr<IDxcResult> compiledShaderBuffer{};
    hr = compiler->Compile(&sourceBuffer,
        compilationArguments.data(),
        static_cast<uint32_t>(compilationArguments.size()),
        includeHandler.Get(),
        IID_PPV_ARGS(&compiledShaderBuffer)
    );
    if (FAILED(hr)) {
        throw std::runtime_error("failed to compile shader");
    }

    ComPtr<IDxcBlobUtf8> errors{};
    ComPtr<IDxcBlobWide> errorsName{};
    hr = compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), &errorsName);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to get error");
    }
    if (errors && errors->GetStringLength() > 0) {
        const LPCSTR errorMessage = errors->GetStringPointer();
        throw std::runtime_error(errorMessage);
    }

    //ComPtr<IDxcBlob> reflectionBlob{};
    //hr = compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob));
    //if (FAILED(hr)) {
    //    throw std::runtime_error("failed to get reflection data");
    //}

    ComPtr<IDxcBlob> shader;
    compiledShaderBuffer->GetResult(&shader);
    m_shaderData[_type] = shader;

    ComPtr<IDxcBlob> reflectionBlob{};
    hr = compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to reflect");
    }

    const DxcBuffer reflectionBuffer{
        .Ptr = reflectionBlob->GetBufferPointer(),
        .Size = reflectionBlob->GetBufferSize(),
        .Encoding = 0
    };

    ComPtr<ID3D12ShaderReflection> shaderReflection{};
    utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
    D3D12_SHADER_DESC shaderDesc{};
    shaderReflection->GetDesc(&shaderDesc);
    if (_type == Type::Pixel) {
        m_renderTargetState.numRenderTargets = shaderDesc.OutputParameters;
        if (shaderDesc.OutputParameters > 1) {
            for (uint32_t i = 0; i < shaderDesc.OutputParameters; i++) {
                m_renderTargetState.rtvFormats[i] = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }
        else {
            m_renderTargetState.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }
}

// Link a shader program after all the shader files have been added
// with the AddShader method.  In case of an error, retrieve and print
// the error log string.
void ShaderProgram::LinkProgram(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device,
    D3D12_CULL_MODE _cullMode,
    D3D12_DEPTH_STENCIL_DESC _depthStencil,
    D3D12_BLEND_DESC _blendDesc
) {
    // Link program and check the status
    using namespace DirectX;
    if (m_shaderData.size() == 1) {
        auto& [type, shader] = *m_shaderData.begin();
        if (type == Type::Vertex) {
            LinkGraphicsProgram(_device, _cullMode, _depthStencil, _blendDesc);
        }
        else if (type == Type::Pixel) {
            throw std::runtime_error("Missing Vertex Shader");
        }
        else if (type == Type::Compute) {
            LinkComputeProgram(_device);
        }
    }
    else if (m_shaderData.size() == 0) {
        throw std::runtime_error("No Shaders");
    }
    else {
        LinkGraphicsProgram(_device, _cullMode, _depthStencil, _blendDesc);
    }
}

void ShaderProgram::LinkComputeProgram(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device
) {
    using namespace DirectX;
    using namespace Microsoft::WRL;
    ComPtr<IDxcBlob> computeShader = m_shaderData[Type::Compute];
    if (!computeShader) {
        throw std::runtime_error("No Compute Shader");
    }

    D3D12_SHADER_BYTECODE computeByteCode{
        .pShaderBytecode = computeShader->GetBufferPointer(),
        .BytecodeLength = computeShader->GetBufferSize(),
    };

    HRESULT hr = _device->CreateRootSignature(
        0, computeShader->GetBufferPointer(),
        computeShader->GetBufferSize(), 
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr)) {
        throw std::runtime_error("failed to create root signature");
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc{
        .pRootSignature = m_rootSignature.Get(),
        .CS = computeByteCode,
        .NodeMask = 0,
        .Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE
    };

    hr = _device->CreateComputePipelineState(
        &pipelineStateDesc,
        IID_PPV_ARGS(&m_pipeline)
    );

    if (FAILED(hr)) {
        throw std::runtime_error("failed to create compute pipeline state");
    }

    m_compute = true;
}
void ShaderProgram::LinkGraphicsProgram(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device,
    D3D12_CULL_MODE _cullMode,
    D3D12_DEPTH_STENCIL_DESC _depthStencil,
    D3D12_BLEND_DESC _blendDesc
) {
    using namespace DirectX;
    
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShader = m_shaderData[Type::Vertex];
    if (!vertexShader) {
        throw std::runtime_error("No Valid Vertex Shader");
    }
    Microsoft::WRL::ComPtr<IDxcBlob> pixelShader = m_shaderData[Type::Pixel];
    D3D12_SHADER_BYTECODE pixelByteCode{};
    if (pixelShader) {
        pixelByteCode = {
        .pShaderBytecode = pixelShader->GetBufferPointer(),
        .BytecodeLength = pixelShader->GetBufferSize(),
        };
    }
    else {
        m_renderTargetState.numRenderTargets = 0;
        m_renderTargetState.rtvFormats[0] = DXGI_FORMAT_UNKNOWN;
        m_renderTargetState.rtvFormats[1] = DXGI_FORMAT_UNKNOWN;
        m_renderTargetState.rtvFormats[2] = DXGI_FORMAT_UNKNOWN;
        m_renderTargetState.rtvFormats[3] = DXGI_FORMAT_UNKNOWN;
        m_renderTargetState.rtvFormats[4] = DXGI_FORMAT_UNKNOWN;
        m_renderTargetState.rtvFormats[5] = DXGI_FORMAT_UNKNOWN;
        m_renderTargetState.rtvFormats[6] = DXGI_FORMAT_UNKNOWN;
        m_renderTargetState.rtvFormats[7] = DXGI_FORMAT_UNKNOWN;
    }

    HRESULT hr = _device->CreateRootSignature(0, vertexShader->GetBufferPointer(),
        vertexShader->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
        throw std::runtime_error("Failed to Create Root Signature");

    auto inputLayout = DirectX::GeometricPrimitive::VertexType::InputLayout;
    auto pipelineState = DirectX::EffectPipelineStateDescription(
        &inputLayout,
        _blendDesc,
        _depthStencil,
        CommonStates::CullCounterClockwise,
        m_renderTargetState
    );
    pipelineState.rasterizerDesc.FrontCounterClockwise = false;
    pipelineState.rasterizerDesc.CullMode = _cullMode;
    D3D12_SHADER_BYTECODE vertexByteCode{
        .pShaderBytecode = vertexShader->GetBufferPointer(),
        .BytecodeLength = vertexShader->GetBufferSize(),
    };


    pipelineState.CreatePipelineState(
        _device.Get(),
        m_rootSignature.Get(),
        vertexByteCode,
        pixelByteCode,
        &m_pipeline
    );
    m_compute = false;
}
