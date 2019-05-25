/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup riff
 * @{
 */
/**
 * @file RIFF chunk.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "chunk.h"
#include "../byteorder.h"
#include "../minmax.h"
#include "../types/chunk.h"

/** Open RIFF file for writing
 *
 * @param fname File name
 * @param rrw   Place to store pointer to RIFF writer
 *
 * @return 0 on success, ENOMEM if out of memory, EIO if failed to open
 *         file.
 */
int riff_wopen(const char *fname, riffw_t **rrw)
{
	riffw_t *rw;

	rw = calloc(1, sizeof(riffw_t));
	if (rw == NULL)
		return ENOMEM;

	rw->f = fopen(fname, "wb");
	if (rw->f == NULL) {
		free(rw);
		return EIO;
	}

	*rrw = rw;
	return 0;
}

/** Close RIFF for writing.
 *
 * @param rw RIFF writer
 * @return 0 on success. On write error EIO is returned and RIFF writer
 *         is destroyed anyway.
 */
int riff_wclose(riffw_t *rw)
{
	int rv;

	rv = fclose(rw->f);
	free(rw);

	return (rv == 0) ? 0 : EIO;
}

/** Write uint32_t value into RIFF file
 *
 * @param rw RIFF writer
 * @param v  Value
 * @return 0 on success, EIO on error.
 */
int riff_write_uint32(riffw_t *rw, uint32_t v)
{
	uint32_t vle;

	vle = host2uint32_t_le(v);
	if (fwrite(&vle, 1, sizeof(vle), rw->f) < sizeof(vle))
		return EIO;

	return 0;
}

/** Begin writing chunk.
 *
 * @param rw     RIFF writer
 * @param ckid   Chunk ID
 * @param wchunk Pointer to chunk structure to fill in
 *
 * @return 0 on success, EIO on write error
 */
int riff_wchunk_start(riffw_t *rw, riff_ckid_t ckid, riff_wchunk_t *wchunk)
{
	long pos;
	int rc;

	pos = ftell(rw->f);
	if (pos < 0)
		return EIO;

	wchunk->ckstart = pos;

	rc = riff_write_uint32(rw, ckid);
	if (rc != 0) {
		assert(rc == EIO);
		return EIO;
	}

	rc = riff_write_uint32(rw, 0);
	if (rc != 0) {
		assert(rc == EIO);
		return EIO;
	}

	return 0;
}

/** Finish writing chunk.
 *
 * @param rw     RIFF writer
 * @param wchunk Pointer to chunk structure
 *
 * @return 0 on success, EIO error.
 */
int riff_wchunk_end(riffw_t *rw, riff_wchunk_t *wchunk)
{
	long pos;
	long cksize;
	int rc;

	pos = ftell(rw->f);
	if (pos < 0)
		return EIO;

	cksize = pos - wchunk->ckstart - 8;

	if (fseek(rw->f, wchunk->ckstart + 4, SEEK_SET) < 0)
		return EIO;

	rc = riff_write_uint32(rw, cksize);
	if (rc != 0) {
		assert(rc == EIO);
		return EIO;
	}

	if (fseek(rw->f, pos, SEEK_SET) < 0)
		return EIO;

	return 0;
}

/** Write data into RIFF file.
 *
 * @param rw    RIFF writer
 * @param data  Pointer to data
 * @param bytes Number of bytes to write
 *
 * @return 0 on success, EIO on error.
 */
int riff_wchunk_write(riffw_t *rw, void *data, size_t bytes)
{
	size_t nw;

	nw = fwrite(data, 1, bytes, rw->f);
	if (nw != bytes)
		return EIO;

	return 0;
}

/** Open RIFF file for reading.
 *
 * @param fname File name
 * @param rrr   Place to store pointer to RIFF reader
 *
 * @return 0 on success, ENOMEM if out of memory, EIO if failed to open
 *         file.
 */
int riff_ropen(const char *fname, riffr_t **rrr)
{
	riffr_t *rr;
	int rc;

	rr = calloc(1, sizeof(riffr_t));
	if (rr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rr->f = fopen(fname, "rb");
	if (rr->f == NULL) {
		rc = EIO;
		goto error;
	}

	*rrr = rr;
	return 0;
error:
	if (rr != NULL && rr->f != NULL)
		fclose(rr->f);
	free(rr);
	return rc;
}

/** Close RIFF for reading.
 *
 * @param rr RIFF reader
 * @return 0 on success, EIO on error.
 */
int riff_rclose(riffr_t *rr)
{
	int rc;

	rc = fclose(rr->f);
	free(rr);
	return rc == 0 ? 0 : EIO;
}

/** Read uint32_t from RIFF file.
 *
 * @param rr RIFF reader
 * @param v  Place to store value
 * @return 0 on success, EIO on error.
 */
int riff_read_uint32(riffr_t *rr, uint32_t *v)
{
	uint32_t vle;

	if (fread(&vle, 1, sizeof(vle), rr->f) < sizeof(vle))
		return EIO;

	*v = uint32_t_le2host(vle);
	return 0;
}

/** Start reading RIFF chunk.
 *
 * @param rr     RIFF reader
 * @param rchunk Pointer to chunk structure to fill in
 *
 * @return 0 on success, EIO on error.
 */
int riff_rchunk_start(riffr_t *rr, riff_rchunk_t *rchunk)
{
	int rc;
	long pos;

	pos = ftell(rr->f);
	if (pos < 0) {
		rc = EIO;
		goto error;
	}

	rchunk->ckstart = pos;
	rc = riff_read_uint32(rr, &rchunk->ckid);
	if (rc != 0)
		goto error;
	rc = riff_read_uint32(rr, &rchunk->cksize);
	if (rc != 0)
		goto error;

	return 0;
error:
	return rc;
}

/** Return file offset where chunk ends
 *
 * @param rchunk RIFF chunk
 * @return File offset just after last data byte of the chunk
 */
static long riff_rchunk_get_end(riff_rchunk_t *rchunk)
{
	return rchunk->ckstart + 8 + rchunk->cksize;
}

/** Return file offset of first (non-padding) byte after end of chunk.
 *
 * @param rchunk RIFF chunk
 * @return File offset of first non-padding byte after end of chunk
 */
static long riff_rchunk_get_ndpos(riff_rchunk_t *rchunk)
{
	long ckend;

	ckend = riff_rchunk_get_end(rchunk);
	if ((ckend % 2) != 0)
		return ckend + 1;
	else
		return ckend;
}

/** Finish reading RIFF chunk.
 *
 * Seek to the first byte after end of chunk.
 *
 * @param rr     RIFF reader
 * @param rchunk Chunk structure
 * @return 0 on success, EIO on error.
 */
int riff_rchunk_end(riffr_t *rr, riff_rchunk_t *rchunk)
{
	long ckend;

	ckend = riff_rchunk_get_ndpos(rchunk);
	if (fseek(rr->f, ckend, SEEK_SET) < 0)
		return EIO;

	return 0;
}

/** Read data from RIFF chunk.
 *
 * Attempt to read @a bytes bytes from the chunk. If there is less data
 * left until the end of the chunk, less will be read. The actual number
 * of bytes read is returned in @a *nbytes (can even be 0).
 *
 * @param rr RIFF reader
 * @param rchunk RIFF chunk
 * @param buf Buffer to read to
 * @param bytes Number of bytes to read
 * @param nread Place to store number of bytes actually read
 *
 * @return 0 on success, EDOM if file position is not within @a rchunk,
 *         EIO on I/O error.
 */
int riff_rchunk_read(riffr_t *rr, riff_rchunk_t *rchunk, void *buf,
    size_t bytes, size_t *nread)
{
	long pos;
	long ckend;
	long toread;

	pos = ftell(rr->f);
	if (pos < 0)
		return EIO;

	ckend = riff_rchunk_get_end(rchunk);
	if (pos < rchunk->ckstart || pos > ckend)
		return EDOM;

	toread = min(bytes, (size_t)(ckend - pos));
	if (toread == 0) {
		*nread = 0;
		return 0;
	}

	*nread = fread(buf, 1, toread, rr->f);
	if (*nread == 0)
		return EIO;

	return 0;
}

/** @}
 */
