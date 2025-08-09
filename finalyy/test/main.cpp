// Flappy Bird in pure Win32 API (Dev-C++ compatible)
// --------------------------------------------------
// Single-file project. No external libs. Double-buffered GDI rendering.
// Controls: SPACE / Left Click = flap, R = restart, ESC = quit.
// Tip: In Dev-C++, create a new Win32 GUI project, replace main file with this.
// Linker: -lgdi32 (Dev-C++ usually adds it automatically for Win32 GUI).

#define UNICODE
#define _UNICODE
#define NOMINMAX


#include <windows.h>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <algorithm> // <-- add this
#include <fstream>   // for high score file I/O


// --------------------- Config ---------------------
static const int   W_WIDTH      = 480;
static const int   W_HEIGHT     = 640;
static const int   GROUND_H     = 80;      // ground strip height
static const int   FPS          = 60;      // target frames per second
static const float DT           = 1.0f / FPS; // fixed dt

// Bird physics
static const float GRAVITY      = 2150.0f; // px/s^2
static const float FLAP_IMPULSE = -540.0f; // px/s (instant velocity set/add)
static const int   BIRD_R       = 15;      // radius

// Pipe settings
static const int   PIPE_W       = 70;
static const int   PIPE_GAP     = 170;     // vertical gap size
static const int   PIPE_SPACING = 210;     // horizontal distance between pipes
static const int   PIPE_SPEED   = 265;     // px/s leftward

// Colors (nice, saturated palette)
#define COL_SKY   RGB(135, 206, 235)   // light sky blue
#define COL_GROUND RGB(222, 184, 135)  // burlywood
#define COL_PIPE  RGB(0, 170, 85)      // green
#define COL_PIPE_DK RGB(0, 120, 60)    // dark edge
#define COL_BIRD  RGB(255, 204, 0)     // yellow
#define COL_BEAK  RGB(255, 102, 0)     // orange
#define COL_EYE   RGB(30, 30, 30)
#define COL_TEXT  RGB(20, 20, 20)

// --------------------- State ----------------------
struct Pipe {
    int x;     // left
    int gapY;  // gap center Y
};

struct GameState {
    bool running = true;     // app running
    bool alive   = true;     // bird alive
    bool started = false;    // has game started (first flap)?

    // bird
    float birdY  = W_HEIGHT/2.0f; // center y
    float birdV  = 0.0f;          // vertical velocity

    // pipes
    std::vector<Pipe> pipes;

    // timers & score
    float spawnTimer = 0.0f; // time since last spawn (s)
    int   score      = 0;
    int   bestScore  = 0;
};

static GameState g;

// -------- High score persistence --------
static const char* HIGHSCORE_FILE = "highscore.txt";

static void SaveBestScore() {
    std::ofstream out(HIGHSCORE_FILE, std::ios::trunc);
    if (out) out << g.bestScore;
}

static void LoadBestScore() {
    std::ifstream in(HIGHSCORE_FILE);
    if (in) in >> g.bestScore; // defaults to 0 if file missing
}

// --------------- Utility / Drawing ---------------
int irand(int a, int b) { return a + (std::rand() % (b - a + 1)); }

void DrawCircle(HDC dc, int cx, int cy, int r, COLORREF fill, COLORREF outline) {
    HBRUSH hBrush = CreateSolidBrush(fill);
    HBRUSH oldB = (HBRUSH)SelectObject(dc, hBrush);
    HPEN hPen = CreatePen(PS_SOLID, 2, outline);
    HPEN oldP = (HPEN)SelectObject(dc, hPen);
    Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(dc, oldB); DeleteObject(hBrush);
    SelectObject(dc, oldP); DeleteObject(hPen);
}

void FillRectColor(HDC dc, int x, int y, int w, int h, COLORREF c) {
    RECT rc{ x, y, x + w, y + h };
    HBRUSH br = CreateSolidBrush(c);
    FillRect(dc, &rc, br);
    DeleteObject(br);
}

void DrawTextMid(HDC dc, int cx, int cy, const std::wstring& text, int height, bool bold=false) {
    HFONT font = CreateFont(
        height, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT old = (HFONT)SelectObject(dc, font);
    SIZE sz{}; GetTextExtentPoint32(dc, text.c_str(), (int)text.size(), &sz);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, COL_TEXT);
    TextOut(dc, cx - sz.cx/2, cy - sz.cy/2, text.c_str(), (int)text.size());
    SelectObject(dc, old); DeleteObject(font);
}

// ------------------- Game Logic -------------------
void ResetGame() {
    g.alive   = true;
    g.started = false;
    g.birdY   = W_HEIGHT / 2.0f;
    g.birdV   = 0.0f;
    g.score   = 0;
    g.pipes.clear();
    g.spawnTimer = 0.0f;
}

