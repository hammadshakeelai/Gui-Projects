#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>

// --- Colour Constants ---------------------------------------------------------
constexpr COLORREF COLOR_MENU_BG        = RGB(32,32,32);
constexpr COLORREF COLOR_PLAY_BG        = RGB(100,140,200);
constexpr COLORREF COLOR_BTN_FILL       = RGB(200,160,100);
constexpr COLORREF COLOR_BTN_TEXT       = RGB(20,20,20);
constexpr COLORREF COLOR_MENU_TITLE_BG  = RGB(200,160,100);
constexpr COLORREF COLOR_MENU_TITLE_TEXT= RGB(32,32,32);
constexpr COLORREF COLOR_PLAY_TITLE_BG  = RGB(50,205,50);
constexpr COLORREF COLOR_PLAY_TITLE_TEXT= RGB(255,255,255);
constexpr COLORREF COLOR_COIN_FILL      = RGB(255,223,0);
constexpr COLORREF COLOR_DINO_ALT       = RGB(180,140,80);
constexpr COLORREF COLOR_OBS_ALT        = RGB(150,120,80);

// --- Global GDI Objects ---------------------------------------------------------
HBRUSH g_hbrMenuBg, g_hbrPlayBg, g_hbrBtn;
HBRUSH g_hbrMenuTitleBg, g_hbrPlayTitleBg;
HBRUSH g_hbrCoin, g_hbrDinoAlt, g_hbrObsAlt;
HFONT  g_hFontTitle;

// --- Game Variables -------------------------------------------------------------
bool inPlay = false, gameOver = false;
int screenW, screenH, groundY;

// Dino sprite (5x5 pixel art)
constexpr int PIXEL = 12;
int dinoSprite[5][5] = {
    {0,1,1,1,0},
    {1,1,1,1,1},
    {1,0,1,0,1},
    {1,1,1,1,1},
    {0,1,1,1,0}
};
int dinoX = 50, dinoY, dinoRows = 5, dinoCols = 5;
int velY = 0;

// Obstacle dimensions and sprite
constexpr int obsRows = 3;
constexpr int obsCols = 3;
int obsSprite[obsRows][obsCols] = {
    {1,1,1},
    {1,0,1},
    {1,1,1}
};

// Coin size
constexpr int COIN_SIZE = PIXEL;

// Obstacle
struct Obstacle { int x, y; bool sky; };
std::vector<Obstacle> obstacles;

// Coins
struct Coin { int x, y; };
std::vector<Coin> coins;

int baseSpeed = 8, speed = baseSpeed;
int score = 0, coinCount = 0;
bool frameToggle = false;

// UI buttons
RECT btnPlay = {300,240,540,300}, btnExit = {300,320,540,380};
RECT btnAgain = {0,0,0,0}, btnBack = {20,20,140,60};
RECT titleMenu = {0,80,0,180}, titlePlay = {0,20,0,100};

// Forward
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SpawnObstacle();
void SpawnCoin();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    std::srand((UINT)time(NULL));
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance   = hInst;
    wc.lpszClassName = "TRexRun";
    wc.hCursor     = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    groundY = screenH - PIXEL*6;
    dinoY   = groundY - PIXEL*dinoRows;

    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, wc.lpszClassName, "T-Rex Run",
        WS_POPUP, 0, 0, screenW, screenH, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Timer and initial spawns
    SetTimer(hwnd, 1, 30, NULL);
    SpawnObstacle(); SpawnObstacle();
    SpawnCoin(); SpawnCoin();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void SpawnObstacle() {
    bool sky = (std::rand() & 1) == 0;
    int y = sky ? groundY - PIXEL*8 : groundY - PIXEL*3;
    obstacles.push_back({screenW + 100, y, sky});
}

