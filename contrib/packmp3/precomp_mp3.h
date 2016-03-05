#include "../packjpg/bitops.h" // for MBITS
#include "pmp3tbl.h"

// function to convert MP3 to PMP and vice versa
bool pmplib_convert_file2file( char* in, char* out, char* msg );

// this function writes versioninfo for the packMP3 DLL to a string
const char* pmplib_version_info( void );
