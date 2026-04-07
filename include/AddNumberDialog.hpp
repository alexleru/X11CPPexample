#pragma once

#include <windows.h>

class AddNumberDialog {
public:
    static bool Show(HWND parent, double& outValue);
};
