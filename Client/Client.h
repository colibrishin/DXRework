#pragma once

namespace Engine
{
	enum eScriptType : UINT
	{
		SCRIPT_T_NONE = 0,
		SCRIPT_T_HITBOX,
		SCRIPT_T_PLAYER,
		SCRIPT_T_HP_TEXT,
		SCRIPT_T_SHADOW,
		SCRIPT_T_PLAYER_HITBOX,
		SCRIPT_T_RIFLE,
		SCRIPT_T_FEZ_PLAYER,
		SCRIPT_T_CUBIFY,
	};

	enum eClientSBUAVType : UINT
	{
		CLIENT_SBUAV_TYPE_INTERSECTION = 7,
	};
}

namespace Client
{
	void    Initialize(HWND hwnd);
	void    Tick();
	LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	UINT GetWidth();
	UINT GetHeight();
	bool IsFullScreen();

	enum eCubeType
	{
		CUBE_TYPE_NORMAL,
		CUBE_TYPE_LADDER,
		CUBE_TYPE_WATER,
	};

	enum eCharacterState
	{
		CHAR_STATE_IDLE,
		CHAR_STATE_WALK,
		CHAR_STATE_RUN,
		CHAR_STATE_JUMP,
		CHAR_STATE_CLIMB,
		CHAR_STATE_POST_CLIMB,
		CHAR_STATE_VAULT,
		CHAR_STATE_SWIM,
		CHAR_STATE_ROTATE,
		CHAR_STATE_POST_ROTATE,
		CHAR_STATE_FALL,
		CHAR_STATE_ATTACK,
		CHAR_STATE_HIT,
		CHAR_STATE_DIE,
		CHAR_STATE_MAX
	};
} // namespace Client
