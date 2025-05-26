#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <fstream>
#include <memory> // Added for smart pointers

// Game Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PILLAR_GAP = 400;
const int PILLAR_WIDTH = 60;
const int BIRD_SIZE = 20;
const int GROUND_HEIGHT = 50;

// Bird Structure
class Bird {
	public:
    int x;
    int y;
    float velocity;
    const float gravity = 0.9f;
    const float jumpForce = -10.0f;
    
    Bird() : x(100), y(SCREEN_HEIGHT/2), velocity(0) {}
    
    // Copy constructor
    Bird(const Bird& other) : 
        x(other.x), 
        y(other.y), 
        velocity(other.velocity),
        gravity(0.9f),  // Initialize const members directly
        jumpForce(-10.0f) {}
    
    // Assignment operator
    Bird& operator=(const Bird& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
            velocity = other.velocity;
        }
        return *this;
    }
    
    void update() {
        velocity += gravity;
        y += velocity;
    }
    
    void jump() {
        velocity = jumpForce;
    }
    
    RECT getRect() {
        return {x - BIRD_SIZE/2, y - BIRD_SIZE/2, 
                x + BIRD_SIZE/2, y + BIRD_SIZE/2};
    }
};

// Pillar Structure
class Pillar {
	public:
    int x;
    int gapY;
    bool passed;
    
    Pillar(int x, int y) : x(x), gapY(y), passed(false) {}
    
    void update() {
        x -= 11;
    }
    
    bool isOffScreen() const {
        return x < -PILLAR_WIDTH;
    }
    
    RECT getTopRect() const {
        return {x, 0, x + PILLAR_WIDTH, gapY - PILLAR_GAP/2};
    }
    
    RECT getBottomRect() const {
        return {x, gapY + PILLAR_GAP/2, x + PILLAR_WIDTH, SCREEN_HEIGHT - GROUND_HEIGHT};
    }
};

// Game Variables
static HWND g_hwnd = nullptr;
static HDC g_hdc = nullptr;
static bool g_gameRunning = true;
static int g_score = 0;
static int g_highScore = 0;
static Bird g_bird;
static std::vector<Pillar> g_pillars;
static int g_pillarTimer = 0;
static const int PILLAR_FREQUENCY = 50;

// Resource Management Class
class GDIResources {
public:
    GDIResources() = default;
    ~GDIResources() {
        if (skyBrush_) DeleteObject(skyBrush_);
        if (groundBrush_) DeleteObject(groundBrush_);
        if (pillarBrush_) DeleteObject(pillarBrush_);
        if (birdBrush_) DeleteObject(birdBrush_);
        if (font_) DeleteObject(font_);
    }

    void initialize(HDC hdc) {
        skyBrush_ = CreateSolidBrush(RGB(135, 206, 235));
        groundBrush_ = CreateSolidBrush(RGB(139, 69, 19));
        pillarBrush_ = CreateSolidBrush(RGB(0, 128, 0));
        birdBrush_ = CreateSolidBrush(RGB(255, 215, 0));
        font_ = CreateFont(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          VARIABLE_PITCH, "Arial");
    }

    HBRUSH getSkyBrush() const { return static_cast<HBRUSH>(skyBrush_); }
    HBRUSH getGroundBrush() const { return static_cast<HBRUSH>(groundBrush_); }
    HBRUSH getPillarBrush() const { return static_cast<HBRUSH>(pillarBrush_); }
    HBRUSH getBirdBrush() const { return static_cast<HBRUSH>(birdBrush_); }
    HGDIOBJ getFont() const { return font_; }

private:
    HGDIOBJ skyBrush_ = nullptr;
    HGDIOBJ groundBrush_ = nullptr;
    HGDIOBJ pillarBrush_ = nullptr;
    HGDIOBJ birdBrush_ = nullptr;
    HGDIOBJ font_ = nullptr;
};

