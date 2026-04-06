// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include <windows.h>

extern int ENGINE_MAIN(int argc, const char** argv);

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return ENGINE_MAIN(__argc, (const char**)__argv);
}
