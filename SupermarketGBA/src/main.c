#include <tonc.h>

int main(void) {
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    while (1) {
        VBlankIntrWait();
    }

    return 0;
}