void InitGame() {
    // Reset bird using constructor
g_bird = Bird();
    
    g_pillars.clear();
    g_pillarTimer = 0;
    g_score = 0;
    
    // Load high score
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> g_highScore;
        file.close();
    }
}

void SpawnPillar() {
    int gapY = 150 + (rand() % (SCREEN_HEIGHT - GROUND_HEIGHT - 300));
    g_pillars.push_back(Pillar(SCREEN_WIDTH, gapY));
}

void UpdateGame() {
    g_bird.update();
    
    if (++g_pillarTimer >= PILLAR_FREQUENCY) {
        SpawnPillar();
        g_pillarTimer = 0;
    }
    
    for (auto& pillar : g_pillars) {
        pillar.update();
        
        if (!pillar.passed && pillar.x + PILLAR_WIDTH < g_bird.x) {
            pillar.passed = true;
            g_score++;
            if (g_score > g_highScore) {
                g_highScore = g_score;
                std::ofstream file("highscore.txt");
                if (file.is_open()) {
                    file << g_highScore;
                    file.close();
                }
            }
        }
    }
    
    if (!g_pillars.empty() && g_pillars.front().isOffScreen()) {
        g_pillars.erase(g_pillars.begin());
    }
    
    RECT birdRect = g_bird.getRect();
    
    // Ground collision
    if (birdRect.bottom > SCREEN_HEIGHT - GROUND_HEIGHT) {
        InitGame();
        return;
    }
    
    // Ceiling collision
    if (birdRect.top < 0) {
        InitGame();
        return;
    }
    
    // Pillar collisions
    for (const auto& pillar : g_pillars) {
        RECT topPillar = pillar.getTopRect();
        RECT bottomPillar = pillar.getBottomRect();
        
        if (birdRect.right > topPillar.left && birdRect.left < topPillar.right) {
            if (birdRect.top < topPillar.bottom || birdRect.bottom > bottomPillar.top) {
                InitGame();
                return;
            }
        }
    }
}

void DrawGame(HDC hdc) {
    GDIResources resources;
    resources.initialize(hdc);
    
    // Clear screen
    RECT skyRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    FillRect(hdc, &skyRect, resources.getSkyBrush());
    
    // Draw ground
    RECT groundRect = {0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
    FillRect(hdc, &groundRect, resources.getGroundBrush());
    
    // Draw pillars
    for (const auto& pillar : g_pillars) {
        RECT topPillar = pillar.getTopRect();
        RECT bottomPillar = pillar.getBottomRect();
        FillRect(hdc, &topPillar, resources.getPillarBrush());
        FillRect(hdc, &bottomPillar, resources.getPillarBrush());
    }
    
    // Draw bird
    RECT birdRect = g_bird.getRect();
    FillRect(hdc, &birdRect, resources.getBirdBrush());
    
    // Draw score
    SelectObject(hdc, resources.getFont());
    
    std::string scoreText = "Score: " + std::to_string(g_score);
    TextOut(hdc, 10, 10, scoreText.c_str(), scoreText.length());
    
    std::string highScoreText = "High Score: " + std::to_string(g_highScore);
    TextOut(hdc, 10, 50, highScoreText.c_str(), highScoreText.length());
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            g_gameRunning = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_SPACE) {
                g_bird.jump();
            }
            return 0;
            
        case WM_LBUTTONDOWN:
            g_bird.jump();
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FlappyBirdWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    
    // Create window
    g_hwnd = CreateWindowEx(0, "FlappyBirdWindowClass", "Flappy Bird",
                           WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
                           CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
                           NULL, NULL, hInstance, NULL);
    ShowWindow(g_hwnd, nCmdShow);
    
    // Initialize game
    InitGame();
    srand(static_cast<unsigned>(time(NULL)));
    
    // Game loop
    MSG msg = {};
    while (g_gameRunning) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        UpdateGame();
        
        HDC hdc = GetDC(g_hwnd);
        DrawGame(hdc);
        ReleaseDC(g_hwnd, hdc);
        
        Sleep(2); // ~60 FPS
    }
    
    return 0;
}