////////////////////////////////////////////////////////////////////////
// A small library of object shapes (ground plane, sphere, and the
// famous Utah teapot), each created as a Vertex Array Object (VAO).
// This is the most efficient way to get geometry into the OpenGL
// graphics pipeline.
//
// Each vertex is specified as four attributes which are made
// available in a vertex shader in the following attribute slots.
//
// position,        vec4,   attribute #0
// normal,          vec3,   attribute #1
// texture coord,   vec3,   attribute #2
// tangent,         vec3,   attribute #3
//
// An instance of any of these shapes is create with a single call:
//    unsigned int obj = CreateSphere(divisions, &quadCount);
// and drawn by:
//    glBindVertexArray(vaoID);
//    glDrawElements(GL_TRIANGLES, vertexcount, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);
////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <stdlib.h>
#include <vector>

#include "math.h"
#include "rply.h"
#include "shapes.h"
#include "simplexnoise.h"
#include <algorithm>

#include <directxtk12/BufferHelpers.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/GeometricPrimitive.h>
#include "scene.h"

const float PI = 3.14159f;
const float rad = PI / 180.0f;

using namespace DirectX;
using namespace DirectX::SimpleMath;
void pushquad(std::vector<XMINT3>& Tri, int i, int j, int k, int l) {
    Tri.push_back(XMINT3(i, j, k));
    Tri.push_back(XMINT3(i, k, l));
}

// Batch up all the data defining a shape to be drawn (example: the
// teapot) as a Vertex Array object (VAO) and send it to the graphics
// card.  Return an OpenGL identifier for the created VAO.
Shapes::Shape::VAO VaoFromTris(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device,
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue,
    std::vector<Vector4> Pnt,
    std::vector<Vector3> Nrm,
    std::vector<Vector2> Tex,
    std::vector<Vector3> Tan,
    std::vector<XMINT3> Tri) {
    struct Vertex {
        Vector4 point;
        Vector3 normal;
        Vector2 tex;
        Vector3 tangent;
    };
    std::vector<Vertex> vertices;
    for (size_t i = 0; i < Pnt.size(); i++) {
        Vertex vertex;
        vertex.point = Pnt[i];
        if (i < Nrm.size()) {
            vertex.normal = Nrm[i];
        }
        if (i < Tex.size()) {
            vertex.tex = Tex[i];
        }
        if (i < Tan.size()) {
            vertex.tangent = Tan[i];
        }
        vertices.push_back(vertex);
    }

    Shapes::Shape::VAO vao;
    printf("VaoFromTris %zu %zu\n", Pnt.size(), Tri.size());
    DirectX::ResourceUploadBatch uploadBatch(_device.Get());
    uploadBatch.Begin();
    DirectX::CreateStaticBuffer(
        _device.Get(),
        uploadBatch,
        vertices.data(),
        vertices.size(),
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        &vao.m_vertexBuffer
    );

    DirectX::CreateStaticBuffer(
        _device.Get(),
        uploadBatch,
        Tri.data(),
        Tri.size(),
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        &vao.m_indexBuffer
    );
    vao.m_indexCount = static_cast<uint32_t>(Tri.size()) * 3;
    uploadBatch.End(_queue.Get()).wait();

    vao.m_vbv = {
        .BufferLocation = vao.m_vertexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(vertices[0])),
        .StrideInBytes = sizeof(vertices[0])
    };

    vao.m_ibv = {
        .BufferLocation = vao.m_indexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = static_cast<UINT>(Tri.size() * 3 * sizeof(UINT)),
        .Format = DXGI_FORMAT_R32_UINT
    };
    return vao;
}

void Shapes::Shape::MakeVAO(
    Microsoft::WRL::ComPtr<ID3D12Device>& _device, 
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue
) {
    m_vao = VaoFromTris(_device, _queue, Pnt, Nrm, Tex, Tan, Tri);
    count = static_cast<unsigned int>(Tri.size());
}

void Shapes::Shape::DrawVAO(CommandList& _cmd) {
    _cmd->IASetVertexBuffers(0, 1, &m_vao.m_vbv);
    _cmd->IASetIndexBuffer(&m_vao.m_ibv);
    _cmd->DrawIndexedInstanced(m_vao.m_indexCount, 1, 0, 0, 0);
    //CHECKERROR;
    //glBindVertexArray(vaoID);
    //CHECKERROR;
    //glDrawElements(GL_TRIANGLES, 3 * count, GL_UNSIGNED_INT, 0);
    //CHECKERROR;
    //glBindVertexArray(0);
}

