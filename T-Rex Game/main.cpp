// Write the T-Rex Win32 C++ game code to a downloadable file.
// code = r'''// T‑Rex (Chrome Dino‑style) — Pure Win32 API, single file
// ------------------------------------------------------------------
// Features
//  - Smooth 60 FPS loop with double‑buffered GDI rendering
//  - Jump (SPACE/UP/Left‑Click), Duck (DOWN)
//  - Procedural obstacles: small/large/double cacti, low/high birds, boulders
//  - Difficulty ramps with speed + spawn rate; parallax clouds
//  - Score + persistent High Score (trex_highscore.dat)
//  - Run log (trex_runs.log with timestamp)
//  - Top‑5 leaderboard persistence (trex_top5.txt)
//  - R = restart, ESC = quit
// Build: Win32 GUI app, link Gdi32 (Visual Studio or MinGW). No assets needed.
// ------------------------------------------------------------------

#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cmath>

// ----------------------- Config -----------------------
static const int   W_WIDTH   = 900;
static const int   W_HEIGHT  = 360;
static const int   GROUND_H  = 64;     // ground strip height
static const int   FPS       = 60;     // target frames per second
static const float DT        = 1.0f / FPS;
static const float GRAVITY   = 2200.0f;  // px/s^2
static const float JUMP_VEL  = 760.0f;   // px/s
static const float BASE_SPD  = 360.0f;   // world scroll speed px/s

// Colors (BGR)
static const COLORREF COL_BG      = RGB(245, 245, 245);
static const COLORREF COL_GROUND  = RGB(60, 60, 60);
static const COLORREF COL_DINO    = RGB(48, 48, 48);
static const COLORREF COL_OBS     = RGB(30, 30, 30);
static const COLORREF COL_BIRD    = RGB(10, 10, 10);
static const COLORREF COL_TEXT    = RGB(10, 10, 10);
static const COLORREF COL_UI      = RGB(0, 120, 215);

// ----------------------- Types ------------------------
enum class GameState { MENU, PLAYING, GAMEOVER };
enum class ObType    { CactusSmall, CactusLarge, CactusDouble, BirdLow, BirdHigh, Boulder };

struct RectF { float x, y, w, h; };
static inline bool Intersect(const RectF&a, const RectF&b){
    return !(a.x + a.w < b.x || b.x + b.w < a.x || a.y + a.h < b.y || b.y + b.h < a.y);
}

struct Obstacle {
    ObType type{};
    float x{}, y{}, w{}, h{}, speed{};
    int anim{0}; // for birds
};

struct Cloud { float x, y, speed; };

struct Dino {
    float x{}, y{};     // top‑left position
    float vy{};         // vertical velocity
    bool onGround{true};
    bool duck{false};
    int  blink{0};      // hit flash

    RectF bbox() const {
        float bw = 44.0f;
        float bh = duck ? 28.0f : 48.0f;
        return RectF{ x, y + (duck ? 20.0f : 0.0f), bw, bh };
    }
};

// -------------------- Globals ------------------------
HINSTANCE   g_hInst;
HWND        g_hWnd;
HDC         g_hMemDC;
HBITMAP     g_hBmp;
HBITMAP     g_hBmpOld;

GameState   g_state = GameState::MENU;
Dino        g_dino;
std::vector<Obstacle> g_obs;
std::vector<Cloud>    g_clouds;
std::mt19937          g_rng{ std::random_device{}() };

float g_worldSpd = BASE_SPD;
float g_spawnTimer = 0.0f;
float g_spawnGapMin = 0.85f, g_spawnGapMax = 1.85f; // seconds
float g_nextSpawnIn = 1.0f;

int   g_score = 0;          // integer score (meters)
int   g_highScore = 0;
std::vector<int> g_top5;

bool  g_leftMouseDown = false;
LARGE_INTEGER g_freq, g_prev;

// ---------------- Persistence ------------------------
int LoadHighScore(){
    std::ifstream f("trex_highscore.dat");
    int hs=0; if(f) f>>hs; return hs;
}

void SaveHighScore(int hs){
    std::ofstream f("trex_highscore.dat", std::ios::trunc);
    if(f) f<<hs;
}

std::vector<int> LoadTop5(){
    std::vector<int> v; std::ifstream f("trex_top5.txt");
    int s; while(f>>s) v.push_back(s);
    std::sort(v.begin(), v.end(), std::greater<int>());
    if(v.size()>5) v.resize(5);
    return v;
}

