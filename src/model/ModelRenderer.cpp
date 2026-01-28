#include "ModelRenderer.h"
#include "rlgl.h"

namespace AAV {

ModelRenderer::ModelRenderer()
    : wireframeMode(false)
    , showBoundingBox(false)
{
}

ModelRenderer::~ModelRenderer() {
}

void ModelRenderer::Render(const Model& model) {
    if (!model.IsLoaded()) return;
    
    Matrix modelTransform = model.GetTransformMatrix();
    
    // Check if we have a node hierarchy
    auto rootNode = model.GetRootNode();
    if (rootNode) {
        // Render using hierarchical transforms
        RenderNodeHierarchy(rootNode.get(), modelTransform, model);
    } else {
        // Fallback: render all meshes directly (flat hierarchy)
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(modelTransform));
        
        if (wireframeMode) {
            rlEnableWireMode();
        }
        
        for (const auto& meshData : model.GetMeshes()) {
            DrawMesh(meshData.mesh, meshData.material, MatrixIdentity());
        }
        
        if (wireframeMode) {
            rlDisableWireMode();
        }
        
        rlPopMatrix();
    }
    
    if (showBoundingBox) {
        RenderBoundingBox(model);
    }
}

void ModelRenderer::RenderNodeHierarchy(NodeData* node, const Matrix& parentTransform, const Model& model) {
    if (!node) return;
    
    // Accumulate transform: local transform * parent transform
    Matrix worldTransform = MatrixMultiply(node->transform, parentTransform);
    
    // Render all meshes attached to this node
    if (!node->meshIndices.empty()) {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(worldTransform));
        
        if (wireframeMode) {
            rlEnableWireMode();
        }
        
        const auto& meshes = model.GetMeshes();
        for (int meshIndex : node->meshIndices) {
            if (meshIndex >= 0 && meshIndex < (int)meshes.size()) {
                const auto& meshData = meshes[meshIndex];
                DrawMesh(meshData.mesh, meshData.material, MatrixIdentity());
            }
        }
        
        if (wireframeMode) {
            rlDisableWireMode();
        }
        
        rlPopMatrix();
    }
    
    // Recursively render all children
    for (const auto& child : node->children) {
        RenderNodeHierarchy(child.get(), worldTransform, model);
    }
}

void ModelRenderer::RenderWireframe(const Model& model) {
    if (!model.IsLoaded()) return;
    
    Matrix transform = model.GetTransformMatrix();
    
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    
    rlEnableWireMode();
    
    for (const auto& meshData : model.GetMeshes()) {
        DrawMesh(meshData.mesh, meshData.material, MatrixIdentity());
    }
    
    rlDisableWireMode();
    
    rlPopMatrix();
}

void ModelRenderer::RenderBoundingBox(const Model& model) {
    if (!model.IsLoaded()) return;
    
    Vector3 min = model.GetBoundingBoxMin();
    Vector3 max = model.GetBoundingBoxMax();
    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Scale(Vector3Add(min, max), 0.5f);
    
    Matrix transform = model.GetTransformMatrix();
    Vector3 transformedCenter = Vector3Transform(center, transform);
    
    // Transform size by scale component only
    Vector3 scale = model.GetScale();
    Vector3 transformedSize = {
        size.x * scale.x,
        size.y * scale.y,
        size.z * scale.z
    };
    
    DrawBoundingBox(
        (BoundingBox){
            Vector3Subtract(transformedCenter, Vector3Scale(transformedSize, 0.5f)),
            Vector3Add(transformedCenter, Vector3Scale(transformedSize, 0.5f))
        },
        YELLOW
    );
}

void ModelRenderer::RenderGrid(float size, int divisions) {
    DrawGrid(divisions, size);
}

} // namespace AAV
