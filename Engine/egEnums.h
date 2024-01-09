#pragma once
#include "egConstant.h"

namespace Engine
{
    enum eTexBindSlot
    {
        BIND_SLOT_TEX = 0,
        BIND_SLOT_TEXARR = BIND_SLOT_TEX + g_max_slot_per_texture,
        BIND_SLOT_TEXCUBE = BIND_SLOT_TEXARR + g_max_slot_per_texture,
        BIND_SLOT_END
    };

    static_assert(BIND_SLOT_END < 128);

    enum eReservedTexBindSlot
    {
        RESERVED_SHADOW_MAP = g_reserved_bind_slot,
        RESERVED_RENDERED,
    };

    enum eSampler
    {
        SAMPLER_TEXTURE = 0,
        SAMPLER_SHADOW,
    };

    enum eObserverState
    {
        OBSERVER_STATE_NONE,
    };

    enum eTaskType
    {
        TASK_NONE = 0,
        TASK_ADD_OBJ,
        TASK_REM_OBJ,
        TASK_INIT_SCENE,
        TASK_SYNC_SCENE,
        TASK_REM_SCENE,
        TASK_ACTIVE_SCENE,
        TASK_MAX
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
        CB_TYPE_GLOBAL_STATE,
        CB_TYPE_MATERIAL,
    };

    enum eSBType
    {
        SB_TYPE_BONE = g_reserved_struct_buffer_slot,
        SB_TYPE_LIGHT,
        SB_TYPE_SHADOW,
    };

    enum eResourceType
    {
        RES_T_UNK = 0,
        RES_T_SHADER,
        RES_T_TEX,
        RES_T_FONT,
        RES_T_SOUND,
        RES_T_BONE_ANIM,
        RES_T_BONE,
        RES_T_BASE_ANIM,
        RES_T_MTR,
        RES_T_MESH,
        RES_T_SHAPE,
    };

    enum eComponentType
    {
        COM_T_UNK = 0,
        COM_T_TRANSFORM,
        COM_T_COLLIDER,
        COM_T_RIDIGBODY,
        COM_T_STATE,
        COMP_T_SOUND_PLAYER,
        COM_T_ANIMATOR,
        COM_T_MODEL_RENDERER,
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

    enum eBoundingType
    {
        BOUNDING_TYPE_BOX = 0,
        BOUNDING_TYPE_SPHERE,
    };

    // THIS ENUM SHOULD BE DEFINED AT THE CLIENT!
    enum eSceneType : UINT;
}
