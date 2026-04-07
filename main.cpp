#include "MainWindow.hpp"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int nCmdShow) {
    MainWindow window;
    return window.Run(instance, nCmdShow);
}
#include <windows.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kWindowWidth = 420;
constexpr int kWindowHeight = 520;
constexpr int kPad = 20;

enum class Operation {
    Sum = 0,
    Multiply = 1
};

enum ControlId {
    ID_BTN_ADD = 1001,
    ID_LIST_NUMBERS,
    ID_BTN_DELETE,
    ID_CMB_OPERATION,
    ID_BTN_CALC,
    ID_LBL_RESULT,
    ID_EDIT_ADD_VALUE = 2001
};

std::wstring ToWide(const std::string& s) {
    if (s.empty()) return L"";
    const int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], size);
    return ws;
}

std::string FormatNumber(double value) {
    std::ostringstream ss;
    ss << std::setprecision(10) << value;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}

class NumberRepository {
public:
    void Add(double value) { numbers_.push_back(value); }

    bool RemoveAt(size_t idx) {
        if (idx >= numbers_.size()) return false;
        numbers_.erase(numbers_.begin() + static_cast<std::ptrdiff_t>(idx));
        return true;
    }

    const std::vector<double>& All() const { return numbers_; }
    bool Empty() const { return numbers_.empty(); }

private:
    std::vector<double> numbers_;
};

class CalculatorEngine {
public:
    static std::wstring BuildResult(const std::vector<double>& values, Operation operation) {
        if (values.empty()) return L"Result: (no numbers)";

        double result = (operation == Operation::Sum) ? 0.0 : 1.0;
        for (double v : values) {
            if (operation == Operation::Sum) {
                result += v;
            } else {
                result *= v;
            }
        }

        const std::wstring prefix =
            (operation == Operation::Sum) ? L"Result (Sum): " : L"Result (Multiply): ";
        return prefix + ToWide(FormatNumber(result));
    }
};

class AddNumberDialog {
public:
    static bool Show(HWND parent, double& outValue) {
        RegisterClassIfNeeded();

        AddNumberDialog dialog;
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
        AddNumberDialog* self =
            reinterpret_cast<AddNumberDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

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

class MainWindow {
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
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<MainWindow*>(cs->lpCreateParams);
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
            if (selected != LB_ERR && repository_.RemoveAt(static_cast<size_t>(selected))) {
                RefreshList();
                SetWindowTextW(result_, L"Result: -");
            }
            return;
        }

        if (id == ID_BTN_CALC) {
            const auto operationIndex = static_cast<int>(SendMessageW(comboOp_, CB_GETCURSEL, 0, 0));
            const Operation op = (operationIndex == 1) ? Operation::Multiply : Operation::Sum;
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int nCmdShow) {
    MainWindow app;
    return app.Run(instance, nCmdShow);
}
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

static const int WIN_W = 420;
static const int WIN_H = 490;
static const int PAD = 20;

enum ControlId {
    ID_BTN_ADD = 1001,
    ID_LIST_NUMBERS,
    ID_BTN_DELETE,
    ID_CMB_OP,
    ID_BTN_CALC,
    ID_LBL_RESULT
};

static HWND gList = nullptr;
static HWND gCombo = nullptr;
static HWND gResult = nullptr;

static std::vector<double> numbers;
static std::wstring resultText = L"Result: -";

static const int ID_EDIT_VALUE = 2001;

static std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], size);
    return ws;
}

static std::string formatNum(double v) {
    std::ostringstream ss;
    ss << std::setprecision(10) << v;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}

