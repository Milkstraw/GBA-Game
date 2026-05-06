/*
 * tonc.h — minimal stub for GBA development (no devkitPro installed)
 * Covers every macro / type / symbol used in this project.
 */
#ifndef TONC_H
#define TONC_H

#include <stddef.h>   /* NULL, size_t */
#include <string.h>   /* memcpy, memset */

/* ----------------------------------------------------------------
 * Primitive types (same as gba_types.h but tonc.h is included first)
 * ---------------------------------------------------------------- */
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;

typedef u16 SCREENENTRY;
typedef u16 COLOR;

/* ----------------------------------------------------------------
 * GBA Memory map pointers
 * ---------------------------------------------------------------- */
#define MEM_VRAM    0x06000000
#define MEM_PAL     0x05000000
#define MEM_OAM     0x07000000

/* PALRAM — 16-bit bus */
#define pal_bg_mem   ((COLOR*)(MEM_PAL))
#define pal_obj_mem  ((COLOR*)(MEM_PAL + 0x200))

/* VRAM screenblock array: se_mem[sbb][row][col]
 * Each screenblock is 32×32 u16 entries = 2KB.
 * 32 screenblocks fit in VRAM's 64KB BG area. */
#define se_mem  ((SCREENENTRY(*)[32][32])(MEM_VRAM))

/* ----------------------------------------------------------------
 * OAM / Sprite structures
 * ---------------------------------------------------------------- */
typedef struct OBJ_ATTR {
    u16 attr0;
    u16 attr1;
    u16 attr2;
    s16 fill;
} __attribute__((aligned(4))) OBJ_ATTR;

#define obj_mem  ((OBJ_ATTR*)(MEM_OAM))

/* --- ATTR0 --- */
#define ATTR0_SQUARE    0x0000u   /* shape=0 (square)  bits[15:14]=00 */
#define ATTR0_WIDE      0x4000u   /* shape=1 (wide)    bits[15:14]=01 */
#define ATTR0_TALL      0x8000u   /* shape=2 (tall)    bits[15:14]=10 */
#define ATTR0_4BPP      0x0000u   /* palette mode 4bpp (bit 13 = 0)  */
#define ATTR0_8BPP      0x2000u   /* palette mode 8bpp (bit 13 = 1)  */
#define ATTR0_REG       0x0000u   /* normal mode       bits[9:8]=00   */
#define ATTR0_AFF       0x0100u   /* affine mode       bits[9:8]=01   */
#define ATTR0_HIDE      0x0200u   /* hidden            bits[9:8]=10   */
#define ATTR0_AFF_DBL   0x0300u   /* affine double     bits[9:8]=11   */
#define ATTR0_MOSAIC    0x1000u

/* --- ATTR1 --- */
#define ATTR1_HFLIP     0x1000u
#define ATTR1_VFLIP     0x2000u
/* Size combined with ATTR0 shape determines sprite dimensions:
 * SQ:   8, 16, 32, 64
 * WIDE: 16x8, 32x8, 32x16, 64x32
 * TALL: 8x16, 8x32, 16x32, 32x64
 */
#define ATTR1_SIZE_8    0x0000u   /* size=0 bits[15:14]=00 */
#define ATTR1_SIZE_16   0x4000u   /* size=1 bits[15:14]=01 */
#define ATTR1_SIZE_32   0x8000u   /* size=2 bits[15:14]=10 */
#define ATTR1_SIZE_64   0xC000u   /* size=3 bits[15:14]=11 */

/* --- ATTR2 --- */
#define ATTR2_PALBANK(n) (((u16)(n) & 0xFu) << 12)
#define ATTR2_PRIO(n)    (((u16)(n) & 0x3u) << 10)

/* ----------------------------------------------------------------
 * REG_DISPCNT bits
 * ---------------------------------------------------------------- */
#define REG_DISPCNT  (*(volatile u16*)0x04000000)
#define DCNT_MODE0   0x0000u
#define DCNT_MODE1   0x0001u
#define DCNT_MODE2   0x0002u
#define DCNT_MODE3   0x0003u
#define DCNT_MODE4   0x0004u
#define DCNT_MODE5   0x0005u
#define DCNT_OBJ_1D  0x0040u   /* OBJ tile mapping: 1D (bit 6) */
#define DCNT_BLANK   0x0080u   /* force blank                   */
#define DCNT_BG0     0x0100u   /* enable BG0 (bit  8)           */
#define DCNT_BG1     0x0200u   /* enable BG1 (bit  9)           */
#define DCNT_BG2     0x0400u   /* enable BG2 (bit 10)           */
#define DCNT_BG3     0x0800u   /* enable BG3 (bit 11)           */
#define DCNT_OBJ     0x1000u   /* enable OBJ (bit 12)           */
#define DCNT_WIN0    0x2000u
#define DCNT_WIN1    0x4000u
#define DCNT_WINOBJ  0x8000u

/* ----------------------------------------------------------------
 * BGxCNT helpers
 * ---------------------------------------------------------------- */
#define BG_PRIO(n)     ((u16)((n) & 3))
#define BG_CBB(n)      ((u16)(((n) & 3) << 2))
#define BG_MOSAIC      (1u << 6)
#define BG_4BPP        0x0000u
#define BG_8BPP        0x0080u
#define BG_SBB(n)      ((u16)(((n) & 31) << 8))
#define BG_WRAP        0x2000u
#define BG_REG_32x32   0x0000u
#define BG_REG_64x32   0x4000u
#define BG_REG_32x64   0x8000u
#define BG_REG_64x64   0xC000u

/* BGxCNT register addresses */
#define REG_BG0CNT  (*(volatile u16*)0x04000008)
#define REG_BG1CNT  (*(volatile u16*)0x0400000A)
#define REG_BG2CNT  (*(volatile u16*)0x0400000C)
#define REG_BG3CNT  (*(volatile u16*)0x0400000E)

/* BGx scroll registers */
#define REG_BG0HOFS (*(volatile u16*)0x04000010)
#define REG_BG0VOFS (*(volatile u16*)0x04000012)
#define REG_BG1HOFS (*(volatile u16*)0x04000014)
#define REG_BG1VOFS (*(volatile u16*)0x04000016)
#define REG_BG2HOFS (*(volatile u16*)0x04000018)
#define REG_BG2VOFS (*(volatile u16*)0x0400001A)
#define REG_BG3HOFS (*(volatile u16*)0x0400001C)
#define REG_BG3VOFS (*(volatile u16*)0x0400001E)

/* ----------------------------------------------------------------
 * IRQ system
 * ---------------------------------------------------------------- */
#define II_VBLANK   0x0001u
#define II_HBLANK   0x0002u
#define II_VCOUNT   0x0004u
#define II_TIMER0   0x0008u

typedef void (*fnptr)(void);

void irq_init(fnptr handler);
void irq_add(u32 irq_type, fnptr handler);
void VBlankIntrWait(void);

/* ----------------------------------------------------------------
 * Misc utility macros
 * ---------------------------------------------------------------- */
#define ALIGN4  __attribute__((aligned(4)))

/* 16-color (4bpp) tile: 32 bytes */
typedef struct {
    u32 data[8];
} TILE4;

/* Charblock arrays: tile4_mem[charblock][tile] */
#define tile4_mem  ((TILE4(*)[512])(MEM_VRAM))

#endif /* TONC_H */
