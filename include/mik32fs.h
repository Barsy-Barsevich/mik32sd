#pragma once

#include "mik32fat.h"
#ifdef MIK32FS_USE_MALLOC
#include <stdlib.h>
#endif

/*
 * fopen, fprintf, fscanf, fgetc, fputc, fclose,
 *
 */

// FILE *	fopen (const char *__restrict _name, const char *__restrict _type);
// int	fclose (FILE *);
// int	fprintf (FILE *__restrict, const char *__restrict, ...);
// int	fscanf (FILE *__restrict, const char *__restrict, ...);

void mik32fs_set_disk(MIK32SD_Descriptor_TypeDef *disk);
void mik32fs_set_fs(MIK32FAT_Descriptor_TypeDef *fs);
FILE *mik32fs_fopen(const char *__restrict _name, const char  _type);
int mik32_fs_write(void *__reent, void *_cookie, const char *src, int len);
int mik32_fs_read(void *__reent, void *_cookie, char *dst, int len);
int mik32_fs_close(void *__reent, void *_cookie);
long mik32_fs_seek(void *__reent, long, int);