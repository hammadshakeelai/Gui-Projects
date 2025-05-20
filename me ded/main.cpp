// -----------------------------
// File: main.cpp
// GUI portion of To-Do List application using Win32 API
// -----------------------------

#define UNICODE
#define _UNICODE

#include <windows.h>
#include "TaskManager.h"

// Control IDs for GUI elements
#define ID_LIST       101
#define ID_EDIT       102
#define ID_ADD_BTN    103
#define ID_REMOVE_BTN 104

// Forward declaration of window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    // Register the window class
    const wchar_t CLASS_NAME[] = L"ToDoAppClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    // Create a fixed-size window (400x400), no maximize button
    HWND hwnd = CreateWindowEx(
        0,                   // Optional window styles
        CLASS_NAME,          // Window class
        L"To-Do List ", // Window title
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList, hEdit, hAddBtn, hRemoveBtn;
    static TaskManager taskManager;

    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        
        // Create an edit box for task input
        hEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 10, 260, 25, hwnd, (HMENU)ID_EDIT, hInst, NULL);

        // Create "Add" button
        hAddBtn = CreateWindow(
            L"BUTTON", L"Add",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            280, 10, 50, 25, hwnd, (HMENU)ID_ADD_BTN, hInst, NULL);

        // Create "Remove" button
        hRemoveBtn = CreateWindow(
            L"BUTTON", L"Remove",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            340, 10, 60, 25, hwnd, (HMENU)ID_REMOVE_BTN, hInst, NULL);

        // Create list box to display tasks
        hList = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            10, 45, 380, 310, hwnd, (HMENU)ID_LIST, hInst, NULL);

        // Disable "Remove" until a task is selected
        EnableWindow(hRemoveBtn, FALSE);

        // Load existing tasks from file
        taskManager.loadFromFile();
        for (int i = 0; i < taskManager.getTaskCount(); ++i) {
            std::wstring display = (taskManager.isCompleted(i) ? L"[X] " : L"[ ] ")
                                   + taskManager.getTask(i);
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
        }
    } return 0;

    case WM_COMMAND: {
        WORD id   = LOWORD(wParam);
        WORD code = HIWORD(wParam);
        
        if (id == ID_ADD_BTN && code == BN_CLICKED) {
            // Add button clicked: read text and add new task
            int len = GetWindowTextLength(hEdit);
            if (len > 0) {
                wchar_t* buffer = new wchar_t[len + 1];
                GetWindowText(hEdit, buffer, len + 1);
                std::wstring newTask(buffer);
                delete[] buffer;

                taskManager.addTask(newTask);
                std::wstring display = L"[ ] " + newTask;
                SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                SetWindowText(hEdit, L"");
                SetFocus(hEdit);
            }
        }
        else if (id == ID_REMOVE_BTN && code == BN_CLICKED) {
            // Remove selected task
            LRESULT idx = SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (idx != LB_ERR) {
                taskManager.removeTask(idx);
                SendMessage(hList, LB_DELETESTRING, (WPARAM)idx, 0);
            }
            EnableWindow(hRemoveBtn, FALSE);
        }
        else if (id == ID_LIST && code == LBN_SELCHANGE) {
            // Enable "Remove" when a task is selected
            LRESULT sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
            EnableWindow(hRemoveBtn, (sel != LB_ERR));
        }
        else if (id == ID_LIST && code == LBN_DBLCLK) {
            // Double-click toggles completion status
            LRESULT sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                taskManager.toggleTask(sel);
                std::wstring updated = (taskManager.isCompleted(sel) ? L"[X] " : L"[ ] ")
                                       + taskManager.getTask(sel);
                SendMessage(hList, LB_DELETESTRING, sel, 0);
                SendMessage(hList, LB_INSERTSTRING, sel, (LPARAM)updated.c_str());
                SendMessage(hList, LB_SETCURSEL, sel, 0);
            }
        }
    } return 0;

    case WM_CLOSE:
        // Save tasks before closing
        taskManager.saveToFile();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}