////////////////////////////////////////////////////////////////////////////////
// Data for the Utah teapot.  It consists of a list of 306 control
// points, and 32 Bezier patches, each defined by 16 control points
// (specified as 1-based indices into the control point array).  This
// evaluates the Bernstein basis functions directly to compute the
// vertices.  This is not the most efficient, but it is the
// easiest. (Which is fine for an operation done once at startup
// time.)
unsigned int TeapotIndex[][16] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    4, 17, 18, 19, 8, 20, 21, 22, 12, 23, 24, 25, 16, 26, 27, 28,
    19, 29, 30, 31, 22, 32, 33, 34, 25, 35, 36, 37, 28, 38, 39, 40,
    31, 41, 42, 1, 34, 43, 44, 5, 37, 45, 46, 9, 40, 47, 48, 13,
    13, 14, 15, 16, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    16, 26, 27, 28, 52, 61, 62, 63, 56, 64, 65, 66, 60, 67, 68, 69,
    28, 38, 39, 40, 63, 70, 71, 72, 66, 73, 74, 75, 69, 76, 77, 78,
    40, 47, 48, 13, 72, 79, 80, 49, 75, 81, 82, 53, 78, 83, 84, 57,
    57, 58, 59, 60, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    60, 67, 68, 69, 88, 97, 98, 99, 92, 100, 101, 102, 96, 103, 104, 105,
    69, 76, 77, 78, 99, 106, 107, 108, 102, 109, 110, 111, 105, 112, 113, 114,
    78, 83, 84, 57, 108, 115, 116, 85, 111, 117, 118, 89, 114, 119, 120, 93,
    121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
    124, 137, 138, 121, 128, 139, 140, 125, 132, 141, 142, 129, 136, 143, 144, 133,
    133, 134, 135, 136, 145, 146, 147, 148, 149, 150, 151, 152, 69, 153, 154, 155,
    136, 143, 144, 133, 148, 156, 157, 145, 152, 158, 159, 149, 155, 160, 161, 69,
    162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
    165, 178, 179, 162, 169, 180, 181, 166, 173, 182, 183, 170, 177, 184, 185, 174,
    174, 175, 176, 177, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
    177, 184, 185, 174, 189, 198, 199, 186, 193, 200, 201, 190, 197, 202, 203, 194,
    204, 204, 204, 204, 207, 208, 209, 210, 211, 211, 211, 211, 212, 213, 214, 215,
    204, 204, 204, 204, 210, 217, 218, 219, 211, 211, 211, 211, 215, 220, 221, 222,
    204, 204, 204, 204, 219, 224, 225, 226, 211, 211, 211, 211, 222, 227, 228, 229,
    204, 204, 204, 204, 226, 230, 231, 207, 211, 211, 211, 211, 229, 232, 233, 212,
    212, 213, 214, 215, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245,
    215, 220, 221, 222, 237, 246, 247, 248, 241, 249, 250, 251, 245, 252, 253, 254,
    222, 227, 228, 229, 248, 255, 256, 257, 251, 258, 259, 260, 254, 261, 262, 263,
    229, 232, 233, 212, 257, 264, 265, 234, 260, 266, 267, 238, 263, 268, 269, 242,
    270, 270, 270, 270, 279, 280, 281, 282, 275, 276, 277, 278, 271, 272, 273, 274,
    270, 270, 270, 270, 282, 289, 290, 291, 278, 286, 287, 288, 274, 283, 284, 285,
    270, 270, 270, 270, 291, 298, 299, 300, 288, 295, 296, 297, 285, 292, 293, 294,
    270, 270, 270, 270, 300, 305, 306, 279, 297, 303, 304, 275, 294, 301, 302, 271
};

