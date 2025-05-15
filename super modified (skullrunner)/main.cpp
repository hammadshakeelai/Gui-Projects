#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>

//---------------------------------------------------------------------------
// T-Rex Runner with Menu and Pixel DINO Sprite (Fixed Entry and WndProc)
//---------------------------------------------------------------------------

// Dimensions
constexpr int DINO_ROWS = 10;
constexpr int DINO_COLS = 10;
constexpr int PIXEL     = 14;
constexpr int DINO_W    = DINO_COLS * PIXEL;
constexpr int DINO_H    = DINO_ROWS * PIXEL;
constexpr int GROUND_H  = PIXEL * 3;
constexpr int COIN_SZ   = PIXEL * 2;
constexpr UINT TIMER_ID = 1;

// Dino sprite
int dinoSprite[DINO_ROWS][DINO_COLS] = {
    {0,0,0,0,0,1,1,1,1,0},
    {0,0,0,0,0,1,1,1,1,0},
    {0,0,0,0,1,1,1,1,1,0},
    {0,0,0,0,1,1,1,0,0,0},
    {1,0,0,0,1,1,1,1,0,0},
    {1,1,0,1,1,1,1,0,0,0},
    {1,1,1,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,0,0,0,0},
    {0,0,1,0,1,0,0,0,0,0},
    {0,0,1,0,1,0,0,0,0,0}
};

// Colors
constexpr COLORREF MENU_BG    = RGB(32,32,32);
constexpr COLORREF GAME_BG    = RGB(248,248,248);
constexpr COLORREF GROUND_COL = RGB(85,85,85);
constexpr COLORREF DINO_COL   = RGB(83,83,83);
constexpr COLORREF OBST_COL   = RGB(52,52,52);
constexpr COLORREF COIN_COL   = RGB(255,215,0);
constexpr COLORREF CLOUD_COL  = RGB(204,204,204);
constexpr COLORREF BTN_BG     = RGB(64,64,64);
constexpr COLORREF BTN_FG     = RGB(200,200,200);
constexpr COLORREF TEXT_COL   = RGB(240,240,240);

// Globals
int screenW, screenH, groundY;
int speed=8, score=0, highScore=0, coinCount=0;
bool inMenu=true, running=false, gameOver=false;

double dinoX=PIXEL*6, dinoY, velY=0;
bool jumping=false;

// Double buffer
static HDC memDC;
static HBITMAP memBmp;

// GDI resources
static HBRUSH brMenu, brGame, brDino, brObs, brCoin, brCloud, brBtn;
static HPEN penGround;
static HFONT hFont;

// Buttons
static RECT btnPlay    = {300,200,580,260};
static RECT btnExit    = {300,300,580,360};
static RECT btnRestart = {300,250,580,310};
static RECT btnExitGame;

// Object struct
struct Obj{ double x,y,h; };
static std::vector<Obj> obstacles, coins, clouds;

// Prototypes
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitGame();
void ResetGame();
void SpawnObstacle();
void SpawnCoin();
void SpawnCloud();
void UpdateState();
void RenderFrame(HWND hwnd);

// Entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(NULL));

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"TRexMenu";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    groundY = screenH - GROUND_H;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName,
        L"T-Rex Runner",
        WS_POPUP,
        0, 0, screenW, screenH,
        NULL, NULL, hInst, NULL
    );
    ShowWindow(hwnd, SW_SHOW);

    HDC hdc = GetDC(hwnd);
    memDC  = CreateCompatibleDC(hdc);
    memBmp = CreateCompatibleBitmap(hdc, screenW, screenH);
    SelectObject(memDC, memBmp);
    ReleaseDC(hwnd, hdc);

    // Create brushes, pens, font
    brMenu    = CreateSolidBrush(MENU_BG);
    brGame    = CreateSolidBrush(GAME_BG);
    brDino    = CreateSolidBrush(DINO_COL);
    brObs     = CreateSolidBrush(OBST_COL);
    brCoin    = CreateSolidBrush(COIN_COL);
    brCloud   = CreateSolidBrush(CLOUD_COL);
    brBtn     = CreateSolidBrush(BTN_BG);
    penGround = CreatePen(PS_SOLID, GROUND_H, GROUND_COL);
    hFont     = CreateFontW(28,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
                  DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                  CLEARTYPE_QUALITY,VARIABLE_PITCH,L"Consolas");

    InitGame();
    timeBeginPeriod(1);
    SetTimer(hwnd, TIMER_ID, 1000/60, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// Initialize or reset game state
void InitGame() {
    obstacles.clear();
    coins.clear();
    clouds.clear();
    ResetGame();
}

void ResetGame() {
    score = coinCount = 0;
    speed = 8;
    dinoY = groundY - DINO_H;
    velY  = 0;
    jumping = false;
    running = !inMenu;
    gameOver = false;

    btnExitGame = { screenW - 100, 0, screenW, 40 };
    SpawnCloud();
    SpawnObstacle();
    SpawnCoin();
}

// Spawners
void SpawnObstacle() {
    double base = PIXEL * (2 + rand() % 3);
    double bonus = PIXEL * (score / 20.0);
    obstacles.push_back({ screenW + rand() % 300, groundY - (base + bonus), base + bonus });
}

void SpawnCoin() {
    coins.push_back({ screenW + rand() % 500, double(rand() % (groundY - COIN_SZ)), 0 });
}

void SpawnCloud() {
    clouds.push_back({ double(rand() % screenW), double(rand() % (groundY / 2)), 0 });
}

// Update physics & state
void UpdateState() {
    velY += 1.0;
    dinoY += velY;
    if (dinoY >= groundY - DINO_H) {
        dinoY = groundY - DINO_H;
        velY  = 0;
        jumping = false;
    }

    for (auto &o : obstacles) o.x -= speed;
    for (auto &c : coins)     c.x -= speed;
    for (auto &c : clouds)    c.x -= speed / 2;

    if (!obstacles.empty() && obstacles.front().x < -PIXEL * 5) {
        obstacles.erase(obstacles.begin());
        SpawnObstacle();
        score++;
        if (score % 10 == 0) speed++;
    }
    if (!coins.empty() && coins.front().x < -COIN_SZ) {
        coins.erase(coins.begin());
        SpawnCoin();
    }
    if (!clouds.empty() && clouds.front().x < -PIXEL * 8) {
        clouds.erase(clouds.begin());
        SpawnCloud();
    }

    RECT dr = { int(dinoX), int(dinoY), int(dinoX + DINO_W), int(dinoY + DINO_H) };
    for (auto &o : obstacles) {
        RECT r = { int(o.x), int(o.y), int(o.x + PIXEL * 3), int(o.y + o.h) };
        if (IntersectRect(&r, &dr, &r)) {
            running  = false;
            gameOver = true;
            highScore = std::max(highScore, score);
        }
    }
    for (auto &c : coins) {
        RECT r = { int(c.x), int(c.y), int(c.x + COIN_SZ), int(c.y + COIN_SZ) };
        if (IntersectRect(&r, &dr, &r)) {
            coinCount++;
            c.x = -COIN_SZ * 2;
        }
    }
}

// Render
void RenderFrame(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    FillRect(memDC, &ps.rcPaint, inMenu ? brMenu : brGame);

    if (inMenu) {
        SelectObject(memDC, hFont);
        SetTextColor(memDC, TEXT_COL);
        SetBkMode(memDC, TRANSPARENT);
        RECT titleRect = { 0, screenH/4 - 50, screenW, screenH/4 + 50 };
        DrawTextW(memDC, L"T-REX RUNNER", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        FillRect(memDC, &btnPlay, brBtn);
        SetTextColor(memDC, BTN_FG);
        DrawTextW(memDC, L"PLAY", -1, &btnPlay, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        FillRect(memDC, &btnExit, brBtn);
        DrawTextW(memDC, L"EXIT", -1, &btnExit, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        SelectObject(memDC, brCloud);
        for (auto &c : clouds) Ellipse(memDC, int(c.x), int(c.y), int(c.x + PIXEL*8), int(c.y + PIXEL*4));

        SelectObject(memDC, penGround);
        MoveToEx(memDC, 0, groundY, NULL);
        LineTo(memDC, screenW, groundY);

        SelectObject(memDC, brObs);
        for (auto &o : obstacles) {
            RECT r = { int(o.x), int(o.y), int(o.x + PIXEL*3), int(o.y + o.h) };
            FillRect(memDC, &r, brObs);
        }

        SelectObject(memDC, brCoin);
        for (auto &c : coins) Ellipse(memDC, int(c.x), int(c.y), int(c.x + COIN_SZ), int(c.y + COIN_SZ));

        SelectObject(memDC, brDino);
        for (int r = 0; r < DINO_ROWS; ++r) {
            for (int c = 0; c < DINO_COLS; ++c) {
                if (dinoSprite[r][c]) {
                    RECT pr = { int(dinoX + c*PIXEL), int(dinoY + r*PIXEL), int(dinoX + (c+1)*PIXEL), int(dinoY + (r+1)*PIXEL) };
                    FillRect(memDC, &pr, brDino);
                }
            }
        }

        SelectObject(memDC, hFont);
        SetTextColor(memDC, RGB(83,83,83));
        SetBkMode(memDC, TRANSPARENT);
        std::wostringstream oss;
        oss << L"Score: " << score << L"  High: " << highScore << L"  Coins: " << coinCount;
        std::wstring ui = oss.str();
        TextOutW(memDC, 20, 20, ui.c_str(), (int)ui.size());

        FillRect(memDC, &btnExitGame, brBtn);
        SetTextColor(memDC, BTN_FG);
        DrawTextW(memDC, L"EXIT", -1, &btnExitGame, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        if (gameOver) {
            FillRect(memDC, &btnRestart, brBtn);
            DrawTextW(memDC, L"RESTART", -1, &btnRestart, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    BitBlt(hdc, 0, 0, screenW, screenH, memDC, 0, 0, SRCCOPY);
    EndPaint(hwnd, &ps);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if (!inMenu) UpdateState();
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (inMenu) {
            if (PtInRect(&btnPlay, pt)) { inMenu = false; ResetGame(); }
            else if (PtInRect(&btnExit, pt)) PostQuitMessage(0);
        } else {
            if (PtInRect(&btnExitGame, pt)) PostQuitMessage(0);
            if (gameOver && PtInRect(&btnRestart, pt)) ResetGame();
        }
        break;
    }
    case WM_KEYDOWN:
        if (!inMenu && wParam == VK_SPACE && !jumping && !gameOver) {
            velY = -PIXEL * 3;
            jumping = true;
        }
        if (inMenu && wParam == VK_RETURN) { inMenu = false; ResetGame(); }
        if (gameOver && wParam == VK_RETURN) ResetGame();
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        break;
    case WM_PAINT:
        RenderFrame(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        timeEndPeriod(1);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        DeleteObject(brMenu);
        DeleteObject(brGame);
        DeleteObject(brDino);
        DeleteObject(brObs);
        DeleteObject(brCoin);
        DeleteObject(brCloud);
        DeleteObject(brBtn);
        DeleteObject(penGround);
        DeleteObject(hFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
