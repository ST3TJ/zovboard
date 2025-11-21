#define _WIN32_WINNT 0x0500
#include <Windows.h>
#include <shellapi.h>
#include <map>
#include "resource.h"

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

HHOOK keyboardHook;
NOTIFYICONDATA nid;
HWND hWindow;

std::map<WCHAR, WCHAR> charReplacements = {
    {L'з', L'Z'},
    {L'в', L'V'},
    {L'о', L'O'}};

void SendChar(WCHAR ch)
{
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = ch;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;

    input[1] = input[0];
    input[1].ki.dwFlags |= KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *kbdStruct = (KBDLLHOOKSTRUCT *)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            if (kbdStruct->flags & LLKHF_INJECTED)
            {
                return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
            }

            BYTE keyboardState[256];
            GetKeyboardState(keyboardState);

            HWND foregroundWindow = GetForegroundWindow();
            DWORD threadId = GetWindowThreadProcessId(foregroundWindow, NULL);
            HKL layout = GetKeyboardLayout(threadId);

            WCHAR buffer[2];
            int result = ToUnicodeEx(kbdStruct->vkCode, kbdStruct->scanCode,
                                     keyboardState, buffer, 2, 0, layout);

            if (result > 0)
            {
                WCHAR typedChar = buffer[0];
                auto it = charReplacements.find(typedChar);
                if (it != charReplacements.end())
                {
                    SendChar(it->second);
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        memset(&nid, 0, sizeof(NOTIFYICONDATA));
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;

        nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));

        if (nid.hIcon == NULL)
        {
            nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        }

        wcscpy_s(nid.szTip, L"ZOVboard");
        Shell_NotifyIcon(NIM_ADD, &nid);
        break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            POINT curPoint;
            GetCursorPos(&curPoint);
            SetForegroundWindow(hwnd);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"ЧЕМOДАН, VOКZАЛ, НАХУЙ!");
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, curPoint.x, curPoint.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT)
        {
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"ZOVboard";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));

    RegisterClass(&wc);

    hWindow = CreateWindowEx(
        0, CLASS_NAME, L"ZOVboard",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    if (hWindow == NULL)
        return 0;

    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (!keyboardHook)
    {
        MessageBox(NULL, L"Failed to install hook!", L"Error", MB_ICONERROR);
        return 1;
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);

    return 0;
}