#pragma once

namespace Client
{
	void fnClient();

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
}