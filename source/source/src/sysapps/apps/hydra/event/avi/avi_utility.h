#ifndef _AVI_UTILITY_H_
#define _AVI_UTILITY_H_

int string_to_fcc(FOURCC* fcc, char* str);
int integer_to_fcc(FOURCC* fcc, DWORD integer);
void avi_setup_chunk_head(ChunkHead* chunkhead, char* name, size_t size);
void avi_setup_list_head(ListHead* listhead, char* name, size_t size);
int avi_setup_iovec(AVI_VEC* iov, void* base, size_t len);
int avi_set_data_vector(AVI_DATA_VEC* data, void* base, size_t len, int type);
int avi_append_data_vector(AVI_DATA_VEC* data, AVI_VEC* vec, size_t cnt, size_t len, int type);

#endif	// _AVI_UTILITY_H_

