/*
 * GZX - George's ZX Spectrum Emulator
 * TZX file format support
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
 * @file TZX file format support
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../adt/list.h"
#include "../types/tape/tzx.h"
#include "../byteorder.h"
#include "tape.h"
#include "tzx.h"

const char *tzx_signature = "ZXTape!";

/** Validate TZX header.
 *
 * @return Zero if header is valid or error code
 */
static int tzx_header_validate(tzx_header_t *header)
{
	if (memcmp(header->signature, tzx_signature,
	    sizeof(header->signature)) != 0) {
		return EINVAL;
	}

	if (header->eof_mark != 0x1a)
		return EINVAL;

	if (header->major != 1)
		return EINVAL;

	return 0;
}

/** Load standard speed data block.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_data(FILE *f, tape_t *tape)
{
	printf("load data block\n");
	return 0;
}

/** Load text structure.
 *
 * This is part of archive info.
 *
 * @param f File to read from
 * @param bremain Pointer to variable holding number of bytes remaining
 *                in archive info block. Will be updated on success.
 * @param rtext Place to store pointer to new tape text
 */
static int tzx_load_text(FILE *f, size_t *bremain, tape_text_t **rtext)
{
	tape_text_t *text = NULL;
	tzx_text_t tzxtext;
	size_t nread;
	int rc;

	printf("load archive info\n");

	if (*bremain < sizeof(tzx_text_t)) {
		rc = EIO;
		goto error;
	}

	nread = fread(&tzxtext, 1, sizeof(tzx_text_t), f);
	if (nread != sizeof(tzx_text_t)) {
		rc = EIO;
		goto error;
	}

	printf("text type:0x%x length=%u\n", tzxtext.text_type,
	    tzxtext.text_len);
	*bremain -= nread;

	rc = tape_text_create(&text);
	if (rc != 0)
		goto error;

	text->text = calloc(tzxtext.text_len + 1, 1);
	if (text->text == NULL) {
		rc = ENOMEM;
		goto error;
	}

	if (*bremain < tzxtext.text_len) {
		rc = EIO;
		goto error;
	}

	nread = fread(text->text, 1, tzxtext.text_len, f);
	if (nread != tzxtext.text_len)
		goto error;

	*bremain -= nread;

	text->text[tzxtext.text_len] = '\0';
	text->text_type = tzxtext.text_type;

	*rtext = text;
	return 0;
error:
	if (text != NULL)
		tape_text_destroy(text);
	return rc;
}

/** Load archive info.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_archive_info(FILE *f, tape_t *tape)
{
	tzx_block_archive_info_t block;
	tblock_archive_info_t *ainfo;
	tape_text_t *ttext = NULL;
	size_t nread;
	size_t bremain;
	uint16_t blen;
	uint8_t i;
	int rc;

	printf("load archive info\n");
	nread = fread(&block, 1, sizeof(tzx_block_archive_info_t), f);
	if (nread != sizeof(tzx_block_archive_info_t))
		return EIO;

	blen = uint16_t_le2host(block.block_len);
	printf("block size:%zu\n", (size_t)blen);
	printf("size of block header:%zu\n", sizeof(tzx_block_archive_info_t));
	printf("# of strings:%zu\n", (size_t)block.nstrings);

	rc = tblock_archive_info_create(&ainfo);
	if (rc != 0)
		goto error;

	bremain = blen;

	for (i = 0; i < block.nstrings; i++) {
		printf("loading string %d\n", i);
		rc = tzx_load_text(f, &bremain, &ttext);
		if (rc != 0)
			goto error;

		printf("text is %d/'%s'\n", ttext->text_type, ttext->text);
		ttext->ainfo = ainfo;
		list_append(&ttext->lainfo, &ainfo->texts);
	}

	return 0;
error:
	if (ainfo != NULL)
		tblock_archive_info_destroy(ainfo);
	return rc;
}

/** Load unknown block conforming to the extension rule.
 *
 * @param f File to read from
 * @param btype Unkown block type
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_unknown(FILE *f, uint8_t btype, tape_t *tape)
{
	tzx_block_unknown_t block;
	tblock_unknown_t *unknown;
	size_t nread;
	uint32_t blen;
	void *data;
	int rc;

	printf("load unknown block (%02x)\n", btype);
	nread = fread(&block, 1, sizeof(tzx_block_unknown_t), f);
	if (nread != sizeof(tzx_block_unknown_t))
		return EIO;

	blen = uint32_t_le2host(block.block_len);
	printf("block size:%zu\n", (size_t)blen);
	data = malloc(blen);
	if (data == NULL)
		return ENOMEM;

	nread = fread(data, 1, blen, f);
	printf("bytes read: %zu\n", nread);
	if (nread != blen) {
		rc = EIO;
		goto error;
	}

	rc = tblock_unknown_create(&unknown);
	if (rc != 0)
		goto error;

	unknown->block->btype = btype;
	unknown->data = data;
	unknown->data_len = blen;

	return 0;
error:
	free(data);
	return rc;
}

/** Load tape from TZX file.
 *
 * @param fname File name
 * @param rtape Place to store pointer to loaded tape
 * @return Zero on success or error code
 */
int tzx_tape_load(const char *fname, tape_t **rtape)
{
	FILE *f;
	tzx_header_t header;
	size_t nread;
	uint8_t btype;
	tape_t *tape = NULL;
	int rc;

	f = fopen(fname, "rb");
	if (f == NULL)
		return ENOENT;

	rc = tape_create(&tape);
	if (rc != 0)
		goto error;

	/* Read TZX header */
	nread = fread(&header, 1, sizeof(tzx_header_t), f);
	if (nread != sizeof(tzx_header_t)) {
		rc = EIO;
		goto error;
	}

	printf("validate header\n");
	rc = tzx_header_validate(&header);
	if (rc != 0) {
		rc = EIO;
		goto error;
	}

	printf("read blocks\n");
	while (!feof(f)) {
		printf("read block type\n");
		/* Read block type */
		nread = fread(&btype, 1, sizeof(uint8_t), f);
		if (nread != sizeof(uint8_t)) {
			rc = EIO;
			goto error;
		}

		printf("process block\n");
		switch (btype) {
		case tzxb_data:
			rc = tzx_load_data(f, tape);
			break;
		case tzxb_archive_info:
			rc = tzx_load_archive_info(f, tape);
			break;
		default:
			rc = tzx_load_unknown(f, btype, tape);
			break;
		}

		if (rc != 0)
			goto error;
	}

	fclose(f);
	return 0;
error:
	if (tape != NULL)
		tape_destroy(tape);
	fclose(f);
	return rc;
}

/** Save tape to TZX file.
 *
 * @param tape Tape
 * @param fname File name
 * @return Zero on success or error code
 */
int tzx_tape_save(tape_t *tape, const char *fname)
{
	return 0;
}