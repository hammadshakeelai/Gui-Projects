#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mmsystem.h> // for timeBeginPeriod/timeEndPeriod
#pragma comment(lib, "winmm.lib")

// --- Colour Constants ---------------------------------------------------------
constexpr COLORREF COLOR_MENU_BG         = RGB(32,32,32);
constexpr COLORREF COLOR_PLAY_BG         = RGB(100,140,200);
constexpr COLORREF COLOR_BTN_FILL        = RGB(200,160,100);
constexpr COLORREF COLOR_BTN_TEXT        = RGB(20,20,20);
constexpr COLORREF COLOR_MENU_TITLE_BG   = RGB(200,160,100);
constexpr COLORREF COLOR_MENU_TITLE_TEXT = RGB(32,32,32);
constexpr COLORREF COLOR_PLAY_TITLE_BG   = RGB(50,205,50);
constexpr COLORREF COLOR_PLAY_TITLE_TEXT = RGB(255,255,255);
constexpr COLORREF COLOR_COIN_FILL       = RGB(255,223,0);
constexpr COLORREF COLOR_DINO_ALT        = RGB(180,140,80);
constexpr COLORREF COLOR_OBS_ALT         = RGB(150,120,80);

// --- Global GDI Objects ---------------------------------------------------------
HBRUSH g_hbrMenuBg, g_hbrPlayBg, g_hbrBtn;
HBRUSH g_hbrMenuTitleBg, g_hbrPlayTitleBg;
HBRUSH g_hbrCoin, g_hbrDinoAlt, g_hbrObsAlt;
HFONT  g_hFontTitle;

// --- Game Variables -------------------------------------------------------------
bool gameRunning = false;
bool gameOver     = false;
int  screenW, screenH;

// Dino sprite (5x5 pixel art)
constexpr int PIXEL = 12;
int dinoSprite[5][5] = {
    {0,1,1,1,0},
    {1,1,1,1,1},
    {1,0,1,0,1},
    {1,1,1,1,1},
    {0,1,1,1,0}
};
int dinoX = 50;
int dinoRows = 5, dinoCols = 5;
int dinoY, velY = 0;

// Obstacle dimensions and sprite
enum { obsRows = 3, obsCols = 3 };
int obsSprite[obsRows][obsCols] = {
    {1,1,1}, {1,0,1}, {1,1,1}
};
struct Obstacle { int x, y; bool sky; };
std::vector<Obstacle> obstacles;

// Coin parameters
constexpr int COIN_SIZE = PIXEL;
struct Coin { int x, y; };
std::vector<Coin> coins;

int baseSpeed = 8, speed = baseSpeed;
int score = 0, coinCount = 0;
bool frameToggle = false;
int groundY;

// UI buttons
RECT btnPlay      = {300,240,540,300};
RECT btnExit      = {300,320,540,380};
RECT btnAgain     = {0,0,0,0};
RECT btnBack      = {20,20,140,60};
RECT titleMenu    = {0,80,screenW,180};
RECT titlePlay    = {0,20,screenW,100};

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SpawnObstacle();
void SpawnCoin();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(NULL));
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "SkullRunner";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    groundY = screenH - PIXEL*6;
    dinoY   = groundY - PIXEL*dinoRows;
    titleMenu.right = screenW;
    titlePlay.right = screenW;

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST, wc.lpszClassName, "Skull Runner",
        WS_POPUP, 0, 0, screenW, screenH,
        NULL, NULL, hInst, NULL
    );
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    timeBeginPeriod(1);
    SetTimer(hwnd, 1, 16, NULL); // roughly 60 FPS

    // initial spawns
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
    bool sky = rand() & 1;
    int y = sky ? groundY - PIXEL*8 : groundY - PIXEL*obsRows;
    obstacles.push_back({screenW + rand()%200, y, sky});
}

