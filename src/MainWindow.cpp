#include "MainWindow.hpp"

#include "AddNumberDialog.hpp"
#include "CalculatorEngine.hpp"
#include "NumberRepository.hpp"
#include "StringUtils.hpp"

namespace {

constexpr int kWindowWidth = 420;
constexpr int kWindowHeight = 520;
constexpr int kPad = 20;

enum ControlId {
    ID_BTN_ADD = 1001,
    ID_LIST_NUMBERS,
    ID_BTN_DELETE,
    ID_CMB_OPERATION,
    ID_BTN_CALC,
    ID_LBL_RESULT
};

class MainWindowImpl {
public:
    int Run(HINSTANCE instance, int nCmdShow) {
        if (!RegisterWindowClass(instance)) return 1;
        if (!Create(instance)) return 1;

        ShowWindow(hwnd_, nCmdShow);
        UpdateWindow(hwnd_);

        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return 0;
    }

private:
    static constexpr const wchar_t* kClassName = L"ModernNumberGridCalculatorClass";

    bool RegisterWindowClass(HINSTANCE instance) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = StaticWndProc;
        wc.hInstance = instance;
        wc.lpszClassName = kClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        return RegisterClassW(&wc) != 0;
    }

    bool Create(HINSTANCE instance) {
        hwnd_ = CreateWindowExW(
            0,
            kClassName,
            L"Number Grid Calculator",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, kWindowWidth, kWindowHeight,
            nullptr,
            nullptr,
            instance,
            this
        );
        return hwnd_ != nullptr;
    }

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        MainWindowImpl* self = reinterpret_cast<MainWindowImpl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<MainWindowImpl*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        }

        if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);
        return self->WndProc(msg, wParam, lParam);
    }

    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE:
            CreateControls();
            return 0;
        case WM_COMMAND:
            HandleCommand(LOWORD(wParam));
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd_, msg, wParam, lParam);
        }
    }

    void CreateControls() {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        CreateWindowW(L"STATIC", L"Number Grid Calculator",
            WS_CHILD | WS_VISIBLE,
            kPad, 8, kWindowWidth - 2 * kPad, 20,
            hwnd_, nullptr, nullptr, nullptr);

        btnAdd_ = CreateWindowW(L"BUTTON", L"Add Number",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            kPad, 30, kWindowWidth - 2 * kPad, 32,
            hwnd_, reinterpret_cast<HMENU>(ID_BTN_ADD), nullptr, nullptr);

        list_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            kPad, 80, kWindowWidth - 2 * kPad, 220,
            hwnd_, reinterpret_cast<HMENU>(ID_LIST_NUMBERS), nullptr, nullptr);

        btnDelete_ = CreateWindowW(L"BUTTON", L"Delete Selected",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            kPad, 308, kWindowWidth - 2 * kPad, 26,
            hwnd_, reinterpret_cast<HMENU>(ID_BTN_DELETE), nullptr, nullptr);

        comboOp_ = CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            kPad, 342, kWindowWidth - 2 * kPad, 200,
            hwnd_, reinterpret_cast<HMENU>(ID_CMB_OPERATION), nullptr, nullptr);

        SendMessageW(comboOp_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sum"));
        SendMessageW(comboOp_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Multiply"));
        SendMessageW(comboOp_, CB_SETCURSEL, 0, 0);

        btnCalc_ = CreateWindowW(L"BUTTON", L"Calculate",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            kPad, 382, kWindowWidth - 2 * kPad, 32,
            hwnd_, reinterpret_cast<HMENU>(ID_BTN_CALC), nullptr, nullptr);

        result_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"Result: -",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            kPad, 428, kWindowWidth - 2 * kPad, 32,
            hwnd_, reinterpret_cast<HMENU>(ID_LBL_RESULT), nullptr, nullptr);

        SendMessageW(btnAdd_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(list_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(btnDelete_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(comboOp_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(btnCalc_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(result_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    void HandleCommand(int id) {
        if (id == ID_BTN_ADD) {
            double value = 0.0;
            if (AddNumberDialog::Show(hwnd_, value)) {
                repository_.Add(value);
                RefreshList();
                SetWindowTextW(result_, L"Result: -");
            }
            return;
        }

        if (id == ID_BTN_DELETE) {
            const int selected = static_cast<int>(SendMessageW(list_, LB_GETCURSEL, 0, 0));
            if (selected != LB_ERR && repository_.RemoveAt(static_cast<std::size_t>(selected))) {
                RefreshList();
                SetWindowTextW(result_, L"Result: -");
            }
            return;
        }

        if (id == ID_BTN_CALC) {
            const int index = static_cast<int>(SendMessageW(comboOp_, CB_GETCURSEL, 0, 0));
            const Operation op = (index == 1) ? Operation::Multiply : Operation::Sum;
            const std::wstring text = CalculatorEngine::BuildResult(repository_.All(), op);
            SetWindowTextW(result_, text.c_str());
            return;
        }
    }

    void RefreshList() {
        SendMessageW(list_, LB_RESETCONTENT, 0, 0);
        for (double value : repository_.All()) {
            const std::wstring item = ToWide(FormatNumber(value));
            SendMessageW(list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
        }
    }

    HWND hwnd_ = nullptr;
    HWND btnAdd_ = nullptr;
    HWND list_ = nullptr;
    HWND btnDelete_ = nullptr;
    HWND comboOp_ = nullptr;
    HWND btnCalc_ = nullptr;
    HWND result_ = nullptr;
    NumberRepository repository_;
};

}  // namespace

int MainWindow::Run(HINSTANCE instance, int nCmdShow) {
    MainWindowImpl app;
    return app.Run(instance, nCmdShow);
}
