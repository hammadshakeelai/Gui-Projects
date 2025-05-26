#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <fstream>

// Configurations
static constexpr int windowWidth = 1520;
static constexpr int windowHeight = 820;
static constexpr int spawnInterval = 30; // frames between pillars
static constexpr int timerInterval = 12; // ms per frame (~83 FPS)
static constexpr int messageDisplayTime = 2000; // Time to display "You Lost" message in milliseconds

// Win32 string constants
static constexpr LPCSTR CLASS_NAME = "FlappyClass";
static constexpr LPCSTR WINDOW_TITLE = "Flappy Bird";

// Bird Class
class Bird {
public:
    Bird(int x, int y) : x_(x), y_(y), velocity_(0.0f) {}

    void update() {
        velocity_ += gravity_;
        y_ += static_cast<int>(velocity_);
    }

    void flap() {
        velocity_ = -18.0f; // Flap effect
    }

    void draw(HDC hdc) const {
        HBRUSH brush = CreateSolidBrush(RGB(255, 215, 0)); // Gold color
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        Ellipse(hdc, x_ - size_ / 2, y_ - size_ / 2, x_ + size_ / 2, y_ + size_ / 2);
        SelectObject(hdc, oldBrush);
        DeleteObject(brush);
    }

    RECT getBounds() const {
        return { x_ - size_ / 2, y_ - size_ / 2, x_ + size_ / 2, y_ + size_ / 2 };
    }

private:
    int x_, y_;
    float velocity_;
    static constexpr int size_ = 10;
    static constexpr float gravity_ = 1.2f; // Gravity effect
};

// Pillar Class
class Pillar {
public:
    Pillar(int x, int gapY) : x_(x), gapY_(gapY) {}

    void update() {
        x_ -= speed_; // Move left
    }

    void draw(HDC hdc) const {
        HBRUSH brush = CreateSolidBrush(RGB(0, 128, 0)); // Green color
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        
        // Draw top pillar
        RECT top = getTopRect();
        Rectangle(hdc, top.left, top.top, top.right, top.bottom);
        
        // Draw bottom pillar
        RECT bottom = getBottomRect();
        Rectangle(hdc, bottom.left, bottom.top, bottom.right, bottom.bottom);
        
        SelectObject(hdc, oldBrush);
        DeleteObject(brush);
    }

    bool isOffScreen() const {
        return x_ + width_ < 0;
    }

    RECT getTopRect() const {
        return { x_, 0, x_ + width_, gapY_ - gap_ / 2 };
    }

    RECT getBottomRect() const {
        return { x_, gapY_ + gap_ / 2, x_ + width_, windowHeight };
    }

private:
    int x_, gapY_;
    static constexpr int width_ = 50;
    static constexpr int gap_ = 500;
    static constexpr int speed_ = 20; // Speed of the pillars
};

