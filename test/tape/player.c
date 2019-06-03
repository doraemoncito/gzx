/*
 * GZX - George's ZX Spectrum Emulator
 * Tape tone generator unit tests
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
 * @file Tape tone generator unit tests.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../tape/player.h"
#include "../../tape/tape.h"
#include "player.h"

enum {
	tone_np = 3,
	pulses_np = 3
};

/** Check waveform generated by tape player matches template.
 *
 * @param player Tape player
 * @param delays Array of pulse lengths
 * @param num_pulses Number of pulses
 *
 * @return Zero on success, non-zero on mismatch or other failure
 */
static int test_check_waveform(tape_player_t *player, uint32_t *delays,
    int num_pulses)
{
	int i;
	uint32_t delay;
	tape_lvl_t lvl;
	tape_lvl_t tlvl;

	tlvl = tlvl_low;

	for (i = 0; i < num_pulses; i++) {
		if (tape_player_is_end(player)) {
			printf("Premature end of waveform.\n");
			return 1;
		}

		lvl = tape_player_cur_lvl(player);
		if (lvl != tlvl) {
			printf("Incorrect level (actual=%d expected=%d).\n",
			    lvl, tlvl);
			return 1;
		}

		tlvl = !tlvl;

		tape_player_get_next(player, &delay, &lvl);
		if (delay != delays[i]) {
			printf("Incorrect pulse length (%d != %d).\n", delay, delays[i]);
			return 1;
		}

		if (lvl != tlvl) {
			printf("Incorrect level (actual=%d expected=%d).\n",
			    lvl, tlvl);
			return 1;
		}
	}

	if (!tape_player_is_end(player)) {
		printf("Expected end of waveform not found.\n");
		return 1;
	}

	return 0;
}

/** Test tape player with tone block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_tone(void)
{
	tape_t *tape;
	tblock_tone_t *tone;
	tape_player_t *player;
	uint32_t delays[tone_np] = { 10, 10, 10 };
	int rc;

	printf("Test tape player with tone block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_tone_create(&tone);
	if (rc != 0)
		return 1;

	tone->num_pulses = 3;
	tone->pulse_len = 10;

	tape_append(tape, tone->block);

	rc = tape_player_create(tape_first(tape), &player);
	if (rc != 0)
		return 1;

	rc = test_check_waveform(player, delays, tone_np);
	if (rc != 0)
		return 1;

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Test tape player with pulses block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_pulses(void)
{
	tape_t *tape;
	tblock_pulses_t *pulses;
	tape_player_t *player;
	uint32_t delays[pulses_np] = { 10, 20, 30 };
	int rc;

	printf("Test tape player with pulses block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_pulses_create(&pulses);
	if (rc != 0)
		return 1;

	pulses->num_pulses = 3;
	pulses->pulse_len = calloc(3, sizeof(uint16_t));
	if (pulses->pulse_len == NULL)
		return 1;

	pulses->pulse_len[0] = 10;
	pulses->pulse_len[1] = 20;
	pulses->pulse_len[2] = 30;

	tape_append(tape, pulses->block);

	rc = tape_player_create(tape_first(tape), &player);
	if (rc != 0)
		return 1;

	rc = test_check_waveform(player, delays, pulses_np);
	if (rc != 0)
		return 1;

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Run tape player unit tests.
 *
 * @return Zero on success, non-zero on failure
 */
int test_tape_player(void)
{
	int rc;

	rc = test_tape_player_tone();
	if (rc != 0)
		return 1;

	rc = test_tape_player_pulses();
	if (rc != 0)
		return 1;

	return 0;
}