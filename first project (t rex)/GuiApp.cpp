#include <windows.h>
#include <cstdlib>
#include <ctime>

// ─── Colour Constants ─────────────────────────────────────────────────────────
constexpr COLORREF COLOR_MENU_BG         = RGB(32, 32, 32);
constexpr COLORREF COLOR_PLAY_BG         = RGB(100, 140, 200);
constexpr COLORREF COLOR_BTN_FILL        = RGB(200, 160, 100);
constexpr COLORREF COLOR_BTN_TEXT        = RGB(20, 20, 20);
constexpr COLORREF COLOR_MENU_TITLE_BG   = RGB(200, 160, 100);
constexpr COLORREF COLOR_MENU_TITLE_TEXT = RGB(32, 32, 32);
constexpr COLORREF COLOR_PLAY_TITLE_BG   = RGB(50, 205, 50);
constexpr COLORREF COLOR_PLAY_TITLE_TEXT = RGB(255, 255, 255);
constexpr COLORREF COLOR_COIN_FILL       = RGB(255, 223, 0);
constexpr COLORREF COLOR_DINO_ALT        = RGB(180, 140, 80);
constexpr COLORREF COLOR_OBS_ALT         = RGB(150, 120, 80);

// ─── Global GDI Objects ─────────────────────────────────────────────────────────
HBRUSH g_hbrMenuBg, g_hbrPlayBg, g_hbrBtn;
HBRUSH g_hbrMenuTitleBg, g_hbrPlayTitleBg;
HBRUSH g_hbrCoin, g_hbrDinoAlt, g_hbrObsAlt;
HFONT  g_hFontTitle;

// ─── UI & Game Variables ────────────────────────────────────────────────────────
bool inPlayScreen = false;
bool gameOver     = false;
int  screenW, screenH;

// T-Rex "pixel art" parameters
constexpr int PIXEL_SIZE = 12;
int dinoSprite[4][5] = {
    {0,1,1,1,0},
    {1,1,1,1,1},
    {1,1,1,1,0},
    {0,1,1,0,0}
};
int dinoRows = 4, dinoCols = 5;
int dinoX = 50;
int groundY;
int dinoY;
int velY = 0;

// Obstacle pixel art
constexpr int obsRows = 3;
constexpr int obsCols = 3;
int obsSprite[obsRows][obsCols] = {
    {1,0,1},
    {1,1,1},
    {1,0,1}
};
int obsX = 800;

// Coin parameters
constexpr int COIN_SIZE = PIXEL_SIZE;
int coinX, coinY;

int baseSpeed = 8, speed = baseSpeed;
int score = 0;
int coinCount = 0;
bool dinoFrame = false;

// Rectangles
RECT btnPlay      = {300, 240, 540, 300};
RECT btnExit      = {300, 320, 540, 380};
RECT btnGoBack    = {20, 20, 140, 60};
RECT btnPlayAgain = {0, 0, 0, 0}; // set in WM_CREATE
RECT rectTitleMenu = {0, 70, 0, 160};
RECT rectTitlePlay = {0, 20, 0, 80};