// Game Class
class Game {
public:
    Game() : bird_(100, 300), score_(0), highestPillarsCrossed_(0), gameOver_(false), frameCount_(0), pillarsCrossed_(0), showLostMessage_(false), showHighScoreMessage_(false) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        loadHighestPillarsCrossed();
    }

    void init() {
        pillars_.clear();
        bird_ = Bird(100, 300);
        score_ = 0;
        gameOver_ = false;
        frameCount_ = 0;
        pillarsCrossed_ = 0; // Reset the counter
        showLostMessage_ = false; // Reset lost message
        showHighScoreMessage_ = false; // Reset high score message
    }

    void update() {
        if (gameOver_) {
            if (showLostMessage_) {
                lostMessageTimer_ += timerInterval;
                if (lostMessageTimer_ >= messageDisplayTime) {
                    showLostMessage_ = false; // Hide the message after the display time
                }
            }
            return;
        }

        bird_.update();

        // Spawn new pillars
        if (++frameCount_ >= spawnInterval) {
            int gapY = 150 + std::rand() % (windowHeight - 300);
            pillars_.emplace_back(windowWidth, gapY);
            frameCount_ = 0;
        }

        // Move and remove pillars
        for (auto &p : pillars_) p.update();
        if (!pillars_.empty() && pillars_.front().isOffScreen()) {
            pillars_.erase(pillars_.begin());
            pillarsCrossed_+=10; // Increment the counter for crossed pillars
            if (pillarsCrossed_ > highestPillarsCrossed_) {
                highestPillarsCrossed_ = pillarsCrossed_; // Update highest if necessary
                showHighScoreMessage_ = true; // Show high score message
            }
        }

        // Collision detection
        RECT b = bird_.getBounds();
        if (b.top < 0 || b.bottom > windowHeight) {
            gameOver_ = true;
        }
        for (auto &p : pillars_) {
            RECT topRect = p.getTopRect();
            RECT bottomRect = p.getBottomRect();
            if (IntersectRect(&b, &topRect, &b) || IntersectRect(&b, &bottomRect, &b)) {
                gameOver_ = true; // End game immediately on collision
                showLostMessage_ = true; // Show lost message
                break;
            }
        }

        if (gameOver_) {
            saveHighestPillarsCrossed(); // Save the highest score when game over
        }
    }

    void draw(HDC hdc) {
        bird_.draw(hdc);
        for (auto &p : pillars_) p.draw(hdc);

        // Draw score in black
        SetTextColor(hdc, RGB(0, 0, 0)); // Black color
        std::string s = "Score: " + std::to_string(pillarsCrossed_);
        TextOut(hdc, 10, 10, s.c_str(), (int)s.size());

        // Draw highest pillars crossed
        std::string h = "Highest: " + std::to_string(highestPillarsCrossed_);
        TextOut(hdc, 10, 30, h.c_str(), (int)h.size());

        // Draw "You Lost" message
        if (showLostMessage_) {
            SetTextColor(hdc, RGB(255, 0, 0)); // Red color for lost message
            std::string lostMessage = "You Lost!";
            TextOut(hdc, windowWidth / 2 - 50, windowHeight / 2, lostMessage.c_str(), (int)lostMessage.size());
        }

        // Draw "You Beat Your High Score!" message
        if (showHighScoreMessage_) {
            SetTextColor(hdc, RGB(0, 255, 0)); // Green color for high score message
            std::string highScoreMessage = "You Beat Your High Score!";
            TextOut(hdc, windowWidth / 2 - 100, windowHeight / 2 + 30, highScoreMessage.c_str(), (int)highScoreMessage.size());
        }
    }

    void flap() {
        if (gameOver_) init();
        bird_.flap();
    }

    bool isGameOver() const { return gameOver_; }

private:
    Bird bird_;
    std::vector<Pillar> pillars_;
    int score_;
    int highestPillarsCrossed_;
    bool gameOver_;
    int frameCount_;
    int pillarsCrossed_;
    bool showLostMessage_;
    bool showHighScoreMessage_;
    int lostMessageTimer_ = 0; // Timer for lost message display

    void loadHighestPillarsCrossed() {
        std::ifstream file("highest_pillars_crossed.txt");
        if (file.is_open()) {
            file >> highestPillarsCrossed_;
            file.close();
        } else {
            highestPillarsCrossed_ = 0; // Default value if file doesn't exist
        }
    }

    void saveHighestPillarsCrossed() {
        std::ofstream file("highest_pillars_crossed.txt");
        if (file.is_open()) {
            file << highestPillarsCrossed_;
            file.close();
        }
    }
};

// Global Game Instance
static Game game;

// Win32 GUI
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, timerInterval, nullptr);
        break;
    case WM_TIMER:
        game.update();
        InvalidateRect(hwnd, nullptr, TRUE);
        break;
    case WM_LBUTTONDOWN:
        game.flap();
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        game.draw(hdc);
        EndPaint(hwnd, &ps);
    } break;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth + 16, windowHeight + 39,
        nullptr, nullptr, hInst, nullptr
    );
    ShowWindow(hwnd, nShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

