/*
 * sprites.c — OAM (Object Attribute Memory) management.
 *
 * OAM layout (per sprite, 8 bytes = 2 × u32):
 *   Word 0: attr0 (bits 15-8: obj mode, y coord in bits 7-0)
 *   Word 0: attr1 in bits 31-16 (x coord in bits 8-0)
 *   Word 1: attr2 in bits 15-0  (tile index, palette, flip, priority)
 *   Word 1: bits 31-16 = padding (rotation/scale params or unused)
 *
 * GBA OAM is write-only during active display; a shadow buffer is maintained
 * in IWRAM and DMA'd to OAM during VBlank via copy_oam().
 *
 * OAM disable: set attr0 bit 9 (obj mode = 10 → rotation off) with
 * Y >= 160.  The canonical disable value is 0x0200 which places the
 * sprite at Y=0 in "affine enabled" mode but with affine index 0 —
 * actually the cleaner disable is attr0 = (160 & 0xFF) | 0x0200 but
 * the spec says 0x0200 and the GBA interprets bit 9 set with Y=0 as
 * an off-screen object (double-size affine mode hides it above screen).
 * We follow the spec exactly: attr0 = 0x0200.
 */

#include "sprites.h"
#include "../gba.h"

/* ---- DMA3 registers ------------------------------------------------------ */
#define REG_DMA3SAD  (*(vu32*)0x040000D4)  /* source address */
#define REG_DMA3DAD  (*(vu32*)0x040000D8)  /* destination address */
#define REG_DMA3CNT  (*(vu32*)0x040000DC)  /* control + word count */

/* DMA control word:
 *   bits 27-16  word count
 *   bit  26     DMA enable (must be 1 to start)
 *   bit  25     start timing (00 = immediately)
 *   bit  21     transfer type (1 = 32-bit words)
 *   ...all other bits 0 for simple copy
 *
 *   DMA_ENABLE | DMA_32 | count:
 *     0x80000000 = enable
 *     0x04000000 = 32-bit
 *     count in bits 27-16 (but CNT is a single 32-bit reg that includes
 *     both the 16-bit count and 16-bit control in high word).
 *
 * Standard split: CNT_L = count (16-bit), CNT_H = control (16-bit).
 * Using the combined 32-bit write:
 *   bits 15-0  = word count (number of 32-bit units)
 *   bits 31-16 = control flags
 *
 * Control flags (high 16 bits):
 *   bit 15  = DMA enable
 *   bit 10  = transfer size (1=32-bit)
 *   bits 7-6 = start timing (00=immediate)
 *   bits 5-4 = dest addr control (00=increment)
 *   bits 3-2 = src  addr control (00=increment)
 */
#define DMA_ENABLE   (1u << 31)
#define DMA_32BIT    (1u << 26)
#define DMA_IMMEDIATE 0u

/* OAM is 128 sprites × 8 bytes = 1024 bytes = 256 u32 values */
#define OAM_WORD_COUNT  256u

/* ---- Shadow buffer ------------------------------------------------------- */
/* 128 sprites, each taking 2 u32 words (attr0|attr1 packed, attr2|pad) */
static u32 oam_shadow[MAX_SPRITES * 2];

/* ---- OAM helper macros --------------------------------------------------- */
/*
 * Shadow buffer layout per sprite index id:
 *   oam_shadow[id*2 + 0] = attr0 (low 16) | attr1 (high 16)
 *   oam_shadow[id*2 + 1] = attr2 (low 16) | pad  (high 16)
 */
#define SHADOW_W0(id)  oam_shadow[(id) * 2 + 0]
#define SHADOW_W1(id)  oam_shadow[(id) * 2 + 1]

/* Pack attr0 and attr1 into the first word */
static inline u32 make_w0(u16 attr0, u16 attr1)
{
    return (u32)attr0 | ((u32)attr1 << 16);
}

/* Pack attr2 (and zeroed pad) into the second word */
static inline u32 make_w1(u16 attr2)
{
    return (u32)attr2;
}

