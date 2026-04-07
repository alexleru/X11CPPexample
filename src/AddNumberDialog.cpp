#include "AddNumberDialog.hpp"

#include <string>

namespace {

constexpr int ID_EDIT_ADD_VALUE = 2001;

class AddNumberDialogImpl {
public:
    static bool Show(HWND parent, double& outValue) {
        RegisterClassIfNeeded();

        AddNumberDialogImpl dialog;
        EnableWindow(parent, FALSE);

        HWND hwnd = CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            kDialogClassName,
            L"Add Number",
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 290, 160,
            parent,
            nullptr,
            GetModuleHandleW(nullptr),
            &dialog
        );

        if (!hwnd) {
            EnableWindow(parent, TRUE);
            return false;
        }

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg;
        while (!dialog.done_ && GetMessageW(&msg, nullptr, 0, 0)) {
            if (!IsDialogMessageW(hwnd, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        EnableWindow(parent, TRUE);
        SetActiveWindow(parent);

        if (!dialog.accepted_ || dialog.value_.empty()) return false;
        try {
            outValue = std::stod(dialog.value_);
            return true;
        } catch (...) {
            MessageBoxW(parent, L"Invalid number format.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }

private:
    static constexpr const wchar_t* kDialogClassName = L"ModernAddNumberDialogClass";

    static void RegisterClassIfNeeded() {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = kDialogClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        if (RegisterClassW(&wc)) {
            registered = true;
        }
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        AddNumberDialogImpl* self =
            reinterpret_cast<AddNumberDialogImpl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

        switch (msg) {
        case WM_CREATE: {
            HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            CreateWindowW(L"STATIC", L"Enter a number:",
                WS_CHILD | WS_VISIBLE, 20, 18, 240, 20, hwnd, nullptr, nullptr, nullptr);

            self->edit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 44, 240, 24, hwnd, reinterpret_cast<HMENU>(ID_EDIT_ADD_VALUE), nullptr, nullptr);

            HWND okBtn = CreateWindowW(L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                20, 84, 110, 30, hwnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);

            HWND cancelBtn = CreateWindowW(L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE,
                150, 84, 110, 30, hwnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);

            SendMessageW(self->edit_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SendMessageW(okBtn, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SendMessageW(cancelBtn, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SetFocus(self->edit_);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                wchar_t buffer[128] = L"";
                GetWindowTextW(self->edit_, buffer, 127);
                self->value_ = buffer;
                self->accepted_ = true;
                self->done_ = true;
                DestroyWindow(hwnd);
                return 0;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                self->done_ = true;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        case WM_CLOSE:
            self->done_ = true;
            DestroyWindow(hwnd);
            return 0;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    HWND edit_ = nullptr;
    bool done_ = false;
    bool accepted_ = false;
    std::wstring value_;
};

}  // namespace

bool AddNumberDialog::Show(HWND parent, double& outValue) {
    return AddNumberDialogImpl::Show(parent, outValue);
}
