// fast_autoclick.cpp
// Build (g++ / MinGW-w64):
// g++ -O3 -march=native -mtune=native -static -s main.cpp -o fast_autoclick.exe -lwinmm

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <cstdio>
#include <string>

#pragma comment(lib, "winmm.lib")

static std::atomic<bool> g_run(false);
static std::atomic<bool> g_quit(false);

static LARGE_INTEGER qpf;

static inline long long qpc_now() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

static inline void spin_wait(long long target) {
    while (qpc_now() < target) {
        YieldProcessor(); // _mm_pause()
    }
}

static inline void send_left_click() {
    INPUT in[2];
    ZeroMemory(in, sizeof(in));

    in[0].type = INPUT_MOUSE;
    in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    in[1].type = INPUT_MOUSE;
    in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(2, in, sizeof(INPUT));
}

int main(int argc, char** argv) {
    timeBeginPeriod(1);

    QueryPerformanceFrequency(&qpf);

    if (argc != 2)
    {
        printf("Usage: fast_autoclick.exe <cps(int) | 0 = maximum cps>\n");
        return -1;
    }

    
    const int targetCPS = std::stoi(argv[1]);
    // =================

    printf("Fast AutoClicker (g++ / SendInput)\n");
    printf("F9 = toggle | ESC = quit\n");
    if (targetCPS == 0)
        printf("Mode: MAX CPS\n");
    else
        printf("Mode: %d CPS\n", targetCPS);

    RegisterHotKey(NULL, 1, 0, VK_F9);
    RegisterHotKey(NULL, 2, 0, VK_ESCAPE);

    MSG msg;
    long long intervalTicks = 0;
    if (targetCPS > 0) {
        intervalTicks = qpf.QuadPart / targetCPS;
        if (intervalTicks < 1) intervalTicks = 1;
    }

    while (!g_quit.load(std::memory_order_relaxed)) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY) {
                if (msg.wParam == 1) {
                    g_run.store(!g_run.load());
                    printf("Run: %s\n", g_run.load() ? "ON" : "OFF");
                } else if (msg.wParam == 2) {
                    g_quit.store(true);
                }
            }
        }

        if (!g_run.load(std::memory_order_relaxed)) {
            Sleep(1);
            continue;
        }

        if (targetCPS == 0) {
            send_left_click();
        } else {
            long long start = qpc_now();
            send_left_click();
            spin_wait(start + intervalTicks);
        }
    }

    UnregisterHotKey(NULL, 1);
    UnregisterHotKey(NULL, 2);

    timeEndPeriod(1);
    return 0;
}
