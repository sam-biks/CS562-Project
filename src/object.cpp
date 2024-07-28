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

#include "math.h"
#include <fstream>
#include <stdlib.h>

//#include <glbinding/Binding.h>
//#include <glbinding/gl/gl.h>
//using namespace gl;
//
//#define GLM_FORCE_CTOR_INIT
//#define GLM_FORCE_RADIANS
//#define GLM_SWIZZLE
//#include <glm/glm.hpp>
//
//
//#include <glu.h> // For gluErrorString
//#define CHECKERROR                                                                                        \
//    {                                                                                                     \
//        GLenum err = glGetError();                                                                        \
//        if (err != GL_NO_ERROR) {                                                                         \
//            fprintf(stderr, "OpenGL error (at line object.cpp:%d): %s\n", __LINE__, gluErrorString(err)); \
//            exit(-1);                                                                                     \
//        }                                                                                                 \
//    }
#include "framework.h"
#include "shapes.h"

#include "../ShaderData.h"
#include <directxtk12/GraphicsMemory.h>

Object::Object(
    std::shared_ptr<DirectX::GeometricPrimitive> _shape,
    const int _objectId,
    const DirectX::SimpleMath::Vector3 _diffuseColor, 
    const DirectX::SimpleMath::Vector3 _specularColor, 
    const float _roughness
)
    : m_diffuseColor(_diffuseColor)
    , m_specularColor(_specularColor)
    , m_roughness(_roughness)
    , m_shape(_shape)
    , m_objectId(_objectId)
    , m_drawMe(true)

{
}

void Object::Draw(
    CommandList& _cmd, 
    std::unique_ptr<ShaderProgram>& _program,
    std::unique_ptr<DirectX::DescriptorPile>& _heap,
    const DirectX::SimpleMath::Matrix& _objectTr
)
{
    using namespace DirectX::SimpleMath;
    ShaderData::Object objectData{};
    objectData.diffuse   = m_diffuseColor;
    objectData.specular  = m_specularColor;
    objectData.roughness = m_roughness;

    objectData.ModelTr  = _objectTr;
    objectData.NormalTr = _objectTr;
    objectData.NormalTr = objectData.NormalTr.Invert();
    objectData.NormalTr = objectData.NormalTr.Transpose();

    if (m_texture) {
        objectData.Textured = true;
        m_texture.BindTexture(_cmd, _heap, 3);
    }
    else {
        objectData.Textured = false;
    }

    auto& graphicsMemory = DirectX::GraphicsMemory::Get();
    auto objectMemory    = graphicsMemory.AllocateConstant(objectData);
    _cmd->SetGraphicsRootConstantBufferView(2, objectMemory.GpuAddress());
    // Draw this object
    if (m_shape)
        if (m_drawMe)
            m_shape->Draw(*_cmd);

    // Recursively draw each sub-objects, each with its own transformation.
    if (m_drawMe)
        for (int i = 0; i < m_instances.size(); i++) {
            Matrix itr = m_animTr * m_instances[i].second  * _objectTr;
            m_instances[i].first->Draw(_cmd, _program, _heap, itr);
        }
}
