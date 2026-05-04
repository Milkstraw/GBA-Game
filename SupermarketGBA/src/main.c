#include <tonc.h>

int main(void) {
    irq_init(NULL);
    irq_enable(II_VBLANK);

    while (1) {
        VBlankIntrWait();
    }

    return 0;
}