// Forward declaration
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    std::srand((unsigned)std::time(nullptr));
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "TrexRunClass";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    rectTitleMenu.right = screenW;
    rectTitlePlay.right = screenW;
    groundY = screenH - PIXEL_SIZE * 6;
    dinoY   = groundY - dinoRows * PIXEL_SIZE;

    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST, wc.lpszClassName, "T-Rex Run",
        WS_POPUP, 0, 0, screenW, screenH,
        NULL, NULL, hInst, NULL
    );
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    SetTimer(hWnd, 1, 30, NULL);
    coinX = screenW + std::rand() % 400;
    coinY = groundY - PIXEL_SIZE * 3;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hbrMenuBg      = CreateSolidBrush(COLOR_MENU_BG);
        g_hbrPlayBg      = CreateSolidBrush(COLOR_PLAY_BG);
        g_hbrBtn         = CreateSolidBrush(COLOR_BTN_FILL);
        g_hbrMenuTitleBg = CreateSolidBrush(COLOR_MENU_TITLE_BG);
        g_hbrPlayTitleBg = CreateSolidBrush(COLOR_PLAY_TITLE_BG);
        g_hbrCoin        = CreateSolidBrush(COLOR_COIN_FILL);
        g_hbrDinoAlt     = CreateSolidBrush(COLOR_DINO_ALT);
        g_hbrObsAlt      = CreateSolidBrush(COLOR_OBS_ALT);
        g_hFontTitle     = CreateFont(
            PIXEL_SIZE * 4, 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            VARIABLE_PITCH, TEXT("Arial")
        );
        btnPlayAgain.left   = screenW / 2 - 120;
        btnPlayAgain.top    = groundY / 2 + 30;
        btnPlayAgain.right  = screenW / 2 + 120;
        btnPlayAgain.bottom = groundY / 2 + 90;
        return 0;

    case WM_TIMER:
        if (inPlayScreen && !gameOver) {
            score++;
            velY += PIXEL_SIZE / 6;
            dinoY += velY;
            if (dinoY > groundY - dinoRows * PIXEL_SIZE) {
                dinoY = groundY - dinoRows * PIXEL_SIZE;
                velY = 0;
            }
            obsX -= speed;
            if (obsX < -obsCols * PIXEL_SIZE) obsX = screenW;
            coinX -= speed;
            if (coinX < -COIN_SIZE) coinX = screenW + std::rand() % 400;
            RECT rd = { dinoX, dinoY, dinoX + PIXEL_SIZE * dinoCols, dinoY + PIXEL_SIZE * dinoRows };
            RECT rc = { coinX, coinY, coinX + COIN_SIZE, coinY + COIN_SIZE };
            if (IntersectRect(&rc, &rd, &rc)) {
                coinCount++;
                coinX = screenW + std::rand() % 400;
            }
            speed = baseSpeed + score / 200;
            dinoFrame = !dinoFrame;
            RECT ro = { obsX, groundY - obsRows * PIXEL_SIZE, obsX + obsCols * PIXEL_SIZE, groundY };
            if (IntersectRect(&ro, &rd, &ro)) gameOver = true;
        }
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;

    case WM_KEYDOWN:
        if (inPlayScreen && !gameOver) {
            if (wParam == VK_SPACE) { velY = -PIXEL_SIZE * 3; }
            if (wParam == VK_DOWN)  { dinoRows = 2; }
        }
        return 0;

    case WM_KEYUP:
        if (wParam == VK_DOWN) { dinoRows = 4; }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hWnd, &ps);

        // Background
        FillRect(dc, &ps.rcPaint, inPlayScreen ? g_hbrPlayBg : g_hbrMenuBg);

        // Title
        RECT rt = inPlayScreen ? rectTitlePlay : rectTitleMenu;
        FillRect(dc, &rt, inPlayScreen ? g_hbrPlayTitleBg : g_hbrMenuTitleBg);
        HFONT oldF = (HFONT)SelectObject(dc, g_hFontTitle);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, inPlayScreen ? COLOR_PLAY_TITLE_TEXT : COLOR_MENU_TITLE_TEXT);
        DrawTextA(dc, "T-Rex Run", -1, &rt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, oldF);

        // UI or Game
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, COLOR_BTN_TEXT);
        if (!inPlayScreen) {
            FillRect(dc, &btnPlay, g_hbrBtn);
            DrawTextA(dc, "PLAY", -1, &btnPlay, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            FillRect(dc, &btnExit, g_hbrBtn);
            DrawTextA(dc, "EXIT", -1, &btnExit, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            // Ground line
            HPEN pen = CreatePen(PS_SOLID, 3, COLOR_BTN_TEXT);
            HGDIOBJ oldP = SelectObject(dc, pen);
            MoveToEx(dc, 0, groundY, NULL);
            LineTo(dc, ps.rcPaint.right, groundY);
            SelectObject(dc, oldP);
            DeleteObject(pen);

            // Dino
            for (int r = 0; r < dinoRows; r++) {
                for (int c = 0; c < dinoCols; c++) {
                    if (dinoSprite[r][c]) {
                        RECT pr = {
                            dinoX + c * PIXEL_SIZE,
                            dinoY + r * PIXEL_SIZE,
                            dinoX + (c + 1) * PIXEL_SIZE,
                            dinoY + (r + 1) * PIXEL_SIZE
                        };
                        FillRect(dc, &pr, dinoFrame ? g_hbrBtn : g_hbrDinoAlt);
                    }
                }
            }

            // Obstacle
            for (int r = 0; r < obsRows; r++) {
                for (int c = 0; c < obsCols; c++) {
                    if (obsSprite[r][c]) {
                        RECT pr = {
                            obsX + c * PIXEL_SIZE,
                            groundY - (r + 1) * PIXEL_SIZE,
                            obsX + (c + 1) * PIXEL_SIZE,
                            groundY - r * PIXEL_SIZE
                        };
                        FillRect(dc, &pr, dinoFrame ? g_hbrMenuTitleBg : g_hbrObsAlt);
                    }
                }
            }

            // Coin
            HBRUSH oldB = (HBRUSH)SelectObject(dc, g_hbrCoin);
            Ellipse(dc, coinX, coinY, coinX + COIN_SIZE, coinY + COIN_SIZE);
            SelectObject(dc, oldB);

            // Counters
            char buf[64];
            wsprintfA(buf, "Score: %d", score);
            TextOutA(dc, 20, 20, buf, lstrlenA(buf));
            wsprintfA(buf, "Coins: %d", coinCount);
            TextOutA(dc, 20, 50, buf, lstrlenA(buf));

            // Game Over
            if (gameOver) {
                SetTextColor(dc, RGB(255, 0, 0));
                TextOutA(dc, ps.rcPaint.right / 2 - 60, groundY / 2, "GAME OVER", 9);
                FillRect(dc, &btnPlayAgain, g_hbrBtn);
                DrawTextA(dc, "PLAY AGAIN", -1, &btnPlayAgain, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                FillRect(dc, &btnGoBack, g_hbrBtn);
                DrawTextA(dc, "GO BACK", -1, &btnGoBack, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT p = { (int)LOWORD(lParam), (int)HIWORD(lParam) };
        if (!inPlayScreen) {
            if (PtInRect(&btnPlay, p)) {
                inPlayScreen = true;
                score = coinCount = 0;
                gameOver = false;
                obsX = coinX = screenW;
            } else if (PtInRect(&btnExit, p)) {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
        } else if (gameOver) {
            if (PtInRect(&btnPlayAgain, p)) {
                score = coinCount = 0;
                gameOver = false;
                obsX = coinX = screenW;
            } else if (PtInRect(&btnGoBack, p)) {
                inPlayScreen = false;
            }
        }
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        DeleteObject(g_hbrMenuBg); DeleteObject(g_hbrPlayBg); DeleteObject(g_hbrBtn);
        DeleteObject(g_hbrMenuTitleBg); DeleteObject(g_hbrPlayTitleBg);
        DeleteObject(g_hbrCoin); DeleteObject(g_hbrDinoAlt); DeleteObject(g_hbrObsAlt);
        DeleteObject(g_hFontTitle);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
