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
