#pragma once
#if _WIN32 || _WIN64
#include <Windows.h>

extern int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline,
	int       iCmdshow
);
#endif