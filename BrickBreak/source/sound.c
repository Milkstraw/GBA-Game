#include "sound.h"
#include "gba.h"

void sound_init(void) {
    REG_SOUNDCNT_X = 0x0080;  /* master enable */
    REG_SOUNDCNT_L = 0x3377;  /* CH1+CH2 max volume, both L+R outputs */
    REG_SOUND1CNT_L = 0;      /* no frequency sweep on CH1 */
}

/* CH1 square wave: ball/brick sounds
 * freq_rate: 0-2047 (Hz = 131072 / (2048 - rate))
 * vol: 0-15, decay: envelope step 1-7 (lower = longer note) */
static void ch1(int freq_rate, int vol, int decay) {
    REG_SOUND1CNT_H = (u16)((0x2 << 6)        |  /* 50% duty */
                             ((vol   & 0xF) << 12) |  /* initial volume */
                             (0      << 11)        |  /* envelope: decrease */
                             ((decay & 0x7) << 8));
    REG_SOUND1CNT_X = (u16)(0x8000 | (1 << 14) | (freq_rate & 0x7FF));
}

/* CH2 square wave: event sounds */
static void ch2(int freq_rate, int vol, int decay) {
    REG_SOUND2CNT_L = (u16)((0x2 << 6)        |
                             ((vol   & 0xF) << 12) |
                             (0      << 11)        |
                             ((decay & 0x7) << 8));
    REG_SOUND2CNT_H = (u16)(0x8000 | (1 << 14) | (freq_rate & 0x7FF));
}

void sound_ball_bounce(void)   { ch1(1500,  8, 3); }
void sound_brick_break(void)   { ch1(1900, 12, 4); }
void sound_powerup_catch(void) { ch2(1970, 15, 5); }
void sound_life_lost(void)     { ch2(1100, 15, 7); }
void sound_laser_fire(void)    { ch1(2010,  8, 2); }
void sound_level_clear(void)   { ch2(1980, 15, 3); }
