#include "../packjpg/bitops.h" // for MBITS
#define PRECOMP_MP3 // to avoid compiler warnings (unused variables), use only a part of the header file
#include "pmp3tbl.h"
#undef PRECOMP_MP3

#define MP3_MAX_MEMORY_SIZE 64 * 1024 * 1024

// function to convert MP3 to PMP and vice versa, file to file
bool pmplib_convert_file2file( char* in, char* out, char* msg );

// functions to convert MP3 to PMP and vice versa, in memory
void pmplib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
bool pmplib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );

// this function writes versioninfo for the packMP3 DLL to a string
const char* pmplib_version_info( void );
