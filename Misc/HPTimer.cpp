#include "StdAfx.h"
#include "HPTimer.h"
#include <chrono>

double NHPTimer::GetSeconds(const STime& time) { return static_cast<double>(time) * 1e-9; }
double NHPTimer::GetClockRate() { return 1e9; }
void NHPTimer::GetTime(STime* time) {
  *time = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
}
double NHPTimer::GetTimePassed(STime* time) {
  const STime old = *time;
  GetTime(time);
  return GetSeconds(*time - old);
}
// Retained source compatibility: a steady clock never needs TSC calibration.
void NHPTimer::UpdateHPTimerFrequency() {}