void SaveTop5(const std::vector<int>& v){
    std::ofstream f("trex_top5.txt", std::ios::trunc);
    for(size_t i=0;i<v.size() && i<5;i++) f<<v[i]<<"\n";
}

void AppendRunLog(int score){
    std::ofstream f("trex_runs.log", std::ios::app);
    if(!f) return;
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    tm = *std::localtime(&t);
#endif
    char buf[64]; std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    f << buf << ", " << score << "\n";
}

// ---------------- Utilities --------------------------
float frand(float a, float b){ std::uniform_real_distribution<float> d(a,b); return d(g_rng);} 
int   irand(int a,int b){ std::uniform_int_distribution<int> d(a,b); return d(g_rng);} 

void ResetGame(){
    g_obs.clear();
    g_clouds.clear();

    // Dino position
    float groundY = (float)(W_HEIGHT - GROUND_H);
    g_dino.x = 80.0f; g_dino.y = groundY - 52.0f; // standing height baseline
    g_dino.vy = 0.0f; g_dino.onGround = true; g_dino.duck=false; g_dino.blink=0;

    // Clouds
    for(int i=0;i<6;i++){
        g_clouds.push_back({ frand(0, (float)W_WIDTH), frand(30, 120.0f), frand(10.0f, 24.0f) });
    }

    g_worldSpd = BASE_SPD;
    g_spawnGapMin = 0.85f; g_spawnGapMax = 1.85f;
    g_spawnTimer = 0.0f; g_nextSpawnIn = frand(g_spawnGapMin, g_spawnGapMax);
    g_score = 0;
}

// Obstacle factory
Obstacle MakeObstacle(){
    float groundY = (float)(W_HEIGHT - GROUND_H);
    ObType t;
    // Weighted selection; birds unlock at higher scores
    int roll = irand(0, 99);
    if(g_score < 200){
        // early: mostly cacti
        if(roll < 50) t = ObType::CactusSmall;
        else if(roll < 85) t = ObType::CactusLarge;
        else t = ObType::CactusDouble;
    } else {
        if(roll < 30) t = ObType::CactusSmall;
        else if(roll < 55) t = ObType::CactusLarge;
        else if(roll < 70) t = ObType::CactusDouble;
        else if(roll < 85) t = ObType::BirdLow;
        else if(roll < 95) t = ObType::BirdHigh;
        else t = ObType::Boulder;
    }

    Obstacle o; o.type = t; o.speed = g_worldSpd; o.anim=0; o.x = (float)W_WIDTH + frand(0, 40);
    switch(t){
        case ObType::CactusSmall:
            o.w=22; o.h=42; o.y = groundY - o.h; break;
        case ObType::CactusLarge:
            o.w=34; o.h=72; o.y = groundY - o.h; break;
        case ObType::CactusDouble:
            o.w=52; o.h=46; o.y = groundY - o.h; break;
        case ObType::BirdLow:
            o.w=44; o.h=26; o.y = groundY - 24.0f - o.h; break; // knee height
        case ObType::BirdHigh:
            o.w=44; o.h=26; o.y = groundY - 88.0f - o.h; break; // head height
        case ObType::Boulder:
            o.w=32; o.h=32; o.y = groundY - o.h; o.speed = g_worldSpd * 1.18f; break;
    }
    return o;
}

// ----------------- Rendering helpers -----------------
void FillRectF(HDC dc, const RectF&r, COLORREF c){
    HBRUSH br = CreateSolidBrush(c);
    RECT rr{ (int)r.x, (int)r.y, (int)(r.x+r.w), (int)(r.y+r.h) };
    FillRect(dc, &rr, br);
    DeleteObject(br);
}

void DrawGround(HDC dc){
    RECT r{0, W_HEIGHT - GROUND_H, W_WIDTH, W_HEIGHT};
    HBRUSH br=CreateSolidBrush(COL_GROUND);
    FillRect(dc,&r,br); DeleteObject(br);
}

void DrawDino(HDC dc){
    RectF b = g_dino.bbox();
    // Body
    FillRectF(dc, RectF{ b.x, b.y, b.w, b.h }, COL_DINO);
    // Head/neck simple shape when standing
    if(!g_dino.duck){
        FillRectF(dc, RectF{ b.x + b.w - 10, b.y - 16, 14, 16 }, COL_DINO);
        // eye
        RECT e{ (int)(b.x + b.w - 4), (int)(b.y - 10), (int)(b.x + b.w - 1), (int)(b.y - 7) };
        HBRUSH br = CreateSolidBrush(RGB(255,255,255)); FillRect(dc,&e,br); DeleteObject(br);
    }
    if(g_dino.blink>0){ // hit flash overlay
        HBRUSH br=CreateSolidBrush(RGB(255,0,0));
        RECT rr{ (int)b.x-2,(int)b.y-2,(int)(b.x+b.w+2),(int)(b.y+b.h+2)};
        FrameRect(dc,&rr,br); DeleteObject(br);
    }
}