void SpawnCoin() {
    int x = screenW + std::rand() % 400;
    int y = groundY - PIXEL*4 - (std::rand() % 3)*PIXEL;
    coins.push_back({x, y});
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // Brushes
        g_hbrMenuBg      = CreateSolidBrush(COLOR_MENU_BG);
        g_hbrPlayBg      = CreateSolidBrush(COLOR_PLAY_BG);
        g_hbrBtn         = CreateSolidBrush(COLOR_BTN_FILL);
        g_hbrMenuTitleBg = CreateSolidBrush(COLOR_MENU_TITLE_BG);
        g_hbrPlayTitleBg = CreateSolidBrush(COLOR_PLAY_TITLE_BG);
        g_hbrCoin        = CreateSolidBrush(COLOR_COIN_FILL);
        g_hbrDinoAlt     = CreateSolidBrush(COLOR_DINO_ALT);
        g_hbrObsAlt      = CreateSolidBrush(COLOR_OBS_ALT);
        g_hFontTitle     = CreateFont(
            PIXEL*3,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
            VARIABLE_PITCH,TEXT("Arial"));
        btnAgain = {screenW/2-120, groundY/2+30, screenW/2+120, groundY/2+90};
        return 0;

    case WM_TIMER:
        if (inPlay && !gameOver) {
            // update score and speed
            score++;
            speed = baseSpeed + score/100;
            // physics
            velY += PIXEL/4;
            dinoY += velY;
            if (dinoY > groundY - PIXEL*dinoRows) { dinoY = groundY - PIXEL*dinoRows; velY = 0; }
            // move obstacles
            for (auto &o : obstacles) o.x -= speed;
            if (!obstacles.empty() && obstacles.front().x < -PIXEL*obsCols) {
                obstacles.erase(obstacles.begin());
                SpawnObstacle();
            }
            // move coins
            for (auto &c : coins) c.x -= speed;
            if (!coins.empty() && coins.front().x < -COIN_SIZE) {
                coins.erase(coins.begin());
                SpawnCoin();
            }
            // collisions
            RECT rd = {dinoX, dinoY, dinoX+PIXEL*dinoCols, dinoY+PIXEL*dinoRows};
            for (auto &c : coins) {
                RECT rc = {c.x, c.y, c.x+COIN_SIZE, c.y+COIN_SIZE};
                if (IntersectRect(&rc, &rd, &rc)) { coinCount++; c.x = -COIN_SIZE; }
            }
            for (auto &o : obstacles) {
                int oy = o.sky ? groundY-PIXEL*8 : groundY-PIXEL*obsRows;
                RECT ro = {o.x, oy, o.x+PIXEL*obsCols, oy+PIXEL*obsRows};
                if (IntersectRect(&ro, &rd, &ro)) gameOver = true;
            }
            frameToggle = !frameToggle;
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_KEYDOWN:
        if (inPlay && !gameOver) {
            if (wParam == VK_SPACE) velY = -PIXEL*4;
            if (wParam == VK_DOWN) dinoRows = 4;
        }
        return 0;
    case WM_KEYUP:
        if (wParam == VK_DOWN) dinoRows = 5;
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        // background
        FillRect(dc, &ps.rcPaint, inPlay ? g_hbrPlayBg : g_hbrMenuBg);
        // title
        RECT rt = inPlay ? titlePlay : titleMenu;
        FillRect(dc, &rt, inPlay ? g_hbrPlayTitleBg : g_hbrMenuTitleBg);
        HFONT oldF = (HFONT)SelectObject(dc, g_hFontTitle);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, inPlay ? COLOR_PLAY_TITLE_TEXT : COLOR_MENU_TITLE_TEXT);
        DrawTextA(dc, "T-Rex Run", -1, &rt, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        SelectObject(dc, oldF);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, COLOR_BTN_TEXT);
        if (!inPlay) {
            FillRect(dc, &btnPlay, g_hbrBtn);
            DrawTextA(dc, "PLAY", -1, &btnPlay, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            FillRect(dc, &btnExit, g_hbrBtn);
            DrawTextA(dc, "EXIT", -1, &btnExit, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        } else {
            // ground line
            HPEN pen = CreatePen(PS_SOLID, 3, COLOR_BTN_TEXT);
            HGDIOBJ oldP = SelectObject(dc, pen);
            MoveToEx(dc, 0, groundY, NULL);
            LineTo(dc, ps.rcPaint.right, groundY);
            SelectObject(dc, oldP);
            DeleteObject(pen);
            // draw dino
            for (int r = 0; r < dinoRows; ++r) {
                for (int c = 0; c < dinoCols; ++c) {
                    if (dinoSprite[r][c]) {
                        RECT pr = { dinoX+c*PIXEL, dinoY+r*PIXEL, dinoX+(c+1)*PIXEL, dinoY+(r+1)*PIXEL };
                        FillRect(dc, &pr, frameToggle ? g_hbrBtn : g_hbrDinoAlt);
                    }
                }
            }
            // draw obstacles
            for (auto &o : obstacles) {
                int oy = o.sky ? groundY-PIXEL*8 : groundY-PIXEL*obsRows;
                for (int r=0; r<obsRows; ++r) for (int c=0; c<obsCols; ++c) {
                    if (obsSprite[r][c]) {
                        RECT pr = { o.x+c*PIXEL, oy+r*PIXEL, o.x+(c+1)*PIXEL, oy+(r+1)*PIXEL };
                        FillRect(dc, &pr, frameToggle ? g_hbrMenuTitleBg : g_hbrObsAlt);
                    }
                }
            }
            // draw coins
            HBRUSH oldB = (HBRUSH)SelectObject(dc, g_hbrCoin);
            for (auto &c : coins) Ellipse(dc, c.x, c.y, c.x+COIN_SIZE, c.y+COIN_SIZE);
            SelectObject(dc, oldB);
            // display scores
            char buf[64];
            wsprintfA(buf, "Score: %d", score);
            TextOutA(dc, 20, 20, buf, lstrlenA(buf));
            wsprintfA(buf, "Coins: %d", coinCount);
            TextOutA(dc, 20, 50, buf, lstrlenA(buf));
            // game over
            if (gameOver) {
                SetTextColor(dc, RGB(255,0,0));
                TextOutA(dc, ps.rcPaint.right/2-60, groundY/2, "GAME OVER", 9);
                FillRect(dc, &btnAgain, g_hbrBtn);
                DrawTextA(dc, "PLAY AGAIN", -1, &btnAgain, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                FillRect(dc, &btnBack, g_hbrBtn);
                DrawTextA(dc, "GO BACK", -1, &btnBack, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        POINT p = {(int)LOWORD(lParam),(int)HIWORD(lParam)};
        if (!inPlay) {
            if (PtInRect(&btnPlay, p)) {
                inPlay = true;
                score = coinCount = 0;
                gameOver = false;
                obstacles.clear(); coins.clear();
                SpawnObstacle(); SpawnObstacle(); SpawnCoin(); SpawnCoin();
            } else if (PtInRect(&btnExit, p)) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
        } else if (gameOver) {
            if (PtInRect(&btnAgain, p)) {
                score = coinCount = 0;
                gameOver = false;
                obstacles.clear(); coins.clear();
                SpawnObstacle(); SpawnObstacle(); SpawnCoin(); SpawnCoin();
            } else if (PtInRect(&btnBack, p)) {
                inPlay = false;
            }
        }
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd,1);
        DeleteObject(g_hbrMenuBg); DeleteObject(g_hbrPlayBg); DeleteObject(g_hbrBtn);
        DeleteObject(g_hbrMenuTitleBg); DeleteObject(g_hbrPlayTitleBg);
        DeleteObject(g_hbrCoin); DeleteObject(g_hbrDinoAlt); DeleteObject(g_hbrObsAlt);
        DeleteObject(g_hFontTitle);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}
