/*
 * GZX - George's ZX Spectrum Emulator
 * Next-gen tape backend
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

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "intdef.h"
#include "gzx.h"
#include "tape/tape.h"
#include "tape/tzx.h"
#include "zx_tape.h"
#include "zxt_fif.h"

static int ng_open_file(char *filename);
static int ng_close_file(void);
static int ng_rewind_file(void);

static int ng_block_type(void);
static int ng_get_b_data_info(tb_data_info_t *info);
static int ng_get_b_voice_info(tb_voice_info_t *info);

static int ng_skip_block(void);
static int ng_open_block(void);
static int ng_close_block(void);

static int ng_b_data_getbytes(int n, u8 *dst);
static int ng_b_voice_getsmps(int n, unsigned *dst);
static int ng_b_tones_gettone(int *pnum, int *plen);

static int ng_b_moredata(void);

tfr_t tfr_ng = {
	ng_open_file,
	ng_close_file,
	ng_rewind_file,
	ng_block_type,
	ng_get_b_data_info,
	ng_get_b_voice_info,
	ng_skip_block,
	ng_open_block,
	ng_close_block,
	ng_b_data_getbytes,
	ng_b_voice_getsmps,
	ng_b_tones_gettone,
	ng_b_moredata
};

static tape_t *tape;
static tape_block_t *tblock;

static int block_open;
static uint8_t *block_data;
static int block_type;
static int block_dlen;
static int block_doff;

//static u8 voice_byte;
//static int voice_bbits;

static int ng_open_file(char *filename)
{
	int rc;

	printf("ng_open_file\n");

	rc = tzx_tape_load(filename, &tape);
	if (rc != 0)
		return -1;

	tblock = tape_first(tape);
	block_open = 0;

	return 0;
}

static int ng_close_file(void)
{
	printf("ng_close_file\n");
	tape_destroy(tape);
	tape = NULL;
	return 0;
}

static int ng_rewind_file(void)
{
	printf("ng_rewind_file\n");
	tblock = tape_first(tape);
	block_open = 0;
	return 0;
}

static int ng_block_type(void)
{
	printf("ng_block_type\n");
	if (block_open) {
		printf("block_open->-1\n");
		return -1;
	}

	if (tblock == NULL) {
		printf("EOT\n");
		return BT_EOT;
	}

	fprintf(logfi, "ng_block_type (%d)\n", tblock->btype);

	switch (tblock->btype) {
	case tb_data:
	case tb_turbo_data:
	case tb_pure_data:
		printf("DATA\n");
		return BT_DATA;
	case tb_tone:
	case tb_pulses:
		printf("TONES\n");
		return BT_TONES;
	case tb_direct_rec:
		printf("VOICE\n");
		return BT_VOICE;
	default:
		printf("UNKNOWN\n");
		return BT_UNKNOWN;
	}
}

static int ng_get_b_data_info(tb_data_info_t *info)
{
	uint8_t bflag;
	tblock_data_t *data;
	tblock_turbo_data_t *tdata;
	tblock_pure_data_t *pdata;

	printf("ng_get_b_data_info\n");

	if (block_open)
		return -1;

	switch (tblock->btype) {
	case tb_data:
		/* Standard speed data block */
		printf("ng_get_b_data_info: std speed data block\n");
		data = (tblock_data_t *) tblock->ext;
		bflag = data->data[0];

		info->rom_timing = 1;
		info->has_leadin = 1;
		info->data_bytes = data->data_len;
		info->used_bits = 8;
		info->pause_after_len = data->pause_after * 3500;

		info->pilot_len = ROM_PILOT_LEN;
		info->sync1_len = ROM_SYNC1_LEN;
		info->sync2_len = ROM_SYNC2_LEN;
		info->zero_len = ROM_ZERO_LEN;
		info->one_len = ROM_ONE_LEN;
		info->pilot_pulses = (!bflag) ? ROM_PPULSES_H : ROM_PPULSES_D;
		break;

	case tb_turbo_data:
		/* Turbo loading data block */
		tdata = (tblock_turbo_data_t *) tblock->ext;

		info->pilot_len = tdata->pilot_len;
		info->sync1_len = tdata->sync1_len;
		info->sync2_len = tdata->sync2_len;
		info->zero_len = tdata->zero_len;
		info->one_len  = tdata->one_len;
		info->pilot_pulses = tdata->pilot_pulses;
		info->used_bits = tdata->lb_bits;
		info->rom_timing = 0;
		info->has_leadin = 1;
		info->data_bytes = tdata->data_len;
		info->pause_after_len = tdata->pause_after * 3500;
		break;

	case tb_pure_data:
		/* Pure data block */
		pdata = (tblock_pure_data_t *) tblock->ext;

		info->pilot_len = 0;
		info->sync1_len = 0;
		info->sync2_len = 0;
		info->zero_len = pdata->zero_len;
		info->one_len = pdata->one_len;
		info->pilot_pulses = 0;
		info->used_bits = pdata->lb_bits;
		info->rom_timing = 0;
		info->has_leadin = 0;
		info->data_bytes = pdata->data_len;
		info->pause_after_len = pdata->pause_after * 3500;
		break;

	default:
		return -1;
	}

	return 0;
}

