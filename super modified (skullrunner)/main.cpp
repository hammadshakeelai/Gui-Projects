#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Color definitions
constexpr COLORREF BG_COLOR       = RGB(32,32,32);
constexpr COLORREF GROUND_COLOR   = RGB(200,200,200);
constexpr COLORREF DINO_COLOR     = RGB(180,140,80);
constexpr COLORREF OBST_COLOR     = RGB(150,120,80);
constexpr COLORREF COIN_COLOR     = RGB(255,223,0);
constexpr COLORREF CLOUD_COLOR    = RGB(255,255,255);
constexpr COLORREF BTN_COLOR      = RGB(200,160,100);
constexpr COLORREF TEXT_COLOR     = RGB(255,255,255);

// Screen
int screenW, screenH, groundY;

// Double buffer
HDC memDC;
HBITMAP memBitmap;

// GDI brushes and font
HBRUSH hbrBG, hbrDino, hbrObs, hbrCoin, hbrCloud, hbrBtn;
HFONT hFont;

// Dino
constexpr int PIXEL = 14;
int dinoX = 80, dinoY = 0, velY = 0;
constexpr int DINO_ROWS = 5, DINO_COLS = 5;
int dinoSprite[DINO_ROWS][DINO_COLS] = {
    {0,1,1,1,0},
    {1,1,1,1,1},
    {1,0,1,0,1},
    {1,1,1,1,1},
    {0,1,1,1,0}
};

// Obstacles
struct Obstacle { int x, y, height; };
std::vector<Obstacle> obstacles;

// Coins
struct Coin { int x, y; };
std::vector<Coin> coins;

// Clouds
struct Cloud { int x, y; };
std::vector<Cloud> clouds;

// Game state
bool gameRunning = false;
bool gameOver = false;
int score = 0, coinCount = 0;
int baseSpeed = 6, speed = baseSpeed;

