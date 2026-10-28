#pragma once
// GOW-Port: GS interrupt events (CSR SIGNAL = bit 0, FINISH = bit 1). The GS raises INTC 0 on each 0 -> 1 change of
// one of those bits while it is unmasked in IMR. The EE scheduler only samples CSR between guest instructions, so a
// FINISH set again between the handler's acknowledge and the next sample looked like "set before, set now": no
// interrupt, the bit was never acknowledged again and no FINISH reached the game after that (God of War II hung with
// its frame flag at 1, asynchronous mode). Whoever sets a bit counts the change here; the scheduler raises the
// interrupt for every new count. Kept out of ps2_memory.h so that changing it does not rebuild the runner.
#include <cstdint>

namespace ps2_gs_csr_events
{
    // `previous` is CSR before the fetch_or that set `bits`: counts the bits that went from 0 to 1.
    void noteSet(uint64_t previous, uint32_t bits);
    // Number of 0 -> 1 changes of CSR bit `bit` (0 = SIGNAL, 1 = FINISH) so far.
    uint32_t count(uint32_t bit);
}
