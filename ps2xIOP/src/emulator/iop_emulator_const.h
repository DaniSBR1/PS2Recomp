#pragma once
#include <cstdint>

constexpr uint32_t kThreadReturnSentinel = 0x1FFFFF00u;
constexpr uint32_t kCallReturnSentinel = 0x1FFFFF04u;
constexpr uint64_t kIopClockHz = 36'864'000ull;
// NTSC field cadence (approximately 59.94 Hz).  VBlank imports are
// scheduler waits, not no-op timing hints: returning immediately lets
// high-priority IRX threads busy-loop and starve RPC server threads.
constexpr uint64_t kVblankPeriodCycles = (kIopClockHz * 1001ull + 30'000ull) / 60'000ull;
constexpr uint64_t kVblankEndPhaseCycles = kVblankPeriodCycles / 16ull;
constexpr uint32_t kDefaultSlice = 256u;
// Was 2'000'000 (~54 ms of real IOP time): legitimate RPC handlers take longer (989snd's external
// command 104 + SMPD_IOP registers ~150 named entries) and were cut off mid-way, returning garbage.
constexpr uint32_t kMaxCallInstructions = 100'000'000u;
// Longest a synchronous call may wait for an IOP thread to signal a semaphore or event flag (1 s).
constexpr uint64_t kSyncWaitBudgetCycles = kIopClockHz;
constexpr uint32_t kModuleLoadBase = 0x00010000u;
constexpr uint32_t kStackGuardBytes = 64u;