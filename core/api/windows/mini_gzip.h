#ifndef _MINI_GZIP_H_
#define _MINI_GZIP_H_

#define MAX_PATH_LEN		1024
#define	MINI_GZ_MIN(a, b)	((a) < (b) ? (a) : (b))
#ifdef __cplusplus
extern "C" {
#endif
struct mini_gzip {
  size_t		total_len;
  size_t		data_len;
  size_t		chunk_size;

  uint32_t	magic;
#define	MINI_GZIP_MAGIC	0xbeebb00b

  uint16_t	fcrc;
  uint16_t	fextra_len;

  uint8_t		*hdr_ptr;
  uint8_t		*fextra_ptr;
  uint8_t		*fname_ptr;
  uint8_t		*fcomment_ptr;

  uint8_t		*data_ptr;
  uint8_t		pad[3];
};

/* mini_gzip.c */
extern int	mini_gz_start(struct mini_gzip *gz_ptr, void *mem, size_t mem_len);
extern void	mini_gz_chunksize_set(struct mini_gzip *gz_ptr, int chunk_size);
extern void	mini_gz_init(struct mini_gzip *gz_ptr);
extern int	mini_gz_unpack(struct mini_gzip *gz_ptr, void **mem_out_ptr, size_t* mem_out_len_ptr);
#define	func_fprintf	fprintf
#define	func_fflush	fflush

#define	MINI_GZ_STREAM	stderr

#ifdef MINI_GZ_DEBUG
#define	GZAS(comp, ...)	do {						\
	if (!((comp))) {						\
		func_fprintf(MINI_GZ_STREAM, "Error: ");				\
		func_fprintf(MINI_GZ_STREAM, __VA_ARGS__);			\
		func_fprintf(MINI_GZ_STREAM, ", %s:%d\n", __func__, __LINE__);	\
		func_fflush(MINI_GZ_STREAM);					\
		exit(1);						\
	}								\
} while (0)

#define	GZDBG(...) do {					\
	func_fprintf(MINI_GZ_STREAM, "%s:%d ", __func__, __LINE__);	\
	func_fprintf(MINI_GZ_STREAM, __VA_ARGS__);			\
	func_fprintf(MINI_GZ_STREAM, "\n");				\
} while (0)
#else	/* MINI_GZ_DEBUG */
#define	GZAS(comp, ...)	
#define	GZDBG(...)
#endif
#ifdef __cplusplus
}
#endif

#endif