void SpawnCoin() {
    int x = screenW + rand()%400;
    int y = groundY - PIXEL*4 - (rand()%3)*PIXEL;
    coins.push_back({x,y});
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
        g_hFontTitle     = CreateFont(PIXEL*3,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
                               DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Arial"));
        btnAgain = {screenW/2-120, groundY/2+30, screenW/2+120, groundY/2+90};
        return 0;

    case WM_TIMER:
        if (gameRunning && !gameOver) {
            score++;
            speed = baseSpeed + score/100;
            velY += PIXEL/4;
            dinoY += velY;
            if (dinoY > groundY-PIXEL*dinoRows) { dinoY=groundY-PIXEL*dinoRows; velY=0; }

            // update obstacles
            for (auto &o: obstacles) o.x -= speed;
            if (!obstacles.empty() && obstacles.front().x < -PIXEL*obsCols) {
                obstacles.erase(obstacles.begin()); SpawnObstacle(); }

            // update coins
            for (auto &c: coins) c.x -= speed;
            if (!coins.empty() && coins.front().x < -COIN_SIZE) {
                coins.erase(coins.begin()); SpawnCoin(); }

            RECT rd = {dinoX,dinoY,dinoX+PIXEL*dinoCols,dinoY+PIXEL*dinoRows};
            for (auto &c: coins) {
                RECT rc={c.x,c.y,c.x+COIN_SIZE,c.y+COIN_SIZE};
                if (IntersectRect(&rc,&rd,&rc)) { coinCount++; c.x=-COIN_SIZE; }
            }
            for (auto &o: obstacles) {
                int oy = o.sky? groundY-PIXEL*8: groundY-PIXEL*obsRows;
                RECT ro={o.x,oy,o.x+PIXEL*obsCols,oy+PIXEL*obsRows};
                if (IntersectRect(&ro,&rd,&ro)) gameOver=true;
            }
            frameToggle = !frameToggle;
        }
        InvalidateRect(hwnd,NULL,TRUE);
        return 0;

    case WM_KEYDOWN:
        if (gameRunning && !gameOver) {
            if (wParam==VK_SPACE) velY=-PIXEL*4;
            if (wParam==VK_DOWN)  dinoRows=4;
        }
        return 0;
    case WM_KEYUP:
        if (wParam==VK_DOWN) dinoRows=5;
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps);
        FillRect(dc,&ps.rcPaint, gameRunning?g_hbrPlayBg:g_hbrMenuBg);
        RECT rt = gameRunning? titlePlay: titleMenu;
        FillRect(dc,&rt, gameRunning? g_hbrPlayTitleBg: g_hbrMenuTitleBg);
        HFONT oldF=(HFONT)SelectObject(dc,g_hFontTitle);
        SetBkMode(dc,TRANSPARENT); SetTextColor(dc, gameRunning?COLOR_PLAY_TITLE_TEXT:COLOR_MENU_TITLE_TEXT);
        DrawTextA(dc,"Skull Runner",-1,&rt,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        SelectObject(dc,oldF);
        SetBkMode(dc,TRANSPARENT); SetTextColor(dc,COLOR_BTN_TEXT);
        if(!gameRunning){
            FillRect(dc,&btnPlay,g_hbrBtn); DrawTextA(dc,"PLAY",-1,&btnPlay,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            FillRect(dc,&btnExit,g_hbrBtn); DrawTextA(dc,"EXIT",-1,&btnExit,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        } else {
            HPEN pen=CreatePen(PS_SOLID,3,COLOR_BTN_TEXT);HGDIOBJ op=SelectObject(dc,pen);
            MoveToEx(dc,0,groundY,NULL);LineTo(dc,ps.rcPaint.right,groundY);
            SelectObject(dc,op);DeleteObject(pen);
            for(int r=0;r<dinoRows;r++)for(int c=0;c<dinoCols;c++)if(dinoSprite[r][c]){
                RECT pr={dinoX+c*PIXEL,dinoY+r*PIXEL,dinoX+(c+1)*PIXEL,dinoY+(r+1)*PIXEL};
                FillRect(dc,&pr,frameToggle?g_hbrBtn:g_hbrDinoAlt);
            }
            for(auto &o:obstacles){int oy=o.sky?groundY-PIXEL*8:groundY-PIXEL*obsRows;
                for(int r=0;r<obsRows;r++)for(int c=0;c<obsCols;c++)if(obsSprite[r][c]){
                    RECT pr={o.x+c*PIXEL,oy+r*PIXEL,o.x+(c+1)*PIXEL,oy+(r+1)*PIXEL};
                    FillRect(dc,&pr,frameToggle?g_hbrMenuTitleBg:g_hbrObsAlt);
                }
            }
            HBRUSH ob=(HBRUSH)SelectObject(dc,g_hbrCoin);
            for(auto &c:coins)Ellipse(dc,c.x,c.y,c.x+COIN_SIZE,c.y+COIN_SIZE);
            SelectObject(dc,ob);
            char buf[64];wsprintfA(buf,"Score:%d",score);TextOutA(dc,20,20,buf,lstrlenA(buf));
            wsprintfA(buf,"Coins:%d",coinCount);TextOutA(dc,20,50,buf,lstrlenA(buf));
            if(gameOver){SetTextColor(dc,RGB(255,0,0));TextOutA(dc,ps.rcPaint.right/2-60,groundY/2,"GAME OVER",9);
                FillRect(dc,&btnAgain,g_hbrBtn);DrawTextA(dc,"PLAY AGAIN",-1,&btnAgain,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                FillRect(dc,&btnBack,g_hbrBtn);DrawTextA(dc,"GO BACK",-1,&btnBack,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            }
        }
        EndPaint(hwnd,&ps);return 0;
    }
    case WM_LBUTTONDOWN:{POINT p={(int)LOWORD(lParam),(int)HIWORD(lParam)};
        if(!gameRunning){if(PtInRect(&btnPlay,p)){gameRunning=true;score=coinCount=0;gameOver=false;obstacles.clear();coins.clear();SpawnObstacle();SpawnObstacle();SpawnCoin();SpawnCoin();}
            else if(PtInRect(&btnExit,p))PostMessage(hwnd,WM_CLOSE,0,0);
        } else if(gameOver){if(PtInRect(&btnAgain,p)){score=coinCount=0;gameOver=false;obstacles.clear();coins.clear();SpawnObstacle();SpawnObstacle();SpawnCoin();SpawnCoin();}
            else if(PtInRect(&btnBack,p))gameRunning=false;}
        return 0;
    }
    case WM_DESTROY:KillTimer(hwnd,1);timeEndPeriod(1);
        DeleteObject(g_hbrMenuBg);DeleteObject(g_hbrPlayBg);DeleteObject(g_hbrBtn);
        DeleteObject(g_hbrMenuTitleBg);DeleteObject(g_hbrPlayTitleBg);
        DeleteObject(g_hbrCoin);DeleteObject(g_hbrDinoAlt);DeleteObject(g_hbrObsAlt);
        DeleteObject(g_hFontTitle);PostQuitMessage(0);return 0;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}
