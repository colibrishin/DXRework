#pragma once
#if PLATFORM == Windows
#include <Windows.h>

extern int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline,
	int       iCmdshow
);
#endif