#pragma once

#include <kernel/core/types.h>

namespace kernel::interrupts {

class Pic {
public:
    static void init();
    static void send_eoi(u8 irq);
    static void mask_irq(u8 irq);
    static void unmask_irq(u8 irq);
    static void disable();
};

} // namespace kernel::interrupts
