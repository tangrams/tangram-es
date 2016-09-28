/*
 * BSD 2-clause license
 * Copyright (c) 2013 Wojciech A. Koszek <wkoszek@FreeBSD.org>
 * 
 * Based on:
 * 
 * https://github.com/strake/gzip.git
 *
 * I had to rewrite it, since strake's version was powered by UNIX FILE* API,
 * while the key objective was to perform memory-to-memory operations
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#ifdef MINI_GZ_DEBUG
#include <stdio.h>
#endif
#include "miniz.h"
#include "mini_gzip.h"

int
mini_gz_start(struct mini_gzip *gz_ptr, void *mem, size_t mem_len)
{
	uint8_t		*hptr, *hauxptr, *mem8_ptr;
	uint16_t	fextra_len;

	assert(gz_ptr != NULL);

	mem8_ptr = (uint8_t *)mem;
	hptr = mem8_ptr + 0;		// .gz header
	hauxptr = mem8_ptr + 10;	// auxillary header

	gz_ptr->hdr_ptr = hptr;
	gz_ptr->data_ptr = 0;
	gz_ptr->data_len = 0;
	gz_ptr->total_len = mem_len;
	gz_ptr->chunk_size = 1024;

	if (hptr[0] != 0x1F || hptr[1] != 0x8B) {
		GZDBG("hptr[0] = %02x hptr[1] = %02x\n", hptr[0], hptr[1]);
		return (-1);
	}
	if (hptr[2] != 8) {
		return (-2);
	}
	if (hptr[3] & 0x4) {
		fextra_len = hauxptr[1] << 8 | hauxptr[0];
		gz_ptr->fextra_len = fextra_len;
		hauxptr += 2;
		gz_ptr->fextra_ptr = hauxptr;
	}
	if (hptr[3] & 0x8) {
		gz_ptr->fname_ptr = hauxptr;
		while (*hauxptr != '\0') {
			hauxptr++;
		}
		hauxptr++;
	}
	if (hptr[3] & 0x10) {
		gz_ptr->fcomment_ptr = hauxptr;
		while (*hauxptr != '\0') {
			hauxptr++;
		}
		hauxptr++;
	}
	if (hptr[3] & 0x2) /* FCRC */ {
		gz_ptr->fcrc = (*(uint16_t *)hauxptr);
		hauxptr += 2;
	}
	gz_ptr->data_ptr = hauxptr;
	gz_ptr->data_len = mem_len - (hauxptr - hptr);
	gz_ptr->magic = MINI_GZIP_MAGIC;
	return (0);
}

void
mini_gz_chunksize_set(struct mini_gzip *gz_ptr, int chunk_size)
{

	assert(gz_ptr != 0);
	assert(gz_ptr->magic == MINI_GZIP_MAGIC);
	gz_ptr->chunk_size = chunk_size;
}

void
mini_gz_init(struct mini_gzip *gz_ptr)
{

	memset(gz_ptr, 0xffffffff, sizeof(*gz_ptr));
	gz_ptr->magic = MINI_GZIP_MAGIC;
	mini_gz_chunksize_set(gz_ptr, 1024);
}


int
mini_gz_unpack(struct mini_gzip *gz_ptr, void **mem_out_ptr, size_t* mem_out_len_ptr)
{
  unsigned char* mem_out = NULL;
  z_stream s;
	int	ret;
  size_t bytes_to_read;
  size_t in_bytes_avail;
	assert(gz_ptr != 0);
	assert(gz_ptr->data_len > 0);
	assert(gz_ptr->magic == MINI_GZIP_MAGIC);

	memset (&s, 0, sizeof (z_stream));
	inflateInit2(&s, -MZ_DEFAULT_WINDOW_BITS);
	in_bytes_avail = gz_ptr->data_len;
	s.next_in = gz_ptr->data_ptr;

  s.avail_out = 1024;
  s.next_out = mem_out = (unsigned char*)malloc(s.avail_out);
	for (;;) {
		bytes_to_read = MINI_GZ_MIN(gz_ptr->chunk_size, in_bytes_avail);
		s.avail_in += bytes_to_read;
		ret = mz_inflate(&s, MZ_SYNC_FLUSH);
		in_bytes_avail -= bytes_to_read;
		if (s.avail_out == 0 && in_bytes_avail != 0) {
      s.avail_out = s.total_out;
      mem_out = (unsigned char*)realloc(mem_out, s.total_out * 2);
      s.next_out = mem_out + s.total_out;
		}
		assert(ret != MZ_BUF_ERROR);
		if (ret == MZ_PARAM_ERROR) {
      free(mem_out);
			return (-1);
		}
		if (ret == MZ_DATA_ERROR) {
      free(mem_out);
      return (-2);
		}
		if (ret == MZ_STREAM_END) {
			break;
		}
	}
	ret = inflateEnd(&s);
	if (ret != Z_OK) {
    free(mem_out);
		return (-4);
	}
  *mem_out_ptr = mem_out;
  *mem_out_len_ptr = s.total_out;
  return 0;
}
