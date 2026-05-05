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
 *
 * PALRAM  0x05000000 — 16-bit bus; 512 B (256 BG + 256 OBJ colours)
 * VRAM    0x06000000 — 16-bit bus for BG charblocks (0–3),
 *                      32-bit bus for OBJ charblocks (4–5)
 * OAM     0x07000000 — 32-bit bus; u16 attribute writes are valid
 *                      but always update via DMA or during VBlank
 * ---------------------------------------------------------------- */
#define PALRAM_BASE  0x05000000
#define VRAM_BASE    0x06000000
#define OAM_BASE     0x07000000

#define PALRAM  ((volatile u16*)PALRAM_BASE)
#define VRAM    ((volatile u16*)VRAM_BASE)
#define OAM     ((volatile u16*)OAM_BASE)

/* ----------------------------------------------------------------
 * Key input  (active-low: bit = 0 means pressed)
 * Use KEYS_RAW() to snapshot once per frame into a local u16;
 * do NOT call it multiple times per frame or compare frames.
 * ---------------------------------------------------------------- */
#define REG_KEYINPUT  (*(volatile u16*)0x04000130)

#define KEY_A       (1u << 0)
#define KEY_B       (1u << 1)
#define KEY_SELECT  (1u << 2)
#define KEY_START   (1u << 3)
#define KEY_RIGHT   (1u << 4)
#define KEY_LEFT    (1u << 5)
#define KEY_UP      (1u << 6)
#define KEY_DOWN    (1u << 7)
#define KEY_R       (1u << 8)
#define KEY_L       (1u << 9)

/* Raw hardware read — inverts active-low, masks 10 valid bits.
 * Result: 1 = pressed, 0 = released. */
#define KEYS_RAW()  ((u16)(~REG_KEYINPUT & 0x03FFu))

#endif /* GBA_TYPES_H */
