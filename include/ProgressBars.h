#ifndef PROGRESS_BARS_H
#define PROGRESS_BARS_H

#include <atomic>

extern std::atomic<bool> isRunning;

void DisplayDeterminateProgressBar(double progress);
void DisplayIndeterminateProgressBar();

#endif // PROGRESS_BARS_H
