#ifndef GBA_H
#define GBA_H

/* ---------- Basic integer types ---------- */
typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed int     s32;
typedef signed short   s16;

/* ---------- Hardware registers ---------- */
#define REG_DISPCNT  (*(volatile u32*)0x04000000)
#define REG_DISPSTAT (*(volatile u16*)0x04000004)
#define REG_VCOUNT   (*(volatile u16*)0x04000006)
#define REG_KEYINPUT (*(volatile u16*)0x04000130)

/* ---------- Display control ---------- */
#define MODE3       0x0003
#define BG2_ENABLE  (1 << 10)

/* ---------- VRAM (Mode 3: u16 per pixel, 240x160) ---------- */
#define VRAM         ((volatile u16*)0x06000000)
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160

/* ---------- Key bit masks (KEYINPUT is active-low) ---------- */
#define KEY_A      (1 << 0)
#define KEY_B      (1 << 1)
#define KEY_SELECT (1 << 2)
#define KEY_START  (1 << 3)
#define KEY_RIGHT  (1 << 4)
#define KEY_LEFT   (1 << 5)
#define KEY_UP     (1 << 6)
#define KEY_DOWN   (1 << 7)
#define KEY_R      (1 << 8)
#define KEY_L      (1 << 9)

/* Invert active-low and mask 10 valid bits: result has 1 = pressed */
#define KEYS_READ() ((u16)(~REG_KEYINPUT & 0x03FF))

/* ---------- Color: 15-bit BGR555 (R=bits0-4, G=bits5-9, B=bits10-14) ---------- */
#define RGB15(r, g, b) ((u16)((r) | ((g) << 5) | ((b) << 10)))

/* ---------- VSync: busy-wait for start of VBlank (scanline 160) ---------- */
static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT <  160);
}

/* ---------- Utility macros ---------- */
#define ABS(x)          ((x) < 0 ? -(x) : (x))
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))

#endif /* GBA_H */
