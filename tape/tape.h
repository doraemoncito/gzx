/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum tape
 *
 * Copyright (c) 1999-2018 Jiri Svoboda
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
 * @file Spectrum tape.
 *
 * This is an in-core, editable, representation of Spectrum tape. It should
 * be able to perfectly represent any TZX file.
 */

#ifndef TAPE_TAPE_H
#define TAPE_TAPE_H

#include <stdint.h>
#include "../types/tape/tape.h"

extern int tape_create(tape_t **);
extern void tape_destroy(tape_t *);
extern int tblock_archive_info_create(tblock_archive_info_t **);
extern void tblock_archive_info_destroy(tblock_archive_info_t *);
extern int tape_text_create(tape_text_t **);
extern void tape_text_destroy(tape_text_t *);
extern int tblock_unknown_create(tblock_unknown_t **);
extern void tblock_unknown_destroy(tblock_unknown_t *);

#endif