#pragma warning(push)
#pragma warning(disable : 4305)
Vector3 TeapotPoints[] = {
    Vector3(1.4f, 0.0f, 2.4f), Vector3(1.4f, -0.784f, 2.4f), Vector3(0.784f, -1.4f, 2.4f),
    Vector3(0.0f, -1.4f, 2.4f), Vector3(1.3375f, 0.0f, 2.53125f),
    Vector3(1.3375f, -0.749f, 2.53125f), Vector3(0.749f, -1.3375f, 2.53125f),
    Vector3(0.0f, -1.3375f, 2.53125f), Vector3(1.4375f, 0.0f, 2.53125f),
    Vector3(1.4375f, -0.805f, 2.53125f), Vector3(0.805f, -1.4375f, 2.53125f),
    Vector3(0.0f, -1.4375f, 2.53125f), Vector3(1.5f, 0.0f, 2.4f), Vector3(1.5f, -0.84f, 2.4f),
    Vector3(0.84f, -1.5f, 2.4f), Vector3(0.0f, -1.5f, 2.4f), Vector3(-0.784f, -1.4f, 2.4f),
    Vector3(-1.4f, -0.784, 2.4), Vector3(-1.4, 0.0, 2.4),
    Vector3(-0.749f, -1.3375, 2.53125), Vector3(-1.3375, -0.749, 2.53125),
    Vector3(-1.3375f, 0.0, 2.53125), Vector3(-0.805, -1.4375, 2.53125),
    Vector3(-1.4375f, -0.805, 2.53125), Vector3(-1.4375, 0.0, 2.53125),
    Vector3(-0.84f, -1.5, 2.4), Vector3(-1.5, -0.84, 2.4), Vector3(-1.5, 0.0, 2.4),
    Vector3(-1.4f, 0.784, 2.4), Vector3(-0.784, 1.4, 2.4), Vector3(0.0, 1.4, 2.4),
    Vector3(-1.3375f, 0.749, 2.53125), Vector3(-0.749, 1.3375, 2.53125),
    Vector3(0.0f, 1.3375, 2.53125), Vector3(-1.4375, 0.805, 2.53125),
    Vector3(-0.805f, 1.4375, 2.53125), Vector3(0.0, 1.4375, 2.53125),
    Vector3(-1.5f, 0.84, 2.4), Vector3(-0.84, 1.5, 2.4), Vector3(0.0, 1.5, 2.4),
    Vector3(0.784f, 1.4, 2.4), Vector3(1.4, 0.784, 2.4), Vector3(0.749, 1.3375, 2.53125),
    Vector3(1.3375f, 0.749, 2.53125), Vector3(0.805, 1.4375, 2.53125),
    Vector3(1.4375f, 0.805, 2.53125), Vector3(0.84, 1.5, 2.4), Vector3(1.5, 0.84, 2.4),
    Vector3(1.75f, 0.0, 1.875), Vector3(1.75, -0.98, 1.875), Vector3(0.98, -1.75, 1.875),
    Vector3(0.0f, -1.75, 1.875), Vector3(2.0, 0.0, 1.35), Vector3(2.0, -1.12, 1.35),
    Vector3(1.12f, -2.0, 1.35), Vector3(0.0, -2.0, 1.35), Vector3(2.0, 0.0, 0.9),
    Vector3(2.0f, -1.12, 0.9), Vector3(1.12, -2.0, 0.9), Vector3(0.0, -2.0, 0.9),
    Vector3(-0.98f, -1.75, 1.875), Vector3(-1.75, -0.98, 1.875),
    Vector3(-1.75f, 0.0, 1.875), Vector3(-1.12, -2.0, 1.35), Vector3(-2.0, -1.12, 1.35),
    Vector3(-2.0f, 0.0, 1.35), Vector3(-1.12, -2.0, 0.9), Vector3(-2.0, -1.12, 0.9),
    Vector3(-2.0f, 0.0, 0.9), Vector3(-1.75, 0.98, 1.875), Vector3(-0.98, 1.75, 1.875),
    Vector3(0.0f, 1.75, 1.875), Vector3(-2.0, 1.12, 1.35), Vector3(-1.12, 2.0, 1.35),
    Vector3(0.0f, 2.0, 1.35), Vector3(-2.0, 1.12, 0.9), Vector3(-1.12, 2.0, 0.9),
    Vector3(0.0f, 2.0, 0.9), Vector3(0.98, 1.75, 1.875), Vector3(1.75, 0.98, 1.875),
    Vector3(1.12f, 2.0, 1.35), Vector3(2.0, 1.12, 1.35), Vector3(1.12, 2.0, 0.9),
    Vector3(2.0f, 1.12, 0.9), Vector3(2.0, 0.0, 0.45), Vector3(2.0, -1.12, 0.45),
    Vector3(1.12f, -2.0, 0.45), Vector3(0.0, -2.0, 0.45), Vector3(1.5, 0.0, 0.225),
    Vector3(1.5f, -0.84, 0.225), Vector3(0.84, -1.5, 0.225), Vector3(0.0, -1.5, 0.225),
    Vector3(1.5f, 0.0, 0.15), Vector3(1.5, -0.84, 0.15), Vector3(0.84, -1.5, 0.15),
    Vector3(0.0f, -1.5, 0.15), Vector3(-1.12, -2.0, 0.45), Vector3(-2.0, -1.12, 0.45),
    Vector3(-2.0f, 0.0, 0.45), Vector3(-0.84, -1.5, 0.225), Vector3(-1.5, -0.84, 0.225),
    Vector3(-1.5f, 0.0, 0.225), Vector3(-0.84, -1.5, 0.15), Vector3(-1.5, -0.84, 0.15),
    Vector3(-1.5f, 0.0, 0.15), Vector3(-2.0, 1.12, 0.45), Vector3(-1.12, 2.0, 0.45),
    Vector3(0.0f, 2.0, 0.45), Vector3(-1.5, 0.84, 0.225), Vector3(-0.84, 1.5, 0.225),
    Vector3(0.0f, 1.5, 0.225), Vector3(-1.5, 0.84, 0.15), Vector3(-0.84, 1.5, 0.15),
    Vector3(0.0f, 1.5, 0.15), Vector3(1.12, 2.0, 0.45), Vector3(2.0, 1.12, 0.45),
    Vector3(0.84f, 1.5, 0.225), Vector3(1.5, 0.84, 0.225), Vector3(0.84, 1.5, 0.15),
    Vector3(1.5f, 0.84, 0.15), Vector3(-1.6, 0.0, 2.025), Vector3(-1.6, -0.3, 2.025),
    Vector3(-1.5f, -0.3, 2.25), Vector3(-1.5, 0.0, 2.25), Vector3(-2.3, 0.0, 2.025),
    Vector3(-2.3f, -0.3, 2.025), Vector3(-2.5, -0.3, 2.25), Vector3(-2.5, 0.0, 2.25),
    Vector3(-2.7f, 0.0, 2.025), Vector3(-2.7, -0.3, 2.025), Vector3(-3.0, -0.3, 2.25),
    Vector3(-3.0f, 0.0, 2.25), Vector3(-2.7, 0.0, 1.8), Vector3(-2.7, -0.3, 1.8),
    Vector3(-3.0f, -0.3, 1.8), Vector3(-3.0, 0.0, 1.8), Vector3(-1.5, 0.3, 2.25),
    Vector3(-1.6f, 0.3, 2.025), Vector3(-2.5, 0.3, 2.25), Vector3(-2.3, 0.3, 2.025),
    Vector3(-3.0f, 0.3, 2.25), Vector3(-2.7, 0.3, 2.025), Vector3(-3.0, 0.3, 1.8),
    Vector3(-2.7f, 0.3, 1.8), Vector3(-2.7, 0.0, 1.575), Vector3(-2.7, -0.3, 1.575),
    Vector3(-3.0f, -0.3, 1.35), Vector3(-3.0, 0.0, 1.35), Vector3(-2.5, 0.0, 1.125),
    Vector3(-2.5f, -0.3, 1.125), Vector3(-2.65, -0.3, 0.9375),
    Vector3(-2.65f, 0.0, 0.9375), Vector3(-2.0, -0.3, 0.9), Vector3(-1.9, -0.3, 0.6),
    Vector3(-1.9f, 0.0, 0.6), Vector3(-3.0, 0.3, 1.35), Vector3(-2.7, 0.3, 1.575),
    Vector3(-2.65f, 0.3, 0.9375), Vector3(-2.5, 0.3, 1.125), Vector3(-1.9, 0.3, 0.6),
    Vector3(-2.0f, 0.3, 0.9), Vector3(1.7, 0.0, 1.425), Vector3(1.7, -0.66, 1.425),
    Vector3(1.7f, -0.66, 0.6), Vector3(1.7, 0.0, 0.6), Vector3(2.6, 0.0, 1.425),
    Vector3(2.6f, -0.66, 1.425), Vector3(3.1, -0.66, 0.825), Vector3(3.1, 0.0, 0.825),
    Vector3(2.3f, 0.0, 2.1), Vector3(2.3, -0.25, 2.1), Vector3(2.4, -0.25, 2.025),
    Vector3(2.4f, 0.0, 2.025), Vector3(2.7, 0.0, 2.4), Vector3(2.7, -0.25, 2.4),
    Vector3(3.3f, -0.25, 2.4), Vector3(3.3, 0.0, 2.4), Vector3(1.7, 0.66, 0.6),
    Vector3(1.7f, 0.66, 1.425), Vector3(3.1, 0.66, 0.825), Vector3(2.6, 0.66, 1.425),
    Vector3(2.4f, 0.25, 2.025), Vector3(2.3, 0.25, 2.1), Vector3(3.3, 0.25, 2.4),
    Vector3(2.7f, 0.25, 2.4), Vector3(2.8, 0.0, 2.475), Vector3(2.8, -0.25, 2.475),
    Vector3(3.525f, -0.25, 2.49375), Vector3(3.525, 0.0, 2.49375),
    Vector3(2.9f, 0.0, 2.475), Vector3(2.9, -0.15, 2.475), Vector3(3.45, -0.15, 2.5125),
    Vector3(3.45f, 0.0, 2.5125), Vector3(2.8, 0.0, 2.4), Vector3(2.8, -0.15, 2.4),
    Vector3(3.2f, -0.15, 2.4), Vector3(3.2, 0.0, 2.4), Vector3(3.525, 0.25, 2.49375),
    Vector3(2.8f, 0.25, 2.475), Vector3(3.45, 0.15, 2.5125), Vector3(2.9, 0.15, 2.475),
    Vector3(3.2f, 0.15, 2.4), Vector3(2.8, 0.15, 2.4), Vector3(0.0, 0.0, 3.15),
    Vector3(0.0f, -0.002, 3.15), Vector3(0.002, 0.0, 3.15), Vector3(0.8, 0.0, 3.15),
    Vector3(0.8f, -0.45, 3.15), Vector3(0.45, -0.8, 3.15), Vector3(0.0, -0.8, 3.15),
    Vector3(0.0f, 0.0, 2.85), Vector3(0.2, 0.0, 2.7), Vector3(0.2, -0.112, 2.7),
    Vector3(0.112f, -0.2, 2.7), Vector3(0.0, -0.2, 2.7), Vector3(-0.002, 0.0, 3.15),
    Vector3(-0.45f, -0.8, 3.15), Vector3(-0.8, -0.45, 3.15), Vector3(-0.8, 0.0, 3.15),
    Vector3(-0.112f, -0.2, 2.7), Vector3(-0.2, -0.112, 2.7), Vector3(-0.2, 0.0, 2.7),
    Vector3(0.0f, 0.002, 3.15), Vector3(-0.8, 0.45, 3.15), Vector3(-0.45, 0.8, 3.15),
    Vector3(0.0f, 0.8, 3.15), Vector3(-0.2, 0.112, 2.7), Vector3(-0.112, 0.2, 2.7),
    Vector3(0.0f, 0.2, 2.7), Vector3(0.45, 0.8, 3.15), Vector3(0.8, 0.45, 3.15),
    Vector3(0.112f, 0.2, 2.7), Vector3(0.2, 0.112, 2.7), Vector3(0.4, 0.0, 2.55),
    Vector3(0.4f, -0.224, 2.55), Vector3(0.224, -0.4, 2.55), Vector3(0.0, -0.4, 2.55),
    Vector3(1.3f, 0.0, 2.55), Vector3(1.3, -0.728, 2.55), Vector3(0.728, -1.3, 2.55),
    Vector3(0.0f, -1.3, 2.55), Vector3(1.3, 0.0, 2.4), Vector3(1.3, -0.728, 2.4),
    Vector3(0.728f, -1.3, 2.4), Vector3(0.0, -1.3, 2.4), Vector3(-0.224, -0.4, 2.55),
    Vector3(-0.4f, -0.224, 2.55), Vector3(-0.4, 0.0, 2.55), Vector3(-0.728, -1.3f, 2.55f),
    Vector3(-1.3f, -0.728, 2.55), Vector3(-1.3, 0.0, 2.55), Vector3(-0.728, -1.3f, 2.4f),
    Vector3(-1.3f, -0.728, 2.4), Vector3(-1.3, 0.0, 2.4), Vector3(-0.4, 0.224f, 2.55f),
    Vector3(-0.224f, 0.4f, 2.55f), Vector3(0.0f, 0.4f, 2.55f), Vector3(-1.3f, 0.728f, 2.55f),
    Vector3(-0.728f, 1.3f, 2.55f), Vector3(0.0f, 1.3f, 2.55f), Vector3(-1.3f, 0.728f, 2.4f),
    Vector3(-0.728f, 1.3f, 2.4f), Vector3(0.0f, 1.3f, 2.4f), Vector3(0.224f, 0.4f, 2.55f),
    Vector3(0.4f, 0.224f, 2.55f), Vector3(0.728f, 1.3f, 2.55f), Vector3(1.3f, 0.728f, 2.55f),
    Vector3(0.728f, 1.3f, 2.4f), Vector3(1.3f, 0.728f, 2.4f), Vector3(0.0f, 0.0f, 0.0f),
    Vector3(1.5f, 0.0f, 0.15f), Vector3(1.5f, 0.84f, 0.15f), Vector3(0.84f, 1.5f, 0.15f),
    Vector3(0.0f, 1.5f, 0.15f), Vector3(1.5f, 0.0f, 0.075f), Vector3(1.5f, 0.84f, 0.075f),
    Vector3(0.84f, 1.5f, 0.075f), Vector3(0.0f, 1.5f, 0.075f), Vector3(1.425f, 0.0f, 0.0f),
    Vector3(1.425f, 0.798f, 0.0f), Vector3(0.798f, 1.425f, 0.0f), Vector3(0.0f, 1.425f, 0.0f),
    Vector3(-0.84f, 1.5f, 0.15f), Vector3(-1.5f, 0.84f, 0.15f), Vector3(-1.5f, 0.0f, 0.15f),
    Vector3(-0.84f, 1.5f, 0.075f), Vector3(-1.5f, 0.84f, 0.075f), Vector3(-1.5f, 0.0f, 0.075f),
    Vector3(-0.798f, 1.425f, 0.0f), Vector3(-1.425f, 0.798f, 0.0f), Vector3(-1.425f, 0.0f, 0.0f),
    Vector3(-1.5f, -0.84f, 0.15f), Vector3(-0.84f, -1.5f, 0.15f), Vector3(0.0f, -1.5f, 0.15f),
    Vector3(-1.5f, -0.84f, 0.075f), Vector3(-0.84f, -1.5f, 0.075f), Vector3(0.0f, -1.5f, 0.075f),
    Vector3(-1.425f, -0.798f, 0.0f), Vector3(-0.798f, -1.425f, 0.0f),
    Vector3(0.0f, -1.425f, 0.0f), Vector3(0.84f, -1.5f, 0.15f), Vector3(1.5f, -0.84f, 0.15f),
    Vector3(0.84f, -1.5f, 0.075f), Vector3(1.5f, -0.84f, 0.075f), Vector3(0.798f, -1.425f, 0.0f),
    Vector3(1.425f, -0.798f, 0.0f)
};
#pragma warning(pop)