void SpawnPipe() {
    // Keep gap away from top and ground
    int topMargin = 60;
    int bottomMargin = GROUND_H + 60;
    int minY = topMargin + PIPE_GAP/2;
    int maxY = W_HEIGHT - bottomMargin - PIPE_GAP/2;
    Pipe p; p.x = W_WIDTH + 10; p.gapY = irand(minY, maxY);
    g.pipes.push_back(p);
}

void Flap() {
    if (!g.alive) return;
    g.birdV = FLAP_IMPULSE; // instant upward kick
    g.started = true;
}

static void OnDeath() {
    g.alive = false;
    if (g.score > g.bestScore) g.bestScore = g.score;
    SaveBestScore(); // write best score after every death
}


bool BirdHitsRect(int left, int top, int right, int bottom) {
    // Circle-rect collision (approx by clamping)
    float cx = W_WIDTH * 0.38f; // bird x
    float cy = g.birdY;

    float closestX = (float)std::max(left,  std::min((int)cx, right));
    float closestY = (float)std::max(top,   std::min((int)cy, bottom));

    float dx = cx - closestX;
    float dy = cy - closestY;
    return (dx*dx + dy*dy) <= (BIRD_R*BIRD_R);
}


void Update(float dt) {
    if (!g.alive) return; // freeze game when dead

    if (g.started) {
        // bird physics
        g.birdV += GRAVITY * dt;
        g.birdY += g.birdV * dt;

        // pipes movement and spawn
        for (auto &p : g.pipes) p.x -= (int)(PIPE_SPEED * dt);
        g.spawnTimer += dt;
        if (g.pipes.empty() || (W_WIDTH - g.pipes.back().x) >= PIPE_SPACING) {
            SpawnPipe();
            g.spawnTimer = 0.0f;
        }

        // remove off-screen pipes & update score
        for (size_t i=0; i<g.pipes.size(); ) {
            if (g.pipes[i].x + PIPE_W < 0) {
                g.pipes.erase(g.pipes.begin() + i);
                continue;
            }
            // score when passing bird x
            int birdX = (int)(W_WIDTH*0.35f);
            if (g.pipes[i].x + PIPE_W == birdX) {
                g.score++;
                if (g.score > g.bestScore) g.bestScore = g.score;
            }
            ++i;
        }

        // collisions
        int groundTop = W_HEIGHT - GROUND_H;
        if (g.birdY + BIRD_R >= groundTop || g.birdY - BIRD_R <= 0) {
		    OnDeath(); return;
		}

        for (auto &p : g.pipes) {
            int gapTop = p.gapY - PIPE_GAP/2;
            int gapBot = p.gapY + PIPE_GAP/2;
            // upper pipe rect
            if (BirdHitsRect(p.x, 0, p.x + PIPE_W, gapTop)) { OnDeath(); return; }

            // lower pipe rect
            if (BirdHitsRect(p.x, gapBot, p.x + PIPE_W, groundTop)) { OnDeath(); return; }
        }
    }
}