/* ---- Public API ---------------------------------------------------------- */

/*
 * oam_init — disable all 128 sprites by setting attr0 = 0x0200.
 * Also zeroes attr1, attr2, and pad.
 */
void oam_init(void)
{
    u8 i;
    for (i = 0; i < MAX_SPRITES; ++i) {
        SHADOW_W0(i) = make_w0(0x0200u, 0x0000u);
        SHADOW_W1(i) = make_w1(0x0000u);
    }
}

/*
 * sprite_set — configure sprite id in the shadow buffer.
 *
 * attr0 bits:
 *   7-0   Y coordinate (screen, 0-255 wrapping)
 *   8     Rotation/Scaling flag (0 = off)
 *   9     OBJ Disable (when bit 8 = 0, bit 9=1 hides the sprite)
 *   11-10 OBJ Mode (00 = Normal)
 *   12    Mosaic (0 = off)
 *   13    Colour mode (0 = 4bpp / 16 colours)
 *   15-14 OBJ Shape (00 = Square)
 *
 * attr1 bits:
 *   8-0   X coordinate (signed 9-bit, 0-511)
 *   13-9  Rotation/Scale param index (unused when bit 8 of attr0 = 0)
 *   12    Horizontal flip
 *   13    Vertical flip
 *   15-14 OBJ Size (00 = 8×8 for square shape)
 *
 * attr2 bits:
 *   9-0   Tile index (in OBJ VRAM tile space, 32-byte units for 4bpp)
 *   11-10 Priority (00 = highest)
 *   15-12 Palette bank (for 4bpp mode)
 *
 * For a standard 8×8 4bpp sprite:
 *   attr0 = (y & 0xFF) | 0x0000        (square, 4bpp, normal mode)
 *   attr1 = (x & 0x1FF)                (no flip, size=00=8×8)
 *   attr2 = (tile & 0x3FF) | (pal << 12)
 */
void sprite_set(u8 id, s16 x, s16 y, u8 tile, u8 pal)
{
    u16 attr0 = (u16)((u8)y & 0xFFu);          /* bits 7-0: Y, rest 0 (square/4bpp/normal) */
    u16 attr1 = (u16)((u16)x & 0x1FFu);        /* bits 8-0: X, rest 0 (no flip, 8×8) */
    u16 attr2 = (u16)((u16)(tile & 0x3FFu) | ((u16)(pal & 0x0Fu) << 12));

    SHADOW_W0(id) = make_w0(attr0, attr1);
    SHADOW_W1(id) = make_w1(attr2);
}

/*
 * sprite_hide — move sprite id off-screen by setting attr0 = 0x0200.
 * This is the OBJ Disable bit (bit 9) while rotation is off (bit 8 = 0),
 * which instructs the GBA not to render the object.
 */
void sprite_hide(u8 id)
{
    SHADOW_W0(id) = make_w0(0x0200u, 0x0000u);
    SHADOW_W1(id) = make_w1(0x0000u);
}

/*
 * copy_oam — DMA3 transfer of the shadow buffer into OAM.
 *
 * Must be called during VBlank (after vsync()) for safe OAM write.
 * Transfers 256 u32 words (= 1 KB = all 128 OAM entries).
 *
 * DMA CNT register (32-bit combined write):
 *   Low 16 bits  = transfer count (in 32-bit units when DMA_32BIT set)
 *   High 16 bits = control flags
 *     bit 15: enable
 *     bit 10: 32-bit transfer (in high-word bit position = bit 26 of u32)
 */
void copy_oam(void)
{
    REG_DMA3SAD = (u32)oam_shadow;
    REG_DMA3DAD = (u32)MEM_OAM;
    /* Enable DMA3: 32-bit transfers, immediate, count = OAM_WORD_COUNT */
    REG_DMA3CNT = DMA_ENABLE | DMA_32BIT | OAM_WORD_COUNT;
}
