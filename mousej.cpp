#include <windows.h>
#include <shellapi.h>
#include <chrono>

// Global variables
const UINT_PTR IDT_TIMER1 = 1;
const int IDLE_THRESHOLD = 120000; // 2 minutes in milliseconds
const int JIGGLE_DISTANCE = 50; // Distance to move the mouse
NOTIFYICONDATA nid;
bool running = true;
HHOOK mouseHook;
HHOOK keyboardHook;
DWORD lastActivityTime;

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        lastActivityTime = GetTickCount();
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        lastActivityTime = GetTickCount();
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void MoveMouse() {
    POINT pos;
    GetCursorPos(&pos);
    SetCursorPos(pos.x + JIGGLE_DISTANCE, pos.y);
    SetCursorPos(pos.x, pos.y);
}

void TimerProc(HWND, UINT, UINT, DWORD) {
    if (GetTickCount() - lastActivityTime > IDLE_THRESHOLD) {
        MoveMouse();
        lastActivityTime = GetTickCount();
    }
}

void AddTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = IDT_TIMER1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    strcpy_s(nid.szTip, "Mouse Jiggler");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            AddTrayIcon(hwnd);
            SetTimer(hwnd, IDT_TIMER1, 1000, (TIMERPROC)TimerProc);
            break;
        case WM_DESTROY:
            RemoveTrayIcon();
            KillTimer(hwnd, IDT_TIMER1);
            PostQuitMessage(0);
            break;
        case WM_USER + 1:
            if (lParam == WM_RBUTTONUP) {
                running = false;
                PostQuitMessage(0);
            }
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    lastActivityTime = GetTickCount();
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    const char CLASS_NAME[] = "MouseJigglerClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
            WS_EX_TOOLWINDOW,      // Use WS_EX_TOOLWINDOW to create a hidden window
            CLASS_NAME,
            "Mouse Jiggler",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            nullptr,
            nullptr,
            hInstance,
            nullptr
    );

    if (hwnd == nullptr) {
        return 0;
    }

    // Do not call ShowWindow to keep the window hidden

    MSG msg = {};
    while (running && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(mouseHook);
    UnhookWindowsHookEx(keyboardHook);

    return 0;
}
