#ifndef GBA_TYPES_H
#define GBA_TYPES_H

/* ----------------------------------------------------------------
 * Primitive types
 * ---------------------------------------------------------------- */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed char    s8;
typedef signed short   s16;
typedef signed int     s32;

/* ----------------------------------------------------------------
 * Display control
 * ---------------------------------------------------------------- */
#define REG_DISPCNT  (*(volatile u16*)0x04000000)

/* ----------------------------------------------------------------
 * Background control registers
 * ---------------------------------------------------------------- */
#define REG_BG0CNT   (*(volatile u16*)0x04000008)
#define REG_BG1CNT   (*(volatile u16*)0x0400000A)
#define REG_BG2CNT   (*(volatile u16*)0x0400000C)
#define REG_BG3CNT   (*(volatile u16*)0x0400000E)

/* ----------------------------------------------------------------
 * Background scroll offsets (write-only hardware registers)
 * ---------------------------------------------------------------- */
#define REG_BG0HOFS  (*(volatile u16*)0x04000010)
#define REG_BG0VOFS  (*(volatile u16*)0x04000012)
#define REG_BG1HOFS  (*(volatile u16*)0x04000014)
#define REG_BG1VOFS  (*(volatile u16*)0x04000016)
#define REG_BG2HOFS  (*(volatile u16*)0x04000018)
#define REG_BG2VOFS  (*(volatile u16*)0x0400001A)
#define REG_BG3HOFS  (*(volatile u16*)0x0400001C)
#define REG_BG3VOFS  (*(volatile u16*)0x0400001E)

/* ----------------------------------------------------------------
 * Memory region base addresses and volatile pointers
 * PALRAM / VRAM / OAM — u16-wide bus (except OAM attr bytes)
 * ---------------------------------------------------------------- */
#define PALRAM_BASE  0x05000000
#define VRAM_BASE    0x06000000
#define OAM_BASE     0x07000000

#define PALRAM  ((volatile u16*)PALRAM_BASE)
#define VRAM    ((volatile u16*)VRAM_BASE)
#define OAM     ((volatile u16*)OAM_BASE)

/* ----------------------------------------------------------------
 * Key input (active-low: bit = 0 means pressed)
 * ---------------------------------------------------------------- */
#define REG_KEYINPUT  (*(volatile u16*)0x04000130)

#define KEY_A       (1 << 0)
#define KEY_B       (1 << 1)
#define KEY_SELECT  (1 << 2)
#define KEY_START   (1 << 3)
#define KEY_RIGHT   (1 << 4)
#define KEY_LEFT    (1 << 5)
#define KEY_UP      (1 << 6)
#define KEY_DOWN    (1 << 7)
#define KEY_R       (1 << 8)
#define KEY_L       (1 << 9)

/* Invert active-low signal; result has 1 = pressed */
#define KEYS_DOWN()  ((u16)(~REG_KEYINPUT & 0x03FF))

#endif /* GBA_TYPES_H */
