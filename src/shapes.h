////////////////////////////////////////////////////////////////////////
// A small library of object shapes (ground plane, sphere, and the
// famous Utah teapot), each created as a Vertex Array Object (VAO).
// This is the most efficient way to get geometry into the OpenGL
// graphics pipeline.
//
// Each vertex is specified as four attributes which are made
// available in a vertex shader in the following attribute slots.
//
// position,        Vector4,   attribute #0
// normal,          Vector3,   attribute #1
// texture coord,   Vector3,   attribute #2
// tangent,         Vector3,   attribute #3
//
// An instance of any of these shapes is create with a single call:
//    unsigned int obj = CreateSphere(divisions, &quadCount);
// and drawn by:
//    glBindVertexArray(vaoID);
//    glDrawElements(GL_TRIANGLES, vertexcount, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);
////////////////////////////////////////////////////////////////////////

#ifndef _SHAPES
#define _SHAPES

//#include "transform.h"
#include <directxtk12/SimpleMath.h>
#include "rply.h"

#include <vector>
#include <wrl.h>
#include <d3d12.h>
struct CommandList;
namespace Shapes {
    class Shape {
    public:

        struct VAO {
            Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer{};
            D3D12_VERTEX_BUFFER_VIEW m_vbv{};
            Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer{};
            D3D12_INDEX_BUFFER_VIEW m_ibv{};
            uint32_t m_indexCount = 0;
        };
        VAO m_vao{};
        // The OpenGL identifier of this VAO
        //unsigned int vaoID;

        // Data arrays
        std::vector<DirectX::SimpleMath::Vector4> Pnt{};
        std::vector<DirectX::SimpleMath::Vector3> Nrm{};
        std::vector<DirectX::SimpleMath::Vector2> Tex{};
        std::vector<DirectX::SimpleMath::Vector3> Tan{};

        // Lighting information
        DirectX::SimpleMath::Vector3 diffuseColor{}, specularColor{};
        float shininess = 0;

        // Geometry defined by indices into data arrays
        std::vector<DirectX::XMINT3> Tri{};
        unsigned int count = 0;

        // Defined by SetTransform by scanning data arrays
        DirectX::SimpleMath::Vector3 minP{}, maxP{};
        DirectX::SimpleMath::Vector3 center{};
        float size = 0;
        bool animate = false;

        // Constructor and destructor
        Shape() {}
        virtual ~Shape() {}

        virtual void MakeVAO(
            Microsoft::WRL::ComPtr<ID3D12Device>& _device,
            Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue
        );
        virtual void DrawVAO(CommandList& _cmd);
    };

    class Box : public Shape {
        void face(const DirectX::SimpleMath::Matrix tr);
    public:
        Box(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue);
    };

    class Sphere : public Shape {
    public:
        Sphere(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue);
    };

    class Disk : public Shape {
    public:
        Disk(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue);
    };

    class Cylinder : public Shape {
    public:
        Cylinder(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue);
    };

    class Teapot : public Shape {
    public:
        Teapot(const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue);
    };

    class Plane : public Shape {
    public:
        Plane(const float range, const int n, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue);
    };

    class ProceduralGround : public Shape {
    public:
        float range;
        float octaves;
        float persistence;
        float scale;
        float low;
        float high;
        float xoff;

        ProceduralGround(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue, const float _range, const int n,
            const float _octaves, const float _persistence, const float _scale,
            const float _low, const float _high);
        float HeightAt(const float x, const float y);
    };

    class Quad : public Shape {
    public:
        Quad(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue, const int n = 1);
    };

    class Ply : public Shape {
    public:
        Ply(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& _queue, const char* name, const bool reverse = false);
        virtual ~Ply() { printf("destruct Ply\n"); };
        static int vertex_cb(p_ply_argument argument);
        static int normal_cb(p_ply_argument argument);
        static int texture_cb(p_ply_argument argument);
        static int face_cb(p_ply_argument argument);
    };
};
#endif
