#pragma once

namespace Engine
{
    enum eShaderType
    {
        SHADER_VERTEX = 0,
        SHADER_PIXEL,
        SHADER_GEOMETRY,
        SHADER_COMPUTE,
        SHADER_HULL,
        SHADER_DOMAIN,
        SHADER_UNKNOWN
    };

    enum eCBType
    {
        CB_TYPE_WVP = 0,
        CB_TYPE_TRANSFORM,
        CB_TYPE_LIGHT,
        CB_TYPE_SHADOW,
        CB_TYPE_SHADOW_CHUNK,
        CB_TYPE_MATERIAL,
    };

    enum eShaderResource
    {
        SR_TEXTURE = 0,
        SR_NORMAL_MAP,
        SR_SHADOW_MAP,
        SR_RENDERED,
        SR_ANIMATION
    };

    enum eSampler
    {
        SAMPLER_TEXTURE = 0,
        SAMPLER_SHADOW,
    };

    enum eLayerType
    {
        LAYER_NONE = 0,
        LAYER_LIGHT,
        LAYER_DEFAULT,
        LAYER_ENVIRONMENT,
        LAYER_SKYBOX,
        LAYER_UI,
        LAYER_CAMERA,
        LAYER_MAX
    };

    enum eObserverState
    {
        OBSERVER_STATE_NONE,
    };

    enum eResourceType
    {
        RES_T_UNK = 0,
        RES_T_SHADER,
        RES_T_TEX,
        RES_T_NORMAL,
        RES_T_MESH,
        RES_T_FONT,
        RES_T_SOUND,
        RES_T_MODEL,
        RES_T_BONE_ANIM,
        RES_T_BONE,
        RES_T_BASE_ANIM,
        RES_T_MTR,
    };

    enum eComponentType
    {
        COM_T_UNK = 0,
        COM_T_TRANSFORM,
        COM_T_COLLIDER,
        COM_T_RIDIGBODY,
        COM_T_STATE,
        COMP_T_SOUND_PLAYER,
        COM_T_MODEL_RENDERER,
        COM_T_ANIMATOR
    };

    enum eDefObjectType
    {
        DEF_OBJ_T_UNK = 0,
        DEF_OBJ_T_NONE,
        DEF_OBJ_T_CAMERA,
        DEF_OBJ_T_LIGHT,
        DEF_OBJ_T_OBSERVER,
        DEF_OBJ_T_TEXT,
        DEF_OBJ_T_DELAY_OBJ
    };

    // THIS ENUM SHOULD BE DEFINED AT THE CLIENT!
    enum eSceneType : UINT;

    enum eBoundingType
    {
        BOUNDING_TYPE_BOX = 0,
        BOUNDING_TYPE_FRUSTUM,
        BOUNDING_TYPE_SPHERE,
    };
}