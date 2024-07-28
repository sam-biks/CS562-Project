////////////////////////////////////////////////////////////////////////
// A lightweight class representing an instance of an object that can
// be drawn onscreen.  An Object consists of a shape (batch of
// triangles), and various transformation, color and texture
// parameters.  It also contains a list of child objects to be drawn
// in a hierarchical fashion under the control of parent's
// transformations.
//
// Methods consist of a constructor, and a Draw procedure, and an
// append for building hierarchies of objects.

#ifndef _OBJECT
#define _OBJECT

#include <directxtk12/SimpleMath.h>
#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/BufferHelpers.h>
#include "texture.h"

#include <utility> // for pair<Object*,Matrix>

class ShaderProgram;
class Object;
struct CommandList;

typedef std::pair<std::shared_ptr<Object>, DirectX::SimpleMath::Matrix> INSTANCE;

// Object:: A shape, and its transformations, colors, and textures and sub-objects.
class Object {
public:
    std::shared_ptr<DirectX::GeometricPrimitive> m_shape; // Polygons
    DirectX::SimpleMath::Matrix m_animTr; // This model's animation transformation
    int m_objectId; // Object id to be sent to the shader
    bool m_drawMe; // Toggle specifies if this object (and children) are drawn.

    DirectX::SimpleMath::Vector3 m_diffuseColor; // Diffuse color of object
    DirectX::SimpleMath::Vector3 m_specularColor; // Specular color of object
    float m_roughness; // Surface roughness value

    std::vector<INSTANCE> m_instances; // Pairs of sub-objects and transformations

    Texture m_texture;

    Object(
        std::shared_ptr<DirectX::GeometricPrimitive> _shape, const int objectId,
        const DirectX::SimpleMath::Vector3 _d = DirectX::SimpleMath::Vector3(),
        const DirectX::SimpleMath::Vector3 _s = DirectX::SimpleMath::Vector3(),
        const float _roughness = 0.5f
    );

    // If this object is to be drawn with a texture, this is a good
    // place to store the texture id (a small positive integer).  The
    // texture id should be set in Scene::InitializeScene and used in
    // Object::Draw.

    void Draw(
        CommandList& _cmd,
        std::unique_ptr<ShaderProgram>& _program,
        std::unique_ptr<DirectX::DescriptorPile>& _heap,
        const DirectX::SimpleMath::Matrix& _objectTr
    );

    void add(std::shared_ptr<Object>& m, DirectX::SimpleMath::Matrix tr = DirectX::SimpleMath::Matrix::Identity) 
    { m_instances.push_back(std::make_pair(m, tr)); }
};

#endif
