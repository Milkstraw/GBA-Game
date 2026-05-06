/* Minimal stub implementations of libtonc IRQ functions */
typedef unsigned int u32;
typedef void (*fnptr)(void);

void irq_init(fnptr handler) { (void)handler; }
void irq_add(u32 irq_type, fnptr handler) { (void)irq_type; (void)handler; }
void VBlankIntrWait(void) { /* busy wait stub */ volatile int i; for(i=0;i<280896;i++){} }
