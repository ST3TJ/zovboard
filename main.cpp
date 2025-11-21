#include <Windows.h>
#include <iostream>
#include <map>

HHOOK keyboardHook;

std::map<WCHAR, WCHAR> charReplacements = {
    {L'з', L'Z'},
    {L'в', L'V'},
    {L'о', L'O'}
};

void SendChar(WCHAR ch) {
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = ch;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;

    input[1] = input[0];
    input[1].ki.dwFlags |= KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            BYTE keyboardState[256];
            GetKeyboardState(keyboardState);

            HWND foregroundWindow = GetForegroundWindow();
            DWORD threadId = GetWindowThreadProcessId(foregroundWindow, NULL);
            HKL layout = GetKeyboardLayout(threadId);

            WCHAR buffer[2];
            int result = ToUnicodeEx(kbdStruct->vkCode, kbdStruct->scanCode,
                keyboardState, buffer, 2, 0, layout);
            if (result > 0) {
                WCHAR typedChar = buffer[0];
                auto it = charReplacements.find(typedChar);
                if (it != charReplacements.end()) {
                    SendChar(it->second);
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

int main() {
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (!keyboardHook) {
        std::cerr << "Failed to install keyboard hook!" << std::endl;
        return 1;
    }
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(keyboardHook);
    return 0;
}