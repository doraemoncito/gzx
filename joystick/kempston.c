/*
 * GZX - George's ZX Spectrum Emulator
 * Kempston joystick interface
 *
 * Copyright (c) 1999-2019 Jiri Svoboda
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

/**
 * @file Kempston joystick interface.
 */

#include <stdint.h>
#include "kempston.h"

/** Initialize Kempston joystick.
 *
 * @param joy Kempston joystick
 */
void kempston_joy_init(kempston_joy_t *joy)
{
	joy->state = 0;
}

/** Read data from Kempston joystick port.
 *
 * @param joy Kempston joystick
 * @return Data from Kempston joystick port
 */
uint8_t kempston_joy_read(kempston_joy_t *joy)
{
	return joy->state;
}

/** Set or clear bits in Kempston joystick state.
 *
 * @param joy Kempston joystick
 * @param mask Mask of bits to set or clear
 * @param set @c true to set bits, @c false to clear bits
 */
void kempston_joy_set_reset(kempston_joy_t *joy, uint8_t mask, bool set)
{
	if (set)
		joy->state |= mask;
	else
		joy->state &= ~mask;
}
