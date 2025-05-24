// badminton_tournament_win32.cpp
// A Win32 C++ GUI application for managing a badminton tournament
// with CSV reading and writing capabilities.
// Tracks Wins and Points per player with grouped, intuitive controls.

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "Comctl32.lib") // Link Common Controls

// Control IDs
#define IDC_GROUPBOX       100
#define IDC_STATIC_NAME    101
#define IDC_EDIT_NAME      102
#define IDC_STATIC_POINTS  103
#define IDC_EDIT_POINTS    104
#define IDC_BTN_ADD        105
#define IDC_BTN_INC_WIN    106
#define IDC_BTN_ADD_PTS    107
#define IDC_BTN_SAVE       108
#define IDC_BTN_LOAD       109
#define IDC_LISTVIEW       110

struct Player { std::string name; int wins; int points; };
static std::vector<Player> players;
HWND hEditName, hEditPoints, hListView;

// Refresh ListView display
void UpdateListView() {
    ListView_DeleteAllItems(hListView);
    for (int i = 0; i < (int)players.size(); ++i) {
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPSTR>(players[i].name.c_str());
        ListView_InsertItem(hListView, &lvi);
        char buf[32];
        // Wins column
        wsprintfA(buf, "%d", players[i].wins);
        ListView_SetItemText(hListView, i, 1, buf);
        // Points column
        wsprintfA(buf, "%d", players[i].points);
        ListView_SetItemText(hListView, i, 2, buf);
    }
}

void SaveCSV(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    file << "Name,Wins,Points\n";
    for (auto &p : players)
        file << p.name << "," << p.wins << "," << p.points << "\n";
}

void LoadCSV(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    players.clear();
    std::string line;
    std::getline(file, line); // header
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name, winsStr, ptsStr;
        if (std::getline(ss, name, ',') && std::getline(ss, winsStr, ',') && std::getline(ss, ptsStr)) {
            players.push_back({ name, std::stoi(winsStr), std::stoi(ptsStr) });
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Initialize common controls
        INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
        InitCommonControlsEx(&icex);
        // Group box
        CreateWindowExA(0, "BUTTON", "Player Actions",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            10, 10, 750, 110, hWnd, (HMENU)IDC_GROUPBOX, NULL, NULL);
        // Name label & edit
        CreateWindowExA(0, "STATIC", "Name:",
            WS_CHILD | WS_VISIBLE,
            30, 35, 40, 20, hWnd, (HMENU)IDC_STATIC_NAME, NULL, NULL);
        hEditName = CreateWindowExA(0, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            80, 32, 180, 24, hWnd, (HMENU)IDC_EDIT_NAME, NULL, NULL);
        // Points label & edit
        CreateWindowExA(0, "STATIC", "Points:",
            WS_CHILD | WS_VISIBLE,
            280, 35, 50, 20, hWnd, (HMENU)IDC_STATIC_POINTS, NULL, NULL);
        hEditPoints = CreateWindowExA(0, "EDIT", "0",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            340, 32, 60, 24, hWnd, (HMENU)IDC_EDIT_POINTS, NULL, NULL);
        // Buttons
        CreateWindowExA(0, "BUTTON", "Add Player",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            430, 32, 100, 24, hWnd, (HMENU)IDC_BTN_ADD, NULL, NULL);
        CreateWindowExA(0, "BUTTON", "Win +1",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            540, 32, 70, 24, hWnd, (HMENU)IDC_BTN_INC_WIN, NULL, NULL);
        CreateWindowExA(0, "BUTTON", "Add Pts",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            620, 32, 70, 24, hWnd, (HMENU)IDC_BTN_ADD_PTS, NULL, NULL);
        CreateWindowExA(0, "BUTTON", "Save CSV",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            430, 70, 100, 24, hWnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
        CreateWindowExA(0, "BUTTON", "Load CSV",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            540, 70, 100, 24, hWnd, (HMENU)IDC_BTN_LOAD, NULL, NULL);
        // ListView
        hListView = CreateWindowExA(0, WC_LISTVIEWA, "",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            10, 130, 750, 430, hWnd, (HMENU)IDC_LISTVIEW, NULL, NULL);
        // Columns
        LVCOLUMNA col = {0}; col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.cx = 300; col.pszText = const_cast<LPSTR>("Name"); ListView_InsertColumn(hListView, 0, &col);
        col.cx = 100; col.pszText = const_cast<LPSTR>("Wins"); ListView_InsertColumn(hListView, 1, &col);
        col.cx = 100; col.pszText = const_cast<LPSTR>("Points"); ListView_InsertColumn(hListView, 2, &col);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_ADD: {
            // Add new player
            int len = GetWindowTextLengthA(hEditName);
            if (len > 0) {
                std::string name(len, '\0');
                GetWindowTextA(hEditName, &name[0], len + 1);
                players.push_back({ name, 0, 0 });
                SetWindowTextA(hEditName, "");
                UpdateListView();
            }
            break;
        }
        case IDC_BTN_INC_WIN: {
            int idx = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (idx >= 0) {
                players[idx].wins++;
                UpdateListView();
            }
            break;
        }
        case IDC_BTN_ADD_PTS: {
            int idx = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (idx >= 0) {
                char buf[16];
                GetWindowTextA(hEditPoints, buf, sizeof(buf));
                int pts = atoi(buf);
                players[idx].points += pts;
                UpdateListView();
            }
            break;
        }
        case IDC_BTN_SAVE:
            SaveCSV("players.csv");
            break;
        case IDC_BTN_LOAD:
            LoadCSV("players.csv");
            UpdateListView();
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "BadmintonTournamentClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    HWND hWnd = CreateWindowExA(0, wc.lpszClassName, "Badminton Tournament Manager",
        WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT, 780, 600,
        NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}