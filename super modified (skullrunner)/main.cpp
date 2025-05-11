#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// --- Colour Constants ---------------------------------------------------------
constexpr COLORREF COLOR_BG            = RGB(32,32,32);
constexpr COLORREF COLOR_GROUND_LINES  = RGB(200,200,200);
constexpr COLORREF COLOR_DINO          = RGB(180,140,80);
constexpr COLORREF COLOR_OBS           = RGB(150,120,80);
constexpr COLORREF COLOR_COIN          = RGB(255,223,0);
constexpr COLORREF COLOR_CLOUD         = RGB(255,255,255);
constexpr COLORREF COLOR_BTN_FILL      = RGB(200,160,100);
constexpr COLORREF COLOR_TEXT          = RGB(255,255,255);

// --- Globals -------------------------------------------------------------------
HBRUSH  hbrBackground, hbrDino, hbrObs, hbrCoin, hbrCloud, hbrBtn;
HFONT   hFont;
HDC     memDC;
HBITMAP memBitmap;

int     screenW, screenH, groundY;
bool    gameRunning = false;
bool    gameOver    = false;

// Dino sprite (5x5 pixel art)
constexpr int PIXEL = 14;
int dinoX = 80;
int dinoY;
int dinoRows = 5, dinoCols = 5;
int velY = 0;
int gravity = 1;
int dinoSprite[5][5] = {
    {0,1,1,1,0},
    {1,1,1,1,1},
    {1,0,1,0,1},
    {1,1,1,1,1},
    {0,1,1,1,0}
};

// Obstacles
constexpr int obsRows = 3;
constexpr int obsCols = 3;
int obsSprite[obsRows][obsCols] = {
    {1,1,1},
    {1,0,1},
    {1,1,1}
};
struct Obstacle { int x, y, rows; bool sky; };
std::vector<Obstacle> obstacles;

// Coins
constexpr int COIN_SIZE = PIXEL;
struct Coin { int x, y; };
std::vector<Coin> coins;

// Clouds
struct Cloud { int x, y; };
std::vector<Cloud> clouds;

// Score & speed
int score = 0;
int coinCount = 0;
int baseSpeed = 6;
int speed = baseSpeed;

// UI buttons
RECT btnPlay   = {300,240,540,300};
RECT btnExit   = {300,320,540,380};
RECT btnAgain  = {0,0,0,0};
RECT btnBack   = {20,20,140,60};

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SpawnObstacle();
void SpawnCoin();
void SpawnCloud();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(nullptr));
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "FlappySkull";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    groundY = screenH - PIXEL * 3;
    dinoY   = groundY - PIXEL * dinoRows;

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST, wc.lpszClassName, "Skull Runner",
        WS_POPUP, 0, 0, screenW, screenH,
        NULL, NULL, hInst, NULL
    );
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Setup double buffer
    HDC hdc = GetDC(hwnd);
    memDC = CreateCompatibleDC(hdc);
    memBitmap = CreateCompatibleBitmap(hdc, screenW, screenH);
    SelectObject(memDC, memBitmap);
    ReleaseDC(hwnd, hdc);

    // High-res timer for ~60 FPS
    timeBeginPeriod(1);
    SetTimer(hwnd, 1, 16, NULL);

    // Initial spawns
    SpawnCloud(); SpawnCloud(); SpawnCloud();
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
    int rows = 3 + rand() % 4; // vary height
    bool sky = rand() & 1;
    int y = sky ? groundY - rows * PIXEL - PIXEL*2 : groundY - rows * PIXEL;
    obstacles.push_back({screenW + rand()%200, y, rows, sky});
}

void SpawnCoin() {
    int x = screenW + rand()%400;
    int y = groundY - PIXEL*4 - (rand()%6)*PIXEL;
    coins.push_back({x, y});
}

