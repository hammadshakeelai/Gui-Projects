#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <fstream>

#pragma comment(lib, "comctl32.lib")

using namespace std;
static HINSTANCE hInst;

struct Item { wstring name; double price; int qty; };
static vector<Item> items;

// Control IDs
enum { ID_ADD = 101, ID_DELETE, ID_SAVE, ID_LOAD };

// HWNDs
static HWND hListView, hName, hPrice, hQty, hTotal;

// Function prototypes
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitListView();
void RefreshList();
void UpdateTotal();
bool SaveItems(const wstring& file);
bool LoadItems(const wstring& file);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);

    hInst = hInstance;
    WNDCLASSEX wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = _T("SupermarketClass");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, _T("Supermarket Management"),
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 750, 500,
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindow(_T("STATIC"), _T("Name:"), WS_CHILD | WS_VISIBLE, 10, 10, 50, 20, hwnd, nullptr, hInst, nullptr);
        hName = CreateWindow(_T("EDIT"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, 70, 10, 200, 24, hwnd, nullptr, hInst, nullptr);
        CreateWindow(_T("STATIC"), _T("Price:"), WS_CHILD | WS_VISIBLE, 280, 10, 50, 20, hwnd, nullptr, hInst, nullptr);
        hPrice = CreateWindow(_T("EDIT"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, 330, 10, 80, 24, hwnd, nullptr, hInst, nullptr);
        CreateWindow(_T("STATIC"), _T("Qty:"), WS_CHILD | WS_VISIBLE, 420, 10, 30, 20, hwnd, nullptr, hInst, nullptr);
        hQty = CreateWindow(_T("EDIT"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, 460, 10, 50, 24, hwnd, nullptr, hInst, nullptr);

        CreateWindow(_T("BUTTON"), _T("Add Item"), WS_CHILD | WS_VISIBLE, 520, 10, 100, 24, hwnd, (HMENU)ID_ADD, hInst, nullptr);
        CreateWindow(_T("BUTTON"), _T("Delete Item"), WS_CHILD | WS_VISIBLE, 630, 10, 100, 24, hwnd, (HMENU)ID_DELETE, hInst, nullptr);
        CreateWindow(_T("BUTTON"), _T("Save"), WS_CHILD | WS_VISIBLE, 630, 390, 100, 30, hwnd, (HMENU)ID_SAVE, hInst, nullptr);
        CreateWindow(_T("BUTTON"), _T("Load"), WS_CHILD | WS_VISIBLE, 630, 430, 100, 30, hwnd, (HMENU)ID_LOAD, hInst, nullptr);

        hListView = CreateWindow(WC_LISTVIEW, nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            10, 50, 610, 330, hwnd, nullptr, hInst, nullptr);
        InitListView();

        hTotal = CreateWindow(_T("STATIC"), _T("Total: 0.00"), WS_CHILD | WS_VISIBLE, 10, 390, 610, 20, hwnd, nullptr, hInst, nullptr);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ADD: {
            TCHAR buf[128];
            GetWindowText(hName, buf, 128);
            wstring name = buf;
            GetWindowText(hPrice, buf, 128);
            double price = _wtof(buf);
            GetWindowText(hQty, buf, 128);
            int qty = _wtoi(buf);
            if (!name.empty() && qty > 0) items.push_back({ name, price, qty });
            RefreshList();
            UpdateTotal();
            break;
        }
        case ID_DELETE: {
            int sel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (sel >= 0 && sel < (int)items.size()) {
                items.erase(items.begin() + sel);
                RefreshList();
                UpdateTotal();
            }
            break;
        }
        case ID_SAVE:
            SaveItems(_T("items.txt"));
            break;
        case ID_LOAD:
            LoadItems(_T("items.txt"));
            RefreshList();
            UpdateTotal();
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void InitListView() {
    LVCOLUMN col = { LVCF_TEXT | LVCF_WIDTH };
    col.cx = 300; col.pszText = _T("Name");    ListView_InsertColumn(hListView, 0, &col);
    col.cx = 100; col.pszText = _T("Price");   ListView_InsertColumn(hListView, 1, &col);
    col.cx = 80;  col.pszText = _T("Qty");     ListView_InsertColumn(hListView, 2, &col);
    col.cx = 120; col.pszText = _T("Subtotal");ListView_InsertColumn(hListView, 3, &col);
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void RefreshList() {
    ListView_DeleteAllItems(hListView);
    for (int i = 0; i < (int)items.size(); ++i) {
        LVITEM it = { LVIF_TEXT };
        it.iItem = i;
        it.iSubItem = 0;
        it.pszText = const_cast<LPWSTR>(items[i].name.c_str());
        ListView_InsertItem(hListView, &it);
        TCHAR buf[64];
        _stprintf(buf, _T("%.2f"), items[i].price);
        ListView_SetItemText(hListView, i, 1, buf);
        _stprintf(buf, _T("%d"), items[i].qty);
        ListView_SetItemText(hListView, i, 2, buf);
        _stprintf(buf, _T("%.2f"), items[i].price * items[i].qty);
        ListView_SetItemText(hListView, i, 3, buf);
    }
}

void UpdateTotal() {
    double sum = 0;
    for (auto &it : items) sum += it.price * it.qty;
    TCHAR buf[64];
    _stprintf(buf, _T("Total: %.2f"), sum);
    SetWindowText(hTotal, buf);
}

bool SaveItems(const wstring& file) {
    wofstream ofs;
    ofs.open(file.c_str());
    if (!ofs.is_open()) return false;
    for (auto &it : items)
        ofs << it.name << L"\t" << it.price << L"\t" << it.qty << L"\n";
    return true;
}

bool LoadItems(const wstring& file) {
    wifstream ifs;
    ifs.open(file.c_str());
    if (!ifs.is_open()) return false;
    items.clear();
    wstring name;
    double price;
    int qty;
    while (ifs >> name >> price >> qty)
        items.push_back({ name, price, qty });
    return true;
}