static int ng_get_b_voice_info(tb_voice_info_t *info) {
	printf("ng_get_b_voice_info\n");
/*  unsigned bstart,btype;
  int used_bits,data_len;

  if(block_open) return -1;

  bstart=ftell(tapf);
  btype=fgetu8(tapf);

  if(btype!=0x15) {
    fseek(tapf,bstart,SEEK_SET);
    return -1;
  }

  fprintf(logfi,"ng_get_b_voice_info: bstart=%d btype=0x%02x\n",
    bstart,btype);

  info->smp_len=fgetu16le(tapf);
  info->pause_after_len=fgetu16le(tapf);
  used_bits=fgetu8(tapf);
  data_len=fgetu24le(tapf);
  info->samples=data_len*8 + (used_bits-8);

  fseek(tapf,bstart,SEEK_SET);
  return 0;*/
  return -1;
}

static int ng_skip_block(void)
{
	printf("ng_skip_block\n");
	if (block_open)
		return -1;
	if (tblock == NULL)
		return -1;

	tblock = tape_next(tblock);
	return 0;
}

static int ng_open_block(void)
{
	tblock_data_t *data;
	tblock_turbo_data_t *tdata;
	tblock_pure_data_t *pdata;

	printf("ng_open_block\n");
	if (block_open)
		return -1;

	switch (tblock->btype) {
	case tb_data:
		/* Standard speed data block */
		data = (tblock_data_t *) tblock->ext;
		block_data = data->data;
		block_type = BT_DATA;
		block_dlen = data->data_len;
		block_doff = 0;
		break;

	case tb_turbo_data: /* turbo loading data block */
		tdata = (tblock_turbo_data_t *) tblock->ext;
		block_data = tdata->data;
		block_type = BT_DATA;
		block_dlen = tdata->data_len;
		block_doff = 0;
		break;

//    case 0x12: /* pure tone */
//      block_dstart=block_start+1;
//      block_type=BT_TONES;
//      block_dlen =4;
//      break;

//    case 0x13: /* sequence of pulses */
//      block_dstart=block_start+2;
//      block_type=BT_TONES;

//      block_dlen =2*fgetu8(tapf);
//      break;

    case tb_pure_data:
	    /* Pure data block */
		pdata = (tblock_pure_data_t *) tblock->ext;
		block_data = pdata->data;
		block_type = BT_DATA;
		block_dlen = pdata->data_len;
		block_doff = 0;
		break;
//    case 0x15: /* direct recording */
//      block_dstart=block_start+9;
//      block_type=BT_VOICE;

//      fseek(tapf,5,SEEK_CUR);
//      block_dlen =fgetu24le(tapf);

//      voice_bbits=0;
//      break;

	default:
		return -1;
	}

	block_open = 1;
	return 0;
}

static int ng_close_block(void)
{
	printf("ng_close_block\n");
	if (!block_open)
		return -1;
	tblock = tape_next(tblock);
	block_open = 0;
	return 0;
}

static int ng_b_data_getbytes(int n, u8 *dst)
{
	size_t bleft;

	printf("ng_b_data_getbytes\n");

	if (!block_open)
		return -1;

	if (block_type != BT_DATA)
		return -1;

	bleft = block_dlen - block_doff;
	if (bleft < n)
		return -1;

	memcpy(dst, block_data + block_doff, n);
	block_doff += n;

	return 0;
}

static int ng_b_voice_getsmps(int n, unsigned *dst) {
/*  int smpleft;
  int i;

//  printf("ng_b_voice_getsmps\n");

  if(!block_open) return -1;
  if(block_type!=BT_VOICE) return -1;
    */
  /* zatim ignorujeme used_bits *//*
  smpleft=(block_end-ftell(tapf))*8 + voice_bbits;
  if(smpleft<n) return -1;

  for(i=0;i<n;i++) {
    if(!voice_bbits) {
      voice_byte=fgetu8(tapf);
      voice_bbits=8;
    }
    dst[i]=(voice_byte & 0x80) ? 1 : 0;
    voice_byte<<=1;
  }

  return 0;*/
  return -1;
}

static int ng_b_tones_gettone(int *pnum, int *plen) {
/*  unsigned pos;

  if(block_type!=BT_TONES) return -1;

  pos=ftell(tapf);
  if(pos>=block_end) return -1;

  switch(block_ng_type) {
     case 0x12:
       *plen=fgetu16le(tapf);
       *pnum=fgetu16le(tapf);
       break;

     case 0x13:
       *plen=fgetu16le(tapf);
       *pnum=1;
       break;
  }

  return 0;*/
  return -1;
}

static int ng_b_moredata(void)
{
	printf("ng_b_moredata\n");
	if (!block_open)
		return -1;

	assert(block_type == BT_DATA); // XXX Other block types
	return block_doff < block_dlen;
}