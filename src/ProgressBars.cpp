#include "ProgressBars.h"
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <iomanip>

std::atomic<bool> isRunning;

void DisplayIndeterminateProgressBar() {
    int barWidth = 50;
    int position = 0;
    bool directionRight = true;
    std::string yellowStart = "\033[1;33m";
    std::string yellowEnd = "\033[0m";
    std::mutex coutMutex;

    while (isRunning) {
        std::unique_lock<std::mutex> lock(coutMutex);
        std::cout << yellowStart << "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i == position) {
                std::cout << (directionRight ? "=>" : "<=");
            } else if (i == position + 1 && !directionRight) {
                std::cout << "=>";
            } else if (i == position - 1 && directionRight) {
                std::cout << "<=";
            } else {
                std::cout << "·";
            }
        }
        std::cout << "]\r" << yellowEnd;
        std::cout.flush();
        lock.unlock();

        position += directionRight ? 1 : -1;

        if (position == barWidth - 2) {
            directionRight = false;
        } else if (position == 0) {
            directionRight = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void DisplayDeterminateProgressBar(double progress) {
    int barWidth = 50;
    std::string yellowStart = "\033[1;33m";
    std::string yellowEnd = "\033[0m";

    std::cout << yellowStart << "[";
    int pos = static_cast<int>(barWidth * progress);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(2) << progress * 100.0 << " %\r" << yellowEnd;
    std::cout.flush();
}