#pragma once
#include "core/types.h"

/* ---- Memory map --------------------------------------------------------- */
#define MEM_PALETTE     ((vu16*)0x05000000)
#define MEM_VRAM        ((vu16*)0x06000000)
#define MEM_OAM         ((vu32*)0x07000000)

/* ---- I/O registers ------------------------------------------------------ */
#define REG_DISPCNT     (*(vu16*)0x04000000)
#define REG_DISPSTAT    (*(vu16*)0x04000004)
#define REG_VCOUNT      (*(vu16*)0x04000006)

#define REG_BG0CNT      (*(vu16*)0x04000008)
#define REG_BG1CNT      (*(vu16*)0x0400000A)
#define REG_BG2CNT      (*(vu16*)0x0400000C)
#define REG_BG3CNT      (*(vu16*)0x0400000E)

#define REG_BG0HOFS     (*(vu16*)0x04000010)
#define REG_BG0VOFS     (*(vu16*)0x04000012)
#define REG_BG1HOFS     (*(vu16*)0x04000014)
#define REG_BG1VOFS     (*(vu16*)0x04000016)
#define REG_BG2HOFS     (*(vu16*)0x04000018)
#define REG_BG2VOFS     (*(vu16*)0x0400001A)
#define REG_BG3HOFS     (*(vu16*)0x0400001C)
#define REG_BG3VOFS     (*(vu16*)0x0400001E)

#define REG_KEYINPUT    (*(vu16*)0x04000130)

#define REG_WIN0H       (*(vu16*)0x04000040)
#define REG_WIN0V       (*(vu16*)0x04000042)
#define REG_WININ       (*(vu16*)0x04000048)
#define REG_WINOUT      (*(vu16*)0x0400004A)

/* ---- DISPCNT bits ------------------------------------------------------- */
#define MODE0           0x0000
#define MODE1           0x0001
#define MODE3           0x0003
#define BG0_ENABLE      (1 << 8)
#define BG1_ENABLE      (1 << 9)
#define BG2_ENABLE      (1 << 10)
#define BG3_ENABLE      (1 << 11)
#define OBJ_ENABLE      (1 << 12)
#define WIN0_ENABLE     (1 << 13)
#define WIN1_ENABLE     (1 << 14)
#define OBJWIN_ENABLE   (1 << 15)

/* ---- Key bitmasks ------------------------------------------------------- */
#define KEY_A           (1 << 0)
#define KEY_B           (1 << 1)
#define KEY_SELECT      (1 << 2)
#define KEY_START       (1 << 3)
#define KEY_RIGHT       (1 << 4)
#define KEY_LEFT        (1 << 5)
#define KEY_UP          (1 << 6)
#define KEY_DOWN        (1 << 7)
#define KEY_R           (1 << 8)
#define KEY_L           (1 << 9)

/* ---- Helpers ------------------------------------------------------------ */
static inline void vsync(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT <  160);
}

#define RGB15(r, g, b)  ((u16)((r) | ((g) << 5) | ((b) << 10)))