// UI buttons
RECT btnPlay   = {300,240,540,300};
RECT btnExit   = {300,320,540,380};
RECT btnRetry  = {0,0,0,0};
RECT btnBack   = {20,20,140,60};
RECT btnExitGame;

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SpawnObstacle();
void SpawnCoin();
void SpawnCloud();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(NULL));
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW|CS_VREDRAW, WndProc,
                      0, 0, hInst, NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL,
                      "FlappySkull", NULL };
    RegisterClassEx(&wc);

    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    groundY = screenH - PIXEL * 3;
    dinoY   = groundY - PIXEL * DINO_ROWS;
    btnExitGame = { screenW-140, 20, screenW-20, 60 };
    btnRetry = { screenW/2-150, groundY/2-30, screenW/2+150, groundY/2+30 };

    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, wc.lpszClassName, "Skull Runner",
        WS_POPUP, 0, 0, screenW, screenH, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, SW_SHOW);

    HDC hdc = GetDC(hwnd);
    memDC = CreateCompatibleDC(hdc);
    memBitmap = CreateCompatibleBitmap(hdc, screenW, screenH);
    SelectObject(memDC, memBitmap);
    ReleaseDC(hwnd, hdc);

    // Create brushes and font
    hbrBG    = CreateSolidBrush(BG_COLOR);
    hbrDino  = CreateSolidBrush(DINO_COLOR);
    hbrObs   = CreateSolidBrush(OBST_COLOR);
    hbrCoin  = CreateSolidBrush(COIN_COLOR);
    hbrCloud = CreateSolidBrush(CLOUD_COLOR);
    hbrBtn   = CreateSolidBrush(BTN_COLOR);
    hFont    = CreateFont(32,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
                          DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                          VARIABLE_PITCH,TEXT("Consolas"));

    // Initial spawn
    SpawnCloud(); SpawnCloud(); SpawnCloud();
    SpawnObstacle(); SpawnCoin();

    // High-res timer
    timeBeginPeriod(1);
    SetTimer(hwnd, 1, 16, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void SpawnObstacle() {
    int h = PIXEL * (2 + rand() % 3);
    obstacles.push_back({ screenW, groundY - h, h });
}

void SpawnCoin() {
    coins.push_back({ screenW, rand() % (groundY - 3*PIXEL) });
}

void SpawnCloud() {
    clouds.push_back({ rand() % screenW, rand() % (groundY/2) });
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if (gameRunning && !gameOver) {
            // Dino physics
            velY += 1;
            dinoY += velY;
            if (dinoY < 0) { dinoY = 0; velY = 0; }
            if (dinoY > groundY - PIXEL * DINO_ROWS) gameOver = true;
            // Move clouds
            for (auto &c : clouds) { if ((c.x -= speed/4) < -PIXEL*3) c.x = screenW; }
            // Move obstacles and coins
            for (auto &o : obstacles) o.x -= speed;
            for (auto &c : coins) c.x -= speed;
            // Spawn new
            if (!obstacles.empty() && obstacles.front().x < -PIXEL*3) { obstacles.erase(obstacles.begin()); SpawnObstacle(); score++; }
            if (!coins.empty() && coins.front().x < -COIN_SIZE) { coins.erase(coins.begin()); SpawnCoin(); }
            // Collisions
            RECT drect = { dinoX, dinoY, dinoX+PIXEL*DINO_COLS, dinoY+PIXEL*DINO_ROWS };
            for (auto &c : coins) {
                RECT crect = { c.x, c.y, c.x+COIN_SIZE, c.y+COIN_SIZE };
                if (IntersectRect(&crect, &drect, &crect)) { coinCount++; c.x = -COIN_SIZE; }
            }
            for (auto &o : obstacles) {
                RECT orect = { o.x, o.y, o.x+PIXEL*3, o.y+o.height };
                if (IntersectRect(&orect, &drect, &orect)) gameOver = true;
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_SPACE) {
            if (!gameRunning) gameRunning = true;
            else if (!gameOver) velY = -PIXEL*4;
        }
        break;
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        if (!gameRunning) {
            if (PtInRect(&btnPlay, pt)) gameRunning = true;
            else if (PtInRect(&btnExit, pt)) PostQuitMessage(0);
        } else if (gameOver) {
            if (PtInRect(&btnRetry, pt)) {
                // Reset
                gameOver = false; score = coinCount = velY = 0;
                obstacles.clear(); coins.clear(); clouds.clear();
                SpawnCloud(); SpawnObstacle(); SpawnCoin();
            } else if (PtInRect(&btnBack, pt)) {
                gameRunning = false;
            }
        } else if (PtInRect(&btnExitGame, pt)) {
            PostQuitMessage(0);
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        // Draw to memDC
        FillRect(memDC, &ps.rcPaint, hbrBG);
        // Clouds
        SelectObject(memDC, hbrCloud);
        for (auto &c : clouds) Ellipse(memDC, c.x, c.y, c.x+PIXEL*3, c.y+PIXEL);
        // Ground line
        HPEN hPen = CreatePen(PS_SOLID, 2, GROUND_COLOR);
        HGDIOBJ oldPen = SelectObject(memDC, hPen);
        MoveToEx(memDC, 0, groundY, NULL); LineTo(memDC, screenW, groundY);
        SelectObject(memDC, oldPen); DeleteObject(hPen);
        // Dino
        SelectObject(memDC, hbrDino);
        for (int r=0; r<DINO_ROWS; ++r)
            for (int c=0; c<DINO_COLS; ++c)
                if (dinoSprite[r][c]) {
                    RECT pr = { dinoX+c*PIXEL, dinoY+r*PIXEL, dinoX+(c+1)*PIXEL, dinoY+(r+1)*PIXEL };
                    FillRect(memDC, &pr, hbrDino);
                }
        // Obstacles
        SelectObject(memDC, hbrObs);
        for (auto &o : obstacles) {
            for (int r=0; r<o.height/PIXEL; ++r) {
                RECT pr = { o.x, o.y+r*PIXEL, o.x+PIXEL*3, o.y+(r+1)*PIXEL };
                FillRect(memDC, &pr, hbrObs);
            }
        }
        // Coins
        SelectObject(memDC, hbrCoin);
        for (auto &c : coins) Ellipse(memDC, c.x, c.y, c.x+COIN_SIZE, c.y+COIN_SIZE);
        // UI Text
        SelectObject(memDC, hFont);
        SetTextColor(memDC, TEXT_COLOR); SetBkMode(memDC, TRANSPARENT);
        char buf[64];
        wsprintfA(buf, "Score: %d", score);
        TextOutA(memDC, 20, 20, buf, lstrlenA(buf));
        wsprintfA(buf, "Coins: %d", coinCount);
        TextOutA(memDC, 20, 60, buf, lstrlenA(buf));
        // Buttons
        SelectObject(memDC, hbrBtn);
        if (!gameRunning) {
            FillRect(memDC, &btnPlay, hbrBtn);
            DrawTextA(memDC, "PLAY", -1, &btnPlay, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            FillRect(memDC, &btnExit, hbrBtn);
            DrawTextA(memDC, "EXIT", -1, &btnExit, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        } else if (!gameOver) {
            FillRect(memDC, &btnExitGame, hbrBtn);
            DrawTextA(memDC, "EXIT", -1, &btnExitGame, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        } else {
            FillRect(memDC, &btnRetry, hbrBtn);
            DrawTextA(memDC, "RETRY", -1, &btnRetry, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            FillRect(memDC, &btnBack, hbrBtn);
            DrawTextA(memDC, "BACK", -1, &btnBack, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }
        // Blit
        BitBlt(hdc, 0, 0, screenW, screenH, memDC, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        timeEndPeriod(1);
        DeleteDC(memDC);
        DeleteObject(memBitmap);
        DeleteObject(hbrBG); DeleteObject(hbrDino); DeleteObject(hbrObs);
        DeleteObject(hbrCoin); DeleteObject(hbrCloud); DeleteObject(hbrBtn);
        DeleteObject(hFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
