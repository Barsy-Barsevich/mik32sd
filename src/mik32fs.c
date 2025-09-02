// #include "mik32fs.h"

// static uint32_t __buffer_length = 512;
// #ifndef MIK32FS_USE_MALLOC
// static void *__free_space_area_startaddr = NULL;
// #endif
// static MIK32FAT_Descriptor_TypeDef *__fs = NULL;
// static MIK32SD_Descriptor_TypeDef *__disk = NULL;

// void mik32fs_set_disk(MIK32SD_Descriptor_TypeDef *disk)
// {
//     __disk = disk;
// }

// void mik32fs_set_fs(MIK32FAT_Descriptor_TypeDef *fs)
// {
//     __fs = fs;
// }

// FILE *mik32fs_fopen(const char *__restrict _name, const char  _type)
// {
//     if (__fs == NULL || __disk == NULL)
//     {
//         return NULL;
//     }

//     FILE *file;
//     MIK32FAT_File_TypeDef *file_desc;
//     char *buffer;
    
//     #ifdef MIK32FS_USE_MALLOC
//     file = (FILE*)malloc(sizeof(FILE));
//     if (file == NULL)
//     {
//         return NULL;
//     }
//     file_desc = (MIK32FAT_File_TypeDef*)malloc(sizeof(MIK32FAT_File_TypeDef));
//     if (file_desc == NULL)
//     {
//         free(file);
//         return NULL;
//     }
//     buffer = (char*)malloc(sizeof(char) * __buffer_length);
//     if (buffer == NULL)
//     {
//         free(file);
//         free(file_desc);
//         return NULL;
//     }
//     #else
//     if (__free_space_area_startaddr == NULL)
//     {
//         return NULL;
//     }
//     file = (FILE*)__free_space_area_startaddr;
//     file_desc = (MIK32FAT_File_TypeDef*)(__free_space_area_startaddr + sizeof(FILE));
//     buffer = (char*)(__free_space_area_startaddr + sizeof(FILE) + sizeof(MIK32FAT_File_TypeDef));
//     #endif

//     file->_bf._base = (unsigned char*)buffer;
//     file->_bf._size = __buffer_length;
//     file->_write = mik32_fs_write;
//     file->_read = mik32_fs_read;
//     file->_close = mik32_fs_close;
//     file->_seek = mik32_fs_seek;
//     // file->_blksize = 
//     file->_p = file->_bf._base + 0;
//     file->_r = 0; /* read space left for getc() */
//     file->_w = 0; /* write space left for putc() */
//     // file->_flags = 0;		/* flags, below; this FILE is free if 0 */
//     file->_file = -1;
//     file->_lbfsize = -file->_bf._size;
//     file->_cookie = (void*)file_desc;

//     MIK32FAT_Status_TypeDef res;
//     res = mik32fat_file_open(file_desc, __fs, _name, _type);
//     if (res != MIK32FAT_STATUS_OK)
//     {
//         #ifdef MIK32FS_USE_MALLOC
//         free(file);
//         free(file_desc);
//         free(buffer);
//         #endif
//         return NULL;
//     }
//     return file;
// }


// int mik32_fs_write(void *__reent, void *_cookie, const char *src, int len)
// {
//     if (_cookie == NULL)
//     {
//         return -1;
//     }
//     if (src == NULL)
//     {
//         return -1;
//     }
//     if (len == 0)
//     {
//         return 0;
//     }
//     MIK32FAT_File_TypeDef *file_desk = (MIK32FAT_File_TypeDef*)_cookie;
//     MIK32FAT_Status_TypeDef res;
//     res = mik32fat_write_file(file_desk, src, len);
//     return (int)res;
// }


// int mik32_fs_read(void *__reent, void *_cookie, char *dst, int len)
// {
//     if (_cookie == NULL)
//     {
//         return -1;
//     }
//     if (dst == NULL)
//     {
//         return -1;
//     }
//     if (len == 0)
//     {
//         return 0;
//     }
//     MIK32FAT_File_TypeDef *file_desk = (MIK32FAT_File_TypeDef*)_cookie;
//     MIK32FAT_Status_TypeDef res;
//     res = mik32fat_read_file(file_desk, dst, len);
//     return (int)res;
// }

// int mik32_fs_close(void *__reent, void *_cookie)
// {
//     if (_cookie == NULL)
//     {
//         return -1;
//     }
//     MIK32FAT_File_TypeDef *file_desk = (MIK32FAT_File_TypeDef*)_cookie;
//     MIK32FAT_Status_TypeDef res;
//     res = mik32fat_file_close(file_desk);
//     return (int)res;
// }

// long mik32_fs_seek(void *__reent, long, int)
// {
//     return 0;
// }