void DrawCactus(HDC dc, const Obstacle&o){ FillRectF(dc, RectF{o.x,o.y,o.w,o.h}, COL_OBS); }

void DrawBird(HDC dc, const Obstacle&o){
    // body
    FillRectF(dc, RectF{o.x, o.y + 6, o.w, o.h - 12}, COL_BIRD);
    // wings animate up/down
    int wing = (o.anim / 8) % 2; // toggle every few frames
    if(wing==0){ // wings up
        RECT w1{ (int)o.x, (int)o.y, (int)(o.x+o.w/2), (int)(o.y+6)};
        RECT w2{ (int)(o.x+o.w/2), (int)o.y, (int)(o.x+o.w), (int)(o.y+6)};
        HBRUSH br=CreateSolidBrush(COL_BIRD); FillRect(dc,&w1,br); FillRect(dc,&w2,br); DeleteObject(br);
    } else { // wings down
        RECT w1{ (int)o.x, (int)(o.y+o.h-6), (int)(o.x+o.w/2), (int)(o.y+o.h)};
        RECT w2{ (int)(o.x+o.w/2), (int)(o.y+o.h-6), (int)(o.x+o.w), (int)(o.y+o.h)};
        HBRUSH br=CreateSolidBrush(COL_BIRD); FillRect(dc,&w1,br); FillRect(dc,&w2,br); DeleteObject(br);
    }
}

void DrawBoulder(HDC dc, const Obstacle&o){
    // Approximate circle with rectangle + frame to stand out
    FillRectF(dc, RectF{o.x, o.y, o.w, o.h}, COL_OBS);
    HBRUSH br=CreateSolidBrush(RGB(200,200,200));
    RECT r{ (int)o.x, (int)o.y, (int)(o.x+o.w), (int)(o.y+o.h) }; FrameRect(dc,&r,br); DeleteObject(br);
}

void DrawCloud(HDC dc, const Cloud&c){
    HBRUSH br = CreateSolidBrush(RGB(200,200,200));
    RECT r1{ (int)c.x, (int)c.y, (int)(c.x+38), (int)(c.y+18) };
    RECT r2{ (int)(c.x+16), (int)(c.y-8), (int)(c.x+56), (int)(c.y+12) };
    RECT r3{ (int)(c.x+30), (int)(c.y+4), (int)(c.x+76), (int)(c.y+22) };
    FillRect(dc,&r1,br); FillRect(dc,&r2,br); FillRect(dc,&r3,br); DeleteObject(br);
}

void DrawTextSimple(HDC dc, int x, int y, const std::wstring& s, COLORREF col=COL_TEXT, int size=18, bool bold=false){
    LOGFONTW lf{}; lf.lfHeight = -size; lf.lfWeight = bold? FW_SEMIBOLD:FW_NORMAL; wcscpy_s(lf.lfFaceName, L"Segoe UI");
    HFONT f=CreateFontIndirectW(&lf); HFONT old=(HFONT)SelectObject(dc,f);
    SetTextColor(dc, col); SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, x, y, s.c_str(), (int)s.size());
    SelectObject(dc,old); DeleteObject(f);
}

// --------------- Game update & logic -----------------
void SpawnIfNeeded(float dt){
    g_spawnTimer += dt;
    if(g_spawnTimer >= g_nextSpawnIn){
        g_spawnTimer = 0.0f;
        g_nextSpawnIn = frand(g_spawnGapMin, g_spawnGapMax);
        // Prevent unfair overlaps: ensure last obstacle is far enough
        if(g_obs.empty() || (W_WIDTH - g_obs.back().x) > 40.0f){
            g_obs.push_back(MakeObstacle());
        }
    }
}

