/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum Screen
 *
 * Copyright (c) 1999-2017 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef ZX_SCR_H
#define ZX_SCR_H

#include "types/video/display.h"
#include "types/video/ula.h"

extern video_ula_t video_ula;
extern video_area_t video_out_area;

extern int zx_scr_init(unsigned long);
extern void zx_scr_reset(void);
extern int zx_scr_init_spec256_pal(void);
extern int zx_scr_load_bg(const char *, int);
extern void zx_scr_prev_bg(void);
extern void zx_scr_next_bg(void);
extern void zx_scr_clear_bg(void);
extern void zx_scr_mode(int mode);
extern void zx_scr_update_pal(void);
extern unsigned long zx_scr_get_clock(void);
extern int zx_scr_set_area(video_area_t);

extern void (*zx_scr_disp)(void);
extern void (*zx_scr_disp_fast)(void);

#endif
