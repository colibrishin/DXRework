#pragma once
#include "egDXAnimCommon.hpp"

namespace Engine::Graphics
{
    namespace DXPacked
    {
        struct ShadowVPResource
        {
            ComPtr<ID3D11Texture2D>          texture;
            ComPtr<ID3D11DepthStencilView>   depth_stencil_view;
            ComPtr<ID3D11ShaderResourceView> shader_resource_view;
        };

        struct RenderedResource
        {
            ComPtr<ID3D11Texture2D>          texture;
            ComPtr<ID3D11ShaderResourceView> srv;
        };
    }

    struct ShadowVP
    {
        Matrix  view[g_max_shadow_cascades];
        Matrix  proj[g_max_shadow_cascades];
        Vector4 end_clip_spaces[g_max_shadow_cascades];
    };

    struct VertexElement
    {
        Vector3 position;
        Vector4 color;
        Vector2 texCoord;

        Vector3 normal;
        Vector3 tangent;
        Vector3 binormal;

        VertexBoneElement bone_element;
    };
}