void UpdateGame(float dt){
    float groundY = (float)(W_HEIGHT - GROUND_H);

    // Difficulty ramp: every 100 score, speed increases, spawn gap shrinks
    float tSpeed = BASE_SPD + std::min(280.0f, (float)g_score * 0.8f);
    g_worldSpd = tSpeed;
    g_spawnGapMin = std::max(0.55f, 0.85f - (float)g_score * 0.0009f);
    g_spawnGapMax = std::max(0.95f, 1.85f - (float)g_score * 0.0009f);

    // Dino physics
    if(!g_dino.onGround){
        g_dino.vy += GRAVITY * dt;
        g_dino.y  += g_dino.vy * dt;
        if(g_dino.y >= groundY - 52.0f){
            g_dino.y = groundY - 52.0f; g_dino.vy = 0.0f; g_dino.onGround = true;
        }
    }

    // Clouds (parallax)
    for(auto &c: g_clouds){
        c.x -= c.speed * dt;
        if(c.x < -80) { c.x = (float)W_WIDTH + frand(0, 140); c.y = frand(30, 130); c.speed = frand(10.0f, 24.0f);}    }

    // Obstacles move & animate
    for(auto &o: g_obs){
        float s = (o.type==ObType::Boulder) ? o.speed : g_worldSpd;
        o.x -= s * dt;
        if(o.type==ObType::BirdLow || o.type==ObType::BirdHigh) o.anim++;
    }
    // remove offscreen
    g_obs.erase(std::remove_if(g_obs.begin(), g_obs.end(), [](const Obstacle&o){return o.x + o.w < -8; }), g_obs.end());

    // Spawn new ones
    SpawnIfNeeded(dt);

    // Score
    g_score += (int)std::round(40.0f * dt); // tweak rate

    // Collision
    RectF dbox = g_dino.bbox();
    for(const auto &o: g_obs){
        RectF obox{ o.x, o.y, o.w, o.h };
        if(Intersect(dbox, obox)){
            g_dino.blink = 14; // flash frames
            g_state = GameState::GAMEOVER;
            // persist scores
            if(g_score > g_highScore){ g_highScore = g_score; SaveHighScore(g_highScore); }
            // update top 5
            g_top5.push_back(g_score);
            std::sort(g_top5.begin(), g_top5.end(), std::greater<int>());
            if(g_top5.size()>5) g_top5.resize(5);
            SaveTop5(g_top5);
            AppendRunLog(g_score);
            break;
        }
    }

    if(g_dino.blink>0) g_dino.blink--;
}

// ---------------------- Paint ------------------------
void EnsureBackbuffer(){
    if(!g_hMemDC){
        HDC hdc = GetDC(g_hWnd);
        g_hMemDC = CreateCompatibleDC(hdc);
        g_hBmp = CreateCompatibleBitmap(hdc, W_WIDTH, W_HEIGHT);
        g_hBmpOld = (HBITMAP)SelectObject(g_hMemDC, g_hBmp);
        ReleaseDC(g_hWnd, hdc);
    }
}

void ReleaseBackbuffer(){
    if(g_hMemDC){
        SelectObject(g_hMemDC, g_hBmpOld);
        DeleteObject(g_hBmp); DeleteDC(g_hMemDC);
        g_hBmp = nullptr; g_hMemDC=nullptr; g_hBmpOld=nullptr;
    }
}

void Render(){
    EnsureBackbuffer();

    HDC dc = g_hMemDC;
    HBRUSH bg=CreateSolidBrush(COL_BG);
    RECT full{0,0,W_WIDTH,W_HEIGHT}; FillRect(dc,&full,bg); DeleteObject(bg);

    // Clouds
    for(const auto &c: g_clouds) DrawCloud(dc, c);

    // Ground
    DrawGround(dc);

    // Obstacles
    for(const auto &o: g_obs){
        switch(o.type){
            case ObType::CactusSmall:
            case ObType::CactusLarge:
            case ObType::CactusDouble: DrawCactus(dc,o); break;
            case ObType::BirdLow:
            case ObType::BirdHigh:     DrawBird(dc,o);   break;
            case ObType::Boulder:      DrawBoulder(dc,o);break;
        }
    }

    // Dino
    DrawDino(dc);

    // UI text
    std::wstringstream ss; ss<<L"Score: "<<g_score<<L"    High: "<<g_highScore;
    DrawTextSimple(dc, W_WIDTH-300, 14, ss.str(), COL_UI, 18, true);

    if(g_state==GameState::MENU){
        DrawTextSimple(dc, 26, 18, L"T‑Rex — Win32 Edition", COL_TEXT, 28, true);
        DrawTextSimple(dc, 26, 52, L"SPACE/UP or Left‑Click: Jump    DOWN: Duck    R: Restart", COL_TEXT, 18, false);
        DrawTextSimple(dc, 26, 78, L"Press SPACE to start", RGB(0,0,0), 22, true);

        DrawTextSimple(dc, 26, 118, L"Top 5:", COL_TEXT, 18, true);
        for(size_t i=0;i<g_top5.size();++i){
            std::wstringstream s2; s2<< (i+1) << L". "<< g_top5[i];
            DrawTextSimple(dc, 26, 140+(int)i*20, s2.str(), COL_TEXT, 18, false);
        }
    }
    else if(g_state==GameState::GAMEOVER){
        DrawTextSimple(dc, 26, 18, L"Game Over", RGB(200,0,0), 30, true);
        DrawTextSimple(dc, 26, 54, L"Press R to retry", COL_TEXT, 20, false);
        std::wstringstream s3; s3<<L"Run: "<<g_score<<L"    High: "<<g_highScore; 
        DrawTextSimple(dc, 26, 82, s3.str(), COL_TEXT, 20, true);

        DrawTextSimple(dc, 26, 118, L"Top 5:", COL_TEXT, 18, true);
        for(size_t i=0;i<g_top5.size();++i){
            std::wstringstream s2; s2<< (i+1) << L". "<< g_top5[i];
            DrawTextSimple(dc, 26, 140+(int)i*20, s2.str(), COL_TEXT, 18, false);
        }
        // subtle hint if new high
        if(g_score==g_highScore){ DrawTextSimple(dc, 26, 140+(int)g_top5.size()*20 + 8, L"NEW HIGH SCORE!", COL_UI, 20, true);}        
    }

    // Blit to screen
    HDC hdc = GetDC(g_hWnd);
    BitBlt(hdc, 0,0, W_WIDTH,W_HEIGHT, dc, 0,0, SRCCOPY);
    ReleaseDC(g_hWnd, hdc);
}