// -------------------- Rendering -------------------
void RenderScene(HDC dc) {
    // back buffer
    HDC memDC = CreateCompatibleDC(dc);
    HBITMAP bmp = CreateCompatibleBitmap(dc, W_WIDTH, W_HEIGHT);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

    // background sky
    FillRectColor(memDC, 0, 0, W_WIDTH, W_HEIGHT, COL_SKY);

    // decorative clouds
    auto cloud = [&](int x, int y) {
        DrawCircle(memDC, x, y, 18, RGB(255,255,255), RGB(255,255,255));
        DrawCircle(memDC, x+20, y+5, 22, RGB(255,255,255), RGB(255,255,255));
        DrawCircle(memDC, x-20, y+6, 15, RGB(255,255,255), RGB(255,255,255));
        DrawCircle(memDC, x+40, y+2, 14, RGB(255,255,255), RGB(255,255,255));
    };
    cloud(90,  80); cloud(220, 50); cloud(360, 100);

    // ground
    FillRectColor(memDC, 0, W_HEIGHT - GROUND_H, W_WIDTH, GROUND_H, COL_GROUND);

    // pipes
    HPEN penPipe = CreatePen(PS_SOLID, 4, COL_PIPE_DK);
    HBRUSH brPipe = CreateSolidBrush(COL_PIPE);
    HPEN oldPen = (HPEN)SelectObject(memDC, penPipe);
    HBRUSH oldBr = (HBRUSH)SelectObject(memDC, brPipe);

    for (auto &p : g.pipes) {
        int gapTop = p.gapY - PIPE_GAP/2;
        int gapBot = p.gapY + PIPE_GAP/2;
        // upper
        Rectangle(memDC, p.x, 0, p.x + PIPE_W, gapTop);
        // cap
        Rectangle(memDC, p.x - 6, gapTop - 24, p.x + PIPE_W + 6, gapTop);
        // lower
        Rectangle(memDC, p.x, gapBot, p.x + PIPE_W, W_HEIGHT - GROUND_H);
        // cap
        Rectangle(memDC, p.x - 6, gapBot, p.x + PIPE_W + 6, gapBot + 24);
    }
    SelectObject(memDC, oldPen); DeleteObject(penPipe);
    SelectObject(memDC, oldBr); DeleteObject(brPipe);

    // bird (body)
    int birdX = (int)(W_WIDTH*0.35f);
    DrawCircle(memDC, birdX, (int)g.birdY, BIRD_R, COL_BIRD, RGB(200,160,0));
    // beak
    POINT beak[3] = {
        { birdX + BIRD_R, (int)g.birdY - 4 },
        { birdX + BIRD_R + 14, (int)g.birdY },
        { birdX + BIRD_R, (int)g.birdY + 4 }
    };
    HBRUSH brBeak = CreateSolidBrush(COL_BEAK);
    HBRUSH oldB = (HBRUSH)SelectObject(memDC, brBeak);
    HPEN penBeak = CreatePen(PS_SOLID, 1, COL_BEAK);
    HPEN oldP = (HPEN)SelectObject(memDC, penBeak);
    Polygon(memDC, beak, 3);
    SelectObject(memDC, oldB); DeleteObject(brBeak);
    SelectObject(memDC, oldP); DeleteObject(penBeak);

    // eye
    DrawCircle(memDC, birdX - 4, (int)g.birdY - 6, 3, RGB(255,255,255), RGB(255,255,255));
    DrawCircle(memDC, birdX - 4, (int)g.birdY - 6, 1, COL_EYE, COL_EYE);

    // UI text
    std::wstring sc = L"Score: " + std::to_wstring(g.score);
    std::wstring best = L"Best: " + std::to_wstring(g.bestScore);
    DrawTextMid(memDC, W_WIDTH - 70, 30, sc, 22, true);
    DrawTextMid(memDC, 80, 30, best, 22, false);

    if (!g.started && g.alive) {
        DrawTextMid(memDC, W_WIDTH/2, 160, L"FLAPPY BIRD", 44, true);
        DrawTextMid(memDC, W_WIDTH/2, 210, L"Press SPACE or Click to start", 22, false);
    }

    if (!g.alive) {
        DrawTextMid(memDC, W_WIDTH/2, 210, L"GAME OVER", 48, true);
        DrawTextMid(memDC, W_WIDTH/2, 255, L"Press R to Restart", 22, false);
    }

    // blit backbuffer
    BitBlt(dc, 0, 0, W_WIDTH, W_HEIGHT, memDC, 0, 0, SRCCOPY);

    // cleanup backbuffer
    SelectObject(memDC, oldBmp);
    DeleteObject(bmp);
    DeleteDC(memDC);
}

// ---------------- Win32 Boilerplate ---------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, (UINT)(1000.0 / FPS), NULL);
        return 0;
    case WM_TIMER:
        Update(DT);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
        if (g.alive) Flap();
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { PostQuitMessage(0); }
        else if (wParam == 'R')  { ResetGame(); InvalidateRect(hwnd,NULL,FALSE); }
        else if (wParam == VK_SPACE) { if (g.alive) Flap(); }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RenderScene(dc);
        EndPaint(hwnd, &ps);
        return 0; }
    case WM_ERASEBKGND:
        return 1; // we draw everything (avoid flicker)
    case WM_DESTROY:
	    SaveBestScore();           // optional safety save
	    KillTimer(hwnd, 1);
	    PostQuitMessage(0);
	    return 0;

    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmd) {
    std::srand((unsigned)std::time(nullptr));

    const wchar_t* CLASS_NAME = L"FlappyBirdWin32";
    WNDCLASS wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) return 0;

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT wr{0,0,W_WIDTH,W_HEIGHT};
    AdjustWindowRect(&wr, style, FALSE);

    HWND hwnd = CreateWindow(
        CLASS_NAME, L"Flappy Bird — Win32 (Dev-C++)",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInst, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmd);
    UpdateWindow(hwnd);
	
	LoadBestScore();


    ResetGame();

    MSG msg;
    while (g.running) {
        // Process all pending messages, then idle a bit
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g.running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(1); // yield
    }

    return 0;
}