////////////////////////////////////////////////////////////////////////////////
// Builds a Vertex Array Object for the Utah teapot.  Each of the 32
// patches is represented by an n by n grid of quads triangulated.
Shapes::Teapot::Teapot(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue) {
    diffuseColor = Vector3(0.5f, 0.5f, 0.1f);
    specularColor = Vector3(1.0f, 1.0f, 1.0f);
    shininess = 120.0;
    animate = true;

    int npatches = sizeof(TeapotIndex) / sizeof(TeapotIndex[0]); // Should be 32 patches for the teapot
    const int nv = npatches * (n + 1) * (n + 1);
    int nq = npatches * n * n;

    for (int p = 0; p < npatches; p++) { // For each patch
        for (int i = 0; i <= n; i++) { // Grid in u direction
            float u = float(i) / n;
            for (int j = 0; j <= n; j++) { // Grid if v direction
                float v = float(j) / n;

                // Four u weights
                float u0 = (1.0f - u) * (1.0f - u) * (1.0f - u);
                float u1 = 3.0f * (1.0f - u) * (1.0f - u) * u;
                float u2 = 3.0f * (1.0f - u) * u * u;
                float u3 = u * u * u;

                // Three du weights
                float du0 = (1.0f - u) * (1.0f - u);
                float du1 = 2.0f * (1.0f - u) * u;
                float du2 = u * u;

                // Four v weights
                float v0 = (1.0f - v) * (1.0f - v) * (1.0f - v);
                float v1 = 3.0f * (1.0f - v) * (1.0f - v) * v;
                float v2 = 3.0f * (1.0f - v) * v * v;
                float v3 = v * v * v;

                // Three dv weights
                float dv0 = (1.0f - v) * (1.0f - v);
                float dv1 = 2.0f * (1.0f - v) * v;
                float dv2 = v * v;

                // Grab the 16 control points for Bezier patch.
                Vector3* p00 = &TeapotPoints[TeapotIndex[p][0] - 1];
                Vector3* p01 = &TeapotPoints[TeapotIndex[p][1] - 1];
                Vector3* p02 = &TeapotPoints[TeapotIndex[p][2] - 1];
                Vector3* p03 = &TeapotPoints[TeapotIndex[p][3] - 1];
                Vector3* p10 = &TeapotPoints[TeapotIndex[p][4] - 1];
                Vector3* p11 = &TeapotPoints[TeapotIndex[p][5] - 1];
                Vector3* p12 = &TeapotPoints[TeapotIndex[p][6] - 1];
                Vector3* p13 = &TeapotPoints[TeapotIndex[p][7] - 1];
                Vector3* p20 = &TeapotPoints[TeapotIndex[p][8] - 1];
                Vector3* p21 = &TeapotPoints[TeapotIndex[p][9] - 1];
                Vector3* p22 = &TeapotPoints[TeapotIndex[p][10] - 1];
                Vector3* p23 = &TeapotPoints[TeapotIndex[p][11] - 1];
                Vector3* p30 = &TeapotPoints[TeapotIndex[p][12] - 1];
                Vector3* p31 = &TeapotPoints[TeapotIndex[p][13] - 1];
                Vector3* p32 = &TeapotPoints[TeapotIndex[p][14] - 1];
                Vector3* p33 = &TeapotPoints[TeapotIndex[p][15] - 1];

                // Evaluate the Bezier patch at (u,v)
                Vector3 V = u0 * v0 * (*p00) + u0 * v1 * (*p01) + u0 * v2 * 
                    (*p02) + u0 * v3 * (*p03) + u1 * v0 * 
                    (*p10) + u1 * v1 * (*p11) + u1 * v2 * 
                    (*p12) + u1 * v3 * (*p13) + u2 * v0 * 
                    (*p20) + u2 * v1 * (*p21) + u2 * v2 * 
                    (*p22) + u2 * v3 * (*p23) + u3 * v0 * 
                    (*p30) + u3 * v1 * (*p31) + u3 * v2 * 
                    (*p32) + u3 * v3 * (*p33);
                //*pp++ = Vector4(V[0], V[1], V[2], 1.0);
                Pnt.push_back(Vector4(V.x, V.y, V.z, 1.0));
                Tex.push_back(Vector2(u, v));

                // Evaluate the u-tangent of the Bezier patch at (u,v)
                Vector3 du = du0 * v0 * (*p10 - *p00) + du0 * v1 * 
                    (*p11 - *p01) + du0 * v2 * 
                    (*p12 - *p02) + du0 * v3 * 
                    (*p13 - *p03) + du1 * v0 * 
                    (*p20 - *p10) + du1 * v1 * 
                    (*p21 - *p11) + du1 * v2 * 
                    (*p22 - *p12) + du1 * v3 * 
                    (*p23 - *p13) + du2 * v0 * 
                    (*p30 - *p20) + du2 * v1 * 
                    (*p31 - *p21) + du2 * v2 * 
                    (*p32 - *p22) + du2 * v3 * 
                    (*p33 - *p23);
                Tan.push_back(du);

                // Evaluate the v-tangent of the Bezier patch at (u,v)
                Vector3 dv = u0 * dv0 * (*p01 - *p00) + u0 * dv1 * (*p02 - *p01) + u0 * dv2 * 
                    (*p03 - *p02) + u1 * dv0 * (*p11 - *p10) + u1 * dv1 * (*p12 - *p11) + u1 * dv2 * 
                    (*p13 - *p12) + u2 * dv0 * (*p21 - *p20) + u2 * dv1 * (*p22 - *p21) + u2 * dv2 * 
                    (*p23 - *p22) + u3 * dv0 * (*p31 - *p30) + u3 * dv1 * (*p32 - *p31) + u3 * dv2 * (*p33 - *p32);

                // Calculate the surface normal as the cross product of the two tangents.
                Nrm.push_back(dv.Cross(du));

                //-(du[1]*dv[2]-du[2]*dv[1]);
                //*np++ = -(du[2]*dv[0]-du[0]*dv[2]);
                //*np++ = -(du[0]*dv[1]-du[1]*dv[0]);

                // Create a quad for all but the first edge vertices
                if (i > 0 && j > 0)
                    pushquad(Tri,
                        p * (n + 1) * (n + 1) + (i - 1) * (n + 1) + (j - 1),
                        p * (n + 1) * (n + 1) + (i - 1) * (n + 1) + (j),
                        p * (n + 1) * (n + 1) + (i) * (n + 1) + (j),
                        p * (n + 1) * (n + 1) + (i) * (n + 1) + (j - 1));
            }
        }
    }
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

////////////////////////////////////////////////////////////////////////
// Generates a box +-1 on all axes
Shapes::Box::Box(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue) {
    diffuseColor = Vector3(0.5, 0.5, 1.0);
    specularColor = Vector3(1.0, 1.0, 1.0);
    shininess = 120.0;

    Matrix I = Matrix::Identity;

    // Six faces, each a rotation of a rectangle placed on the z axis.
    face(I);
    float r90 = PI / 2;
    face(Matrix::CreateRotationX(r90));
    face(Matrix::CreateRotationX(-r90));
    face(Matrix::CreateRotationY(r90));
    face(Matrix::CreateRotationY(-r90));
    face(Matrix::CreateRotationX(PI));

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

void Shapes::Box::face(const Matrix tr) {
    int n = static_cast<int>(Pnt.size());

    float verts[8] = { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f };
    float texcd[8] = { 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

    // Four vertices to make a single face, with its own normal and
    // texture coordinates.
    for (int i = 0; i < 8; i += 2) {
        Pnt.push_back(Vector4::Transform(Vector4(verts[i], verts[i + 1], 1.0f, 1.0f), tr));
        Nrm.push_back(Vector3::TransformNormal(Vector3(0.0f, 0.f, 1.f), tr));
        Tex.push_back(Vector2(texcd[i], texcd[i + 1]));

        Tan.push_back(Vector3::TransformNormal(Vector3(1.0f, 0.0f, 0.0f), tr));
    }

    pushquad(Tri, n, n + 1, n + 2, n + 3);
}

////////////////////////////////////////////////////////////////////////
// Generates a sphere of radius 1.0 centered at the origin.
//   n specifies the number of polygonal subdivisions
Shapes::Sphere::Sphere(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue) {
    diffuseColor = Vector3(0.5, 0.5, 1.0);
    specularColor = Vector3(1.0, 1.0, 1.0);
    shininess = 120.0;

    float d = 2.0f * PI / float(n * 2);
    for (int i = 0; i <= n * 2; i++) {
        float s = i * 2.0f * PI / float(n * 2);
        for (int j = 0; j <= n; j++) {
            float t = j * PI / float(n);
            float x = cos(s) * sin(t);
            float y = sin(s) * sin(t);
            float z = cos(t);
            Pnt.push_back(Vector4(x, y, z, 1.0f));
            Nrm.push_back(Vector3(x, y, z));
            Tex.push_back(Vector2(s / (2 * PI), t / PI));
            Tan.push_back(Vector3(-sin(s), cos(s), 0.0));
            if (i > 0 && j > 0) {
                pushquad(Tri, (i - 1) * (n + 1) + (j - 1),
                    (i - 1) * (n + 1) + (j),
                    (i) * (n + 1) + (j),
                    (i) * (n + 1) + (j - 1));
            }
        }
    }
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

////////////////////////////////////////////////////////////////////////
// Generates a radius disk aroudn the origin in the XY plane
//   n specifies the number of polygonal subdivisions
Shapes::Disk::Disk(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue) {
    diffuseColor = Vector3(0.5, 0.5, 1.0);
    specularColor = Vector3(1.0, 1.0, 1.0);
    shininess = 120.0;

    // Push center point
    Pnt.push_back(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    Nrm.push_back(Vector3(0.0f, 0.0f, 1.0f));
    Tex.push_back(Vector2(0.5, 0.5));
    Tan.push_back(Vector3(1.0f, 0.0f, 0.0f));

    float d = 2.0f * PI / float(n);
    for (int i = 0; i <= n; i++) {
        float s = i * 2.0f * PI / float(n);
        float x = cos(s);
        float y = sin(s);
        Pnt.push_back(Vector4(x, y, 0.0f, 1.0f));
        Nrm.push_back(Vector3(0.0f, 0.0f, 1.0f));
        Tex.push_back(Vector2(x * 0.5f + 0.5f, y * 0.5f + 0.5f));
        Tan.push_back(Vector3(1.0f, 0.0f, 0.0f));
        if (i > 0) {
            Tri.push_back(DirectX::XMINT3(0, i + 1, i));
        }
    }
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

////////////////////////////////////////////////////////////////////////
// Generates a Z-aligned cylinder of radius 1 from -1 to 1 in Z
//   n specifies the number of polygonal subdivisions
Shapes::Cylinder::Cylinder(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue) {
    diffuseColor = Vector3(0.5, 0.5, 1.0);
    specularColor = Vector3(1.0, 1.0, 1.0);
    shininess = 120.0;

    float d = 2.0f * PI / float(n);
    for (int i = 0; i <= n; i++) {
        float s = i * 2.0f * PI / float(n);
        for (int j = 0; j <= 1; j++) {
            float t = static_cast<float>(j);
            float x = cos(s);
            float y = sin(s);
            float z = t * 2.0f - 1.0f;
            Pnt.push_back(Vector4(x, y, z, 1.0f));
            Nrm.push_back(Vector3(x, y, 0.0f));
            Tex.push_back(Vector2(s / (2.0f * PI), t));
            Tan.push_back(Vector3(-sin(s), cos(s), 0.0));
            if (i > 0 && j > 0) {
                pushquad(Tri, (i - 1) * (2) + (j - 1),
                    (i - 1) * (2) + (j),
                    (i) * (2) + (j),
                    (i) * (2) + (j - 1));
            }
        }
    }
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
Shapes::Ply::Ply(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue, const char* name, const bool reverse) {
    diffuseColor = Vector3(0.8f, 0.8f, 0.5f);
    specularColor = Vector3(1.0f, 1.0f, 1.0f);
    shininess = 120.0f;

    // Open PLY file and read header;  Exit on any failure.
    p_ply ply = ply_open(name, NULL, 0, NULL);
    if (!ply) {
        throw std::exception();
    }
    if (!ply_read_header(ply)) {
        throw std::exception();
    }

    // Setup callback for vertices
    ply_set_read_cb(ply, "vertex", "x", vertex_cb, this, 0);
    ply_set_read_cb(ply, "vertex", "y", vertex_cb, this, 1);
    ply_set_read_cb(ply, "vertex", "z", vertex_cb, this, 2);

    ply_set_read_cb(ply, "vertex", "nx", normal_cb, this, 0);
    ply_set_read_cb(ply, "vertex", "ny", normal_cb, this, 1);
    ply_set_read_cb(ply, "vertex", "nz", normal_cb, this, 2);

    ply_set_read_cb(ply, "vertex", "s", texture_cb, this, 0);
    ply_set_read_cb(ply, "vertex", "t", texture_cb, this, 1);

    // Setup callback for faces
    ply_set_read_cb(ply, "face", "vertex_indices", face_cb, this, 0);

    // Read the PLY file filling the arrays via the callbacks.
    if (!ply_read(ply)) {
        printf("Failure in ply_read\n");
        exit(-1);
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

XMVECTOR staticPnt;
XMVECTOR staticNrm;
XMVECTOR staticTex;
XMVECTOR staticTri;

// Vertex callback;  Must be static (stupid C++)
int Shapes::Ply::vertex_cb(p_ply_argument argument) {
    long index;
    Ply* ply{};
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    double c = ply_get_argument_value(argument);
    staticPnt.m128_f32[index] = static_cast<float>(c);
    if (index == 2) {
        staticPnt.m128_f32[3] = 1.0f;
        ply->Pnt.push_back(staticPnt);
        ply->Tan.push_back(Vector3());
    }
    return 1;
}
// Normal callback;  Must be static (stupid C++)
int Shapes::Ply::normal_cb(p_ply_argument argument) {
    long index;
    Ply* ply;
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    double c = ply_get_argument_value(argument);
    staticNrm.m128_f32[index] = static_cast<float>(c);
    if (index == 2) {
        ply->Nrm.push_back(staticNrm);
    }
    return 1;
}
// Texture callback;  Must be static (stupid C++)
int Shapes::Ply::texture_cb(p_ply_argument argument) {
    long index;
    Ply* ply = nullptr;
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    double c = ply_get_argument_value(argument);
    staticTex.m128_f32[index] = static_cast<float>(c);
    if (index == 1) {
        ply->Tex.push_back(staticTex);
    }
    return 1;
}

void ComputeTangent(Shapes::Ply* ply) {
    int t = static_cast<int>(ply->Tri.size() - 1);
    int i = ply->Tri[t].x;
    int j = ply->Tri[t].y;
    int k = ply->Tri[t].z;
    Vector2 A = ply->Tex[i] - ply->Tex[k];
    Vector2 B = ply->Tex[j] - ply->Tex[k];
    float d = A.x * B.y - A.y * B.x;
    float a = B.y / d;
    float b = -A.y / d;
    Vector4 Tan = a * ply->Pnt[i] + b * ply->Pnt[j] + (1.0f - a - b) * ply->Pnt[k];
    Tan.Normalize();
    ply->Tan[i] = ply->Tan[j] = ply->Tan[k] = static_cast<Vector3>(Tan);//glm::normalize(Tan.xyz());
}

// Face callback;  Must be static (stupid C++)
int Shapes::Ply::face_cb(p_ply_argument argument) {
    long length, value_index;
    long index;
    Ply* ply;
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    ply_get_argument_property(argument, NULL, &length, &value_index);

    if (value_index == -1) {
    }
    else {
        if (value_index < 2) {
            staticTri.m128_i32[value_index] = (int)ply_get_argument_value(argument);
        }
        else if (value_index == 2) {
            staticTri.m128_i32[2] = (int)ply_get_argument_value(argument);
            ply->Tri.push_back(XMINT3(staticTri.m128_i32));
            ComputeTangent(ply);
        }
        else if (value_index == 3) {
            staticTri.m128_i32[1] = staticTri.m128_i32[2];
            staticTri.m128_i32[2] = (int)ply_get_argument_value(argument);
            ply->Tri.push_back(XMINT3(staticTri.m128_i32));
            ComputeTangent(ply);
        }
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
Shapes::Plane::Plane(const float r, const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue) {
    diffuseColor = Vector3(0.3f, 0.2f, 0.1f);
    specularColor = Vector3(1.0f, 1.0f, 1.0f);
    shininess = 120.0f;

    for (int i = 0; i <= n; i++) {
        float s = i / float(n);
        for (int j = 0; j <= n; j++) {
            float t = j / float(n);
            Pnt.push_back(Vector4(s * 2.0f * r - r, t * 2.0f * r - r, 0.0f, 1.0f));
            Nrm.push_back(Vector3(0.0f, 0.0f, 1.0f));
            Tex.push_back(Vector2(s, t));
            Tan.push_back(Vector3(1.0f, 0.0f, 0.0f));
            if (i > 0 && j > 0) {
                pushquad(Tri, (i - 1) * (n + 1) + (j - 1),
                    (i - 1) * (n + 1) + (j),
                    (i) * (n + 1) + (j),
                    (i) * (n + 1) + (j - 1));
            }
        }
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
Shapes::ProceduralGround::ProceduralGround(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue, 
    const float _range, const int n,
    const float _octaves, const float _persistence, const float _scale,
    const float _low, const float _high)
    : range(_range)
    , octaves(_octaves)
    , persistence(_persistence)
    , scale(_scale)
    , low(_low)
    , high(_high) {
    diffuseColor = Vector3(0.3f, 0.2f, 0.1f);
    specularColor = Vector3(1.0f, 1.0f, 1.0f);
    shininess = 10.0;
    specularColor = Vector3(0.0f, 0.0f, 0.0f);
    xoff = range * (time(NULL) % 1000);

    float h = 0.001f;
    for (int i = 0; i <= n; i++) {
        float s = i / float(n);
        for (int j = 0; j <= n; j++) {
            float t = j / float(n);
            float x = s * 2.0f * range - range;
            float y = t * 2.0f * range - range;
            float z = HeightAt(x, y);
            float zu = HeightAt(x + h, y);
            float zv = HeightAt(x, y + h);
            Pnt.push_back(Vector4(x, y, z, 1.0f));
            Vector3 du(1.0f, 0.0f, (zu - z) / h);
            Vector3 dv(0.0f, 1.0f, (zv - z) / h);
            //Nrm.push_back(glm::normalize(glm::cross(du, dv)));
            du = du.Cross(dv);
            du.Normalize();
            Nrm.push_back(du);
            Tex.push_back(Vector2(s, t));
            Tan.push_back(Vector3(1.0f, 0.0f, 0.0f));
            if (i > 0 && j > 0) {
                pushquad(Tri,
                    (i - 1) * (n + 1) + (j - 1),
                    (i - 1) * (n + 1) + (j),
                    (i) * (n + 1) + (j),
                    (i) * (n + 1) + (j - 1));
            }
        }
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}

static float smoothstep(float edge0, float edge1, float x) {
    // Scale, and clamp x to 0..1 range
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    return x * x * (3.0f - 2.0f * x);
}

float Shapes::ProceduralGround::HeightAt(const float x, const float y) {
    Vector3 highPoint = Vector3(0.0f, 0.0f, 0.01f);
    float rs = smoothstep(range - 20.0f, range, sqrtf(x * x + y * y));
    float noise = scaled_octave_noise_2d(octaves, persistence, scale, low, high, x + xoff, y);
    float z = (1 - rs) * noise + rs * low;

    float hs = smoothstep(15.0f, 45.0f,
        (Vector3(x, y, 0) - Vector3(highPoint.x, highPoint.y, 0)).Length());
    return (1 - hs) * highPoint.z + hs * z;
}

////////////////////////////////////////////////////////////////////////
// Generates a square divided into nxn quads;  +-1 in X and Y at Z=0
Shapes::Quad::Quad(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue, const int n) {
    diffuseColor = Vector3(0.3f, 0.2f, 0.1f);
    specularColor = Vector3(1.0f, 1.0f, 1.0f);
    shininess = 120.0f;

    float r = 1.0;
    for (int i = 0; i <= n; i++) {
        float s = i / float(n);
        for (int j = 0; j <= n; j++) {
            float t = j / float(n);
            Pnt.push_back(Vector4(s * 2.0f * r - r, t * 2.0f * r - r, 0.0f, 1.0f));
            Nrm.push_back(Vector3(0.0f, 0.0f, 1.0f));
            Tex.push_back(Vector2(s, 1.f - t));
            Tan.push_back(Vector3(1.0f, 0.0f, 0.0f));
            if (i > 0 && j > 0) {
                pushquad(Tri,
                    (i - 1) * (n + 1) + (j - 1),
                    (i - 1) * (n + 1) + (j),
                    (i) * (n + 1) + (j),
                    (i) * (n + 1) + (j - 1));
            }
        }
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    _queue->GetDevice(IID_PPV_ARGS(&device));
    MakeVAO(device, _queue);
}