// -------------------- Input --------------------------
void DoJump(){
    if(g_state==GameState::MENU){ g_state=GameState::PLAYING; }
    if(g_state==GameState::PLAYING && g_dino.onGround){
        g_dino.onGround=false; g_dino.vy = -JUMP_VEL;
    }
}

void SetDuck(bool down){
    if(g_state==GameState::PLAYING){ g_dino.duck = down && g_dino.onGround; }
}

// -------------------- Window proc --------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
    case WM_CREATE:
        QueryPerformanceFrequency(&g_freq); QueryPerformanceCounter(&g_prev);
        SetTimer(hWnd, 1, 1000/FPS, NULL);
        g_highScore = LoadHighScore();
        g_top5 = LoadTop5();
        ResetGame();
        return 0;
    case WM_TIMER: {
        LARGE_INTEGER now; QueryPerformanceCounter(&now);
        double dt = double(now.QuadPart - g_prev.QuadPart) / double(g_freq.QuadPart);
        g_prev = now;
        // clamp dt to avoid huge jumps
        if(dt > 0.08) dt = 0.08;
        // fixed‑step: accumulate in steps of DT
        static double acc=0.0; acc += dt;
        while(acc >= DT){
            if(g_state==GameState::PLAYING) UpdateGame((float)DT);
            acc -= DT;
        }
        Render();
        return 0; }
    case WM_LBUTTONDOWN: DoJump(); return 0;
    case WM_KEYDOWN:
        if(wParam==VK_SPACE || wParam==VK_UP) DoJump();
        else if(wParam==VK_DOWN) SetDuck(true);
        else if(wParam=='R') { g_state=GameState::PLAYING; ResetGame(); }
        else if(wParam==VK_ESCAPE) DestroyWindow(hWnd);
        return 0;
    case WM_KEYUP:
        if(wParam==VK_DOWN) SetDuck(false);
        return 0;
    case WM_SIZE:
        Render();
        return 0;
    case WM_DESTROY:
        KillTimer(hWnd,1);
        ReleaseBackbuffer();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// --------------------- WinMain -----------------------
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int){
    g_hInst = hInst;

    WNDCLASSW wc{}; wc.style = CS_HREDRAW|CS_VREDRAW; wc.lpfnWndProc=WndProc; wc.hInstance=hInst;
    wc.hCursor=LoadCursor(NULL, IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); wc.lpszClassName=L"TRexWin32";
    RegisterClassW(&wc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r{0,0,W_WIDTH,W_HEIGHT}; AdjustWindowRect(&r, style, FALSE);
    g_hWnd = CreateWindowW(L"TRexWin32", L"T‑Rex — Win32 (C++/GDI)", style,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top,
                           NULL, NULL, hInst, NULL);
    ShowWindow(g_hWnd, SW_SHOW); UpdateWindow(g_hWnd);

    // message loop
    MSG msg; while(GetMessageW(&msg, NULL, 0,0)) { TranslateMessage(&msg); DispatchMessageW(&msg);}    
    return (int)msg.wParam;
}