static void refreshList() {
    SendMessageW(gList, LB_RESETCONTENT, 0, 0);
    for (double v : numbers) {
        std::wstring item = toWide(formatNum(v));
        SendMessageW(gList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    }
}

static void refreshResult() {
    SetWindowTextW(gResult, resultText.c_str());
}

static void calculate() {
    if (numbers.empty()) {
        resultText = L"Result: (no numbers)";
        return;
    }

    int op = static_cast<int>(SendMessageW(gCombo, CB_GETCURSEL, 0, 0));
    double res = (op == 0) ? 0.0 : 1.0;
    for (double v : numbers) {
        if (op == 0) res += v;
        else res *= v;
    }

    std::wstring prefix = (op == 0) ? L"Result (Sum): " : L"Result (Multiply): ";
    resultText = prefix + toWide(formatNum(res));
}

struct AddDialogState {
    HWND edit = nullptr;
    bool done = false;
    bool accepted = false;
    std::wstring value;
};

static LRESULT CALLBACK AddDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AddDialogState* state = reinterpret_cast<AddDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_CREATE: {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowW(L"STATIC", L"Enter a number:",
            WS_CHILD | WS_VISIBLE, 20, 18, 240, 20, hwnd, nullptr, nullptr, nullptr);
        state->edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            20, 42, 240, 24, hwnd, reinterpret_cast<HMENU>(ID_EDIT_VALUE), nullptr, nullptr);
        HWND okBtn = CreateWindowW(L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            20, 80, 110, 28, hwnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        HWND cancelBtn = CreateWindowW(L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE,
            150, 80, 110, 28, hwnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);

        SendMessageW(state->edit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(okBtn, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(cancelBtn, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SetFocus(state->edit);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buffer[128] = L"";
            GetWindowTextW(state->edit, buffer, 127);
            state->value = buffer;
            state->accepted = true;
            state->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            state->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        state->done = true;
        DestroyWindow(hwnd);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static bool inputDialog(HWND parent, double& outVal) {
    static const wchar_t CLASS_NAME[] = L"AddNumberDialogClass";
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = AddDialogProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        if (!RegisterClassW(&wc)) return false;
        classRegistered = true;
    }

    AddDialogState state;
    EnableWindow(parent, FALSE);
    HWND dlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        CLASS_NAME,
        L"Add Number",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        290,
        155,
        parent,
        nullptr,
        GetModuleHandleW(nullptr),
        &state
    );
    if (!dlg) {
        EnableWindow(parent, TRUE);
        return false;
    }

    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);

    MSG msg;
    while (!state.done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);

    if (!state.accepted || state.value.empty()) return false;
    try {
        outVal = std::stod(state.value);
        return true;
    } catch (...) {
        MessageBoxW(parent, L"Invalid number format.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        CreateWindowW(L"STATIC", L"Number Grid Calculator",
            WS_CHILD | WS_VISIBLE, PAD, 8, WIN_W - 2 * PAD, 20, hwnd, nullptr, nullptr, nullptr);

        HWND btnAdd = CreateWindowW(L"BUTTON", L"Add Number",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, PAD, 30, WIN_W - 2 * PAD, 32,
            hwnd, reinterpret_cast<HMENU>(ID_BTN_ADD), nullptr, nullptr);

        gList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            PAD, 80, WIN_W - 2 * PAD, 210, hwnd,
            reinterpret_cast<HMENU>(ID_LIST_NUMBERS), nullptr, nullptr);

        HWND btnDel = CreateWindowW(L"BUTTON", L"Delete Selected",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, PAD, 300, WIN_W - 2 * PAD, 26,
            hwnd, reinterpret_cast<HMENU>(ID_BTN_DELETE), nullptr, nullptr);

        gCombo = CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            PAD, 334, WIN_W - 2 * PAD, 220, hwnd,
            reinterpret_cast<HMENU>(ID_CMB_OP), nullptr, nullptr);
        SendMessageW(gCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sum"));
        SendMessageW(gCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Multiply"));
        SendMessageW(gCombo, CB_SETCURSEL, 0, 0);

        HWND btnCalc = CreateWindowW(L"BUTTON", L"Calculate",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, PAD, 374, WIN_W - 2 * PAD, 32,
            hwnd, reinterpret_cast<HMENU>(ID_BTN_CALC), nullptr, nullptr);

        gResult = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", resultText.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            PAD, 424, WIN_W - 2 * PAD, 28, hwnd,
            reinterpret_cast<HMENU>(ID_LBL_RESULT), nullptr, nullptr);

        SendMessageW(btnAdd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(gList, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(btnDel, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(gCombo, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(btnCalc, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(gResult, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_ADD) {
            double val = 0.0;
            if (inputDialog(hwnd, val)) {
                numbers.push_back(val);
                refreshList();
                resultText = L"Result: -";
                refreshResult();
            }
        } else if (id == ID_BTN_DELETE) {
            int idx = static_cast<int>(SendMessageW(gList, LB_GETCURSEL, 0, 0));
            if (idx != LB_ERR && idx >= 0 && idx < static_cast<int>(numbers.size())) {
                numbers.erase(numbers.begin() + idx);
                refreshList();
                resultText = L"Result: -";
                refreshResult();
            }
        } else if (id == ID_BTN_CALC) {
            calculate();
            refreshResult();
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"NumberGridCalculatorWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) return 1;

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Number Grid Calculator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WIN_W, WIN_H + 30,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// ── Constants ─────────────────────────────────────────────────────────────────
static const int WIN_W = 420, WIN_H = 490, PAD = 20, ROW_H = 26;

// ── X11 globals ───────────────────────────────────────────────────────────────
static Display* dpy;
static int      scr;
static Window   mainWin;
static GC       gc;

// ── Xft globals ───────────────────────────────────────────────────────────────
static XftFont*  xFont    = nullptr;
static XftDraw*  xftDraw  = nullptr;  // current draw target
static XftColor  xftBlack, xftWhite;

static void initFont() {
    xFont = XftFontOpenName(dpy, scr, "DejaVu Sans:size=11");
    if (!xFont)
        xFont = XftFontOpenName(dpy, scr, "sans-serif:size=11");

    Visual*  vis = DefaultVisual(dpy, scr);
    Colormap cm  = DefaultColormap(dpy, scr);
    XftColorAllocName(dpy, vis, cm, "black", &xftBlack);
    XftColorAllocName(dpy, vis, cm, "white", &xftWhite);
}

// Point the Xft draw context at a window before calling any drawStr functions
static void setXftTarget(Window win) {
    if (xftDraw) XftDrawDestroy(xftDraw);
    xftDraw = XftDrawCreate(dpy, win,
        DefaultVisual(dpy, scr), DefaultColormap(dpy, scr));
}

static int textWidth(const std::string& s) {
    XGlyphInfo ext;
    XftTextExtentsUtf8(dpy, xFont,
        reinterpret_cast<const FcChar8*>(s.c_str()), (int)s.size(), &ext);
    return ext.xOff;
}

// ── Colors ────────────────────────────────────────────────────────────────────
static unsigned long cWhite, cBlack, cBlue, cLightBlue, cGray, cLightGray, cYellow,
                     cRed, cSelectRow;

static void initColors() {
    Colormap cm = DefaultColormap(dpy, scr);
    auto alloc = [&](const char* name) {
        XColor c; XAllocNamedColor(dpy, cm, name, &c, &c); return c.pixel;
    };
    cWhite     = WhitePixel(dpy, scr);
    cBlack     = BlackPixel(dpy, scr);
    cBlue      = alloc("#3a78c9");
    cLightBlue = alloc("#cce4ff");
    cGray      = alloc("#888888");
    cLightGray = alloc("#f0f0f0");
    cYellow    = alloc("#fffde7");
    cRed       = alloc("#c0392b");
    cSelectRow = alloc("#ffe0b2");
}

// ── Geometry ──────────────────────────────────────────────────────────────────
struct Rect { int x, y, w, h; };
static bool hit(const Rect& r, int px, int py) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

// ── Draw helpers ──────────────────────────────────────────────────────────────
static void fillR(Window win, unsigned long col, Rect r) {
    XSetForeground(dpy, gc, col);
    XFillRectangle(dpy, win, gc, r.x, r.y, r.w, r.h);
}
static void strokeR(Window win, unsigned long col, Rect r) {
    XSetForeground(dpy, gc, col);
    XDrawRectangle(dpy, win, gc, r.x, r.y, r.w, r.h);
}

// Draw text at baseline (x, y) using current xftDraw target
static void drawStr(int x, int y, const std::string& s, XftColor* col = nullptr) {
    XftDrawStringUtf8(xftDraw, col ? col : &xftBlack, xFont,
        x, y, reinterpret_cast<const FcChar8*>(s.c_str()), (int)s.size());
}
// Center text horizontally and vertically inside a rect, white color (for buttons)
static void drawStrC(Rect r, const std::string& s, XftColor* col = nullptr) {
    int tw = textWidth(s);
    int tx = r.x + (r.w - tw) / 2;
    int ty = r.y + (r.h + xFont->ascent - xFont->descent) / 2;
    drawStr(tx, ty, s, col ? col : &xftWhite);
}

static std::string fmtNum(double v) {
    std::ostringstream ss;
    ss << std::setprecision(10) << v;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

// ── App state ─────────────────────────────────────────────────────────────────
static std::vector<double> numbers;
static int  selOp      = 0;   // 0 = Sum, 1 = Multiply
static int  selectedIdx = -1; // selected grid row (-1 = none)
static bool dropOpen   = false;
static std::string resultStr = "Result: —";
static const std::string OPS[2] = {"Sum", "Multiply"};

// ── Layout ────────────────────────────────────────────────────────────────────
static const Rect rBtnAdd  = {PAD,  20,  WIN_W - 2*PAD, 32};
static const Rect rGrid    = {PAD,  70,  WIN_W - 2*PAD, 215};
static const Rect rBtnDel  = {PAD,  292, WIN_W - 2*PAD, 26};
static const Rect rDrop    = {PAD,  328, WIN_W - 2*PAD, 30};
static const Rect rDropI[2]= {
    {PAD, 358, WIN_W - 2*PAD, ROW_H},
    {PAD, 384, WIN_W - 2*PAD, ROW_H},
};
static const Rect rBtnCalc = {PAD,  378, WIN_W - 2*PAD, 32};
static const Rect rResult  = {PAD,  426, WIN_W - 2*PAD, 32};

// ── Widgets ───────────────────────────────────────────────────────────────────
static void drawButton(Window win, Rect r, const std::string& label,
                       unsigned long bg = 0) {
    fillR(win, bg ? bg : cBlue, r);
    strokeR(win, cGray, r);
    setXftTarget(win);
    drawStrC(r, label);  // white, centered
}

static void drawGrid(Window win) {
    Rect r = rGrid;
    fillR(win, cLightGray, r);
    strokeR(win, cGray, r);

    Rect hdr = {r.x, r.y, r.w, ROW_H};
    fillR(win, cLightBlue, hdr);
    strokeR(win, cGray, hdr);

    setXftTarget(win);
    int baseY = r.y + (ROW_H + xFont->ascent - xFont->descent) / 2;
    drawStr(r.x + 8, baseY, "Numbers");

    int visRows = (r.h - ROW_H) / ROW_H;
    int start   = (int)numbers.size() > visRows
                  ? (int)numbers.size() - visRows : 0;

    for (int i = start; i < (int)numbers.size(); i++) {
        int row = i - start;
        int ry  = r.y + ROW_H + row * ROW_H;
        Rect rowR = {r.x + 1, ry, r.w - 2, ROW_H};
        unsigned long rowCol = (i == selectedIdx) ? cSelectRow
                             : (row % 2 == 0 ? cWhite : cLightGray);
        fillR(win, rowCol, rowR);
        setXftTarget(win);
        int ty = ry + (ROW_H + xFont->ascent - xFont->descent) / 2;
        drawStr(r.x + 8, ty, fmtNum(numbers[i]));
    }
}

static void drawDropdown(Window win) {
    fillR(win, cWhite, rDrop);
    strokeR(win, cGray, rDrop);
    setXftTarget(win);
    int ty = rDrop.y + (rDrop.h + xFont->ascent - xFont->descent) / 2;
    drawStr(rDrop.x + 8, ty,
            "Operation: " + OPS[selOp] + (dropOpen ? "  [^]" : "  [v]"));

    if (dropOpen) {
        for (int i = 0; i < 2; i++) {
            fillR(win, i == selOp ? cLightBlue : cWhite, rDropI[i]);
            strokeR(win, cGray, rDropI[i]);
            setXftTarget(win);
            int ity = rDropI[i].y + (ROW_H + xFont->ascent - xFont->descent) / 2;
            drawStr(rDropI[i].x + 8, ity, OPS[i]);
        }
    }
}

static void drawResult(Window win) {
    fillR(win, cYellow, rResult);
    strokeR(win, cGray, rResult);
    setXftTarget(win);
    int ty = rResult.y + (rResult.h + xFont->ascent - xFont->descent) / 2;
    drawStr(rResult.x + 8, ty, resultStr);
}

static void redrawAll() {
    fillR(mainWin, cWhite, {0, 0, WIN_W, WIN_H});
    setXftTarget(mainWin);
    drawStr(PAD, xFont->ascent + 4, "Number Grid Calculator");
    drawButton(mainWin, rBtnAdd, "Add Number");
    drawGrid(mainWin);
    drawButton(mainWin, rBtnDel, "Delete Selected",
               selectedIdx >= 0 ? cRed : cGray);
    drawButton(mainWin, rBtnCalc, "Calculate");
    drawResult(mainWin);
    drawDropdown(mainWin);  // drawn last — items overlay Calculate when open
    XFlush(dpy);
}

// ── Dialog: enter a number ────────────────────────────────────────────────────
static bool showAddDialog(double& out) {
    const int DW = 300, DH = 160;
    Window dlg = XCreateSimpleWindow(
        dpy, mainWin,
        (WIN_W - DW) / 2, (WIN_H - DH) / 2,
        DW, DH, 2, cBlack, cWhite);
    XStoreName(dpy, dlg, "Add Number");
    XSelectInput(dpy, dlg, ExposureMask | KeyPressMask | ButtonPressMask);
    XMapRaised(dpy, dlg);
    XGrabKeyboard(dpy, dlg, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XFlush(dpy);

    const Rect rInput  = {20, 55, DW - 40, 28};
    const Rect rOK     = {20, 112, 115, 30};
    const Rect rCancel = {DW - 135, 112, 115, 30};

    std::string input;
    bool ok = false, running = true;

    auto dlgDraw = [&]() {
        fillR(dlg, cWhite, {0, 0, DW, DH});
        setXftTarget(dlg);
        drawStr(20, xFont->ascent + 15, "Enter a number:");
        fillR(dlg, cLightGray, rInput);
        strokeR(dlg, cGray, rInput);
        setXftTarget(dlg);
        int ity = rInput.y + (rInput.h + xFont->ascent - xFont->descent) / 2;
        drawStr(rInput.x + 5, ity, input + "|");
        drawButton(dlg, rOK, "OK");
        drawButton(dlg, rCancel, "Cancel");
        XFlush(dpy);
    };

    XEvent ev;
    while (running) {
        XNextEvent(dpy, &ev);
        if (ev.xany.window != dlg) continue;

        switch (ev.type) {
        case Expose:
            if (ev.xexpose.count == 0) dlgDraw();
            break;
        case ButtonPress: {
            int bx = ev.xbutton.x, by = ev.xbutton.y;
            if      (hit(rOK, bx, by) && !input.empty()) { ok = true;  running = false; }
            else if (hit(rCancel, bx, by))                 {             running = false; }
            break;
        }
        case KeyPress: {
            char buf[32]{}; KeySym ks;
            int n = XLookupString(&ev.xkey, buf, 31, &ks, nullptr);
            if (ks == XK_Return || ks == XK_KP_Enter) {
                if (!input.empty()) { ok = true; running = false; }
            } else if (ks == XK_Escape) {
                running = false;
            } else if (ks == XK_BackSpace) {
                if (!input.empty()) { input.pop_back(); dlgDraw(); }
            } else if (n == 1) {
                char c = buf[0];
                bool valid = (c >= '0' && c <= '9')
                          || (c == '.' && input.find('.') == std::string::npos)
                          || (c == '-' && input.empty());
                if (valid) { input += c; dlgDraw(); }
            }
            break;
        }
        }
    }

    XUngrabKeyboard(dpy, CurrentTime);
    if (xftDraw) { XftDrawDestroy(xftDraw); xftDraw = nullptr; }
    XDestroyWindow(dpy, dlg);
    XFlush(dpy);

    if (ok && !input.empty()) {
        try { out = std::stod(input); return true; }
        catch (...) {}
    }
    return false;
}

// ── Calculate ─────────────────────────────────────────────────────────────────
static void calculate() {
    if (numbers.empty()) {
        resultStr = "Result: (no numbers)";
        return;
    }
    double res;
    if (selOp == 0) {
        res = 0.0;
        for (double v : numbers) res += v;
        resultStr = "Result (Sum): " + fmtNum(res);
    } else {
        res = 1.0;
        for (double v : numbers) res *= v;
        resultStr = "Result (Multiply): " + fmtNum(res);
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
    setenv("DISPLAY", ":0", 0); // use :0 if DISPLAY is not already set
    dpy = XOpenDisplay(nullptr);
    if (!dpy) return 1;
    scr = DefaultScreen(dpy);

    mainWin = XCreateSimpleWindow(
        dpy, RootWindow(dpy, scr),
        100, 100, WIN_W, WIN_H, 2,
        BlackPixel(dpy, scr), WhitePixel(dpy, scr));
    XStoreName(dpy, mainWin, "Number Grid Calculator");
    XSelectInput(dpy, mainWin, ExposureMask | ButtonPressMask);

    Atom wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, mainWin, &wmDelete, 1);

    gc = XCreateGC(dpy, mainWin, 0, nullptr);
    initColors();
    initFont();
    XMapWindow(dpy, mainWin);

    XEvent ev;
    for (;;) {
        XNextEvent(dpy, &ev);

        if (ev.type == ClientMessage &&
            (Atom)ev.xclient.data.l[0] == wmDelete) break;

        if (ev.xany.window != mainWin) continue;

        if (ev.type == Expose && ev.xexpose.count == 0) {
            redrawAll();
        }

        if (ev.type == ButtonPress) {
            int mx = ev.xbutton.x, my = ev.xbutton.y;

            if (dropOpen) {
                dropOpen = false;
                for (int i = 0; i < 2; i++) {
                    if (hit(rDropI[i], mx, my)) { selOp = i; break; }
                }
                redrawAll();
            } else if (hit(rDrop, mx, my)) {
                dropOpen = true;
                redrawAll();
            } else if (hit(rGrid, mx, my)) {
                // Click on a grid row (below header) → select/deselect
                int relY = my - rGrid.y - ROW_H;
                if (relY >= 0) {
                    int visRows = (rGrid.h - ROW_H) / ROW_H;
                    int start   = (int)numbers.size() > visRows
                                  ? (int)numbers.size() - visRows : 0;
                    int idx = start + relY / ROW_H;
                    if (idx < (int)numbers.size())
                        selectedIdx = (selectedIdx == idx) ? -1 : idx;
                }
                redrawAll();
            } else if (hit(rBtnDel, mx, my)) {
                if (selectedIdx >= 0 && selectedIdx < (int)numbers.size()) {
                    numbers.erase(numbers.begin() + selectedIdx);
                    selectedIdx = -1;
                    resultStr = "Result: —";
                }
                redrawAll();
            } else if (hit(rBtnAdd, mx, my)) {
                double val;
                if (showAddDialog(val)) numbers.push_back(val);
                redrawAll();
            } else if (hit(rBtnCalc, mx, my)) {
                calculate();
                redrawAll();
            }
        }
    }

    if (xftDraw) XftDrawDestroy(xftDraw);
    XftFontClose(dpy, xFont);
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, mainWin);
    XCloseDisplay(dpy);
    return 0;
}