void SpawnCloud() {
    int x = rand()%screenW;
    int y = rand()%(groundY/2);
    clouds.push_back({x, y});
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hbrBackground = CreateSolidBrush(COLOR_BG);
        hbrDino       = CreateSolidBrush(COLOR_DINO);
        hbrObs        = CreateSolidBrush(COLOR_OBS);
        hbrCoin       = CreateSolidBrush(COLOR_COIN);
        hbrCloud      = CreateSolidBrush(COLOR_CLOUD);
        hbrBtn        = CreateSolidBrush(COLOR_BTN_FILL);
        hFont         = CreateFont(32,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
                          DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Consolas"));
        btnAgain      = {screenW/2-150, groundY/2-30, screenW/2+150, groundY/2+30};
        return 0;
    }
    case WM_TIMER:
        if (gameRunning && !gameOver) {
            // physics
            velY += gravity;
            dinoY += velY;
            if (dinoY < 0) { dinoY = 0; velY = 0; }
            if (dinoY > groundY - PIXEL*dinoRows) { gameOver = true; }
            // move clouds
            for (auto &c: clouds) {
                c.x -= speed/4;
                if (c.x < -PIXEL*3) c.x = screenW;
            }
            // move obstacles
            for (auto &o: obstacles) o.x -= speed;
            if (!obstacles.empty() && obstacles.front().x < -PIXEL*obsCols) {
                obstacles.erase(obstacles.begin()); SpawnObstacle(); score++; }
            // move coins
            for (auto &c: coins) c.x -= speed;
            if (!coins.empty() && coins.front().x < -COIN_SIZE) {
                coins.erase(coins.begin()); SpawnCoin(); coinCount--; }
            // collisions
            RECT rd={dinoX,dinoY,dinoX+PIXEL*dinoCols,dinoY+PIXEL*dinoRows};
            for (auto &c: coins) {
                RECT rc={c.x,c.y,c.x+COIN_SIZE,c.y+COIN_SIZE};
                if (IntersectRect(&rc,&rd,&rc)) { coinCount++; c.x=-COIN_SIZE; }
            }
            for (auto &o: obstacles) {
                int oy=o.y;
                RECT ro={o.x,oy,o.x+PIXEL*obsCols,oy+PIXEL*o.rows};
                if (IntersectRect(&ro,&rd,&ro)) gameOver=true;
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_KEYDOWN:
        if (!gameRunning && wParam==VK_SPACE) { gameRunning=true; }
        else if (gameRunning && !gameOver && wParam==VK_SPACE) {
            velY = -PIXEL*4;
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc=BeginPaint(hwnd,&ps);
        // draw off-screen
        FillRect(memDC,&ps.rcPaint,hbrBackground);
        // clouds
        SelectObject(memDC,hbrCloud);
        for (auto &c:clouds) Ellipse(memDC,c.x,c.y,c.x+PIXEL*3,c.y+PIXEL);
        // ground line
        HPEN pen=CreatePen(PS_SOLID,2,COLOR_GROUND_LINES);
        HGDIOBJ oldP=SelectObject(memDC,pen);
        MoveToEx(memDC,0,groundY,NULL); LineTo(memDC,screenW,groundY);
        SelectObject(memDC,oldP); DeleteObject(pen);
        // dino
        SelectObject(memDC,hbrDino);
        for(int r=0;r<dinoRows;r++)for(int c=0;c<dinoCols;c++) if(dinoSprite[r][c]){
            RECT pr={dinoX+c*PIXEL,dinoY+r*PIXEL,dinoX+(c+1)*PIXEL,dinoY+(r+1)*PIXEL};
            FillRect(memDC,&pr,hbrDino);
        }
        // obstacles
        SelectObject(memDC,hbrObs);
        for(auto &o:obstacles){int oy=o.y;
            for(int r=0;r<o.rows;r++)for(int c=0;c<obsCols;c++) if(obsSprite[r][c]){
                RECT pr={o.x+c*PIXEL,oy+r*PIXEL,o.x+(c+1)*PIXEL,oy+(r+1)*PIXEL};
                FillRect(memDC,&pr,hbrObs);
            }
        }
        // coins
        SelectObject(memDC,hbrCoin);
        for(auto &c:coins) Ellipse(memDC,c.x,c.y,c.x+COIN_SIZE,c.y+COIN_SIZE);
        // text
        SelectObject(memDC,hFont);
        SetTextColor(memDC,COLOR_TEXT);
        SetBkMode(memDC,TRANSPARENT);
        char buf[64];
        wsprintfA(buf,"Score: %d",score);
        TextOutA(memDC,20,20,buf,strlen(buf));
        wsprintfA(buf,"Coins: %d",coinCount);
        TextOutA(memDC,20,60,buf,strlen(buf));
        // game over UI
        if(gameOver){
            SetTextColor(memDC,RGB(255,0,0));
            TextOutA(memDC,screenW/2-100,groundY/2,"GAME OVER",9);
            SelectObject(memDC,hbrBtn);
            FillRect(memDC,&btnAgain,hbrBtn); DrawTextA(memDC,"PLAY AGAIN",-1,&btnAgain,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            FillRect(memDC,&btnBack,hbrBtn); DrawTextA(memDC,"GO BACK",-1,&btnBack,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }
        // blit
        BitBlt(hdc,0,0,screenW,screenH,memDC,0,0,SRCCOPY);
        EndPaint(hwnd,&ps);
        return 0;
    }
    case WM_LBUTTONDOWN:{
        POINT p={(int)LOWORD(lParam),(int)HIWORD(lParam)};
        if(!gameRunning){if(PtInRect(&btnPlay,p)) gameRunning=true;
            else if(PtInRect(&btnExit,p)) PostMessage(hwnd,WM_CLOSE,0,0);
        } else if(gameOver){
            if(PtInRect(&btnAgain,p)){
                gameOver=false; score=coinCount=0; velY=0;
                obstacles.clear(); coins.clear(); clouds.clear();
                SpawnCloud();SpawnCloud();SpawnCloud();SpawnObstacle();SpawnObstacle();SpawnCoin();SpawnCoin();
            } else if(PtInRect(&btnBack,p)){
                gameRunning=false;
            }
        }
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd,1);
        timeEndPeriod(1);
        DeleteDC(memDC);
        DeleteObject(memBitmap);
        DeleteObject(hbrBackground);DeleteObject(hbrDino);DeleteObject(hbrObs);
        DeleteObject(hbrCoin);DeleteObject(hbrCloud);DeleteObject(hbrBtn);
        DeleteObject(hFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}
