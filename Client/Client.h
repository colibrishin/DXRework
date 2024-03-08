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

  enum eCharacterState
  {
    CHAR_STATE_IDLE,
    CHAR_STATE_WALK,
    CHAR_STATE_RUN,
    CHAR_STATE_JUMP,
    CHAR_STATE_ATTACK,
    CHAR_STATE_HIT,
    CHAR_STATE_DIE,
    CHAR_STATE_MAX
  };
} // namespace Client
