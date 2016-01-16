#include <stdlib.h>
#include <string.h>
#include <ctime>

#include "pmp3tbl.h"
#include "pmp3bitlen.h"
#include "../packjpg/bitops.h"
#include "../packjpg/aricoder.h"
#include "huffmp3.h"
#include "huffmp3tbl.h"

#if defined BUILD_DLL // define BUILD_LIB from the compiler options if you want to compile a DLL!
	#define BUILD_LIB
#endif

#if defined BUILD_LIB // define BUILD_LIB from the compiler options if you want to compile a library!
	#include "packmp3lib.h"
#endif

#define INTERN static

#define INIT_MODEL_S(a,b,c) new model_s( a, b, c, 511 )
#define INIT_MODEL_B(a,b)   new model_b( a, b, 511 )

#define ABS(v1)			( (v1 < 0) ? -v1 : v1 )
#define ABSDIFF(v1,v2)	( (v1 > v2) ? (v1 - v2) : (v2 - v1) )
#define ROUND_F(v1)		( (v1 < 0) ? (int) (v1 - 0.5) : (int) (v1 + 0.5) )
#define CLAMPED(l,h,v)	( ( v < l ) ? l : ( v > h ) ? h : v )

#define MEM_ERRMSG	"out of memory error"
#define FRD_ERRMSG	"could not read file / file not found"
#define FWR_ERRMSG	"could not write file / file write-protected"
#define MSG_SIZE	128
#define BARLEN		36

// special realloc with guaranteed free() of previous memory
static inline void* frealloc( void* ptr, size_t size ) {
	void* n_ptr = realloc( ptr, size );
	if ( n_ptr == NULL ) free( ptr );
	return n_ptr;
}


	
/* -----------------------------------------------
	function declarations: main interface
	----------------------------------------------- */
#if !defined( BUILD_LIB )
INTERN void initialize_options( int argc, char** argv );
INTERN void process_ui( void );
INTERN inline const char* get_status( bool (*function)() );
INTERN void show_help( void );
#endif
INTERN void process_file( void );
INTERN void execute( bool (*function)() );


/* -----------------------------------------------
	function declarations: main functions
	----------------------------------------------- */
#if !defined( BUILD_LIB )
INTERN bool check_file( void );
INTERN bool swap_streams( void );
INTERN bool compare_output( void );
#endif
INTERN bool reset_buffers( void );
INTERN bool read_mp3( void );
INTERN bool write_mp3( void );
INTERN bool analyze_frames( void );
INTERN bool compress_mp3( void );
INTERN bool uncompress_pmp( void );


/* -----------------------------------------------
	function declarations: MP3-specific
	----------------------------------------------- */

INTERN inline mp3Frame* mp3_read_frame( unsigned char* data, int max_size );
INTERN inline mp3Frame* mp3_build_frame( void );
INTERN inline bool mp3_append_frame( mp3Frame* frame );
INTERN inline bool mp3_discard_frame( mp3Frame* frame );
INTERN inline bool mp3_mute_frame( mp3Frame* frame );
INTERN inline bool mp3_unmute_frame( mp3Frame* frame );
INTERN inline unsigned char* mp3_build_fixed( mp3Frame* frame );
INTERN inline int mp3_seek_firstframe( unsigned char* data, int size );
INTERN inline int mp3_get_id3_size( unsigned char* id3tag, int max_size );
INTERN inline unsigned short mp3_calc_layer3_crc( unsigned char* header, unsigned char* sideinfo, int sidesize );
INTERN inline granuleData*** mp3_decode_frame( huffman_reader* dec, mp3Frame* frame );


/* -----------------------------------------------
	function declarations: PMP-specific
	----------------------------------------------- */
	
INTERN inline bool pmp_write_header( iostream* str );
INTERN inline bool pmp_read_header( iostream* str );
#if !defined( STORE_ID3 )
INTERN inline bool pmp_encode_id3( aricoder* enc );
INTERN inline bool pmp_decode_id3( aricoder* dec );
#else
INTERN inline int pmp_store_data( iostream* str, unsigned char* data, int size );
INTERN inline int pmp_unstore_data( iostream* str, unsigned char** data );
#endif
INTERN inline bool pmp_encode_padding( aricoder* enc );
INTERN inline bool pmp_decode_padding( aricoder* dec );
INTERN inline bool pmp_encode_block_types( aricoder* enc );
INTERN inline bool pmp_decode_block_types( aricoder* dec );
INTERN inline bool pmp_decode_stereo_ms( aricoder* dec );
INTERN inline bool pmp_encode_global_gain( aricoder* enc );
INTERN inline bool pmp_decode_global_gain( aricoder* dec );
INTERN inline bool pmp_encode_slength( aricoder* enc );
INTERN inline bool pmp_decode_slength( aricoder* dec );
INTERN inline bool pmp_encode_stereo_ms( aricoder* enc );
INTERN inline bool pmp_decode_stereo_ms( aricoder* dec );
INTERN inline bool pmp_encode_region_data( aricoder* enc );
INTERN inline bool pmp_decode_region_data( aricoder* dec );
INTERN inline bool pmp_encode_sharing( aricoder* enc );
INTERN inline bool pmp_decode_sharing( aricoder* dec );
INTERN inline bool pmp_encode_preemphasis( aricoder* enc );
INTERN inline bool pmp_decode_preemphasis( aricoder* dec );
INTERN inline bool pmp_encode_coarse_sf( aricoder* enc );
INTERN inline bool pmp_decode_coarse_sf( aricoder* dec );
INTERN inline bool pmp_encode_subblock_gain( aricoder* enc );
INTERN inline bool pmp_decode_subblock_gain( aricoder* dec );
INTERN inline bool pmp_encode_main_data( aricoder* enc );
INTERN inline bool pmp_decode_main_data( aricoder* dec );
INTERN inline bool pmp_store_unmute_data( iostream* str );
INTERN inline bool pmp_unstore_unmute_data( iostream* str );
INTERN inline bool pmp_build_context( void );
INTERN inline unsigned char* pmp_predict_lame_anc( int nbits, unsigned char* ref );


/* -----------------------------------------------
	function declarations: miscelaneous helpers
	----------------------------------------------- */
#if !defined( BUILD_LIB )
INTERN inline void progress_bar( int current, int last );
INTERN inline char* create_filename( const char* base, const char* extension );
INTERN inline char* unique_filename( const char* base, const char* extension );
INTERN inline void set_extension( const char* filename, const char* extension );
INTERN inline void add_underscore( char* filename );
#endif
INTERN inline bool file_exists( const char* filename );


/* -----------------------------------------------
	function declarations: developers functions
	----------------------------------------------- */

// these are developers functions, they are not needed
// in any way to compress MP3 or decompress PMP
#if !defined(BUILD_LIB) && defined(DEV_BUILD)
INTERN bool write_file( const char* base, const char* ext, void* data, int bpv, int size );
INTERN bool write_errfile( void );
INTERN bool write_file_analysis( void );
INTERN bool write_block_analysis( void );
INTERN bool write_stat_analysis( void );
INTERN bool visualize_headers( void );
INTERN bool visualize_decoded_data( void );
INTERN bool dump_main_sizes( void );
INTERN bool dump_aux_sizes( void );
INTERN bool dump_bitrates( void );
INTERN bool dump_stereo_ms( void );
INTERN bool dump_padding( void );
INTERN bool dump_main_data_bits( void );
INTERN bool dump_big_value_ns( void );
INTERN bool dump_global_gain( void );
INTERN bool dump_slength( void );
INTERN bool dump_block_types( void );
INTERN bool dump_sharing( void );
INTERN bool dump_preemphasis( void );
INTERN bool dump_coarse( void );
INTERN bool dump_htable_sel( void );
INTERN bool dump_region_sizes( void );
INTERN bool dump_subblock_gains( void );
INTERN bool dump_data_files( void );
INTERN bool dump_gg_ctx( void );
INTERN bool dump_decoded_data( void );
#endif


/* -----------------------------------------------
	global variables: library only variables
	----------------------------------------------- */
#if defined(BUILD_LIB)
INTERN int lib_in_type  = -1;
INTERN int lib_out_type = -1;
#endif


/* -----------------------------------------------
	global variables: data storage
	----------------------------------------------- */

INTERN mp3Frame*      firstframe		=	NULL;	// first physical frame
INTERN mp3Frame*      lastframe		=	NULL;	// last physical frame
INTERN unsigned char* main_data		=	NULL;	// (mainly) huffman coded data
INTERN unsigned char* data_before		=	NULL;	// data before (should be ID3v2 tag)
INTERN unsigned char* data_after		=	NULL;	// data after (should be ID3v1 or ID3v2 tag)
INTERN unsigned char* unmute_data		=	NULL;	// fix data (to reverse muted frames)
INTERN int            main_data_size	=     0 ;	// size of main data
INTERN int            data_before_size =     0 ;	// size of data before
INTERN int            data_after_size  =     0 ;   // size of data after
INTERN int            unmute_data_size =     0 ;   // size of fix data
INTERN int            n_bad_first      =     0 ;   // # of bad first frames (should be zero!)
INTERN unsigned char* gg_context[2]	= {NULL};	// universal context based on global gain

/* -----------------------------------------------
	global variables: info about audio file
	----------------------------------------------- */

INTERN int  g_nframes     =   0;  // number of frames
INTERN int  g_nchannels   =   0;  // number of channels
INTERN int  g_samplerate  =   0;  // sample rate
INTERN int  g_bitrate     =   0;  // bit rate - global or zero for vbr


/* -----------------------------------------------
	global variables: frame analysis info
	----------------------------------------------- */

INTERN char i_mpeg			= -1; // mpeg - non changing
INTERN char i_layer			= -1; // layer - non changing
INTERN char i_samplerate	= -1; // sample rate - non changing
INTERN char i_bitrate		= -1; // bit rate - value or -1 (variable)
INTERN char i_protection	= -1; // checksum - for all (1), none (0) or some (-1) frames
INTERN char i_padding		= -1; // padding - for all (1), none (0) or some (-1) frames
INTERN char i_privbit		= -1; // private bit - value or -1 (variable)
INTERN char i_channels		= -1; // channel mode - non changing
INTERN char i_stereo_ms		= -1; // ms stereo - for all (1), none (0) or some (-1) frames
INTERN char i_stereo_int	= -1; // int stereo - for all (1), none (0) or some (-1) frames
INTERN char i_copyright		= -1; // copyright bit - value or -1 (variable)
INTERN char i_original		= -1; // original bit - value or -1 (variable)
INTERN char i_emphasis		= -1; // emphasis - value or -1 (variable)
INTERN char i_padbits		= -1; // side info padding bits - value or -1 (variable)
INTERN char i_bit_res		= -1; // bit reservoir - is used (1) or not used (0)
INTERN char i_share			= -1; // scalefactor sharing - is used (1) or not used (0)
INTERN char i_sblocks		= -1; // special blocks - are used (1) or not used (0)
INTERN char i_mixed			= -1; // mixed blocks - are used (1) or not used (0)
INTERN char i_preemphasis	= -1; // preemphasis - value or -1 (variable)
INTERN char i_coarse		= -1; // coarse scalefactors - value or -1 (variable)
INTERN char i_sbgain		= -1; // subblock gain - used properly (1), not used (0) or used for non-short (-1)
INTERN char i_aux_h			= -1; // auxiliary data handling - none (0), at begin and end (1), between frames (-1)
INTERN char i_sb_diff		= -1; // special blocks diffs between ch0 and ch1 - none (0) or some (-1)
	

/* -----------------------------------------------
	global variables: info about files
	----------------------------------------------- */
	
INTERN char*  mp3filename = NULL;	// name of MP3 file
INTERN char*  pmpfilename = NULL;	// name of PMP file
INTERN int    mp3filesize;			// size of MP3 file
INTERN int    pmpfilesize;			// size of PMP file
INTERN int    filetype;				// type of current file
INTERN iostream* str_in  = NULL;	// input stream
INTERN iostream* str_out = NULL;	// output stream

#if !defined(BUILD_LIB)
INTERN iostream* str_str = NULL;	// storage stream

INTERN char** filelist = NULL;		// list of files to process 
INTERN int    file_cnt = 0;			// count of files in list
INTERN int    file_no  = 0;			// number of current file

INTERN char** err_list = NULL;		// list of error messages 
INTERN int*   err_tp   = NULL;		// list of error types
#endif


/* -----------------------------------------------
	global variables: messages
	----------------------------------------------- */

INTERN char errormessage [ 128 ];
INTERN bool (*errorfunction)();
INTERN int  errorlevel;
// meaning of errorlevel:
// -1 -> wrong input
// 0 -> no error
// 1 -> warning
// 2 -> fatal error


/* -----------------------------------------------
	global variables: settings
	----------------------------------------------- */

#if !defined( BUILD_LIB )
INTERN int  verbosity  = -1;		// level of verbosity
INTERN bool overwrite  = false;		// overwrite files yes / no
INTERN bool wait_exit  = true;		// pause after finished yes / no
INTERN int  verify_lv  = 0;			// verification level ( none (0), simple (1), detailed output (2) )
INTERN int  err_tol    = 1;			// error threshold ( proceed on warnings yes (2) / no (1) )

INTERN bool developer  = false;		// allow developers functions yes/no
INTERN int  action     = A_COMPRESS;// what to do with MP3/PMP files

INTERN FILE*  msgout   = stdout;	// stream for output of messages
INTERN bool   pipe_on  = false;		// use stdin/stdout instead of filelist
#else
INTERN int  err_tol    = 1;			// error threshold ( proceed on warnings yes (2) / no (1) )
INTERN int  action     = A_COMPRESS;// what to do with MP3/PMP files
#endif



/* -----------------------------------------------
	global variables: info about program
	----------------------------------------------- */

INTERN const unsigned char appversion = 10;
INTERN const char*  subversion   = "f";
INTERN const char*  apptitle     = "packMP3";
INTERN const char*  appname      = "packMP3";
INTERN const char*  versiondate  = "11/21/2014";
INTERN const char*  author       = "Matthias Stirner";
#if !defined( BUILD_LIB )
INTERN const char*  website      = "http://packjpg.encode.ru/";
INTERN const char*  email        = "packjpg@htw-aalen.de";
INTERN const char*	copyright    = "2010-2014 Ratisbon University & Matthias Stirner";
INTERN const char*  pmp_ext      = "pmp";
INTERN const char*  mp3_ext      = "mp3";
#endif
INTERN const char   pmp_magic[] = { 'M', 'S' };


/* -----------------------------------------------
	main-function
	----------------------------------------------- */

#if !defined(BUILD_LIB)
int main( int argc, char** argv )
{	
	sprintf( errormessage, "no errormessage specified" );
	
	clock_t begin, end;
	
	int error_cnt = 0;
	int warn_cnt  = 0;
	
	double acc_mp3size = 0;
	double acc_pmpsize = 0;
	
	int kbps;
	double cr;
	double total;
	
	errorlevel = 0;
	
	
	// read options from command line
	initialize_options( argc, argv );
	
	// write program info to screen
	fprintf( msgout,  "\n--> %s v%i.%i%s (%s) by %s <--\n",
			apptitle, appversion / 10, appversion % 10, subversion, versiondate, author );
	fprintf( msgout, "Copyright %s\nAll rights reserved\n\n", copyright );
	
	// check if user input is wrong, show help screen if it is
	if ( ( file_cnt == 0 ) ||
		( ( !developer ) && ( (action != A_COMPRESS) || (verify_lv > 1) ) ) ) {
		show_help();
		return -1;
	}
	
	// (re)set program has to be done first
	reset_buffers();
	
	// process file(s) - this is the main function routine
	begin = clock();
	for ( file_no = 0; file_no < file_cnt; file_no++ ) {	
		// process current file
		process_ui();
		// store error message and type if any
		if ( errorlevel > 0 ) {
			err_list[ file_no ] = (char*) calloc( MSG_SIZE, sizeof( char ) );
			err_tp[ file_no ] = errorlevel;
			if ( err_list[ file_no ] != NULL )
				strcpy( err_list[ file_no ], errormessage );
		}
		// count errors / warnings / file sizes
		if ( errorlevel >= err_tol ) error_cnt++;
		else {
			if ( errorlevel == 1 ) warn_cnt++;
			acc_mp3size += mp3filesize;
			acc_pmpsize += pmpfilesize;
		}
	}
	end = clock();
	
	// errors summary: only needed for -v2 or progress bar
	if ( ( verbosity == -1 ) || ( verbosity == 2 ) ) {
		// print summary of errors to screen
		if ( error_cnt > 0 ) {
			fprintf( stderr, "\n\nfiles with errors:\n" );
			fprintf( stderr, "------------------\n" );
			for ( file_no = 0; file_no < file_cnt; file_no++ ) {
				if ( err_tp[ file_no ] >= err_tol ) {
					fprintf( stderr, "%s (%s)\n", filelist[ file_no ], err_list[ file_no ] );
				}
			}
		}
		// print summary of warnings to screen
		if ( warn_cnt > 0 ) {
			fprintf( stderr, "\n\nfiles with warnings:\n" );
			fprintf( stderr, "------------------\n" );
			for ( file_no = 0; file_no < file_cnt; file_no++ ) {
				if ( err_tp[ file_no ] == 1 ) {
					fprintf( stderr, "%s (%s)\n", filelist[ file_no ], err_list[ file_no ] );
				}
			}
		}
	}
	
	// show statistics
	fprintf( msgout,  "\n\n-> %i file(s) processed, %i error(s), %i warning(s)\n",
		file_cnt, error_cnt, warn_cnt );
	if ( ( file_cnt > error_cnt ) && ( verbosity != 0 ) && ( action == A_COMPRESS ) ) {
		acc_mp3size /= 1024.0; acc_pmpsize /= 1024.0;
		total = (double) ( end - begin ) / CLOCKS_PER_SEC; 
		kbps  = ( total > 0 ) ? ( acc_mp3size / total ) : acc_mp3size;
		cr    = ( acc_mp3size > 0 ) ? ( 100.0 * acc_pmpsize / acc_mp3size ) : 0;
		
		fprintf( msgout,  " -------------------------------- \n" );
		if ( total >= 0 ) {
			fprintf( msgout,  " total time        : %8.2f sec\n", total );
			fprintf( msgout,  " avrg. kbyte per s : %8i kbps\n", kbps );
		}
		else {
			fprintf( msgout,  " total time        : %8s sec\n", "N/A" );
			fprintf( msgout,  " avrg. kbyte per s : %8s kbps\n", "N/A" );
		}
		fprintf( msgout,  " avrg. comp. ratio : %8.2f %%\n", cr );		
		fprintf( msgout,  " -------------------------------- \n" );
	}
	
	// pause before exit
	if ( wait_exit && ( msgout != stderr ) ) {
		fprintf( msgout, "\n\n< press ENTER >\n" );
		fgetc( stdin );
	}
	
	
	return 0;
}
#endif

/* ----------------------- Begin of library only functions -------------------------- */

/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT bool pmplib_convert_stream2stream( char* msg )
{
	// process in main function
	return pmplib_convert_stream2mem( NULL, NULL, msg ); 
}
#endif


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */

#if defined(BUILD_LIB)
EXPORT bool pmplib_convert_file2file( char* in, char* out, char* msg )
{
	// init streams
	pmplib_init_streams( (void*) in, 0, 0, (void*) out, 0 );
	
	// process in main function
	return pmplib_convert_stream2mem( NULL, NULL, msg ); 
}
#endif


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT bool pmplib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg )
{
	clock_t begin, end;
	int total;
	float cr;	
	
	
	// (re)set buffers
	reset_buffers();
	action = A_COMPRESS;
	
	// main compression / decompression routines
	begin = clock();
	
	// process one file
	process_file();
	
	// fetch pointer and size of output (only for memory output)
	if ( ( errorlevel < err_tol ) && ( lib_out_type == 1 ) &&
		 ( out_file != NULL ) && ( out_size != NULL ) ) {
		*out_size = str_out->getsize();
		*out_file = str_out->getptr();
	}
	
	// close iostreams
	if ( str_in  != NULL ) delete( str_in  ); str_in  = NULL;
	if ( str_out != NULL ) delete( str_out ); str_out = NULL;
	
	end = clock();
	
	// copy errormessage / remove files if error (and output is file)
	if ( errorlevel >= err_tol ) {
		if ( lib_out_type == 0 ) {
			if ( filetype == F_MP3 ) {
				if ( file_exists( pmpfilename ) ) remove( pmpfilename );
			} else if ( filetype == F_PMP ) {
				if ( file_exists( mp3filename ) ) remove( mp3filename );
			}
		}
		if ( msg != NULL ) strcpy( msg, errormessage );
		return false;
	}
	
	// get compression info
	total = (int) ( (double) (( end - begin ) * 1000) / CLOCKS_PER_SEC );
	cr    = ( mp3filesize > 0 ) ? ( 100.0 * pmpfilesize / mp3filesize ) : 0;
	
	// write success message else
	if ( msg != NULL ) {
		switch( filetype )
		{
			case F_MP3:
				sprintf( msg, "Compressed to %s (%.2f%%) in %ims",
					pmpfilename, cr, ( total >= 0 ) ? total : -1 );
				break;
			case F_PMP:
				sprintf( msg, "Decompressed to %s (%.2f%%) in %ims",
					mp3filename, cr, ( total >= 0 ) ? total : -1 );
				break;
			case F_UNK:
				sprintf( msg, "Unknown filetype" );
				break;	
		}
	}
	
	
	return true;
}
#endif


/* -----------------------------------------------
	DLL export init input (file/mem)
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT void pmplib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type )
{
	/* a short reminder about input/output stream types:
	
	if input is file
	----------------
	in_scr -> name of input file
	in_type -> 0
	in_size -> ignore
	
	if input is memory
	------------------
	in_scr -> array containg data
	in_type -> 1
	in_size -> size of data array
	
	if input is *FILE (f.e. stdin)
	------------------------------
	in_src -> stream pointer
	in_type -> 2
	in_size -> ignore
	
	vice versa for output streams! */
	
	unsigned char buffer[ 2 ];
	
	
	// (re)set errorlevel
	errorfunction = NULL;
	errorlevel = 0;
	mp3filesize = 0;
	pmpfilesize = 0;
	
	// open input stream, check for errors
	str_in = new iostream( in_src, in_type, in_size, 0 );
	if ( str_in->chkerr() ) {
		sprintf( errormessage, "error opening input stream" );
		errorlevel = 2;
		return;
	}	
	
	// open output stream, check for errors
	str_out = new iostream( out_dest, out_type, 0, 1 );
	if ( str_out->chkerr() ) {
		sprintf( errormessage, "error opening output stream" );
		errorlevel = 2;
		return;
	}
	
	// free memory from filenames if needed
	if ( mp3filename != NULL ) free( mp3filename ); mp3filename = NULL;
	if ( pmpfilename != NULL ) free( pmpfilename ); pmpfilename = NULL;
	
	// check input stream
	str_in->read( buffer, 1, 2 );
	str_in->rewind();
	if ( (buffer[0] == pmp_magic[0]) && (buffer[1] == pmp_magic[1]) ) {
		// file is PMP
		filetype = F_PMP;
		// copy filenames
		pmpfilename = (char*) calloc( (  in_type == 0 ) ? strlen( (char*) in_src   ) + 1 : 32, sizeof( char ) );
		mp3filename = (char*) calloc( ( out_type == 0 ) ? strlen( (char*) out_dest ) + 1 : 32, sizeof( char ) );
		strcpy( pmpfilename, (  in_type == 0 ) ? (char*) in_src   : "PMP in memory" );
		strcpy( mp3filename, ( out_type == 0 ) ? (char*) out_dest : "MP3 in memory" );
	} else {
		// file is MP3
		filetype = F_MP3;
		// copy filenames
		mp3filename = (char*) calloc( (  in_type == 0 ) ? strlen( (char*) in_src   ) + 1 : 32, sizeof( char ) );
		pmpfilename = (char*) calloc( ( out_type == 0 ) ? strlen( (char*) out_dest ) + 1 : 32, sizeof( char ) );
		strcpy( mp3filename, (  in_type == 0 ) ? (char*) in_src   : "MP3 in memory" );
		strcpy( pmpfilename, ( out_type == 0 ) ? (char*) out_dest : "PMP in memory" );
	}
	
	// store types of in-/output
	lib_in_type  = in_type;
	lib_out_type = out_type;
}
#endif


/* -----------------------------------------------
	DLL export version information
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT const char* pmplib_version_info( void )
{
	static char v_info[ 256 ];
	
	// copy version info to string
	sprintf( v_info, "--> %s library v%i.%i%s (%s) by %s <--",
			apptitle, appversion / 10, appversion % 10, subversion, versiondate, author );
			
	return (const char*) v_info;
}
#endif


/* -----------------------------------------------
	DLL export version information
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT const char* pmplib_short_name( void )
{
	static char v_name[ 256 ];
	
	// copy version info to string
	sprintf( v_name, "%s v%i.%i%s",
			apptitle, appversion / 10, appversion % 10, subversion );
			
	return (const char*) v_name;
}
#endif

/* ----------------------- End of libary only functions -------------------------- */

/* ----------------------- Begin of main interface functions -------------------------- */


/* -----------------------------------------------
	reads in commandline arguments
	----------------------------------------------- */
	
#if !defined(BUILD_LIB)	
INTERN void initialize_options( int argc, char** argv )
{	
	int tmp_val;
	char** tmp_flp;
	int i;
	
	
	// get memory for filelist & preset with NULL
	filelist = (char**) calloc( argc, sizeof( char* ) );
	for ( i = 0; i < argc; i++ )
		filelist[ i ] = NULL;
	
	// preset temporary filelist pointer
	tmp_flp = filelist;
	
	
	// read in arguments
	while ( --argc > 0 ) {
		argv++;
		// switches begin with '-'
		if ( strcmp((*argv), "-p" ) == 0 ) {
			err_tol = 2;
		}
		else if ( strcmp((*argv), "-ver" ) == 0 ) {
			verify_lv = ( verify_lv < 1 ) ? 1 : verify_lv;
		}
		else if ( sscanf( (*argv), "-v%i", &tmp_val ) == 1 ){
			verbosity = tmp_val;
			verbosity = ( verbosity < 0 ) ? 0 : verbosity;
			verbosity = ( verbosity > 2 ) ? 2 : verbosity;			
		}
		else if ( strcmp((*argv), "-vp" ) == 0 ) {
			verbosity = -1;
		}
		else if ( strcmp((*argv), "-np" ) == 0 ) {
			wait_exit = false;
		}
		else if ( strcmp((*argv), "-o" ) == 0 ) {
			overwrite = true;
		}
		#if defined(DEV_BUILD)
		else if ( strcmp((*argv), "-dev") == 0 ) {
			developer = true;
		}
		else if ( strcmp((*argv), "-test") == 0 ) {
			verify_lv = 2;
		}
		else if ( strcmp((*argv), "-san") == 0 ) {
			if ( file_exists( STAT_ANALYSIS_CSV ) )
				remove( STAT_ANALYSIS_CSV );
			action = A_STATS_ANALYSIS;
		}
		else if ( strcmp((*argv), "-fan") == 0 ) {
			if ( file_exists( FILE_ANALYSIS_CSV ) )
				remove( FILE_ANALYSIS_CSV );
			action = A_FILE_ANALYSIS;
		}
		else if ( strcmp((*argv), "-ban") == 0 ) {
			action = A_BLOCK_ANALYSIS;
		}
		else if ( strcmp((*argv), "-dmp") == 0 ) {
			action = A_DUMP_SEPERATE;
		}
		else if ( strcmp((*argv), "-pgm") == 0 ) {
			action = A_PGM_INFO;
		}
	   	else if ( ( strcmp((*argv), "-comp") == 0) ) {
			action = A_COMPRESS;
		}
		#endif
		else if ( strcmp((*argv), "-") == 0 ) {
			// switch standard message out stream
			msgout = stderr;
			// use "-" as placeholder for stdin
			*(tmp_flp++) = (char*) "-";
		}
		else {
			// if argument is not switch, it's a filename
			*(tmp_flp++) = *argv;
		}		
	}
	
	// count number of files (or filenames) in filelist
	for ( file_cnt = 0; filelist[ file_cnt ] != NULL; file_cnt++ );
	
	// alloc arrays for error messages and types storage
	err_list = (char**) calloc( file_cnt, sizeof( char* ) );
	err_tp   = (int*) calloc( file_cnt, sizeof( int ) );
}
#endif


/* -----------------------------------------------
	UI for processing one file
	----------------------------------------------- */
	
#if !defined(BUILD_LIB)
INTERN void process_ui( void )
{
	clock_t begin, end;
	const char* actionmsg  = NULL;
	const char* errtypemsg = NULL;
	int total, bpms;
	float cr;	
	
	
	errorfunction = NULL;
	errorlevel = 0;
	mp3filesize = 0;
	pmpfilesize = 0;	
	#if !defined(DEV_BUILD)
	action = A_COMPRESS;
	#endif
	
	// compare file name, set pipe if needed
	if ( ( strcmp( filelist[ file_no ], "-" ) == 0 ) && ( action == A_COMPRESS ) ) {
		pipe_on = true;
		filelist[ file_no ] = (char*) "STDIN";
	}
	else {		
		pipe_on = false;
	}
	
	if ( verbosity >= 0 ) { // standard UI
		fprintf( msgout,  "\nProcessing file %i of %i \"%s\" -> ",
					file_no + 1, file_cnt, filelist[ file_no ] );
		
		if ( verbosity > 1 )
			fprintf( msgout,  "\n----------------------------------------" );
		
		// check input file and determine filetype
		execute( check_file );
		
		// get specific action message
		if ( filetype == F_UNK ) actionmsg = "unknown filetype";
		else switch ( action ) {
			case A_COMPRESS: ( filetype == F_MP3 ) ? actionmsg = "Compressing" : actionmsg = "Decompressing";
				break;
			case A_DUMP_SEPERATE: actionmsg = "Dumping binary data"; break;
			case A_PGM_INFO: actionmsg = "Writing PGM"; break;
			case A_FILE_ANALYSIS: actionmsg = "Analysing files"; break;
			case A_BLOCK_ANALYSIS: actionmsg = "Analysing frames"; break;
			case A_STATS_ANALYSIS: actionmsg = "Analysing statistics"; break;
		}
		
		if ( verbosity < 2 ) fprintf( msgout, "%s -> ", actionmsg );
	}
	else { // progress bar UI
		// update progress message
		fprintf( msgout, "Processing file %2i of %2i ", file_no + 1, file_cnt );
		progress_bar( file_no, file_cnt );
		fprintf( msgout, "\r" );
		execute( check_file );
	}
	fflush( msgout );
	
	
	// main function routine
	begin = clock();
	
	// streams are initiated, start processing file
	process_file();
	
	// close iostreams
	if ( str_in  != NULL ) delete( str_in  ); str_in  = NULL;
	if ( str_out != NULL ) delete( str_out ); str_out = NULL;
	if ( str_str != NULL ) delete( str_str ); str_str = NULL;
	// delete if broken or if output not needed
	if ( ( !pipe_on ) && ( ( errorlevel >= err_tol ) || ( action != A_COMPRESS ) ) ) {
		if ( filetype == F_MP3 ) {
			if ( file_exists( pmpfilename ) ) remove( pmpfilename );
		} else if ( filetype == F_PMP ) {
			if ( file_exists( mp3filename ) ) remove( mp3filename );
		}
	}
	
	end = clock();	
	
	// speed and compression ratio calculation
	total = (int) ( (double) (( end - begin ) * 1000) / CLOCKS_PER_SEC );
	bpms  = ( total > 0 ) ? ( mp3filesize / total ) : mp3filesize;
	cr    = ( mp3filesize > 0 ) ? ( 100.0 * pmpfilesize / mp3filesize ) : 0;

	
	if ( verbosity >= 0 ) { // standard UI
		if ( verbosity > 1 )
			fprintf( msgout,  "\n----------------------------------------" );
		
		// display success/failure message
		switch ( verbosity ) {
			case 0:			
				if ( errorlevel < err_tol ) {
					if ( action == A_COMPRESS ) fprintf( msgout,  "%.2f%%", cr );
					else fprintf( msgout, "DONE" );
				}
				else fprintf( msgout,  "ERROR" );
				if ( errorlevel > 0 ) fprintf( msgout,  "\n" );
				break;
			
			case 1:
				fprintf( msgout, "%s\n",  ( errorlevel < err_tol ) ? "DONE" : "ERROR" );
				break;
			
			case 2:
				if ( errorlevel < err_tol ) fprintf( msgout,  "\n-> %s OK\n", actionmsg );
				else  fprintf( msgout,  "\n-> %s ERROR\n", actionmsg );
				break;
		}
		
		// set type of error message
		switch ( errorlevel ) {
			case 0:	errtypemsg = "none"; break;
			case 1: ( err_tol > 1 ) ?  errtypemsg = "warning (ignored)" : errtypemsg = "warning (skipped file)"; break;
			case 2: errtypemsg = "fatal error"; break;
		}
		
		// error/ warning message
		if ( errorlevel > 0 ) {
			fprintf( msgout, " %s -> %s:\n", get_status( errorfunction ), errtypemsg  );
			fprintf( msgout, " %s\n", errormessage );
		}
		if ( (verbosity > 0) && (errorlevel < err_tol) && (action == A_COMPRESS) ) {
			if ( total >= 0 ) {
				fprintf( msgout,  " time taken  : %7i msec\n", total );
				fprintf( msgout,  " byte per ms : %7i byte\n", bpms );
			}
			else {
				fprintf( msgout,  " time taken  : %7s msec\n", "N/A" );
				fprintf( msgout,  " byte per ms : %7s byte\n", "N/A" );
			}
			fprintf( msgout,  " comp. ratio : %7.2f %%\n", cr );		
		}	
		if ( ( verbosity > 1 ) && ( action == A_COMPRESS ) )
			fprintf( msgout,  "\n" );
	}
	else { // progress bar UI
		// if this is the last file, update progress bar one last time
		if ( file_no + 1 == file_cnt ) {
			// update progress message
			fprintf( msgout, "Processed %2i of %2i files ", file_no + 1, file_cnt );
			progress_bar( 1, 1 );
			fprintf( msgout, "\r" );
		}	
	}
}
#endif


/* -----------------------------------------------
	gets statusmessage for function
	----------------------------------------------- */
	
#if !defined(BUILD_LIB)
INTERN inline const char* get_status( bool (*function)() )
{	
	if ( function == NULL ) {
		return "unknown action";
	} else if ( function == *check_file ) {
		return "Determining filetype";
	} else if ( function == *read_mp3 ) {
		return "Reading MP3";
	} else if ( function == *write_mp3 ) {
		return "Writing MP3";
	} else if ( function == *analyze_frames ) {
		return "Analysing frames";
	} else if ( function == *compress_mp3 ) {
		return "Compressing to PMP";
	} else if ( function == *uncompress_pmp ) {
		return "Uncompressing PMP";
	} else if ( function == *swap_streams ) {
		return "Swapping input/output streams";
	} else if ( function == *compare_output ) {
		return "Verifying output stream";
	} else if ( function == *reset_buffers ) {
		return "Resetting program";
	}
	#if defined(DEV_BUILD)
	else if ( function == *write_file_analysis ) {
		return "Writing file analysis to csv";
	} else if ( function == *write_block_analysis ) {
		return "Writing block analysis to csv";
	} else if ( function == *write_stat_analysis ) {
		return "Writing statistic analysis to csv";
	} else if ( function == *visualize_headers ) {
		return "Writing binary info PGM";
	} else if ( function == *visualize_decoded_data ) {
		return "Writing decoded data PGMs";
	} else if ( function == *dump_main_sizes ) {
		return "Dumping main data sizes";
	} else if ( function == *dump_aux_sizes ) {
		return "Dumping aux data sizes";
	} else if ( function == *dump_bitrates ) {
		return "Dumping bitrates";
	} else if ( function == *dump_stereo_ms ) {
		return "Dumping MS stereo bits";
	} else if ( function == *dump_padding ) {
		return "Dumping padding bits";
	} else if ( function == *dump_main_data_bits ) {
		return "Dumping main data bits";
	} else if ( function == *dump_big_value_ns ) {
		return "Dumping big value pair #s";
	} else if ( function == *dump_global_gain ) {
		return "Dumping global gain";
	} else if ( function == *dump_slength ) {
		return "Dumping slength values";
	} else if ( function == *dump_block_types ) {
		return "Dumping block types";
	} else if ( function == *dump_sharing ) {
		return "Dumping sharing bits";
	} else if ( function == *dump_preemphasis ) {
		return "Dumping preemphasis setting";
	} else if ( function == *dump_coarse ) {
		return "Dumping coarse scf settings";
	} else if ( function == *dump_htable_sel ) {
		return "Dumping hufftable selections";
	} else if ( function == *dump_region_sizes ) {
		return "Dumping region sizes";
	} else if ( function == *dump_subblock_gains ) {
		return "Dumping subblock gain";
	} else if ( function == *dump_data_files ) {
		return "Dumping data files";
	} else if ( function == *dump_gg_ctx ) {
		return "Dumping global gain context";
	} else if ( function == *dump_decoded_data ) {
		return "Dumping decoded data";
	}
	#endif
	else {
		return "Function description missing!";
	}
}
#endif


/* -----------------------------------------------
	shows help in case of wrong input
	----------------------------------------------- */
	
#if !defined(BUILD_LIB)
INTERN void show_help( void )
{	
	fprintf( msgout, "\n" );
	fprintf( msgout, "Website: %s\n", website );
	fprintf( msgout, "Email  : %s\n", email );
	fprintf( msgout, "\n" );
	fprintf( msgout, "Usage: %s [switches] [filename(s)]", appname );
	fprintf( msgout, "\n" );
	fprintf( msgout, "\n" );
	fprintf( msgout, " [-ver]   verify files after processing\n" );
	fprintf( msgout, " [-v?]    set level of verbosity (max: 2) (def: 0)\n" );
	fprintf( msgout, " [-np]    no pause after processing files\n" );
	fprintf( msgout, " [-o]     overwrite existing files\n" );
	fprintf( msgout, " [-p]     proceed on warnings\n" );
	#if defined(DEV_BUILD)
	if ( developer ) {
	fprintf( msgout, "\n" );
	fprintf( msgout, " [-fan]   write files analysis to CSV file\n" );
	fprintf( msgout, " [-ban]   write block analysis to CSV file\n" );
	fprintf( msgout, " [-san]   write stats analysis to CSV file\n" );
	fprintf( msgout, " [-pgm]   visualize data as PGM image\n" );
	fprintf( msgout, " [-dmp]   dump data to several binary files\n" );
	}
	#endif
	fprintf( msgout, "\n" );
	fprintf( msgout, "Examples: \"%s -v1 -o luka.%s\"\n", appname, pmp_ext );
	fprintf( msgout, "          \"%s -p *.%s\"\n", appname, mp3_ext );	
}
#endif


/* -----------------------------------------------
	processes one file
	----------------------------------------------- */

INTERN void process_file( void )
{	
	if ( filetype == F_MP3 ) {
		switch ( action ) {
			case A_COMPRESS:
				execute( read_mp3 );
				execute( analyze_frames );
				execute( compress_mp3 );
				#if !defined(BUILD_LIB)	
				if ( verify_lv > 0 ) { // verifcation
					execute( reset_buffers );
					execute( swap_streams );
					execute( uncompress_pmp );
					execute( write_mp3 );
					execute( compare_output );
				}
				#endif
				break;
				
			#if !defined(BUILD_LIB) && defined(DEV_BUILD)
			case A_DUMP_SEPERATE:
				execute( read_mp3 );
				execute( analyze_frames );
				execute( dump_main_sizes );
				execute( dump_aux_sizes );
				execute( dump_main_data_bits );
				execute( dump_big_value_ns );
				execute( dump_global_gain );
				execute( dump_bitrates );
				execute( dump_htable_sel );
				execute( dump_region_sizes );
				execute( dump_slength );
				execute( dump_stereo_ms );
				execute( dump_padding );
				execute( dump_block_types );
				execute( dump_sharing );
				execute( dump_preemphasis );
				execute( dump_coarse );
				execute( dump_subblock_gains );
				execute( dump_data_files );
				execute( dump_gg_ctx );
				execute( dump_decoded_data );
				break;
			
			case A_PGM_INFO:
				execute( read_mp3 );
				execute( analyze_frames );
				execute( visualize_headers );
				execute( visualize_decoded_data );
				break;
				
			case A_FILE_ANALYSIS:
				execute( read_mp3 );
				execute( analyze_frames );
				execute( write_file_analysis );
				break;
			
			case A_BLOCK_ANALYSIS:
				execute( read_mp3 );
				execute( analyze_frames );
				execute( write_block_analysis );
				break;
			
			case A_STATS_ANALYSIS:
				execute( read_mp3 );
				execute( analyze_frames );
				execute( write_stat_analysis );
				break;
			#else
			default:
				break;
			#endif
		}
	}
	else if ( filetype == F_PMP )	{
		switch ( action )
		{
			case A_COMPRESS:
				execute( uncompress_pmp );
				execute( write_mp3 );
				#if !defined(BUILD_LIB)
				if ( verify_lv > 0 ) { // verify
					execute( reset_buffers );
					execute( swap_streams );
					execute( read_mp3 );
					execute( analyze_frames );
					execute( compress_mp3 );
					execute( compare_output );
				}
				#endif
				break;
				
			#if !defined(BUILD_LIB) && defined(DEV_BUILD)
			case A_DUMP_SEPERATE:
				execute( uncompress_pmp );
				execute( dump_main_sizes );
				execute( dump_aux_sizes );
				execute( dump_main_data_bits );
				execute( dump_big_value_ns );
				execute( dump_global_gain );
				execute( dump_bitrates );
				execute( dump_htable_sel );
				execute( dump_region_sizes );
				execute( dump_slength );
				execute( dump_stereo_ms );
				execute( dump_padding );
				execute( dump_block_types );
				execute( dump_sharing );
				execute( dump_preemphasis );
				execute( dump_coarse );
				execute( dump_subblock_gains );
				execute( dump_data_files );
				execute( dump_gg_ctx );
				execute( dump_decoded_data );
				break;
				
			case A_PGM_INFO:
				execute( uncompress_pmp );
				execute( visualize_headers );
				execute( visualize_decoded_data );
				break;
				
			case A_FILE_ANALYSIS:
				execute( uncompress_pmp );
				execute( write_file_analysis );
				break;
			
			case A_BLOCK_ANALYSIS:
				execute( uncompress_pmp );
				execute( write_block_analysis );
				break;
			
			case A_STATS_ANALYSIS:
				execute( uncompress_pmp );
				execute( write_stat_analysis );
				break;
			#else
			default:
				break;
			#endif
		}
	}	
	#if !defined(BUILD_LIB) && defined(DEV_BUILD)
	// write error file if verify lv > 1
	if ( ( verify_lv > 1 ) && ( errorlevel >= err_tol ) )
		write_errfile();
	#endif
	// reset buffers
	reset_buffers();
}


/* -----------------------------------------------
	main-function execution routine
	----------------------------------------------- */

INTERN void execute( bool (*function)() )
{	
	if ( errorlevel < err_tol ) {
		#if !defined BUILD_LIB
		clock_t begin, end;
		bool success;
		int total;
		
		// write statusmessage
		if ( verbosity == 2 ) {
			fprintf( msgout,  "\n%s ", get_status( function ) );
			for ( int i = strlen( get_status( function ) ); i <= 30; i++ )
				fprintf( msgout,  " " );			
		}
		
		// set starttime
		begin = clock();
		// call function
		success = ( *function )();
		// set endtime
		end = clock();
		
		if ( ( errorlevel > 0 ) && ( errorfunction == NULL ) )
			errorfunction = function;
		
		// write time or failure notice
		if ( success ) {
			total = (int) ( (double) (( end - begin ) * 1000) / CLOCKS_PER_SEC );
			if ( verbosity == 2 ) fprintf( msgout,  "%6ims", ( total >= 0 ) ? total : -1 );
		}
		else {
			errorfunction = function;
			if ( verbosity == 2 ) fprintf( msgout,  "%8s", "ERROR" );
		}
		#else
		// call function
		( *function )();
		
		// store errorfunction if needed
		if ( ( errorlevel > 0 ) && ( errorfunction == NULL ) )
			errorfunction = function;
		#endif
	}
}

/* ----------------------- End of main interface functions -------------------------- */

/* ----------------------- Begin of main functions -------------------------- */


/* -----------------------------------------------
	check file and determine filetype
	----------------------------------------------- */

#if !defined(BUILD_LIB)
INTERN bool check_file( void )
{	
	unsigned char fileid[ 2 ] = { 0, 0 };
	const char* filename = filelist[ file_no ];
	
	
	// open input stream, check for errors
	str_in = new iostream( (void*) filename, ( !pipe_on ) ? 0 : 2, 0, 0 );
	if ( str_in->chkerr() ) {
		sprintf( errormessage, FRD_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// free memory from filenames if needed
	if ( mp3filename != NULL ) free( mp3filename ); mp3filename = NULL;
	if ( pmpfilename != NULL ) free( pmpfilename ); pmpfilename = NULL;
	
	// immediately return error if 2 bytes can't be read
	if ( str_in->read( fileid, 1, 2 ) != 2 ) { 
		filetype = F_UNK;
		sprintf( errormessage, "file doesn't contain enough data" );
		errorlevel = 2;
		return false;
	}
	
	// rewind (need to start from the beginning)
	if ( str_in->rewind() != 0 ) {
		sprintf( errormessage, FRD_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// check file id, determine filetype
	if ( ( fileid[0] == pmp_magic[0] ) && ( fileid[1] == pmp_magic[1] ) ) {
		// PMP marker -> file is PMP
		filetype = F_PMP;
		// create filenames
		if ( !pipe_on ) {
			pmpfilename = (char*) calloc( strlen( filename ) + 1, sizeof( char ) );
			strcpy( pmpfilename, filename );
			mp3filename = ( overwrite ) ?
				create_filename( filename, (char*) mp3_ext ) :
				unique_filename( filename, (char*) mp3_ext );
		}
		else {
			mp3filename = create_filename( "STDOUT", NULL );
			pmpfilename = create_filename( "STDIN", NULL );
		}
		// open output stream, check for errors
		str_out = new iostream( (void*) mp3filename, ( !pipe_on ) ? 0 : 2, 0, 1 );
		if ( str_out->chkerr() ) {
			sprintf( errormessage, FWR_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}
	else {
		// for all other cases we assume that file might be MPEG X LAYER Y
		filetype = F_MP3;
		// create filenames
		if ( !pipe_on ) {
			mp3filename = (char*) calloc( strlen( filename ) + 1, sizeof( char ) );
			strcpy( mp3filename, filename );
			pmpfilename = ( overwrite ) ?
				create_filename( filename, (char*) pmp_ext ) :
				unique_filename( filename, (char*) pmp_ext );
		}
		else {
			mp3filename = create_filename( "STDIN", NULL );
			pmpfilename = create_filename( "STDOUT", NULL );
		}
		// open output stream, check for errors
		str_out = new iostream( (void*) pmpfilename, ( !pipe_on ) ? 0 : 2, 0, 1 );
		if ( str_out->chkerr() ) {
			sprintf( errormessage, FWR_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}
	
	
	return true;
}
#endif


/* -----------------------------------------------
	swap streams / init verification
	----------------------------------------------- */
	
#if !defined(BUILD_LIB)
INTERN bool swap_streams( void )	
{
	// store input stream
	str_str = str_in;
	str_str->rewind();
	
	// replace input stream by output stream / switch mode for reading
	str_in = str_out;
	str_in->switch_mode();
	
	// open new stream for output / check for errors
	str_out = new iostream( NULL, 1, 0, 1 );
	if ( str_out->chkerr() ) {
		sprintf( errormessage, "error opening comparison stream" );
		errorlevel = 2;
		return false;
	}
	
	
	return true;
}
#endif


/* -----------------------------------------------
	comparison between input & output
	----------------------------------------------- */

#if !defined(BUILD_LIB)
INTERN bool compare_output( void )
{
	unsigned char* buff_ori;
	unsigned char* buff_cmp;
	int bsize = 1024;
	int dsize;
	int i, b;
	
	
	// init buffer arrays
	buff_ori = ( unsigned char* ) calloc( bsize, sizeof( char ) );
	buff_cmp = ( unsigned char* ) calloc( bsize, sizeof( char ) );
	if ( ( buff_ori == NULL ) || ( buff_cmp == NULL ) ) {
		if ( buff_ori != NULL ) free( buff_ori );
		if ( buff_cmp != NULL ) free( buff_cmp );
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// switch output stream mode / check for stream errors
	str_out->switch_mode();
	while ( true ) {
		if ( str_out->chkerr() )
			sprintf( errormessage, "error in comparison stream" );
		else if ( str_in->chkerr() )
			sprintf( errormessage, "error in output stream" );
		else if ( str_str->chkerr() )
			sprintf( errormessage, "error in input stream" );
		else break;
		errorlevel = 2;
		return false;
	}
	
	// compare sizes
	dsize = str_str->getsize();
	if ( str_out->getsize() != dsize ) {
		sprintf( errormessage, "file sizes do not match" );
		errorlevel = 2;
		return false;
	}
	
	// compare files byte by byte
	for ( i = 0; i < dsize; i++ ) {
		b = i % bsize;
		if ( b == 0 ) {
			str_str->read( buff_ori, sizeof( char ), bsize );
			str_out->read( buff_cmp, sizeof( char ), bsize );
		}
		if ( buff_ori[ b ] != buff_cmp[ b ] ) {
			sprintf( errormessage, "difference found at 0x%X", i );
			errorlevel = 2;
			return false;
		}
	}
	
	
	return true;
}
#endif


/* -----------------------------------------------
	set each variable to its initial value
	----------------------------------------------- */

INTERN bool reset_buffers( void )
{
	mp3Frame* frame;
	
	
	
	// --- free frame data ---
	// start from first frame, throw away all frame data
	if ( firstframe != NULL ) {
		frame = firstframe;
		while ( true ) {
			if ( frame->next != NULL ) {
				frame = frame->next;
				mp3_discard_frame( frame->prev );
				frame->prev = NULL;
			} else {
				mp3_discard_frame( frame );
				break;
			}
		}
	}
	firstframe = NULL;
	lastframe = NULL;
	
	// throw away huffman coded data block
	if ( main_data != NULL ) free ( main_data );
	main_data = NULL;
	main_data_size = 0;
	
	// --- free other data ---
	if ( data_before != NULL ) free ( data_before );
	if ( data_after != NULL ) free( data_after );
	if ( unmute_data != NULL ) free( unmute_data );
	data_before = NULL;
	data_after = NULL;
	unmute_data = NULL;
	data_before_size = 0;
	data_after_size = 0;
	unmute_data_size = 0;
	if ( gg_context[0] != NULL ) free ( gg_context[0] );
	if ( gg_context[1] != NULL ) free ( gg_context[1] );
	gg_context[0] = NULL;
	gg_context[1] = NULL;
	
	// --- reset global variables ---
	g_nframes    =  0; // number of frames
	g_nchannels  =  0; // number of channels
	g_samplerate =  0; // sample rate
	g_bitrate    =  0; // average bit rate
	n_bad_first  =  0; // # of bad first frames
	
	// --- reset frame analysis variables ---
	i_mpeg			= -1; // mpeg - non changing
	i_layer			= -1; // layer - non changing
	i_samplerate	= -1; // sample rate - non changing
	i_bitrate		= -1; // bit rate - value or -1 (variable)
	i_protection	= -1; // checksum - for all (1), none (0) or some (-1) frames
	i_padding		= -1; // padding - for all (1), none (0) or some (-1) frames
	i_privbit		= -1; // private bit - value or -1 (variable)
	i_channels		= -1; // channel mode - non changing
	i_stereo_ms		= -1; // ms stereo - for all (1), none (0) or some (-1) frames
	i_stereo_int	= -1; // int stereo - for all (1), none (0) or some (-1) frames
	i_copyright		= -1; // copyright bit - value or -1 (variable)
	i_original		= -1; // original bit - value or -1 (variable)
	i_emphasis		= -1; // emphasis - value or -1 (variable)
	i_padbits		= -1; // side info padding bits - value or -1 (variable)
	i_bit_res		= -1; // bit reservoir - is used (1) or not used (0)
	i_share			= -1; // scalefactor sharing - is used (1) or not used (0)
	i_sblocks		= -1; // special blocks - are used (1) or not used (0)
	i_mixed			= -1; // mixed blocks - are used (1) or not used (0)
	i_preemphasis	= -1; // preemphasis - value or -1 (variable)
	i_coarse		= -1; // coarse scalefactors - value or -1 (variable)
	i_sbgain		= -1; // subblock gain - is used (1) or not used (0)
	i_aux_h			= -1; // auxiliary data handling - none (0), at begin and end (1), between frames (-1)
	i_sb_diff		= -1; // special blocks diffs between ch0 and ch1 - none (0) or some (-1)
	
	
	return true;
}


/* -----------------------------------------------
	parse MP3 frame structure
	----------------------------------------------- */

INTERN bool read_mp3( void )
{
	unsigned char* mp3data;
	int main_data_begin;
	int main_data_end;
	
	char mpeg = -1;
	char layer = -1;
	char samples = -1;
	char channels = -1;
	
	abytewriter* data_writer;
	mp3Frame* frame = NULL;
	bool incomplete_last_frame = false;
	int type;
	int pos;
	
	
	// read the first few bytes from the file into memory
	// (these will be used to check for proper frames)
	mp3filesize = str_in->getsize();
	if ( mp3filesize > FIRST_FRAME_AREA ) mp3filesize = FIRST_FRAME_AREA;
	mp3data = (unsigned char*) calloc( mp3filesize + 1, sizeof( char ) );
	if ( ( mp3data == NULL ) || ( mp3filesize <= 0 ) ) {
		if ( mp3data != NULL ) free( mp3data );
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return false;
	}
	str_in->read( mp3data, 1, mp3filesize );
	
	// find first proper mpeg audio frame
	pos = mp3_seek_firstframe( mp3data, mp3filesize );
	if ( pos == -1 ) {
		sprintf( errormessage, "no mpeg audio data recognized" );
		errorlevel = 2;
		free( mp3data );
		return false;
	}
	main_data_begin = pos;
	
	// check first frame header for MPEG and LAYER version
	if ( mp3filesize - pos >= 2 ) {
		type = MBITS( mp3data[pos+1], 5, 1 );
		if ( type != MPEG1_LAYER_III ) {
			sprintf( errormessage, "file is %s, not supported", filetype_description[type] );
			errorlevel = 2;
			free( mp3data );
			return false;
		}
	}
	
	// if theres still more data, read the whole file into memory now
	// (bad for mem consumption, but for now -> so what?)
	if ( mp3filesize == FIRST_FRAME_AREA ) {
		mp3filesize = str_in->getsize(); // check size of file again
		mp3data = (unsigned char*) frealloc( mp3data, mp3filesize * sizeof( char ) );
		if ( mp3data == NULL ) {
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
		str_in->read( mp3data + FIRST_FRAME_AREA, 1, mp3filesize - FIRST_FRAME_AREA );
	}
	
	
	// now, read frames until the end of the stream or a bad frame is encountered
	data_writer = new abytewriter(0);
	while ( pos < mp3filesize ) {
		// read frame, check result
		frame = mp3_read_frame( mp3data + pos, mp3filesize - pos );
		if ( frame == NULL ) {
			if ( errorlevel == 2 ) {
				// append some useful info to the existing error message
				sprintf( errormessage + strlen( errormessage ), " (frame #%i at 0x%X)",
					( lastframe != NULL ) ? lastframe->n + 1 : 0, pos );
				free( mp3data );
				delete( data_writer );
				return false;
			}
			else break;
		} else if ( frame->frame_size > mp3filesize - pos ) {
			// check for incomplete last frame
			incomplete_last_frame = true;
			mp3_discard_frame( frame );	
			break;
		} else if ( ( frame->mpeg != mpeg ) || ( frame->layer != layer ) ||
			( frame->samples != samples ) || ( frame->channels != channels ) ) {
			// check for inconsistencies
			if ( mpeg == -1 ) {
				mpeg     = frame->mpeg;
				layer    = frame->layer;
				samples  = frame->samples;
				channels = frame->channels;
			} else {			
				mp3_discard_frame( frame );			
				break;
			}
		}
		if ( lastframe != NULL ) { // fix previous frames aux size, check for problems
			if ( frame->aux_size < 0 ) { // main data out of bounds
				if ( lastframe->n > MAX_BAD_FIRST - 2 ) { mp3_discard_frame( frame ); break; }
				else n_bad_first = lastframe->n + 2;
			} else if ( lastframe->aux_size < frame->bit_reservoir ) { // overlapping main data
				if ( lastframe->n > MAX_BAD_FIRST - 1 ) { mp3_discard_frame( frame ); break; }
				else n_bad_first = lastframe->n + 1;
			} else if ( frame->n == -1 ) { // obvious contradiction (see mp3_read_frame())
				if ( lastframe->n > MAX_BAD_FIRST - 2 ) { mp3_discard_frame( frame ); break; }
				else n_bad_first = lastframe->n + 2;
			}
			// fix previous frame aux size
			lastframe->aux_size -= frame->bit_reservoir;
		} else if ( ( frame->aux_size < 0 ) || ( frame->n == -1 ) ) n_bad_first = 1;
		// update main index, store main data
		frame->main_index = data_writer->getpos() - frame->bit_reservoir;
		data_writer->write_n( mp3data + pos + frame->fixed_size, frame->frame_size - frame->fixed_size );
		// insert frame into the frame chain
		mp3_append_frame( frame );
		// check for pointing to empty space
		if ( frame->main_index < 0 ) n_bad_first = frame->n + 1;
		// advance to next physical frame
		pos += frame->frame_size;		
	}
	
	// store main data
	main_data_size = data_writer->getpos();
	if ( main_data_size > 0 ) main_data = data_writer->getptr();
	delete( data_writer );
	
	// check number of proper frames (must be at least 5)
	if ( lastframe->n + 1 - n_bad_first < 5  ) {
		sprintf( errormessage, "corrupted file, compression not possible" );
		errorlevel = 2;
		return false;
	}
	
	
	// end of main mp3 data processing
	main_data_end = pos;
	// check for ID3 tags at end of file
	if ( !incomplete_last_frame && ( main_data_end < mp3filesize ) ) {
		if ( main_data_end + mp3_get_id3_size( mp3data+main_data_end, mp3filesize-main_data_end ) != mp3filesize ) {
			// allow for a tolerance of 64K unidentified garbage data at EOF
			if ( mp3filesize - main_data_end > GARBAGE_TOLERANCE ) {
				sprintf( errormessage, "synching failure (frame #%i at 0x%X)", lastframe->n + 1, pos );
				errorlevel = 2;
				free( mp3data );
				return false;
			}/* else { // (!!!) strict mode?
				sprintf( errormessage, "%i byte of unidentified garbage after EOF", mp3filesize - main_data_end );
				errorlevel = 1;
			}*/
		}
	}
	
	// clean up and store id3 tags and garbage data
	data_after_size = mp3filesize - main_data_end;
	data_before_size = main_data_begin;
	if ( data_after_size > 0 ) {
		data_after = (unsigned char*) calloc( data_after_size, sizeof( char ) );
		if ( ( mp3data == NULL ) || ( mp3filesize <= 0 ) ) {
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
		memcpy( data_after, mp3data + main_data_end, data_after_size );
	}
	if ( data_before_size > 0 )
		data_before = (unsigned char*) frealloc( mp3data, data_before_size * sizeof(char) );
	else free( mp3data );
	
	
	return true;
}


/* -----------------------------------------------
	write MP3 from frame structure
	----------------------------------------------- */

INTERN bool write_mp3( void )
{
	unsigned char* data = main_data;
	unsigned char* fixed;
	mp3Frame* frame;
	
	
	// write data before (usually ID3v2 tag)
	str_out->write( data_before, 1, data_before_size );
	
	// write physical frames
	// main data has to be in order!
	for ( frame = firstframe; frame != NULL; frame = frame->next ) {
		fixed = mp3_build_fixed( frame );
		str_out->write( fixed, 1, frame->fixed_size );
		str_out->write( data, 1, frame->frame_size - frame->fixed_size );
		data += frame->frame_size - frame->fixed_size;
	}
	
	// write data after
	str_out->write( data_after, 1, data_after_size );
	
	// errormessage if write error
	if ( str_out->chkerr() ) {
		sprintf( errormessage, "write error, possibly drive is full" );
		errorlevel = 2;		
		return false;
	}
	
	// get filesize
	mp3filesize = str_out->getsize();
	
	
	return true;
}


/* -----------------------------------------------
	analyse frames for redundancies and errors
	----------------------------------------------- */

INTERN bool analyze_frames( void )
{
	mp3Frame* frame;
	granuleInfo* granule;
	bool aux0 = false;
	bool aux1 = false;
	bool sbx_gain = false;
	int nch;
	int ch, gr;
	
	
	// (re)set analysis data
	i_mpeg			= firstframe->mpeg;
	i_layer			= firstframe->layer;
	i_samplerate	= firstframe->samples;
	i_bitrate		= firstframe->bits;
	i_protection	= firstframe->protection;
	i_padding		= firstframe->padding;
	i_privbit		= firstframe->privbit;
	i_channels		= firstframe->channels;
	i_stereo_ms		= firstframe->stereo_ms;
	i_stereo_int	= firstframe->stereo_int;
	i_copyright		= firstframe->copyright;
	i_original		= firstframe->original;
	i_emphasis		= firstframe->emphasis;
	i_padbits		= firstframe->padbits;
	i_bit_res = ( firstframe->bit_reservoir > 0 ) ? 1 : 0;
	if ( firstframe->aux_size > 0 ) {
		i_aux_h = 1;
		aux0 = true;
	} else i_aux_h = 0;
	// for granules
	granule = firstframe->granules[0][0];
	i_share = ( granule->share != 0 ) ? 1 : 0;
	i_sblocks = ( granule->window_switching != 0 ) ? 1 : 0;
	i_mixed = ( granule->mixed_flag != 0 ) ? 1 : 0;
	i_preemphasis = granule->preemphasis;
	i_coarse = granule->coarse_scalefactors;
	// easy way out for these - will be tested later
	i_sb_diff = 0;
	i_sbgain = 0;
	
	
	// analyse frames, check for redundancies and errors
	for ( frame = firstframe; frame != NULL; frame = frame->next ) {
		// no need to check mpeg, layer, samples, channels
		// already did this in read_mp3()!
		
		// analyse (mainly) frame header data
		// special tolerance for the following, as they often get mixed up in frame #0
		if ( ( i_privbit != -1 ) && ( i_privbit != frame->privbit ) ) {
			if ( frame->n == 1 ) { // tolerance for first frame
				if ( n_bad_first == 0 ) n_bad_first = 1;
				i_privbit = frame->privbit;
			} else i_privbit = -1;
		}		
		if ( ( i_copyright != -1 ) && ( i_copyright != frame->copyright ) ) {
			if ( frame->n == 1 ) { // tolerance for first frame
				if ( n_bad_first == 0 ) n_bad_first = 1;
				i_copyright = frame->copyright;
			} else i_copyright = -1;
		}
		if ( ( i_original != -1 ) && ( i_original != frame->original ) ) {
			if ( frame->n == 1 ) { // tolerance for first frame
				if ( n_bad_first == 0 ) n_bad_first = 1;
				i_original = frame->original;
			} else i_original = -1;
		}
		if ( ( i_protection != -1 ) && ( i_protection != frame->protection ) ) i_protection = -1;
		if ( ( i_bitrate != -1 ) && ( i_bitrate != frame->bits ) ) i_bitrate = -1;
		if ( ( i_padding != -1 ) && ( i_padding != frame->padding ) ) i_padding = -1;
		if ( ( i_stereo_ms != -1 ) && ( i_stereo_ms != frame->stereo_ms ) ) i_stereo_ms = -1;
		if ( ( i_stereo_int != -1 ) && ( i_stereo_int != frame->stereo_int ) ) i_stereo_int = -1;
		if ( ( i_emphasis != -1 ) && ( i_emphasis != frame->emphasis ) ) i_emphasis = -1;
		if ( ( i_padbits != -1 ) && ( i_padbits != frame->padbits ) ) i_padbits = -1;
		if ( ( i_bit_res != 1 ) && ( frame->bit_reservoir > 0 ) ) i_bit_res = 1;
		if ( i_aux_h != -1 ) {
			// no auxiliary before aux0 or after aux1 
			if  ( frame->aux_size > 0 ) {
				if ( !aux0 ) aux1 = true;
			} else {
				if ( aux1 ) i_aux_h = -1;
				if ( aux0 ) aux0 = false;
			}
		}
		nch = frame->nchannels;
		for ( ch = 0; ch < nch; ch++ ) {
			for ( gr = 0; gr < 2; gr++ ) {
				granule = frame->granules[ch][gr];
				sbx_gain =
					( granule->sb_gain[0] != 0 ) ||
					( granule->sb_gain[1] != 0 ) ||
					( granule->sb_gain[2] != 0 );
				if ( ( i_share != 1 ) && ( granule->share != 0 ) ) i_share = 1;
				if ( ( i_sblocks != 1 ) && ( granule->window_switching ) ) i_sblocks = 1;
				if ( ( i_mixed != 1 ) && ( granule->mixed_flag ) ) i_mixed = 1;
				if ( ( i_preemphasis != -1 ) && ( i_preemphasis != granule->preemphasis ) ) i_preemphasis = -1;
				if ( ( i_coarse != -1 ) && ( i_coarse != granule->coarse_scalefactors ) ) i_coarse = -1;
				if ( ( i_sbgain != -1 ) && ( sbx_gain ) ) i_sbgain = ( granule->block_type == SHORT_BLOCK ) ? 1 : -1;
				if ( ( i_sb_diff != 1 ) && ( ch == 1 ) )
					if ( granule->block_type != frame->granules[0][gr]->block_type ) i_sb_diff = 1;
			}
		}
	}
	
	// last check for aux data handling
	if ( ( i_aux_h != -1 ) && ( aux1 ) ) i_aux_h = 1;
	
	// fill in some global info
	g_nframes = lastframe->n + 1;
	g_nchannels = ( i_channels == MP3_MONO ) ? 1 : 2;  // number of channels
	g_samplerate = mp3_samplerate_table[(int)i_samplerate];  // sample rate
	g_bitrate = ( i_bitrate == -1 ) ? 0 : mp3_bitrate_table[(int)i_bitrate]; // bit rate
	
	
	return true;
}


INTERN bool compress_mp3( void )
{
	aricoder* encoder;
	
	
	// --- check for incompatibilities and problems ---
	
	// unsupported stuff -> not recoverable
	if ( i_protection == -1 ) { // inconsistent use of checksums
		sprintf( errormessage, "inconsistent use of checksums, not supported" );
		errorlevel = 2;
		return false;
	}
	if ( i_stereo_int == -1 ) { // inconsistent use of intensity stereo
		sprintf( errormessage, "inconsistent use of int stereo, not supported" );
		errorlevel = 2;
		return false;
	}
	if ( i_emphasis == -1 ) { // inconsistent use of emphasis
		sprintf( errormessage, "inconsistent use of emphasis, not supported" );
		errorlevel = 2;
		return false;
	}
	if ( i_mixed != 0x0 ) { // mixed blocks
		sprintf( errormessage, "mixed blocks used, not supported" );
		errorlevel = 2;
		return false;
	}
	
	// inconsistencies -> recoverable
	if ( i_privbit == -1 ) { // inconsistent private bit -> steganograhy?
		sprintf( errormessage, "inconsistent private bit" );
		errorlevel = 1;
		i_privbit = 0;
	}
	if ( i_copyright == -1 ) { // inconsistent copyright bit -> steganograhy?
		sprintf( errormessage, "inconsistent copyright bit" );
		errorlevel = 1;
		i_copyright = 0;
	}
	if ( i_original == -1 ) { // inconsistent original bit -> steganograhy?
		sprintf( errormessage, "inconsistent original bit" );
		errorlevel = 1;
		i_original = 1;
	}
	if ( i_padbits != 0 ) { // non-zero padbits -> steganograhy?
		sprintf( errormessage, "non-zero padbits found" );
		errorlevel = 1;
		i_padbits = 0;
	}
	
	
	// --- write PMP header with some basic info ---
	
	// PMP magic number & version byte
	str_out->write( (void*) pmp_magic, 1, 2 );
	str_out->write( (void*) &appversion, 1, 1 );
	
	// PMP header data
	if ( !pmp_write_header( str_out ) ) return false;
	
	
	#if defined( STORE_ID3 )
	// --- store ID3 data instead of compressing ---
	
	if ( data_before_size > 0 )
		if ( pmp_store_data( str_out, data_before, data_before_size ) != data_before_size ) return false;
	if ( data_after_size > 0 )
		if ( pmp_store_data( str_out, data_after, data_after_size ) != data_after_size ) return false;
	#endif
	
	
	// --- mute frames, store fix data (only if broken) ---
	
	if ( n_bad_first > 0 ) {
		// mute all frames up to last bad
		for ( mp3Frame* frame = firstframe; n_bad_first > 0; frame = frame->next, n_bad_first-- )
			mp3_mute_frame( frame );
		// store fix data
		if ( !pmp_store_unmute_data( str_out ) ) return false;
	}
	
	
	// --- actual compressed data writing starts here ---
	
	// init arithmetic compression
	encoder = new aricoder( str_out, 1 );
	
	#if !defined( STORE_ID3 )
	// id3 tags / other tags / garbage
	if ( ( data_before_size > 0 ) || ( data_after_size > 0 ) )
		if ( !pmp_encode_id3( encoder ) ) return false;
	#endif

	// frame header data
	if ( i_padding != 0 ) // padding bits
		if ( !pmp_encode_padding( encoder ) ) return false;
	if ( i_sblocks != 0 ) // special block types
		if ( !pmp_encode_block_types( encoder ) ) return false;
	// global gain
	if ( !pmp_encode_global_gain( encoder ) ) return false;
	// build context from global gain
	if ( !pmp_build_context() ) return false;
	// slength
	if ( !pmp_encode_slength( encoder ) ) return false;
	/*
	// region sizes
	if ( !pmp_encode_region_bounds( encoder ) ) return false;
	// huffman table selections
	if ( !pmp_encode_htable_selection( encoder ) ) return false;
	*/
	if ( !pmp_encode_region_data( encoder ) ) return false;
	if ( i_share != 0 ) // scalefactor sharing
		if ( !pmp_encode_sharing( encoder ) ) return false;
	if ( i_preemphasis != 0 ) // preemphasis
		if ( !pmp_encode_preemphasis( encoder ) ) return false;
	if ( i_coarse != 0 ) // coarse setting
		if ( !pmp_encode_coarse_sf( encoder ) ) return false;
	if ( i_sbgain != 0 ) // subblock gain
		if ( !pmp_encode_subblock_gain( encoder ) ) return false;
	if ( i_stereo_ms != 0 ) // ms stereo settings
		if ( !pmp_encode_stereo_ms( encoder ) ) return false;
	// main data
	if ( !pmp_encode_main_data( encoder ) ) return false;
	
	// finalize arithmetic compression
	delete( encoder );
	
	
	// --- final checks ---
	
	// errormessage if write error
	if ( str_out->chkerr() ) {
		sprintf( errormessage, "write error, possibly drive is full" );
		errorlevel = 2;		
		return false;
	}
	
	// get filesize
	pmpfilesize = str_out->getsize();
	
	
	return true;
}


INTERN bool uncompress_pmp( void )
{
	aricoder* decoder;
	unsigned char hcode;
	
	
	// --- no error checks needed! ---
	// (we already did this when compressing)
	
	
	// --- read PMP header and analyse basic info ---
	
	// PMP magic number
	// skip 2 bytes (no need to check again)
	str_in->read( &hcode, 1, 1 );
	str_in->read( &hcode, 1, 1 );
	
	// version number
	str_in->read( &hcode, 1, 1 );
	// compare version number
	if ( hcode != appversion ) {
		sprintf( errormessage, "incompatible file, use %s v%i.%i",
			appname, hcode / 10, hcode % 10 );
		errorlevel = 2;
		return false;
	}
	
	// read and analyse header
	if ( !pmp_read_header( str_in ) ) return false;
	
	
	#if defined( STORE_ID3 )
	// --- unstore ID3 data ---
	
	if ( data_before_size > 0 ) {
		data_before_size = pmp_unstore_data( str_in, &data_before );
		if ( data_before_size <= 0 ) return false;
	}
	if ( data_after_size > 0 ) {
		data_after_size = pmp_unstore_data( str_in, &data_after );
		if ( data_after_size <= 0 ) return false;
	}
	#endif
	
	
	// --- unstore unmute fix data for later use (only if needed) ---
	
	// unstore fix data
	if ( n_bad_first > 0 ) {
		if ( !pmp_unstore_unmute_data( str_in ) ) return false;
		n_bad_first = 0; // not a beautiful solution
	}
	
	
	// --- actual decompression starts here ---
	
	// init arithmetic decompression
	decoder = new aricoder( str_in, 0 );
	
	#if !defined( STORE_ID3 )
	// id3 tags / other tags / garbage
	if ( ( data_before_size > 0 ) || ( data_after_size > 0 ) )
		if ( !pmp_decode_id3( decoder ) ) return false;
	#endif
	
	// frame header data
	if ( i_padding != 0 ) // padding bits
		if ( !pmp_decode_padding( decoder ) ) return false;
	if ( i_sblocks != 0 ) // special block types
		if ( !pmp_decode_block_types( decoder ) ) return false;
	// global gain
	if ( !pmp_decode_global_gain( decoder ) ) return false;
	// build context from global gain
	if ( !pmp_build_context() ) return false;
	// slength
	if ( !pmp_decode_slength( decoder ) ) return false;
	/*
	// region sizes
	if ( !pmp_decode_region_bounds( decoder ) ) return false;
	// huffman table selections
	if ( !pmp_decode_htable_selection( decoder ) ) return false;
	*/
	if ( !pmp_decode_region_data( decoder ) ) return false;
	if ( i_share != 0 ) // scalefactor sharing
		if ( !pmp_decode_sharing( decoder ) ) return false;
	if ( i_preemphasis != 0 ) // preemphasis
		if ( !pmp_decode_preemphasis( decoder ) ) return false;
	if ( i_coarse != 0 ) // coarse setting
		if ( !pmp_decode_coarse_sf( decoder ) ) return false;
	if ( i_sbgain != 0 ) // subblock gain
		if ( !pmp_decode_subblock_gain( decoder ) ) return false;
	if ( i_stereo_ms != 0 ) // ms stereo settings
		if ( !pmp_decode_stereo_ms( decoder ) ) return false;
	// main data
	if ( !pmp_decode_main_data( decoder ) ) return false;
	
	// finalize arithmetic compression
	delete( decoder );
	
	
	// --- unmute frames and reverse to bad (but bitwise identical) state ---
	
	if ( unmute_data_size > 0 ) // unmute all frames up to # found in unmute data first byte
		for ( mp3Frame* frame = firstframe; n_bad_first < *unmute_data; frame = frame->next, n_bad_first++ )
			mp3_unmute_frame( frame );
	
			
	// --- final checks ---
	
	// get filesize
	pmpfilesize = str_in->getsize();
	
	
	return true;
}

/* ----------------------- End of main functions -------------------------- */

/* ----------------------- Begin of MP3 specific functions -------------------------- */


/* -----------------------------------------------
	read one physical frame
	----------------------------------------------- */
INTERN inline mp3Frame* mp3_read_frame( unsigned char* data, int max_size )
{
	mp3Frame* frame;
	granuleInfo* granule;
	abitreader* side_reader;
	unsigned char* header;
	unsigned char* sideinfo;
	unsigned short crc;
	int nsb = 0;
	int nch = 0;
	int ch, gr;

	
	// immediately return if not enough data (min size for header and side info = 21)
	if ( max_size < 21 ) return NULL;

	// --- frame header ---
	header = data + 0;
	
	// check syncword
	if ( ( header[0] != 0xFF ) || ( (header[1]&0xFA) != 0xFA ) ) {
		// no syncword or not MPEG-1 LAYER III
		// might be end of audio data -> process in other function
		return NULL;
	}
	
	// alloc memory for frame
	frame = (mp3Frame*) calloc( 1, sizeof( mp3Frame ) );
	if ( frame == NULL ) {
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return NULL;
	}
	
	// preset pointers
	frame->granules = NULL;
	frame->prev = NULL;
	frame->next = NULL;
	frame->n = 0;
	
	// extract data
	frame->mpeg       = (header[1]>>3)&0x3;
	frame->layer      = (header[1]>>1)&0x3;
	frame->protection = ((header[1]>>0)&0x1)^1;
	frame->bits       = (header[2]>>4)&0xF;
	frame->samples    = (header[2]>>2)&0x3;
	frame->padding    = (header[2]>>1)&0x1;
	frame->privbit    = (header[2]>>0)&0x1;
	frame->channels   = (header[3]>>6)&0x3;
	frame->stereo_ms  = (header[3]>>5)&0x1;
	frame->stereo_int = (header[3]>>4)&0x1;
	frame->copyright  = (header[3]>>3)&0x1;
	frame->original   = (header[3]>>2)&0x1;
	frame->emphasis   = (header[3]>>0)&0x3;
	
	// check data for problems
	// this also contains the never used free form stream flag -> so what?
	if ( ( frame->bits == 0x0 ) || ( frame->bits == 0xF ) || ( frame->samples == 0x3 ) ) {
		free( frame );
		return NULL;
	}
	
	// number of channels
	frame->nchannels = ( frame->channels == MP3_MONO ) ? 1 : 2;
	nch = frame->nchannels;
	
	// alloc memory for granules
	frame->granules = (granuleInfo***) calloc( nch, sizeof( granuleInfo** ) );
	for ( ch = 0; ch < nch; ch++ ) {
		frame->granules[ch] = (granuleInfo**) calloc( 2, sizeof( granuleInfo* ) );
		for ( gr = 0; gr < 2; gr++ )
			frame->granules[ch][gr] = (granuleInfo*) calloc( 1, sizeof( granuleInfo ) );
	}
	
	// calculate size of frame - use the lookup table
	frame->frame_size = mp3_frame_size_table[(int)frame->samples][(int)frame->bits];
	if ( frame->padding ) frame->frame_size++;
	
	// this is the actual calculation routine, which is now no more used 
	/* if ( frame->layer != LAYER_I ) frame->frame_size = (int) ( frame->padding +
			( ( 144000 * bitrate_table[frame->mpeg][frame->layer][frame->bits] ) /
			samplerate_table[frame->mpeg][frame->samples] ) );
	else frame->frame_size = (int) ( ( ( frame->padding ) ? 4 : 0 ) +
			( ( 48000 * bitrate_table[frame->mpeg][frame->layer][frame->bits] ) /
			samplerate_table[frame->mpeg][frame->samples] ) );*/	

	// --- side information... ---
	nsb = ( nch == 1 ) ? 17 : 32;
	frame->fixed_size = 4 + nsb + (frame->protection ? 2 : 0);
	// check if enough data is available
	if ( frame->fixed_size > max_size ) {
		free( frame );
		return NULL;
	}
	// --- ...and optional crc checksum ---
	if ( frame->protection == 0x1 ) {
		sideinfo = data + 6;
		// if there is a crc: check and discard
		crc = (data[4]<<8) + (data[5]<<0);
		if ( crc != mp3_calc_layer3_crc( header, sideinfo, nsb ) ) {
			sprintf( errormessage, "crc checksum mismatch" );
			errorlevel = 1; // (!!!) careful - file might be broken
		}
	}
	else sideinfo = data + 4;
	side_reader = new abitreader( sideinfo, nsb );
	// frame global side info
	frame->bit_reservoir = (short) side_reader->read( 9 );
	frame->padbits = (char) side_reader->read( (nch==1) ? 5 : 3 );
	for ( ch = 0; ch < nch; ch++ ) {
		frame->granules[ch][0]->share = (char) side_reader->read( 4 );
		frame->granules[ch][1]->share = 0x0;
	}
	// granule specific side info
	for ( gr = 0; gr < 2; gr++ ) {
		for ( ch = 0; ch < nch; ch++ ) {
			granule = frame->granules[ch][gr];
			granule->main_data_bit = (short) side_reader->read( 12 );
			granule->big_val_pairs = (short) side_reader->read( 9 );
			granule->global_gain = (short) side_reader->read( 8 );
			granule->slength = (char) side_reader->read( 4 );
			granule->window_switching = (char) side_reader->read( 1 );
			if ( granule->window_switching == 0 ) { // for normal blocks
				granule->region_table[0] = (char) side_reader->read( 5 );
				granule->region_table[1] = (char) side_reader->read( 5 );
				granule->region_table[2] = (char) side_reader->read( 5 );
				granule->region0_size = (char) side_reader->read( 4 );
				granule->region1_size = (char) side_reader->read( 3 );
				if ( granule->region0_size+granule->region1_size > 20 ) {
					sprintf( errormessage, "region size out of bounds" );
					errorlevel = 2;
					return NULL;
				}
				granule->region_bound[0] =
					mp3_bandwidth_bounds[(int)frame->samples][(int)granule->region0_size+1];
				granule->region_bound[1] =
					mp3_bandwidth_bounds[(int)frame->samples][(int)granule->region0_size+granule->region1_size+2];
				granule->region_bound[2] = granule->big_val_pairs << 1;
				if ( granule->region_bound[0] > granule->region_bound[2] ) {
					granule->region_bound[0] = granule->region_bound[2];
					granule->region_bound[1] = granule->region_bound[2];
				} else if ( granule->region_bound[1] > granule->region_bound[2] )
					granule->region_bound[1] = granule->region_bound[2];
				granule->block_type = LONG_BLOCK;
				granule->mixed_flag = 0;
				granule->sb_gain[0] = 0;
				granule->sb_gain[1] = 0;
				granule->sb_gain[2] = 0;
			} else { // for special blocks
				granule->block_type = (char) side_reader->read( 2 );
				granule->mixed_flag = (char) side_reader->read( 1 );
				granule->region_table[0] = (char) side_reader->read( 5 );
				granule->region_table[1] = (char) side_reader->read( 5 );
				granule->sb_gain[0] = (char) side_reader->read( 3 );
				granule->sb_gain[1] = (char) side_reader->read( 3 );
				granule->sb_gain[2] = (char) side_reader->read( 3 );
				if ( granule->block_type != SHORT_BLOCK ) {
					// region sizes for different block types
					granule->region0_size = 8;
					granule->region1_size = 0;
					granule->region_bound[0] =
						mp3_bandwidth_bounds[(int)frame->samples][8];
				} else { // special treatment for mixed blocks needed (!)
					granule->region0_size = 9;
					granule->region1_size = 0;
					granule->region_bound[0] =
						mp3_bandwidth_bounds_short[(int)frame->samples][9/3] * 3;
				}
				granule->region_bound[1] = granule->big_val_pairs << 1;
				if ( granule->region_bound[0] > granule->region_bound[1] )
					granule->region_bound[0] = granule->region_bound[1];
				granule->region_bound[2] = granule->region_bound[1];
				granule->region_table[2] = 0;
			}
			granule->preemphasis = (char) side_reader->read( 1 );
			granule->coarse_scalefactors = (char) side_reader->read( 1 );
			granule->select_htabB = (char) side_reader->read( 1 );
			granule->sv_bound = 0;
			// check for obvious problems/contradictions
			if ( granule->main_data_bit == 0 ) {
				if ( ( granule->big_val_pairs != 0 ) || ( granule->slength != 0 ) )
					frame->n = -1; // mistreat frame number as trouble indicator :-)
			}
		}
	}
	delete( side_reader );
	
	// calculate total size of main (not aux) data (in byte)
	frame->main_bits = 0;
	for ( gr = 0; gr < 2; gr++ )
		for ( ch = 0; ch < nch; ch++ )
			frame->main_bits += frame->granules[ch][gr]->main_data_bit;
	frame->main_size = (frame->main_bits+7)>>3;
	
	// calculate temporary size of aux data (in byte), subject to change
	frame->aux_size =
		frame->frame_size -
		frame->fixed_size +
		frame->bit_reservoir -
		frame->main_size;
	
	
	return frame;
}


/* -----------------------------------------------
	build frame from i_variables
	----------------------------------------------- */
INTERN inline mp3Frame* mp3_build_frame( void )
{
	mp3Frame* frame;
	granuleInfo* granule;
	int ch, gr;
	int nch;
	
	
	// alloc memory for frame
	frame = (mp3Frame*) calloc( 1, sizeof( mp3Frame ) );
	if ( frame == NULL ) {
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return NULL;
	}
	
	// preset pointers
	frame->granules = NULL;
	frame->prev = NULL;
	frame->next = NULL;
	
	// fill in some info
	frame->mpeg		 		= i_mpeg;
	frame->layer 			= i_layer;
	frame->protection 		= i_protection;
	frame->bits 			= i_bitrate;
	frame->samples 			= i_samplerate;
	frame->padding			= i_padding;
	frame->privbit			= i_privbit;
	frame->channels			= i_channels;
	frame->stereo_ms		= i_stereo_ms;
	frame->stereo_int		= i_stereo_int;
	frame->copyright		= i_copyright;
	frame->original			= i_original;
	frame->emphasis			= i_emphasis;
	frame->bit_reservoir 	= 0;
	frame->padbits			= i_padbits;
	
	// stuff that is known right now (# channels and fixed size)
	if ( frame->channels == MP3_MONO ) {
		frame->fixed_size = ( frame->protection ) ? 4 + 2 + 17 : 4 + 17;
		frame->nchannels = 1; nch = frame->nchannels;
	} else {
		frame->fixed_size = ( frame->protection ) ? 4 + 2 + 32 : 4 + 32;
		frame->nchannels = 2; nch = frame->nchannels;
	}
	
	// alloc memory for granules
	frame->granules = (granuleInfo***) calloc( nch, sizeof( granuleInfo** ) );
	for ( ch = 0; ch < nch; ch++ ) {
		frame->granules[ch] = (granuleInfo**) calloc( 2, sizeof( granuleInfo* ) );
		for ( gr = 0; gr < 2; gr++ )
			frame->granules[ch][gr] = (granuleInfo*) calloc( 1, sizeof( granuleInfo ) );
	}
	
	// granule specific stuff
	for ( ch = 0; ch < nch; ch++ ) {
		for ( gr = 0; gr < 2; gr++ ) {
			granule = frame->granules[ch][gr];
			granule->share				= ( gr == 0 ) ? i_share : 0;
			granule->window_switching	= i_sblocks;
			granule->mixed_flag			= i_mixed;
			granule->block_type			= LONG_BLOCK;
			granule->sb_gain[0]			= i_sbgain;
			granule->sb_gain[1]			= i_sbgain;
			granule->sb_gain[2]			= i_sbgain;
			granule->preemphasis		= i_preemphasis;
			granule->coarse_scalefactors = i_coarse;
		}
	}
	
	
	return frame;
}


/* -----------------------------------------------
	append frame to the frame chain
	----------------------------------------------- */
INTERN inline bool mp3_append_frame( mp3Frame* frame )
{
	static granuleInfo* lastgranule[2] = { NULL, NULL };
	static int n = 0;
	int ch;
	
	
	// insert frame into the frame chain, set up links between frames
	// aux size correction has to take place elsewhere
	if ( lastframe == NULL ) {
		firstframe = frame;
		frame->prev = NULL;
		lastgranule[0] = NULL;
		lastgranule[1] = NULL;
		n = 0;
	} else {
		lastframe->next = frame;
		frame->prev = lastframe;
	}
	lastframe = frame;
	
	// set up links for granules between each other
	for ( ch = 0; ch < frame->nchannels; ch++ ) {
		frame->granules[ch][0]->n = n<<1;
		frame->granules[ch][1]->n = frame->granules[ch][0]->n|1;
		frame->granules[ch][0]->next = frame->granules[ch][1];
		frame->granules[ch][1]->prev = frame->granules[ch][0];
		frame->granules[ch][0]->prev = lastgranule[ch];
		frame->granules[ch][1]->next = NULL;
		if ( lastgranule[ch] != NULL )
			lastgranule[ch]->next = frame->granules[ch][0];
		lastgranule[ch] = frame->granules[ch][1];
	}
	
	// some generic stuff
	frame->n = n++;
	frame->next = NULL;
	
	
	return true;
}


/* -----------------------------------------------
	discard frame data
	----------------------------------------------- */
INTERN inline bool mp3_discard_frame( mp3Frame* frame )
{
	int nch;
	int ch, gr;
	
	
	// discard all data in one frame
	nch = frame->nchannels;
	if ( frame->granules != NULL ) {
		for ( ch = 0; ch < nch; ch++ ) {
			for ( gr = 0; gr < 2; gr++ )
				free ( frame->granules[ch][gr] );
			free ( frame->granules[ch] );
		}
		free ( frame->granules );
	}
	free ( frame );	
	
	
	return true;
}


/* -----------------------------------------------
	mute a single frame
	----------------------------------------------- */
INTERN inline bool mp3_mute_frame( mp3Frame* frame )
{
	granuleInfo* granule;
	unsigned char* ptr;
	int nch, ums;
	int ch, gr;
	
	
	// mute a single frame -> dangerous, be careful!
	nch = frame->nchannels;
	ums = 2 + ( nch * 2 * 4 );
	
	// store reconstruction data
	if ( unmute_data_size == 0 ) { // first alloc - no mem check needed 
		unmute_data = (unsigned char*) calloc( ++unmute_data_size, sizeof( char ) );
		unmute_data_size = 1;
		*unmute_data = 0;
	}	
	unmute_data = (unsigned char*) // realloc for new size
		frealloc( unmute_data, ( unmute_data_size + ums ) * sizeof( char ) );
	if ( unmute_data == NULL ) {
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return false;
	}
	ptr = unmute_data + unmute_data_size;
	unmute_data_size += ums;
	// some bits are wasted here
	// normally muting frames shouldn't be necessary at all anyways
	(*ptr)    = (frame->privbit&0x1)    << 7;
	(*ptr)   |= (frame->original&0x1)   << 6;
	(*ptr)   |= (frame->copyright&0x1)  << 5;
	(*ptr++) |= (frame->bit_reservoir >> 8) & 0x1;
	(*ptr++)  = (frame->bit_reservoir >> 0) & 0xFF;
	for ( ch = 0; ch < nch; ch++ ) {
		for ( gr = 0; gr < 2; gr++ ) {
			granule = frame->granules[ch][gr];
			(*ptr++) = (granule->main_data_bit >> 4) & 0xFF;
			(*ptr++) = ( (granule->main_data_bit << 4) & 0xF0 ) | ( granule->slength & 0x0F );
			(*ptr++) = (granule->big_val_pairs >> 1) & 0xFF;
			(*ptr++) = (granule->big_val_pairs << 7) & 0x80;
		}
	}
	// take count - careful this doesn't excede 255
	(*unmute_data)++;
	
	// mute frame - this has to be done in order
	// we assume that all previous frames are muted, too!
	if ( frame->prev != NULL ) {
		// make some room, use prev frame aux too!
		frame->bit_reservoir += frame->prev->aux_size;
		if ( frame->bit_reservoir >= 512 ) {
			frame->prev->aux_size = frame->bit_reservoir - 511;
			frame->bit_reservoir = 511;
		} else frame->prev->aux_size = 0;
	} else frame->bit_reservoir = 0;
	frame->aux_size = frame->frame_size - frame->fixed_size + frame->bit_reservoir;
	if ( frame->next != NULL ) frame->aux_size -= frame->next->bit_reservoir;
	frame->main_size = 0;
	frame->main_bits = 0;
	if ( frame->granules != NULL ) {
		for ( ch = 0; ch < nch; ch++ ) {
			for ( gr = 0; gr < 2; gr++ ) {
				granule = frame->granules[ch][gr];
				granule->main_data_bit = 0;
				granule->big_val_pairs = 0;
				granule->slength = 0;
				// fix region bounds
				granule->region_bound[0] = 0;
				granule->region_bound[1] = 0;
				granule->region_bound[2] = 0;
			}
		}
	}

	
	return true;
}


/* -----------------------------------------------
	unmute a single frame
	----------------------------------------------- */
INTERN inline bool mp3_unmute_frame( mp3Frame* frame )
{
	granuleInfo* granule;
	unsigned char* ptr;
	int nch, ums;
	int ch, gr;
	
	
	// restore a single frame to it's original, broken state
	// enough fix data has to be present, channel mode has to be consistent
	nch = g_nchannels;
	ums = 2 + ( nch * 2 * 4 );
	ptr = unmute_data + 1 + ( n_bad_first * ums ); 
	
	// unmute frame - has to be done in order, too!
	frame->privbit    = ((*ptr)>>7) & 0x1;
	frame->original   = ((*ptr)>>6) & 0x1;
	frame->copyright  = ((*ptr)>>5) & 0x1;
	frame->bit_reservoir  = (*ptr++) << 8;
	frame->bit_reservoir |= (*ptr++) << 0;
	for ( ch = 0; ch < nch; ch++ ) {
		for ( gr = 0; gr < 2; gr++ ) {
			granule = frame->granules[ch][gr];
			granule->main_data_bit = (*ptr++) << 4;
			granule->main_data_bit |= (*ptr) >> 4;
			granule->slength = (*ptr++) & 0x0F;
			granule->big_val_pairs = (*ptr++) << 1;
			granule->big_val_pairs |= (*ptr++) >> 7;
		}
	} // no need to accomodate for the changed frame params - for now (!!!)
	
	
	return true;
}


/* -----------------------------------------------
	build fixed part of physical frame
	----------------------------------------------- */
INTERN inline unsigned char* mp3_build_fixed( mp3Frame* frame )
{
	static unsigned char* fixed = ( unsigned char* ) calloc( 64, 1 );
	unsigned char* tmp_ptr;
	
	granuleInfo* granule;
	abitwriter* side_writer;
	
	unsigned char* header;
	unsigned char* sideinfo;
	unsigned short crc;
	
	int nsb = 0;
	int nch = 0;
	int ch, gr;

	
	// preparations
	memset( fixed, 0, 64 );
	nch = frame->nchannels;
	nsb = ( nch == 1 ) ? 17 : 32;
	header = fixed + 0;
	sideinfo = fixed + ( ( frame->protection ) ? 4 + 2 : 4 );
	
	
	// --- frame header ---
	
	// insert data
	header[0]  = 0xFF;
	header[1]  = 0xFA;
	header[1] |= frame->mpeg       << 3;
	header[1] |= frame->layer      << 1;
	header[1] |= frame->protection  ^ 1;
	header[2] |= frame->bits       << 4;
	header[2] |= frame->samples    << 2;
	header[2] |= frame->padding    << 1;
	header[2] |= frame->privbit    << 0;
	header[3] |= frame->channels   << 6;
	header[3] |= frame->stereo_ms  << 5;
	header[3] |= frame->stereo_int << 4;
	header[3] |= frame->copyright  << 3;
	header[3] |= frame->original   << 2;
	header[3] |= frame->emphasis   << 0;

	
	// --- side information... ---
	
	// init abitwriter
	side_writer = new abitwriter( 64 );
	
	// frame global side info
	side_writer->write( frame->bit_reservoir, 9 );
	side_writer->write( frame->padbits, (nch==1) ? 5 : 3 );
	for ( ch = 0; ch < nch; ch++ )
		side_writer->write( frame->granules[ch][0]->share, 4 );
	
	// granule specific side info
	for ( gr = 0; gr < 2; gr++ ) {
		for ( ch = 0; ch < nch; ch++ ) {
			granule = frame->granules[ch][gr];
			side_writer->write( granule->main_data_bit, 12 );
			side_writer->write( granule->big_val_pairs, 9 );
			side_writer->write( granule->global_gain, 8 );
			side_writer->write( granule->slength, 4 );
			side_writer->write( granule->window_switching, 1 );
			if ( granule->window_switching == 0 ) { // for normal blocks
				side_writer->write( granule->region_table[0], 5 );
				side_writer->write( granule->region_table[1], 5 );
				side_writer->write( granule->region_table[2], 5 );
				side_writer->write( granule->region0_size, 4 );
				side_writer->write( granule->region1_size, 3 );
			} else { // for special blocks
				side_writer->write( granule->block_type, 2 );
				side_writer->write( granule->mixed_flag, 1 );
				side_writer->write( granule->region_table[0], 5 );
				side_writer->write( granule->region_table[1], 5 );
				side_writer->write( granule->sb_gain[0], 3 );
				side_writer->write( granule->sb_gain[1], 3 );
				side_writer->write( granule->sb_gain[2], 3 );
			}
			side_writer->write( granule->preemphasis, 1 );
			side_writer->write( granule->coarse_scalefactors, 1 );
			side_writer->write( granule->select_htabB, 1 );
		}
	}
	
	// get pointer, store and free up memory
	tmp_ptr = side_writer->getptr();
	memcpy( sideinfo, tmp_ptr, nsb ); 
	delete( side_writer );
	free( tmp_ptr );
	
	
	// --- ...and optional crc checksum ---
	
	if ( frame->protection == 0x1 ) {
		crc = mp3_calc_layer3_crc( header, sideinfo, nsb );
		header[ 4 ] = (crc>>8)&0xFF;
		header[ 5 ] = (crc>>0)&0xFF;
	}
	
	
	return fixed;	
}


/* -----------------------------------------------
	seeks for the first proper MPEG audio frame
	----------------------------------------------- */
INTERN inline int mp3_seek_firstframe( unsigned char* data, int size )
{
	int mpeg = -1;
	int layer = -1;
	int samples = -1;
	int channels = -1;
	int protection = -1;
	
	int bits;
	int padding;
	int frame_size;
	
	int pos0 = 0;
	int pos1 = 0;
	int pos, n;
	
	
	// check for ID3 or other tag at beginning of data
	pos0 = mp3_get_id3_size( data, size );
	
	// calculate last tolerable seek position
	pos1 = pos0 + GARBAGE_TOLERANCE;
	if ( pos1 > size - 4 ) pos1 = size - 4;
	
	// mp3 frame seeker loop
	// conditions for proper first frame: 5 consecutive frames,
	// same channel, mpeg, layer, samples setting
	while( true ) {
		// seek for first frame candidate
		for ( ; pos0 < pos1; pos0++ )
			if ( data[pos0] == 0xFF ) if ( (data[pos0+1]&0xE0) == 0xE0 ) break;
		// nothing found -> give up
		if ( pos0 == pos1 ) return -1;
		// check for consecutive frames
		for ( pos = pos0, n = 0; ( n < 5 ) && ( pos < size - 4 ); n++, pos += frame_size ) {
			// check syncword
			if ( ( data[pos] != 0xFF ) || ( (data[pos+1]&0xE0) != 0xE0 ) ) break;
			// extract and store or compare data from header
			if ( n == 0 ) {
				mpeg        = (data[pos+1]>>3)&0x3;
				layer       = (data[pos+1]>>1)&0x3;
				protection  = (data[pos+1]>>0)&0x1;
				samples     = (data[pos+2]>>2)&0x3;
				channels    = (data[pos+3]>>6)&0x3;
			} else if (
				( mpeg       != ((data[pos+1]>>3)&0x3) ) ||
				( layer      != ((data[pos+1]>>1)&0x3) ) ||
				( protection != ((data[pos+1]>>0)&0x1) ) ||
				( samples    != ((data[pos+2]>>2)&0x3) ) ||
				( channels   != ((data[pos+3]>>6)&0x3) ) ) break;
			bits     = (data[pos+2]>>4)&0xF;
			padding  = (data[pos+2]>>1)&0x1;			
			// check for problems
			if ( ( mpeg == 0x1 ) || ( layer == 0x0 ) ||
				( bits == 0x0 ) || ( bits == 0xF ) || ( samples == 0x3 ) ) break;
			// find out frame size
			frame_size = frame_size_table[mpeg][layer][samples][bits];
			if ( padding ) frame_size += (layer == LAYER_I) ? 4 : 1;
		}
		// consider first frame proper if 5 consecutive frames are found
		if ( n >= 5 ) break;
		pos0++;		
	}
	
	
	return pos0;
}


/* -----------------------------------------------
	extract ID3v2 tag from beginning of file
	----------------------------------------------- */
INTERN inline int mp3_get_id3_size( unsigned char* id3tag, int max_size )
{
	static const char* id3v1_begin = "TAG";
	static const char* id3v2_begin = "ID3";
	static const char* lyrics3_begin = "LYRICSBEGIN";
	static const char* apetag_begin = "APETAGEX";
	bool unsynchronized = false;
	int size;
	
	
	// check if at least 10 bytes are available
	if ( max_size < 10 ) return 0;
	
	if ( memcmp( id3v2_begin, id3tag, 3 ) == 0 ) {
		// ID3v2 tag -> find out size, start with 10
		size = 10;
		// check for unsynchronization
		unsynchronized = ( BITN( id3tag[5], 7 ) == 1 );
		// check for footer
		size += ( BITN( id3tag[5], 4 ) == 1 ) ? 10 : 0;
		// calculate size
		size += id3tag[9] <<  0;
		size += id3tag[8] <<  7;
		size += id3tag[7] << 14;
		size += id3tag[6] << 21;		
		// check if size is ok
		if ( size > max_size ) return 0;
		// pay attention to unsynchronization where needed
		if ( unsynchronized ) for ( int pos = 0; pos < size-1; pos++ ) {
			if ( id3tag[pos] == 0xFF ) if ( id3tag[pos+1] == 0x00 ) {
				pos++; size++;
				if ( size > max_size ) return 0;
			}
		}
	}
	else if ( memcmp( id3v1_begin, id3tag, 3 ) == 0 ) {
		// ID3v1 tag -> size is exactle 128 byte
		size = 128;
		// check if size is ok
		if ( size > max_size ) return 0;
	}
	else if ( ( memcmp( lyrics3_begin, id3tag, 6 ) == 0 ) && ( max_size >= 148 ) ) {
		// LYRICS3 tag, must be followed by ID3v1 -> doublecheck and find out size
		if ( ( memcmp( lyrics3_begin, id3tag, 11 ) == 0 ) && ( memcmp( id3v1_begin, id3tag + max_size - 128, 3 ) == 0 ) && ( memcmp( lyrics3_begin, (char*) id3tag + max_size - 128 - 9, 6 ) == 0 ) )
			size = max_size;
		else return 0;
	}
	else if ( memcmp( apetag_begin, id3tag, 8 ) == 0 ) {
		// APE tag -> keep all the data after (ok solution for now)
		size = max_size;
	}
	else {
		// no tag header found -> not a proper tag
		return 0;
	}
	
	
	return size;
}


/* -----------------------------------------------
	calculate frame crc
	----------------------------------------------- */
INTERN inline unsigned short mp3_calc_layer3_crc( unsigned char* header, unsigned char* sideinfo, int sidesize )
{
	// crc has a start value of 0xFFFF
	unsigned short crc = 0xFFFF;
	
	// process two last bytes from header...
	crc = (crc << 8) ^ crc_table[(crc>>8) ^ header[2]];
	crc = (crc << 8) ^ crc_table[(crc>>8) ^ header[3]];
	// ... and all the bytes from the side information
	for ( int i = 0; i < sidesize; i++ )
		crc = (crc << 8) ^ crc_table[(crc>>8) ^ sideinfo[i]];
	
	return crc;
}


/* -----------------------------------------------
	decode one MP3 frame
	----------------------------------------------- */
INTERN inline granuleData*** mp3_decode_frame( huffman_reader* dec, mp3Frame* frame )
{
	// storage
	static granuleData*** frame_data = NULL;
	granuleInfo* granule;
	signed short* coefs;
	unsigned char* scfs;
	// scalefactors settings
	const int* slen;
	int sl;	
	// coefficients settings
	short* region_bounds;
	char* region_tables;
	huffman_dec_table* bv_table;
	huffman_conv_set* conv_set;
	signed short* cf_start;
	int linbits;
	// general settings
	unsigned char vals[4];
	int bitp = 0;
	int lmaxp = 0;
	char share;
	// counters
	int ch, gr;
	int p, g, r;
	int i;
	
	
	
	// alloc mem for frame data if not done before
	// no need to free this memory again!
	if ( frame_data == NULL ) {
		frame_data = ( granuleData*** ) calloc( 2, sizeof( granuleData** ) );
		for ( ch = 0; ch < 2; ch++ ) {
			frame_data[ ch ] = ( granuleData** ) calloc( 2, sizeof( granuleData* ) );
			for ( gr = 0; gr < 2; gr++ ) {
				frame_data[ ch ][ gr ] = ( granuleData* ) calloc( 1, sizeof( granuleData ) );
				frame_data[ ch ][ gr ]->scalefactors = ( unsigned char* ) calloc( 36, sizeof( char ) );
				frame_data[ ch ][ gr ]->coefficients = ( signed short* ) calloc( 578, sizeof( short ) );
			}
		}
	}
	
	
	// decode frame
	for ( gr = 0; gr < 2; gr++ ) {
		for ( ch = 0; ch < frame->nchannels; ch++ ) {
			// --- preparations ---
			granule = frame->granules[ch][gr];
			coefs = frame_data[ch][gr]->coefficients;
			scfs = frame_data[ch][gr]->scalefactors;
			cf_start = coefs;
			// clear memory (= set coefficents/scalefactors zero)
			memset( coefs, 0, sizeof( short ) * 576 );
			memset( scfs, 0, sizeof( char ) * 36 );
			// set position in stream
			dec->setpos( frame->main_index + ( bitp / 8 ), 8 - ( bitp % 8 ) );
			dec->reset_counter();
			// take count of inner frame main data bits
			lmaxp = granule->main_data_bit;
			bitp += lmaxp;
			// set decoding parameters
			slen = slength_table[ (int) granule->slength ];
			region_bounds = granule->region_bound;
			region_tables = granule->region_table;
			
			// --- scale factors ---
			// read long block scalefactors (with sharing)
			if ( granule->block_type != SHORT_BLOCK ) {
				// get sharing params if any
				share = ( gr ) ? frame->granules[ch][0]->share : 0;
				// read 21 (max) scalefactors
				for ( g = 0, p = 0; g < 4; g++ ) {
					if ( (share>>(3-g)) & 0x1 ) { // shared
						memcpy( scfs, frame_data[ch][0]->scalefactors + p, sizeof( char) * scf_width[ g ] );
						scfs += scf_width[ g ];
						p = scf_bounds[ g ];
					} else for ( sl = slen[ (g<2)?0:1 ]; p < scf_bounds[ g ]; p++ ) { // non shared
						*(scfs++) = dec->read_bits( sl );
					}
				}
			} else { // read short block scfs
				// read 36 (3*12) scalefactors
				for ( i = 0; i < 3; i++ ) // 3 subblocks
					for ( g = 0, p = 0; g < 3; g++ ) // 3 groups
						for ( sl = slen[ (g==0)?0:1 ]; p < scf_bounds_short[ g ]; p++ ) // no sharing!
							*(scfs++) = dec->read_bits( sl );
			}
			
			// --- coefficients / big values ---
			for ( p = 0, r = 0; r < 3; r++ ) {
				if ( region_tables[ r ] == 0 ) { // tbl0 skipping
					coefs = cf_start + region_bounds[ r ];
					p = region_bounds[ r ];			
					continue;
				}
				// decoding with other tables
				bv_table = bv_dec_table + region_tables[ r ];
				if ( bv_table == NULL ) return NULL;
				conv_set = bv_table->h;
				linbits = bv_table->linbits;
				if ( linbits == 0 ) { // without linbits
					for ( ; p < region_bounds[ r ]; p += 2, coefs += 2 ) {
						dec->decode_pair( conv_set, vals );
						for ( i = 0; i < 2; i++ ) if ( vals[i] > 0 ) 
							coefs[i] = ( dec->read_bit() ) ? -vals[i] : vals[i];
						if ( dec->get_count() > lmaxp ) {
							// high error tolerance!
							memset( coefs, 0, sizeof( short ) * 2 );
							break;
						}
					}
				} else { // with linbits
					for ( ; p < region_bounds[ r ]; p += 2, coefs += 2 ) {
						dec->decode_pair( conv_set, vals );
						for ( i = 0; i < 2; i++ ) if ( vals[i] > 0 ) {
							coefs[i] = ( vals[i] == 15 ) ? 15 + dec->read_bits( linbits ) : vals[i];
							if ( dec->read_bit() ) coefs[i] = - coefs[i];
						}
						if ( dec->get_count() > lmaxp ) {
							// high error tolerance!
							memset( coefs, 0, sizeof( short ) * 2 );
							break;
						}
					}
				}
			}
			
			// --- coefficients / small values ---
			conv_set = ( granule->select_htabB ) ? &htabB_dec : &htabA_dec;
			for ( ; ( dec->get_count() < lmaxp ) && ( p < 576 ); p += 4, coefs += 4 ) {
				dec->decode_quadruple( conv_set, vals );
				for ( i = 0; i < 4; i++ ) if ( vals[i] ) 
					coefs[i] = ( dec->read_bit() ) ? -1 : 1;
				if ( dec->get_count() > lmaxp ) {
					memset( coefs, 0, sizeof( short ) * 4 );
					break;
				}
			}
			
			// set sv_bound
			granule->sv_bound = p;
		}
	}
	
	
	
	return frame_data;
}

/* ----------------------- End of MP3 specific functions -------------------------- */

/* ----------------------- Begin of PMP specific functions -------------------------- */


/* -----------------------------------------------
	writes the PMP file header
	----------------------------------------------- */	
INTERN inline bool pmp_write_header( iostream* str )
{
	unsigned char header[4] = { 0 };
	unsigned char nframes[4] = { 0 };
	
	
	// build the header[]
	// store necessary information, ignore unsupported
	// 1st byte: global samples and channels, and...
	// ...bitrate (zero if vbr/not global)
	header[0] |= i_samplerate << 6; // sample rate
	header[0] |= i_channels << 4; // channel mode
	header[0] |= ( (i_bitrate!=-1) ? i_bitrate : 0 ) << 0; // bitrate
	// 2nd byte: usage of padding, ms/int stereo, special blocks, subblock gain, ...
	// ... sharing, preemphasis and coarse scalefactors
	header[1] |= ( (i_padding==0) ? 0 : 1 ) << 7; // padding
	header[1] |= ( (i_stereo_ms==0 ) ? 0 : 1 ) << 6; // ms stereo
	header[1] |= ( (i_stereo_int==0 ) ? 0 : 1 ) << 5; // int stereo
	header[1] |= ( (i_sblocks==0 ) ? 0 : 1 ) << 4; // special blocks
	header[1] |= ( (i_sbgain==0) ? 0 : 1 ) << 3; // subblock gain
	header[1] |= ( (i_share==0) ? 0 : 1 ) << 2; // sharing
	header[1] |= ( (i_preemphasis==0) ? 0 : 1 ) << 1; // preemphasis
	header[1] |= ( (i_coarse==0) ? 0 : 1 ) << 0; // coarse scfs
	// 3rd byte: setting of protection, original, copyright and private bits, ...
	// ... emphasis setting and indicators for data before/after, special block diffs, bad first frames
	header[2] |= i_protection << 7; // protection
	header[2] |= i_original << 6; // original bit
	header[2] |= i_copyright << 5; // copyright bit
	header[2] |= i_privbit << 4; // private bit
	header[2] |= i_emphasis << 2; // emphasis
	header[2] |= ( (data_before_size>0) ? 1 : 0 ) << 1; // data before
	header[2] |= ( (data_after_size>0) ? 1 : 0 ) << 0; // data after
	// 4th byte: usage of bit reservoir, special block diffs, bad first frames
	// ... still 5 free (use for max comp)
	header[3] |= ( (i_bit_res==0) ? 0 : 1 ) << 7; // bit reservoir
	header[3] |= ( (i_sb_diff==0) ? 0 : 1 ) << 6; // special block diffs
	header[3] |= ( (n_bad_first>0) ? 1 : 0 ) << 5; // bad first frames
	
	// store # of frames in nframes[] in little endian
	// yup, that's a waste, but - so what?
	nframes[0] = ( g_nframes >> 24 ) & 0xFF;
	nframes[1] = ( g_nframes >> 16 ) & 0xFF;
	nframes[2] = ( g_nframes >>  8 ) & 0xFF;
	nframes[3] = ( g_nframes >>  0 ) & 0xFF;
	
	// 8 bytes to write, together with magic # and version: 11 bytes
	str->write( (void*) header, 1, 4 );
	str->write( (void*) nframes, 1, 4 );
	
		
	return true;
}


/* -----------------------------------------------
	reads the PMP file header
	----------------------------------------------- */
INTERN inline bool pmp_read_header( iostream* str )
{
	unsigned char header[4] = { 0 };
	unsigned char nframes[4] = { 0 };
	
	mp3Frame* frame = NULL;
	int n;
	
	
	// read header and size from file
	str->read( (void*) header, 1, 4 );
	str->read( (void*) nframes, 1, 4 );
	if ( str->chkeof() ) {
		sprintf( errormessage, "unexpected end of data" );
		errorlevel = 2;
		return false;
	}
	
	// extract information from header[]
	// 1st byte: global samples and channels, and...
	// ...bitrate (zero if vbr/not global)
	i_samplerate 	= (header[0]>>6)&0x3;
	i_channels 		= (header[0]>>4)&0x3;
	i_bitrate		= (header[0]>>0)&0xF;
	if ( i_bitrate == 0 ) i_bitrate =-1;
	// 2nd byte: usage of padding, ms/int stereo, special blocks, subblock gain, ...
	// ... sharing, preemphasis and coarse scalefactors
	i_padding 		= -((header[1]>>7)&0x1);
	i_stereo_ms		= -((header[1]>>6)&0x1);
	i_stereo_int	=  ((header[1]>>5)&0x1);
	i_sblocks		=  ((header[1]>>4)&0x1);
	i_sbgain 		= -((header[1]>>3)&0x1);
	i_share			= -((header[1]>>2)&0x1);
	i_preemphasis	= -((header[1]>>1)&0x1);
	i_coarse		= -((header[1]>>0)&0x1);
	// 3rd byte: setting of protection, original, copyright and private bits, ...
	// ... emphasis setting and indicators for data before/after, special block diffs, bad first frames
	i_protection	= (header[2]>>7)&0x1;
	i_original		= (header[2]>>6)&0x1;
	i_copyright		= (header[2]>>5)&0x1;
	i_privbit		= (header[2]>>4)&0x1;
	i_emphasis		= (header[2]>>2)&0x3;
	data_before_size	= (header[2]>>1)&0x1;
	data_after_size		= (header[2]>>0)&0x1;
	// 4th byte: usage of bit reservoir, special block diffs, bad first frames
	// ... still 5 free (use for max comp)
	i_bit_res		= -((header[3]>>7)&0x1);
	i_sb_diff		=  ((header[3]>>6)&0x1);
	n_bad_first		=  (header[3]>>5)&0x1;
	
	// set nframes known i_variables
	i_mpeg			= MP3_V1_0;
	i_layer			= LAYER_III;
	i_padbits		= 0;
	i_mixed			= 0;
	i_aux_h			= -2;
	
	// check for possible mistakes made by myself
	if ( ( i_bitrate == 0xF ) || ( i_samplerate == 0x3 ) ) {
		sprintf( errormessage, "not a proper PMP file" );
		errorlevel = 2;
		return false;
	}
	
	// extract # of frames from nframes[] in little endian
	g_nframes  = 0;
	g_nframes |= nframes[0] << 24;
	g_nframes |= nframes[1] << 16;
	g_nframes |= nframes[2] <<  8;
	g_nframes |= nframes[3] <<  0;
		
	// build frames structure
	for ( n = 0; n < g_nframes; n++ ) {
		// read frame, check result
		frame = mp3_build_frame();
		// bad result -> use existing error message and exit
		if ( frame == NULL ) return false;
		// insert frame into the frame chain 
		mp3_append_frame( frame );
	}
	
	// make some safe assumptions
	g_nchannels = ( i_channels == MP3_MONO ) ? 1 : 2;  // number of channels
	g_samplerate = mp3_samplerate_table[(int)i_samplerate];  // sample rate
	g_bitrate = ( i_bitrate == -1 ) ? 0 : mp3_bitrate_table[(int)i_bitrate]; // bit rate
	
	
	return true;
}


#if !defined( STORE_ID3 )
/* -----------------------------------------------
	encodes a stream of generic data (8bit)
	----------------------------------------------- */
INTERN bool pmp_encode_id3( aricoder* enc )
{
	// this will be used for id3 tags, other tags and garbage
	// little is known, so we'll just use a generic markov model
	model_s* model;
	int i;
	
	
	// arithmetic encode data
	model = INIT_MODEL_S( 256 + 1, 256, 0 );
	// data before main data
	if ( data_before_size > 0 ) {
		for ( i = 0; i < data_before_size; i++ )
			encode_ari( enc, model, data_before[ i ] );
		// encode end-of-data symbol (256)
		encode_ari( enc, model, 256 );
	}
	// data after main data
	if ( data_after_size > 0 ) {
		for ( i = 0; i < data_after_size; i++ )
			encode_ari( enc, model, data_after[ i ] );
		// encode end-of-data symbol (256)
		encode_ari( enc, model, 256 );
	}
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decodes a stream of generic data (8 bit)
	----------------------------------------------- */
INTERN bool pmp_decode_id3( aricoder* dec )
{
	abytewriter* bwrt;
	model_s* model;
	int c;
	
	
	// decode max. 2 chunks of data, ending with 256 symbol
	model = INIT_MODEL_S( 256 + 1, 256, 0 );
	
	// data before main data
	if ( data_before_size > 0 ) {
		bwrt = new abytewriter( 1024 ); // start byte writer
		while ( true ) {
			c = decode_ari( dec, model );
			if ( c == 256 ) break;
			bwrt->write( (unsigned char) c );
		}	
		// check for out of memory
		if ( bwrt->error ) {
			delete bwrt; delete model;
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
		// get data/true length and close byte writer
		data_before = bwrt->getptr();
		data_before_size = bwrt->getpos();
		delete bwrt;
	}
	
	// data after main data
	if ( data_after_size > 0 ) {
		bwrt = new abytewriter( 1024 ); // start byte writer
		while ( true ) {
			c = decode_ari( dec, model );
			if ( c == 256 ) break;
			bwrt->write( (unsigned char) c );
		}	
		// check for out of memory
		if ( bwrt->error ) {
			delete bwrt; delete model;
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
		// get data/true length and close byte writer
		data_after = bwrt->getptr();
		data_after_size = bwrt->getpos();
		delete bwrt;
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}
#else
/* -----------------------------------------------
	stores a stream of generic data (8bit)
	----------------------------------------------- */
INTERN inline int pmp_store_data( iostream* str, unsigned char* data, int size )
{
	unsigned char ezis[4]; // little endian size
	
	
	// store size in little endian
	ezis[0] = ( size >> 24 ) & 0xFF;
	ezis[1] = ( size >> 16 ) & 0xFF;
	ezis[2] = ( size >>  8 ) & 0xFF;
	ezis[3] = ( size >>  0 ) & 0xFF;
	
	// the rest is as simple as it gets...
	str->write( ezis, sizeof( char ),    4 );
	str->write( data, sizeof( char ), size );
	
	
	return size;
}


/* -----------------------------------------------
	unstores a stream of generic data (8bit)
	----------------------------------------------- */
INTERN inline int pmp_unstore_data( iostream* str, unsigned char** data )
{
	unsigned char ezis[4]; // little endian size
	int size = 0; // integer size
	
	
	// unstore little endian size
	str->read( ezis, sizeof( char ), 4 );
	size |= ezis[0] << 24;
	size |= ezis[1] << 16;
	size |= ezis[2] <<  8;
	size |= ezis[3] <<  0;
	
	// alloc memory for data
	*data = (unsigned char*) calloc( size, sizeof( char ) );
	if ( *data == NULL ) {
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return -1;
	}
	
	// read data to memory
	str->read( *data, sizeof( char ), size );
	
	// check for eof trouble
	if ( str->chkeof() ) {
		sprintf( errormessage, "unexpected end of data" );
		errorlevel = 2;
		return -1;
	}
	
	
	return size;	
}
#endif


/* -----------------------------------------------
	encode the padding bit (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_padding( aricoder* enc )
{
	// padding bit:
	// - no correlation with others at all
	// - always comes in perfectly predictable run/length pairs
	// -> use run length encoding and arithmetic compression
	// (can still be improved upon)
	
	model_s* model;
	int run = 0;
	char bit = 0;
	
	
	// encoding start
	model = INIT_MODEL_S( PADDING_MAX_RUN+1, PADDING_MAX_RUN+1, 1 );
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
		if ( frame->padding != bit ) {
			bit ^= 0x1;
			while ( run >= PADDING_MAX_RUN+1 ) {
				encode_ari( enc, model, PADDING_MAX_RUN );
				model->shift_context( PADDING_MAX_RUN );
				encode_ari( enc, model, 0 );
				model->shift_context( 0 );
				run -= PADDING_MAX_RUN;
			}
			encode_ari( enc, model, run );
			model->shift_context( run );
			run = 0;
		}
		run++;
	}
	// encode last run
	if ( run > 0 ) {
		while ( run >= PADDING_MAX_RUN+1 ) {
			encode_ari( enc, model, PADDING_MAX_RUN );
			model->shift_context( PADDING_MAX_RUN );
			encode_ari( enc, model, 0 );
			model->shift_context( 0 );
			run -= PADDING_MAX_RUN;
		}
		encode_ari( enc, model, run );
	}
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode the padding bit (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_padding( aricoder* dec )
{
	model_s* model;
	int run = 0;
	char bit = 0;
	
	
	// decoding start
	model = INIT_MODEL_S( PADDING_MAX_RUN+1, PADDING_MAX_RUN+1, 1 );
	// decode the first run
	run = decode_ari( dec, model );
	model->shift_context( run );
	// ... and all others
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
		while ( run == 0 ) {
			bit ^= 0x1;
			run = decode_ari( dec, model );
			model->shift_context( run );			
		}
		run--;
		frame->padding = bit;
	}
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode block types (1-3 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_block_types( aricoder* enc )
{
	// block types:
	// - consist of two things: switching bit and block type spec
	// - safe for the first few frames, block type spec is predictable
	// - switching occurs periodically
	// - ch1 block types are often identical to ch0 block types
	// -> use run length encoding for switching and predict specs
	
	granuleInfo* granule;
	model_s* mod_sw;
	model_s* mod_bt;
	int run = 0;
	char bit = 0;
	int ctx;
	int c;
	
	
	// init models
	mod_sw = INIT_MODEL_S( SWITCHING_MAX_RUN+1, SWITCHING_MAX_RUN+1, 1 );
	mod_bt = INIT_MODEL_S( 4, 4, 1 );
	
	// encoding start
	for ( int ch = 0; ch < g_nchannels; ch++ ) {
		if ( ( i_sb_diff == 0 ) && ( ch == 1 ) ) break;	
		mod_sw->shift_context( 0 ); run = 0;
		// first step - encode the switching runs
		for ( bit = 0, granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			// encode the switching run
			if ( granule->window_switching != bit ) {
				bit ^= 0x1;
				while ( run >= SWITCHING_MAX_RUN+1 ) {
					encode_ari( enc, mod_sw, SWITCHING_MAX_RUN );
					mod_sw->shift_context( SWITCHING_MAX_RUN );
					encode_ari( enc, mod_sw, 0 );
					mod_sw->shift_context( 0 );
					run -= SWITCHING_MAX_RUN;
				}
				encode_ari( enc, mod_sw, run );
				mod_sw->shift_context( run );
				run = 0;
			}
			run++;
		}
		if ( run > 0 ) { // encode last run
			while ( run >= SWITCHING_MAX_RUN+1 ) {
				encode_ari( enc, mod_sw, SWITCHING_MAX_RUN );
				mod_sw->shift_context( SWITCHING_MAX_RUN );
				encode_ari( enc, mod_sw, 0 );
				mod_sw->shift_context( 0 );
				run -= SWITCHING_MAX_RUN;
			}
			encode_ari( enc, mod_sw, run );
			mod_sw->shift_context( run );
		}		
		// second step - encode the block types
		c = STOP_BLOCK;
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			if ( granule->window_switching ) {
				if ( c == STOP_BLOCK ) ctx = START_BLOCK;
				else if ( granule->next != NULL )
					ctx = ( granule->next->window_switching ) ? SHORT_BLOCK : STOP_BLOCK;
				else ctx = STOP_BLOCK;
				mod_bt->shift_context( ctx );
				c = granule->block_type;
				encode_ari( enc, mod_bt, c );				
			}
		}
	}

	// done, delete models
	delete( mod_sw );
	delete( mod_bt );
	
	
	return true;
}


/* -----------------------------------------------
	decode block types (1-3 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_block_types( aricoder* dec )
{
	granuleInfo* granule0;
	granuleInfo* granule1;
	model_s* mod_sw;
	model_s* mod_bt;
	char bit = 0;
	int run;
	int ctx;
	int c;
	
	
	// init models
	mod_sw = INIT_MODEL_S( SWITCHING_MAX_RUN+1, SWITCHING_MAX_RUN+1, 1 );
	mod_bt = INIT_MODEL_S( 4, 4, 1 );
	
	// decoding start
	for ( int ch = 0; ch < g_nchannels; ch++ ) {
		if ( ( i_sb_diff == 0 ) && ( ch == 1 ) ) break;
		mod_sw->shift_context( 0 );
		// first step - decode the switching runs
		// decode the first run
		run = decode_ari( dec, mod_sw );
		mod_sw->shift_context( run );
		// ... and all others
		for ( bit = 0, granule0 = firstframe->granules[ch][0]; granule0 != NULL; granule0 = granule0->next ) {
			while ( run == 0 ) {
				bit ^= 0x1;
				run = decode_ari( dec, mod_sw );
				mod_sw->shift_context( run );			
			}
			run--;
			granule0->window_switching = bit;
		}
		// second step - decode the block types
		c = STOP_BLOCK;
		for ( granule0 = firstframe->granules[ch][0]; granule0 != NULL; granule0 = granule0->next ) {
			if ( granule0->window_switching ) {
				if ( c == STOP_BLOCK ) ctx = START_BLOCK;
				else if ( granule0->next != NULL )
					ctx = ( granule0->next->window_switching ) ? SHORT_BLOCK : STOP_BLOCK;
				else ctx = STOP_BLOCK;
				mod_bt->shift_context( ctx );
				c = decode_ari( dec, mod_bt );
				granule0->block_type = c;
			} else granule0->block_type = LONG_BLOCK;
		}
	}

	// done, delete model
	delete( mod_sw );
	delete( mod_bt );
	
	// copy ch1 block types to ch0 if identical
	if ( ( i_sb_diff == 0 ) && ( g_nchannels == 2 ) ) {
		granule1 = firstframe->granules[1][0];
		for ( granule0 = firstframe->granules[0][0]; granule0 != NULL; granule0 = granule0->next ) {
			granule1->window_switching = granule0->window_switching;
			granule1->block_type = granule0->block_type;
			granule1 = granule1->next;
		}
	}
	
	
	return true;
}


/* -----------------------------------------------
	encode the global gain (8 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_global_gain( aricoder* enc )
{
	// global gain:
	// - high correlation with everything else
	// - high noise
	// - difficult to predict from each other
	// -> encode using differential coding and 0th order model
	// this will serve as context for everything else
	
	granuleInfo* granule0;
	granuleInfo* granule1;
	model_s* model;
	int last = 0;
	int c = 0;
	
	
	// set up model (will use one model for both channels)
	model = INIT_MODEL_S( 256, 0, 0 );
	
	// --- first channel ---
	// model->shift_context( 0 );
	for ( granule0 = firstframe->granules[0][0]; granule0 != NULL; granule0 = granule0->next ) {
		c = ( granule0->global_gain - last ) & 0xFF;
		last = granule0->global_gain;
		encode_ari( enc, model, c );
	}
	
	// --- second channel ---
	if ( g_nchannels == 2 ) {
		// model->shift_context( 0 );
		granule0 = firstframe->granules[0][0];
		for ( granule1 = firstframe->granules[1][0]; granule1 != NULL; granule1 = granule1->next ) {
			c = ( granule1->global_gain - granule0->global_gain ) & 0xFF;
			granule0 = granule0->next;
			encode_ari( enc, model, c );
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode the global gain (8 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_global_gain( aricoder* dec )
{
	granuleInfo* granule0;
	granuleInfo* granule1;
	model_s* model;
	int last = 0;
	int c = 0;
	
	
	// set up model (will use one model for both channels)
	model = INIT_MODEL_S( 256, 0, 0 );
	
	// --- first channel ---
	// model->shift_context( 0 );
	for ( granule0 = firstframe->granules[0][0]; granule0 != NULL; granule0 = granule0->next ) {
		c = decode_ari( dec, model );
		// model->shift_context( c );
		last = ( c + last ) & 0xFF;
		granule0->global_gain = last;
	}
	
	// --- second channel ---
	if ( g_nchannels == 2 ) {
		// model->shift_context( 0 );
		granule0 = firstframe->granules[0][0];
		for ( granule1 = firstframe->granules[1][0]; granule1 != NULL; granule1 = granule1->next ) {
			c = decode_ari( dec, model );
			// model->shift_context( c );
			granule1->global_gain = ( c + granule0->global_gain ) & 0xFF;
			granule0 = granule0->next;
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode the slength (4 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_slength( aricoder* enc )
{
	// slength:
	// - high correlation with global gain
	// - different correlation for ch0 and ch1
	// - predictable from each other to some degree
	// -> encode using gg_context and markov model
	
	unsigned char* gg_ctx;
	granuleInfo* granule;
	model_s* model;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_S( 16, (GG_CONTEXT_SIZE > 16 ) ? GG_CONTEXT_SIZE : 16, 2 );
	
	// encoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		gg_ctx = gg_context[ ch ]; c = 0;
		// encode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			shift_model( model, *(gg_ctx++), c );
			c = granule->slength;
			encode_ari( enc, model, c );
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode the slength (4 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_slength( aricoder* dec )
{
	unsigned char* gg_ctx;
	granuleInfo* granule;
	model_s* model;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_S( 16, (GG_CONTEXT_SIZE > 16 ) ? GG_CONTEXT_SIZE : 16, 2 );
	
	// decoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		gg_ctx = gg_context[ ch ]; c = 0;
		// decode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			shift_model( model, *(gg_ctx++), c );
			c = decode_ari( dec, model );
			granule->slength = c;
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode ms stereo setting (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_stereo_ms( aricoder* enc )
{
	// stereo ms bit:
	// - only occurs in joint stereo mode
	// - predictable from each other to some degree
	// -> encode using markov model
	
	model_b* model;
	unsigned char ctx = 0;
	
	
	// encoding start
	model = INIT_MODEL_B( 16, 1 );
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
		model->shift_context( ctx );
		encode_ari( enc, model, frame->stereo_ms );
		ctx = ( ( ctx << 1 ) | frame->stereo_ms ) & 0xF;
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode ms stereo setting (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_stereo_ms( aricoder* dec )
{
	model_b* model;
	unsigned char ctx = 0;
	
	
	// decoding start
	model = INIT_MODEL_B( 16, 1 );
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
		model->shift_context( ctx );
		frame->stereo_ms = decode_ari( dec, model );
		ctx = ( ( ctx << 1 ) | frame->stereo_ms ) & 0xF;
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode region bounds and tables (32 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_region_data( aricoder* enc )
{
	// region bounds:
	// - little correlation with global gain
	// - high correlation among each others
	// - no good context for bv bounds at all
	// - sv bounds have to be processed elsewhere
	// -> different treatment for r0, r1 and bv:
	// -> r0: markov model and bv context
	// -> r1: r0 and bv context
	// -> bv: block type context
	//
	// huffman table selection:
	// - high correlation with global gain
	// - high correlation between neighbors and tables
	// -> encode using gg_context and markov model
	// -> each region gets its own model

	
	const int* bw_conv = mp3_bandwidth_conv[ (int) i_samplerate ];
	unsigned char* gg_ctx;
	granuleInfo* granule;
	model_s* mod_t0;
	model_s* mod_t1;
	model_s* mod_t2;
	model_b* mod_ts;
	model_s* mod_s0;
	model_s* mod_s1;
	model_s* mod_bv;
	int t_r0, t_r1, t_r2, t_sv;
	int s_r0, s_r2;
	// int s_r0, s_r1, s_r2;
	int ctx_sv;
	int ch;
	
	
	// set up models (one for each region table and size)
	mod_t0 = INIT_MODEL_S( 32, (GG_CONTEXT_SIZE > 32 ) ? GG_CONTEXT_SIZE : 32, 2 );
	mod_t1 = INIT_MODEL_S( 32, 32, 2 );
	mod_t2 = INIT_MODEL_S( 32, 32, 2 );
	mod_ts = INIT_MODEL_B( 16, 1 );
	mod_s0 = INIT_MODEL_S( 16, 22, 2 );
	mod_s1 = INIT_MODEL_S( 8, 22, 2 );
	mod_bv = INIT_MODEL_S( (576/2)+1, 1+1, 1 );
	
	// encoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush models
		mod_s0->flush_model( 1 );
		mod_s1->flush_model( 1 );
		mod_bv->flush_model( 1 );
		// reset context
		gg_ctx = gg_context[ ch ]; ctx_sv = 0;
		t_r0 = 0; t_r1 = 0; t_r2 = 0; t_sv = 0;
		// s_r0 = 0; s_r1 = 0; s_r2 = 0;
		s_r0 = 0; s_r2 = 0;
		// encode all region bounds and tables for one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			// --- region bounds (big_val_pairs, region0_size, region2_size) ---
			if ( granule->big_val_pairs > 576 / 2 ) {
				sprintf( errormessage, "big value pairs out of bounds (%ch>%ch)", granule->big_val_pairs, 576 / 2 );
				errorlevel = 2;
				return false;
			}
			if ( granule->window_switching ) {
				// region 2 size (# big value pairs)
				mod_bv->shift_context( ( granule->block_type == SHORT_BLOCK ) ? 1 : 0 );
				encode_ari( enc, mod_bv, granule->big_val_pairs );
				s_r0 = 0; // s_r1 = 0;
			} else {
				s_r2 = bw_conv[ granule->big_val_pairs << 1 ];
				// region 2 size (# big value pairs)
				mod_bv->shift_context( 0 );
				encode_ari( enc, mod_bv, granule->big_val_pairs );
				// region 0 size
				shift_model( mod_s0, s_r0, s_r2 );
				s_r0 = granule->region0_size;
				encode_ari( enc, mod_s0, s_r0 );
				// region 1 size
				shift_model( mod_s1, s_r0, s_r2 );
				encode_ari( enc, mod_s1, granule->region1_size );
				// context customizations
				s_r0++; // s_r1 = s_r0 + granule->region1_size + 1;
			}
			// --- region tables (region0/1/2_table, select_htabB) ---
			// region 0 table
			shift_model( mod_t0, *gg_ctx, t_r0 );
			// shift_model( mod_t0, s_r0, t_r0 );
			t_r0 = granule->region_table[0];
			encode_ari( enc, mod_t0, t_r0 );
			// region 1 table
			shift_model( mod_t1, t_r0, s_r0 );
			t_r1 = granule->region_table[1];
			encode_ari( enc, mod_t1, t_r1 );
			// region 2 table
			if ( !granule->window_switching ) {
				shift_model( mod_t2, t_r0, t_r1 );
				t_r2 = granule->region_table[2];
				encode_ari( enc, mod_t2, t_r2 );
			}
			// small values table
			mod_ts->shift_context( ctx_sv );
			t_sv = granule->select_htabB;
			encode_ari( enc, mod_ts, t_sv );
			ctx_sv = ( ( ctx_sv << 1 ) | t_sv ) & 0xF;
			// advance context
			gg_ctx++;
		}
	}
	
	// done, delete models
	delete( mod_t0 );
	delete( mod_t1 );
	delete( mod_t2 );
	delete( mod_ts );
	delete( mod_s0 );
	delete( mod_s1 );
	delete( mod_bv );
	
	
	return true;
}


/* -----------------------------------------------
	decode region sizes and table selection (32 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_region_data( aricoder* dec )
{	
	const int* bw_conv = mp3_bandwidth_conv[ (int) i_samplerate ];
	unsigned char* gg_ctx;
	granuleInfo* granule;
	model_s* mod_t0;
	model_s* mod_t1;
	model_s* mod_t2;
	model_b* mod_ts;
	model_s* mod_s0;
	model_s* mod_s1;
	model_s* mod_bv;
	int t_r0, t_r1, t_r2, t_sv;
	int s_r0, s_r2;
	// int s_r0, s_r1, s_r2;
	int ctx_sv;
	int ch;
	
	
	// set up models (one for each region table and size)
	mod_t0 = INIT_MODEL_S( 32, (GG_CONTEXT_SIZE > 32 ) ? GG_CONTEXT_SIZE : 32, 2 );
	mod_t1 = INIT_MODEL_S( 32, 32, 2 );
	mod_t2 = INIT_MODEL_S( 32, 32, 2 );
	mod_ts = INIT_MODEL_B( 16, 1 );
	mod_s0 = INIT_MODEL_S( 16, 22, 2 );
	mod_s1 = INIT_MODEL_S( 8, 22, 2 );
	mod_bv = INIT_MODEL_S( (576/2)+1, 1+1, 1 );
	
	// decoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		mod_s0->flush_model( 1 );
		mod_s1->flush_model( 1 );
		mod_bv->flush_model( 1 );
		// reset context
		gg_ctx = gg_context[ ch ]; ctx_sv = 0;
		t_r0 = 0; t_r1 = 0; t_r2 = 0; t_sv = 0;
		// s_r0 = 0; s_r1 = 0; s_r2 = 0;
		s_r0 = 0; s_r2 = 0;
		// decode all region tables for one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			// --- region bounds (big_val_pairs, region0_size, region2_size) ---
			if ( !granule->window_switching ) {
				// region 2
				mod_bv->shift_context( 0 );
				granule->big_val_pairs = decode_ari( dec, mod_bv );
				s_r2 = bw_conv[ granule->big_val_pairs << 1 ];
				// region 0
				shift_model( mod_s0, s_r0, s_r2 );
				s_r0 = decode_ari( dec, mod_s0 );
				granule->region0_size = s_r0;
				// region 1
				shift_model( mod_s1, s_r0, s_r2 );
				granule->region1_size = decode_ari( dec, mod_s1 );
				// set region 1/2 bounds
				granule->region_bound[0] =
					mp3_bandwidth_bounds[(int)i_samplerate][s_r0+1];
				granule->region_bound[1] =
					mp3_bandwidth_bounds[(int)i_samplerate][s_r0+granule->region1_size+2];
				// set bv bound and check other bounds
				granule->region_bound[2] = granule->big_val_pairs << 1;
				if ( granule->region_bound[0] > granule->region_bound[2] ) {
					granule->region_bound[0] = granule->region_bound[2];
					granule->region_bound[1] = granule->region_bound[2];
				} else if ( granule->region_bound[1] > granule->region_bound[2] )
					granule->region_bound[1] = granule->region_bound[2];
				// context customizations
				s_r0++; // s_r1 = s_r0 + granule->region1_size + 1;
			} else if ( granule->block_type != SHORT_BLOCK ) {
				// region sizes for different block types
				granule->region0_size = 8;
				granule->region1_size = 0;
				// set region bound
				granule->region_bound[0] =
					mp3_bandwidth_bounds[(int)i_samplerate][8];
				// decode bv pairs, set bound and check r0 bound
				mod_bv->shift_context( 0 );
				granule->big_val_pairs = decode_ari( dec, mod_bv );
				granule->region_bound[1] = granule->big_val_pairs << 1;
				if ( granule->region_bound[0] > granule->region_bound[1] )
					granule->region_bound[0] = granule->region_bound[1];
				granule->region_bound[2] = granule->region_bound[1];
				// context setting
				s_r0 = 0; // s_r1 = 0;
			} else { // special treatment for mixed blocks needed (!)
				granule->region0_size = 9;
				granule->region1_size = 0;
				// set region bound
				granule->region_bound[0] =
					mp3_bandwidth_bounds_short[(int)i_samplerate][9/3] * 3;
				// decode bv pairs, set bound and check r0 bound
				mod_bv->shift_context( 1 );
				granule->big_val_pairs = decode_ari( dec, mod_bv );
				granule->region_bound[1] = granule->big_val_pairs << 1;
				if ( granule->region_bound[0] > granule->region_bound[1] )
					granule->region_bound[0] = granule->region_bound[1];
				granule->region_bound[2] = granule->region_bound[1];
				// context setting
				s_r0 = 0; // s_r1 = 0;
			}
			// --- region tables (region0/1/2_table, select_htabB) ---
			// region 0 table
			shift_model( mod_t0, *gg_ctx, t_r0 );
			t_r0 = decode_ari( dec, mod_t0 );
			granule->region_table[0] = t_r0;
			// region 1 table
			shift_model( mod_t1, t_r0, s_r0 );
			t_r1 = decode_ari( dec, mod_t1 );
			granule->region_table[1] = t_r1;
			// region 2 table
			if ( !granule->window_switching ) {
				shift_model( mod_t2, t_r0, t_r1 );
				t_r2 = decode_ari( dec, mod_t2 );
				granule->region_table[2] = t_r2;
			} else granule->region_table[2] = 0;
			// small values table
			mod_ts->shift_context( ctx_sv );
			t_sv = decode_ari( dec, mod_ts );
			granule->select_htabB = t_sv;
			ctx_sv = ( ( ctx_sv << 1 ) | t_sv ) & 0xF;
			// advance context
			gg_ctx++;
		}
	}
	
	// done, delete models
	delete( mod_t0 );
	delete( mod_t1 );
	delete( mod_t2 );
	delete( mod_ts );
	delete( mod_s0 );
	delete( mod_s1 );
	delete( mod_bv );
	
	
	return true;
}

/* -----------------------------------------------
	encode scalefactor sharing (2/4 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_sharing( aricoder* enc )
{
	// scalefactor sharing:
	// - comes in packs of four
	// - only every second granule matters
	// - high correlation with slength
	// - high correlation among fourpacks
	// -> encode using markov model + slength context
	
	granuleInfo* granule;
	model_s* model;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_S( 16, 16, 3 );
	
	// encoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		c = 0;
		// encode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next->next ) {
			shift_model( model, c, granule->slength, granule->next->slength );
			c = granule->share;
			encode_ari( enc, model, c );
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode scalefactor sharing (2/4 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_sharing( aricoder* dec )
{
	granuleInfo* granule;
	model_s* model;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_S( 16, 16, 3 );
	
	// decoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		c = 0;
		// decode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next->next ) {
			shift_model( model, c, granule->slength, granule->next->slength );
			c = decode_ari( dec, model );
			granule->share = c;
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode the preemphasis setting (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_preemphasis( aricoder* enc )
{
	// preemphasis:
	// - little correlation with global gain
	// - near impossible to predict from each other
	// - high differences between ch0 and ch1 (joint)
	// -> encode using simple markov model
	// -> improve later
	
	granuleInfo* granule;
	model_b* model;
	int ctx;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_B( 16, 1 );
	
	// encoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		ctx = 0;
		// encode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			model->shift_context( ctx );
			c = granule->preemphasis;
			encode_ari( enc, model, c );
			ctx = ( ( ctx << 1 ) | c ) & 0xF;
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode the preemphasis setting (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_preemphasis( aricoder* dec )
{
	granuleInfo* granule;
	model_b* model;
	int ctx;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_B( 16, 1 );
	
	// decoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		ctx = 0;
		// decode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			model->shift_context( ctx );
			c = decode_ari( dec, model );
			granule->preemphasis = c;
			ctx = ( ( ctx << 1 ) | c ) & 0xF;
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode the coarse sf setting (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_coarse_sf( aricoder* enc )
{
	// coarse scalefactors
	// - little correlation with global gain
	// - near impossible to predict from each other
	// - differences between ch0 and ch1 (joint)
	// -> encode using simple markov model
	// -> improve later
	
	granuleInfo* granule;
	model_b* model;
	int ctx;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_B( 16, 1 );
	
	// encoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		ctx = 0;
		// encode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			model->shift_context( ctx );
			c = granule->coarse_scalefactors;
			encode_ari( enc, model, c );
			ctx = ( ( ctx << 1 ) | c ) & 0xF;
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode the coarse sf setting (1 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_coarse_sf( aricoder* dec )
{
	granuleInfo* granule;
	model_b* model;
	int ctx;
	int ch;
	int c;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_B( 16, 1 );
	
	// decoding start
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// reset and flush model
		model->flush_model( 1 );
		ctx = 0;
		// decode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			model->shift_context( ctx );
			c = decode_ari( dec, model );
			granule->coarse_scalefactors = c;
			ctx = ( ( ctx << 1 ) | c ) & 0xF;
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode the subblock gain (9 bit)
	----------------------------------------------- */
INTERN inline bool pmp_encode_subblock_gain( aricoder* enc )
{
	// subblock gain:
	// - only occurs for short blocks
	// - little correlation with each other
	// - no visible correlation with anything else
	// -> use simple markov model
	
	granuleInfo* granule;
	model_s* model;
	int ch, sb;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_S( 8, 8, 1 );
	
	// decoding start
	model->shift_context( 0 );
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// encode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			if ( granule->window_switching ) {
				for ( sb = 0; sb < 3; sb++ ) {
					encode_ari( enc, model, granule->sb_gain[sb] );
					model->shift_context( granule->sb_gain[sb] );
				}
			}
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	decode the subblock gain (9 bit)
	----------------------------------------------- */
INTERN inline bool pmp_decode_subblock_gain( aricoder* dec )
{
	granuleInfo* granule;
	model_s* model;
	int ch, sb;
	
	
	// set up model (one model for both channels, flush in between)
	model = INIT_MODEL_S( 8, 8, 1 );
	
	// decoding start
	model->shift_context( 0 );
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// decode one channel
		for ( granule = firstframe->granules[ch][0]; granule != NULL; granule = granule->next ) {
			if ( granule->window_switching ) {
				for ( sb = 0; sb < 3; sb++ ) {
					granule->sb_gain[sb] = decode_ari( dec, model );
					model->shift_context( granule->sb_gain[sb] );
				}
			} else memset( granule->sb_gain, 0, sizeof( char ) * 3 );
		}
	}
	
	// done, delete model
	delete( model );
	
	
	return true;
}


/* -----------------------------------------------
	encode the main data
	----------------------------------------------- */
INTERN inline bool pmp_encode_main_data( aricoder* enc )
{
	// main data:
	// consists of scalefactors & coefficients
	// several other data is also encoded here:
	// - sv bound (for performance reasons)
	// - aux data, aux data size & padding bits
	// - reconstruction information (stuffing bits)
	// - bitrate (for performance reasons)
	//
	// scalefactors:
	// correlation with temporal and local neighbours
	// -> encode with neighborhood context
	//
	// coefficients:
	// good routines are already included in mp3 standard,
	// but no temporal context is used there
	// -> remodel mp3 coding with arithmetic coding
	// -> use neighborhood context
	//
	// stuffing bits (between granules):
	// this data either follows a pattern or is garbage
	// -> use 4 bit prev bits context
	//
	// padding bits/aux data (at end of frame):
	// usually follows a specific pattern
	// -> try prediction
	// -> use 8 bit prev bits context
	//
	// sv bound:
	// should be predictable from neighborhood
	// and bv bound
	// -> encode diff with bv bound
	// -> use prev and bv bound context
	//
	// bitrate:
	// high correlation with main size
	// -> encode using main size prediction as context
	
	// context / storage
	static unsigned char* pad_and_aux= ( unsigned char* ) calloc( 2048, 1 ); // !!! (length)
	mp3Frame* frame;
	granuleInfo* granule;
	unsigned char* scf_c[2];
	unsigned char* scf_l_long[2];
	unsigned char* scf_l_short[2];
	unsigned char* abs_c[2];
	unsigned char* sgn_c[2];
	unsigned char* len_c[2];
	unsigned short* lbt_c[2];
	unsigned char* absl_ctx_h[2];
	unsigned char* abss_ctx_h[2];
	unsigned char* sgnl_ctx_h[2];
	unsigned char* sgns_ctx_h[2];
	unsigned char* lenl_ctx_h[2];
	unsigned char* lens_ctx_h[2];
	unsigned char* scf_prev;
	unsigned char* ctx_h_abs;
	unsigned char* ctx_h_sgn;
	unsigned char* ctx_h_len;
	unsigned char* scf;
	unsigned char* abs;
	unsigned char* sgn;
	unsigned char* len;
	unsigned short* lbt;
	unsigned char* swap;
	unsigned char* pna_c;
	unsigned char ctx_scf;
	unsigned char ctx_abs;
	unsigned char ctx_pat;
	unsigned char ctx_svb[2] = { 0, 0 };
	unsigned char ctx_nst = 0;
	unsigned char ctx_bst = 0;
	unsigned char ctx_aux = 0;
	unsigned char ctx_pad = 0;
	// statistical models
	model_s* mod_scf[2][4][10]; // scalefactors
	model_s* mod_abv[2][8][32]; // absolutes big values <= 15
	model_b* mod_asv[2][8][2]; // absolulte small values
	model_b* mod_sgn[2][8]; // signs
	model_s* mod_len[2][14]; // residual bitlengths
	model_b* mod_res; // residual bits
	model_s* mod_svb; // small values bound
	model_s* mod_bvf; // fix for sv bound (usually none)
	model_s* mod_nst; // number of stuffing bits (usually zero)
	model_b* mod_bst; // stuffing bits
	model_b* mod_pad; // padding and ancillary bits
	model_b* mod_pap; // predcition for p&a bits
	model_s* mod_aux; // byte size of ancillary data
	model_s* mod_btr; // frame bitrate
	model_s* mod_abc; // current absolute big values (shortcut)
	model_b* mod_asc; // current absolute small values (shortcut)
	model_b* mod_sgc; // current signs (shortcut)
	model_s* mod_lnc; // current bitlengths (shortcut)
	model_s* mod_sfc; // current scalefactors (shortcut)
	// lookup tables
	const int* bitrate_pred = mp3_bitrate_pred[(int)i_samplerate];
	// huffman decoder
	huffman_reader* huffman;
	// mp3 decoding settings
	short* region_bounds;
	char* region_tables;
	huffman_dec_table* bv_table;
	huffman_conv_set* conv_set;
	int linbits;
	const int* slen;
	int rlb, sl;	
	// general settings
	int bitp = 0;
	int bitc = 0;
	int lmaxp = 0;
	int sbl = 0;
	int flags = 0;
	bool j_coding;
	bool bad_coding = false;
	char shared[2][4];
	// counters
	int ch, gr;
	int p, g, r;
	int i, c, n;
	
	
	// --- PRE-PROCESSING PREPARATIONS: INIT STATISTICAL MODELS/HUFFMAN DECODER/CONTEXT ---
	// decide if joint context will be used (will be used for joint & standard stereo)
	j_coding = ( i_channels == MP3_STEREO ) || ( i_channels == MP3_JOINT_STEREO );
	// init huffman decoder/reader
	huffman = new huffman_reader( main_data, main_data_size );
	// reset ancillary predictor
	pmp_predict_lame_anc( -1, NULL );
	// init statistical models
	mod_svb = INIT_MODEL_S( ( 576 / 4 ) + 1 + 1, ( 576 / 4 ) + 1 + 1, 1 );
	mod_bvf = INIT_MODEL_S( ( 576 / 2 ) + 1, 0, 0 );
	mod_nst = INIT_MODEL_S( STUFFING_STEP + 1, STUFFING_STEP + 1, 1 );
	mod_bst = INIT_MODEL_B( 16, 1 );
	mod_pad = INIT_MODEL_B( 256, 1 );
	mod_pap = INIT_MODEL_B( 0, 0 );
	mod_aux = INIT_MODEL_S( AUX_DATA_STEP + 1, AUX_DATA_STEP + 1, 1 );
	mod_btr = INIT_MODEL_S( 16, 16, 1 );
	mod_res = INIT_MODEL_B( 13 + 1, 2 );
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( g = 0; g < 10; g++ ) {
			mod_scf[ch][0][g] = INIT_MODEL_S(  2, 16, 2 );
			mod_scf[ch][1][g] = INIT_MODEL_S(  4, 16, 2 );
			mod_scf[ch][2][g] = INIT_MODEL_S(  8, 16, 2 );
			mod_scf[ch][3][g] = INIT_MODEL_S( 16, 16, 2 );
		}
		for ( flags = 0x0; flags < ( (j_coding&&ch) ? 0x8 : 0x2 ); flags++ ) {
			mod_abv[ch][flags][ 0] = NULL;
			mod_abv[ch][flags][ 1] = INIT_MODEL_S(  1 + 1, 16, 2 );
			mod_abv[ch][flags][ 2] = INIT_MODEL_S(  2 + 1, 16, 2 );
			mod_abv[ch][flags][ 3] = INIT_MODEL_S(  2 + 1, 16, 2 );
			mod_abv[ch][flags][ 4] = NULL;
			mod_abv[ch][flags][ 5] = INIT_MODEL_S(  3 + 1, 16, 2 );
			mod_abv[ch][flags][ 6] = INIT_MODEL_S(  3 + 1, 16, 2 );
			mod_abv[ch][flags][ 7] = INIT_MODEL_S(  5 + 1, 16, 2 );
			mod_abv[ch][flags][ 8] = INIT_MODEL_S(  5 + 1, 16, 2 );
			mod_abv[ch][flags][ 9] = INIT_MODEL_S(  5 + 1, 16, 2 );
			mod_abv[ch][flags][10] = INIT_MODEL_S(  7 + 1, 16, 2 );
			mod_abv[ch][flags][11] = INIT_MODEL_S(  7 + 1, 16, 2 );
			mod_abv[ch][flags][12] = INIT_MODEL_S(  7 + 1, 16, 2 );
			mod_abv[ch][flags][13] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][14] = NULL;
			mod_abv[ch][flags][15] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][16] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][17] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][18] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][19] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][20] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][21] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][22] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][23] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][24] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][25] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][26] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][27] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][28] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][29] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][30] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][31] = mod_abv[ch][flags][24];
			mod_asv[ch][flags][0] = INIT_MODEL_B( 16, 2 );
			mod_asv[ch][flags][1] = INIT_MODEL_B( 16, 2 );
			mod_sgn[ch][flags] = INIT_MODEL_B( 16, 2 );
		}
		mod_len[ch][0] = NULL;
		for ( i = 1; i <= 13; i++ )
			mod_len[ch][i] = INIT_MODEL_S( i + 1, 13 + 1, 1 );
	}
	
	
	// --- PRE-PROCESSING PREPARATIONS: ALLOCATION OF MEMORY ---
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// alloc memory
		scf_c[ch]       = ( unsigned char* ) calloc(  21, sizeof( char ) );
		scf_l_long[ch]  = ( unsigned char* ) calloc(  21, sizeof( char ) );
		scf_l_short[ch] = ( unsigned char* ) calloc(  21, sizeof( char ) );
		abs_c[ch]       = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		sgn_c[ch]       = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		len_c[ch]       = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		lbt_c[ch]      = ( unsigned short* ) calloc( 576, sizeof( short ) );
		absl_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		abss_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		sgnl_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		sgns_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		lenl_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		lens_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		// check for problems
		if ( ( scf_c[ch] == NULL ) || ( scf_l_long[ch] == NULL ) || ( scf_l_short[ch] == NULL ) ||
			 ( abs_c[ch] == NULL ) || ( absl_ctx_h[ch] == NULL ) || ( abss_ctx_h[ch] == NULL ) ||
			 ( sgn_c[ch] == NULL ) || ( sgnl_ctx_h[ch] == NULL ) || ( sgns_ctx_h[ch] == NULL ) ||
			 ( len_c[ch] == NULL ) || ( lenl_ctx_h[ch] == NULL ) || ( lens_ctx_h[ch] == NULL ) ||
			 ( lbt_c[ch] == NULL ) ) {
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}
	
	
	// --- MAIN PROCESSING LOOP: ENCODING AND DECODING ---
	for ( frame = firstframe; frame != NULL; frame = frame->next, bitp = 0 ) {
		for ( gr = 0; gr < 2; gr++ ) {
			for ( ch = 0; ch < g_nchannels; ch++ ) {
				
				// --- MAIN DATA DECODING: PREPARATIONS ---
				// initialize shortcuts
				granule = frame->granules[ch][gr];
				sbl = ( granule->block_type == SHORT_BLOCK ) ? 1 : 0;
				lmaxp = granule->main_data_bit;
				slen = slength_table[ (int) granule->slength ];
				region_bounds = granule->region_bound;
				region_tables = granule->region_table;
				abs = abs_c[ch] + 1;
				sgn = sgn_c[ch] + 1;
				len = len_c[ch] + 1;
				lbt = lbt_c[ch];
				// set compression flags
				flags = ( !j_coding || ( ch == 0 ) ) ? sbl :
					( sbl << 0 ) | // long (0) or short (1) block
					( frame->stereo_ms << 1 ) | // stereo ms on/off
					( ( flags ^ sbl ) << 2 ); // ch0/ch1 block type diffs y/n
				// store sharing info
				if ( gr == 0 ) {
					shared[ch][0] = ( granule->share >> 3 ) & 0x1;
					shared[ch][1] = ( granule->share >> 2 ) & 0x1;
					shared[ch][2] = ( granule->share >> 1 ) & 0x1;
					shared[ch][3] = ( granule->share >> 0 ) & 0x1;
				}
				// reset counter
				huffman->reset_counter();
				bitc = 0;
				bad_coding = false;
				
				
				// ---> SCALEFACTOR PROCESSING <---
				if ( !sbl ) {
					
					// --- SCALEFACTOR READING/ENCODING: LONG BLOCKS ---
					scf = scf_c[ch];
					scf_prev = scf_l_long[ch];
					ctx_scf = 0;
					
					// read/encode 21 scalefactors with/without sharing
					for ( g = 0, p = 0; g < 4; g++ ) {
						sl = slen[ ( g < 2 ) ? 0 : 1 ];
						if ( ( gr ) & ( shared[ch][g] ) ) { // shared
							// loop invariant conditions (-funswitch-loops)
							memcpy( scf + p, scf_prev + p, scf_width[ g ] );
							p = scf_bounds[ g ];
							ctx_scf = scf[p-1];
						} else if ( sl == 0 ) { // zero slength
							memset( scf + p, 0, scf_width[ g ] );
							p = scf_bounds[ g ];
							ctx_scf = 0;
						} else {
							mod_sfc = mod_scf[ch][sl-1][(shared[ch][g])?g|4:g];
							for ( ; p < scf_bounds[ g ]; p++ ) { // non shared
								scf[p] = huffman->read_bits( sl );
								shift_model( mod_sfc, ctx_scf, scf_prev[p] );
								encode_ari( enc, mod_sfc, scf[p] );
								ctx_scf = scf[p];
							}
						}
					}
					
					// --- SCALEFACTORS FINISHED: SWAP DATA ---
					swap = scf_c[ch]; scf_c[ch] = scf_l_long[ch]; scf_l_long[ch] = swap;
					
				} else {
					
					// --- SCALEFACTOR READING/ENCODING: SHORT BLOCKS ---
					// encode (only non shared)
					for ( i = 0; i < 3; i++ ) { // 3 subblocks
						scf = scf_c[ch];
						scf_prev = scf_l_short[ch];
						ctx_scf = 0;
						for ( g = 0, p = 0; g < 2; g++ ) { // lo/hi groups
							sl = slen[ g ];
							if ( sl == 0 ) { // zero slength
								memset( scf + p, 0, scf_lh_width_short[ g ] );
								p = scf_lh_bounds_short[ g ];
							} else {
								mod_sfc = mod_scf[ch][sl-1][g|0x8];
								for ( ; p < scf_lh_bounds_short[ g ]; p++ ) {
									scf[p] = huffman->read_bits( sl );
									shift_model( mod_sfc, ctx_scf, scf_prev[p] );
									encode_ari( enc, mod_sfc, scf[p] );
									ctx_scf = scf[p];
								}
							}
						}
						// --- SWAP DATA ---
						swap = scf_c[ch]; scf_c[ch] = scf_l_short[ch]; scf_l_short[ch] = swap;
					}
				}
				
				// sanity check
				if ( huffman->get_count() > lmaxp ) { // main data not big enough - no rollback for scfs
					sprintf( errormessage, "huffman decoding error (in frame #%i)", frame->n );
					errorlevel = 2;
					return false;
				}
				
				
				// ---> COEFFICIENTS PROCESSING <---
				
				// --- COEFFICIENT DECODING: BIG VALUES ---
				for ( p = 0, r = 0; ( r < 3 ) && ( !bad_coding ); r++ ) {
					if ( region_tables[ r ] == 0 ) { // tbl0 skipping
						p = region_bounds[ r ];			
						continue;
					}
					// set table and linbits
					bv_table = bv_dec_table + region_tables[ r ];
					if ( bv_table == NULL ) { // illegal table? 
						sprintf( errormessage, "bad huffman table (%i) used (in frame #%i)",
							region_tables[ r ], frame->n );
						errorlevel = 2;
						return false;
					}
					conv_set = bv_table->h;
					linbits = bv_table->linbits;
					
					// decoding with/without linbits
					for ( ; p < region_bounds[ r ]; bitc = huffman->get_count() ) {
						huffman->decode_pair( conv_set, abs + p );
						for ( i = 0; i < 2; i++, p++ ) if ( abs[p] > 0 ) {
							if ( linbits > 0 ) if ( abs[p] == 15 ) {
								// loop invariant condition (-unswitch-loops)
								lbt[p] = huffman->read_bits( linbits );
								len[p] = BITLEN8192P(lbt[p]);
							}
							sgn[p] = huffman->read_bit();
						}
						if ( huffman->get_count() > lmaxp ) { // bad coding rollback
							bad_coding = true;
							abs[--p] = 0; sgn[p] = 0; len[p] = 0;
							abs[--p] = 0; sgn[p] = 0; len[p] = 0;
							huffman->rewind_bits( huffman->get_count() - bitc );
							break; // rollback complete
						}
					}
				}
				
				// --- COEFFICIENT DECODING: SMALL VALUES ---
				if ( !bad_coding ) {
					conv_set = ( granule->select_htabB ) ? &htabB_dec : &htabA_dec;
					for ( bitc = huffman->get_count(); p < 576; bitc = huffman->get_count() ) {
						if ( bitc == lmaxp ) break;
						huffman->decode_quadruple( conv_set, abs + p );
						for ( i = 0; i < 4; i++, p++ ) if ( abs[p] > 0 )
							sgn[p] = huffman->read_bit();
						if ( huffman->get_count() > lmaxp ) { // bad coding rollback
							bad_coding = true;
							abs[--p] = 0; sgn[p] = 0;
							abs[--p] = 0; sgn[p] = 0;
							abs[--p] = 0; sgn[p] = 0;
							abs[--p] = 0; sgn[p] = 0;
							huffman->rewind_bits( huffman->get_count() - bitc );
							break; // rollback complete
						}
					}
				}
				
				
				// --- SETTING AND ENCODING OF SV BOUND ---
				granule->sv_bound = p; // set sv_bound
				if ( !sbl )
					mod_svb->shift_context( ctx_svb[ch] );
				else mod_svb->shift_context( 144 + 1 );
				if ( p >= granule->region_bound[ 2 ] ) { // for proper files: encode actual sv bound		
					c = granule->sv_bound >> 2;
					encode_ari( enc, mod_svb, c );
					if ( !sbl ) ctx_svb[ch] = c; 
				} else { // for broken files: encode negative diff with r2 bound
					encode_ari( enc, mod_svb, 144 + 1 );
					c = ( granule->region_bound[ 2 ] - granule->sv_bound ) >> 1;
					encode_ari( enc, mod_bvf, c );
					// fix bounds
					granule->region_bound[ 2 ] = p;
					for ( i = 1; i >= 0; i-- ) if ( p < granule->region_bound[i] )
						granule->region_bound[i] = p;
					else break;
				}
				
				// --- COEFFICIENT ENCODING: PREPARATIONS ---
				if ( !sbl ) {
					ctx_h_abs = absl_ctx_h[ch]+1;
					ctx_h_sgn = sgnl_ctx_h[ch]+1;
					ctx_h_len = lenl_ctx_h[ch]+1;					
				} else {
					ctx_h_abs = abss_ctx_h[ch]+1;
					ctx_h_sgn = sgns_ctx_h[ch]+1;
					ctx_h_len = lens_ctx_h[ch]+1;
				}
				ctx_abs = 0; ctx_pat = 0;
				rlb = region_bounds[2];
				mod_sgc = mod_sgn[ch][flags];
				
				// --- COEFFICIENT ENCODING: SMALL VALUES ---
				mod_asc = mod_asv[ch][flags][(int) granule->select_htabB];
				for ( p = granule->sv_bound-1; p >= rlb; p-- ) {
					shift_model( mod_asc, ctx_pat, ctx_h_abs[p] );
					encode_ari( enc, mod_asc, abs[p] ); // absolutes
					ctx_pat = ( (ctx_pat<<1) | abs[p] ) & 0xF;
					ctx_abs = ( 2 * abs[p] + ctx_abs + 2 ) / 3;
					if ( abs[p] == 1 ) {
						shift_model( mod_sgc, ctx_h_abs[p], ctx_h_sgn[p] );
						encode_ari( enc, mod_sgc, sgn[p] ); // signs
					}
				}
				
				// --- COEFFICIENT ENCODING: BIG VALUES ---
				for ( r = 2; r >= 0; r-- ) {
					rlb = (r==0) ? 0 : region_bounds[ r-1 ];
					if ( p < rlb ) continue;
					if ( region_tables[ r ] == 0 ) { // tbl0 skipping
						memset( abs + rlb, 0, p - rlb );
						p = rlb; ctx_abs = 0;
						continue;
					}
					// set table and linbits
					bv_table = bv_dec_table + region_tables[ r ];
					linbits = bv_table->linbits;
					mod_abc = mod_abv[ch][flags][(int) region_tables[ r ]];
					mod_lnc = mod_len[ch][linbits];
					
					// encoding with/without linbits
					for ( ; p >= rlb; p-- ) {
						shift_model( mod_abc, ctx_abs, ctx_h_abs[p] );
						encode_ari( enc, mod_abc, abs[p] ); // absolutes
						ctx_abs = ( 2 * abs[p] + ctx_abs + 2 ) / 3;
						if ( abs[p] > 0 ) {
							shift_model( mod_sgc, ctx_h_abs[p], ctx_h_sgn[p] );
							encode_ari( enc, mod_sgc, sgn[p] ); // signs
							if ( linbits > 0 ) if ( abs[p] == 15 ) {
								// loop invariant condition (-funswitch-loops)
								mod_lnc->shift_context( ctx_h_len[p] );
								encode_ari( enc, mod_lnc, len[p] ); // bitlengths
								for ( i = len[p] - 2; i >= 0; i-- ) {
									shift_model( mod_res, len[p], i ); // bit residuals
									encode_ari( enc, mod_res, BITN( lbt[p], i ) );
								}
							}
						}						
					}
				}
				
				// --- COEFFICIENTS FINISHED: UPDATE CONTEXT ---
				p = granule->sv_bound;
				if ( p < 578 ) memset( abs + p, 0, 578 - p );
				// loop invariant condition
				if ( !j_coding || ( ch == 0 ) ) { // !!!
					// channel 0 context
					for ( i = 578-1; i >= 0; i-- ) {
						ctx_h_abs[i] = ( 3 * abs[i] + abs[i-1] + abs[i+1] + 2 * ctx_h_abs[i] + 4 ) / 7;
						if ( abs[i] > 0 ) {
							ctx_h_sgn[i] = ( ( ctx_h_sgn[i] << 1 ) | ( sgn[i] & 0x1 ) ) & 0xF;
							if ( abs[i] == 15 ) ctx_h_len[i] = ( 2 * len[i] + ctx_h_len[i] + 2 ) / 3;
							else ctx_h_len[i] >>= 1;
						} else {
							ctx_h_sgn[i] = ( ctx_h_sgn[i] << 1 ) & 0xF;
							ctx_h_len[i] >>= 1;
						}
					}
					// loop invariant condition
					if ( j_coding ) {
						// channel 1 context
						if ( !sbl ) {
							ctx_h_abs = absl_ctx_h[1]+1;
							ctx_h_sgn = sgnl_ctx_h[1]+1;
							ctx_h_len = lenl_ctx_h[1]+1;
						} else {
							ctx_h_abs = abss_ctx_h[1]+1;
							ctx_h_sgn = sgns_ctx_h[1]+1;
							ctx_h_len = lens_ctx_h[1]+1;
						}
						if ( p < 578 ) {
							memset( ctx_h_abs + p, 0, 578 - p );
							memset( ctx_h_sgn + p, 2, 578 - p ); // !!!
							memset( ctx_h_len + p, 0, 578 - p );
						}
						for ( i = p-1; i >= 0; i-- ) {
							ctx_h_abs[i] = abs[i];
							if ( abs[i] > 0 ) {
								ctx_h_sgn[i] = sgn[i];
								if ( abs[i] == 15 ) ctx_h_len[i] = len[i];
								else ctx_h_len[i] = 0;
							} else {
								ctx_h_sgn[i] = 2;
								ctx_h_len[i] = 0;
							}
						}
					}
				}
			
				
				// ---> RECONSTRUCTION INFORMATION: STUFFING BITS <---
				n = lmaxp - bitc; // # of stuffing bits
				if ( n == 0 ) { // the prefered case
					mod_nst->shift_context( ctx_nst );
					encode_ari( enc, mod_nst, 0 );
					ctx_nst = 0;
				} else { // encode # of stuff bits ( should be zero! )
					for ( ; n >= STUFFING_STEP; n -= STUFFING_STEP ) {
						mod_nst->shift_context( ctx_nst );
						encode_ari( enc, mod_nst, STUFFING_STEP );
						ctx_nst = STUFFING_STEP;
					} 
					mod_nst->shift_context( ctx_nst );
					encode_ari( enc, mod_nst, n );
					ctx_nst = n;
					// encode stuffing bits
					for ( ; bitc < lmaxp; bitc++ ) {
						mod_bst->shift_context( ctx_bst );
						c = huffman->read_bit();
						encode_ari( enc, mod_bst, c );
						ctx_bst = ( (ctx_bst<<1) | c ) & 0xF;
					}
				}
				
				
				// ---> GRANULE FINISHED! <---
				bitp += lmaxp;
				/*if ( ( granule->n >= 0 ) && ( granule->n <= 0 ) ) {
					fprintf( stderr, "\ngranule %i channel %i block_type: %i flags: %i\n", granule->n, ch, granule->block_type, flags );
					fprintf( stderr, "\ngranule %i channel %i nst: %i bst: %i aus: %i aux: %i pad: %i abs: %i\n", granule->n, ch, ctx_nst, ctx_bst, ctx_aux, ctx_aux, ctx_pad, ctx_abs );
					fprintf( stderr, "\ngranule %i channel %i bounds: %i/%i/%i/%i/%i\n", granule->n, ch, granule->region_bound[0], granule->region_bound[1], granule->region_bound[2], granule->sv_bound, granule->main_data_bit );
					fprintf( stderr, "\ngranule %i channel %i tables: %i/%i/%i/%i\n", granule->n, ch, granule->region_table[0], granule->region_table[1], granule->region_table[2], granule->select_htabB );
					fprintf( stderr, "\ngranule %i channel %i pos: %i\n", granule->n, ch, huffman->getpos() );
					fprintf( stderr, "\ngranule %i channel %i bad_coding: %s\n", granule->n, ch, bad_coding ? "yes" : "no" );
					fprintf( stderr, "\ngranule %i channel %i scfs: (%i/%i)\n", granule->n, ch, granule->share, granule->slength );
					for ( i = 0; i < 21; i++ ) fprintf( stderr, "%i, ", scf[i] );
					fprintf( stderr, "\n" );
					fprintf( stderr, "\ngranule %i channel %i coefs:\n", granule->n, ch );
					for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", abs[i] );
					fprintf( stderr, "\n" );
					for ( i = 0; i < 576; i++ ) if ( abs[i] == 15 ) fprintf( stderr, "%i, ", lbt[i] );
					fprintf( stderr, "\n" );
					for ( i = 0; i < 578; i++ ) if ( abs[i] > 0 ) fprintf( stderr, "%s", (sgn[i]) ? "+" : "-" );
					fprintf( stderr, "\n" );
					// fprintf( stderr, "\ngranule %i channel %i abs_ctx:\n", granule->n, ch );
					// for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", ctx_h_abs[i] );
					// fprintf( stderr, "\n" );
					// fprintf( stderr, "\ngranule %i channel %i sgn_ctx:\n", granule->n, ch );
					// for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", ctx_h_sgn[i] );
					// fprintf( stderr, "\n" );
					// fprintf( stderr, "\ngranule %i channel %i len_ctx:\n", granule->n, ch );
					// for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", ctx_h_len[i] );
					// fprintf( stderr, "\n" );
				}*/
			}
		}
		
		
		// ---> ENCODE BITRATE <---
		if ( i_bitrate == -1 ) {
			mod_btr->shift_context( bitrate_pred[ frame->main_size ] ); // !!! (bit-reservoir?)
			encode_ari( enc, mod_btr, frame->bits );
		}
		
		
		// ---> RECONSTRUCTION INFORMATION: AUX DATA AND PADDING <---
		if ( ( frame != lastframe ) && ( i_bit_res != 0 ) ) {
			if ( frame->aux_size + frame->next->bit_reservoir < 511 )
				n = frame->aux_size; // byte size of aux data
			else n = 511 - frame->next->bit_reservoir; // space taken from bit reservoir
			for ( ; n >= AUX_DATA_STEP; n -= AUX_DATA_STEP ) {
				mod_aux->shift_context( ctx_aux );
				encode_ari( enc, mod_aux, AUX_DATA_STEP );
				ctx_aux = AUX_DATA_STEP;
			}
			mod_aux->shift_context( ctx_aux );
			encode_ari( enc, mod_aux, n );
			ctx_aux = n;
		}
		// # of padding and aux data bits
		n = ( ( frame->main_size + frame->aux_size ) * 8 ) - bitp;
		// locally store padding and aux data
		for ( pna_c = pad_and_aux, i = n; i >= 8; i -= 8 )
			*(pna_c++) = huffman->read_bits( 8 );
		*pna_c = huffman->read_bits( i );
		// make and check prediction
		if ( pmp_predict_lame_anc( n, pad_and_aux ) == NULL )
			// prediction matches
			encode_ari( enc, mod_pap, 1 );
		else { // prediction doesn't match
			encode_ari( enc, mod_pap, 0 );
			huffman->rewind_bits( n );
			for ( ctx_pad = 0xFF; n > 0; n-- ) {
				mod_pad->shift_context( ctx_pad );
				c = huffman->read_bit();
				encode_ari( enc, mod_pad, c );
				ctx_pad = ( (ctx_pad<<1) | c ) & 0xFF;
			}
		}
		// encode padding bits(convert to full bytes)
		/*for ( c = 0xFF; n >= 8; n -= 8 ) {
			mod_pad->shift_context( c );
			c = huffman->read_bits( 8 );
			encode_ari( enc, mod_pad, c );
		}
		if ( n > 0 ) {
			mod_pad->shift_context( 0xFF + n );
			c = huffman->read_bits( n );
			encode_ari( enc, mod_pad, c );
		}*/
		
		// ---> FRAME FINISHED! <----
	}
	
	
	// ---> AFTER ENCODING: CLEAN UP <---
	
	// --- CLEAN UP: MODELS AND HUFFMAN CODER ---
	delete( huffman );
	delete( mod_svb );
	delete( mod_bvf );
	delete( mod_nst );
	delete( mod_bst );
	delete( mod_pad );
	delete( mod_pap );
	delete( mod_aux );
	delete( mod_btr );
	delete( mod_res );
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( sl = 0; sl < 4; sl++ )
			for ( g = 0; g < 6; g++ )
				delete( mod_scf[ch][sl][g] );
		for ( flags = 0x0; flags < ( (j_coding&&ch) ? 0x8 : 0x2 ); flags++ ) {
			delete( mod_abv[ch][flags][ 1] );
			delete( mod_abv[ch][flags][ 2] );
			delete( mod_abv[ch][flags][ 3] );
			delete( mod_abv[ch][flags][ 5] );
			delete( mod_abv[ch][flags][ 6] );
			delete( mod_abv[ch][flags][ 7] );
			delete( mod_abv[ch][flags][ 8] );
			delete( mod_abv[ch][flags][ 9] );
			delete( mod_abv[ch][flags][10] );
			delete( mod_abv[ch][flags][11] );
			delete( mod_abv[ch][flags][12] );
			delete( mod_abv[ch][flags][13] );
			delete( mod_abv[ch][flags][15] );
			delete( mod_abv[ch][flags][16] );
			delete( mod_abv[ch][flags][24] );
			delete( mod_asv[ch][flags][0] );
			delete( mod_asv[ch][flags][1] );
			delete( mod_sgn[ch][flags] );
		}
		for ( i = 0; i <= 13; i++ )
			delete( mod_len[ch][i] );
	}
	
	// --- CLEAN UP: MEMORY DEALLOCATION ---
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		free( scf_c[ch] ); free( scf_l_long[ch] ); free( scf_l_short[ch] );
		free( abs_c[ch] ); free( absl_ctx_h[ch] ); free( abss_ctx_h[ch] );
		free( sgn_c[ch] ); free( sgnl_ctx_h[ch] ); free( sgns_ctx_h[ch] );
		free( len_c[ch] ); free( lenl_ctx_h[ch] ); free( lens_ctx_h[ch] );
		free( lbt_c[ch] );
	}
	
	
	return true;
}


/* -----------------------------------------------
	decode the main data
	----------------------------------------------- */
INTERN inline bool pmp_decode_main_data( aricoder* dec )
{
	// context / storage
	static unsigned char* pad_and_aux= ( unsigned char* ) calloc( 2048, 1 );
	mp3Frame* frame;
	granuleInfo* granule;
	unsigned char* scf_c[2];
	unsigned char* scf_l_long[2];
	unsigned char* scf_l_short[2];
	unsigned char* abs_c[2];
	unsigned char* sgn_c[2];
	unsigned char* len_c[2];
	unsigned short* lbt_c[2];
	unsigned char* absl_ctx_h[2];
	unsigned char* abss_ctx_h[2];
	unsigned char* sgnl_ctx_h[2];
	unsigned char* sgns_ctx_h[2];
	unsigned char* lenl_ctx_h[2];
	unsigned char* lens_ctx_h[2];
	unsigned char* scf_prev;
	unsigned char* ctx_h_abs;
	unsigned char* ctx_h_sgn;
	unsigned char* ctx_h_len;
	unsigned char* scf;
	unsigned char* abs;
	unsigned char* sgn;
	unsigned char* len;
	unsigned short* lbt;
	unsigned char* swap;
	unsigned char* pna_c;
	unsigned char ctx_scf;
	unsigned char ctx_abs;
	unsigned char ctx_pat;
	unsigned char ctx_svb[2] = { 0, 0 };
	unsigned char ctx_nst = 0;
	unsigned char ctx_bst = 0;
	unsigned char ctx_aux = 0;
	unsigned char ctx_pad = 0;
	// statistical models
	model_s* mod_scf[2][4][10];
	model_s* mod_abv[2][8][32];
	model_b* mod_asv[2][8][2];
	model_b* mod_sgn[2][8];
	model_s* mod_len[2][14];
	model_b* mod_res;
	model_s* mod_svb;
	model_s* mod_bvf;
	model_s* mod_nst;
	model_b* mod_bst;
	model_b* mod_pad;
	model_b* mod_pap;
	model_s* mod_aux;
	model_s* mod_btr;
	model_s* mod_abc;
	model_b* mod_asc;
	model_b* mod_sgc;
	model_s* mod_lnc;
	model_s* mod_sfc;
	// lookup tables
	const int* bitrate_pred = mp3_bitrate_pred[(int)i_samplerate];
	const int* frame_size = mp3_frame_size_table[(int)i_samplerate];
	// huffman encoder
	huffman_writer* huffman;
	// mp3 encoding settings
	short* region_bounds;
	char* region_tables;
	huffman_enc_table* bv_table;
	huffman_code** hcodes;
	huffman_code* hcode;
	int bitres = 0;
	int linbits;
	const int* slen;
	int rlb, sl;	
	// general settings
	int bitp = 0;
	int sbl = 0;
	int flags = 0;
	bool j_coding;
	char shared[2][4];
	// counters
	int ch, gr;
	int p, g, r;
	int i, c, n;
	
	
	
	// --- PRE-PROCESSING PREPARATIONS: INIT STATISTICAL MODELS/HUFFMAN DECODER/CONTEXT ---
	// decide if joint context will be used (will be used for joint & standard stereo)
	j_coding = ( i_channels == MP3_STEREO ) || ( i_channels == MP3_JOINT_STEREO );
	// init huffman encoder/writer
	huffman = new huffman_writer( 0 );
	// reset ancillary predictor
	pmp_predict_lame_anc( -1, NULL );
	// init statistical models
	mod_svb = INIT_MODEL_S( ( 576 / 4 ) + 1 + 1, ( 576 / 4 ) + 1 + 1, 1 );
	mod_bvf = INIT_MODEL_S( ( 576 / 2 ) + 1, 0, 0 );
	mod_nst = INIT_MODEL_S( STUFFING_STEP + 1, STUFFING_STEP + 1, 1 );
	mod_bst = INIT_MODEL_B( 16, 1 );
	mod_pad = INIT_MODEL_B( 256, 1 );
	mod_pap = INIT_MODEL_B( 0, 0 );
	mod_aux = INIT_MODEL_S( AUX_DATA_STEP + 1, AUX_DATA_STEP + 1, 1 );
	mod_btr = INIT_MODEL_S( 16, 16, 1 );
	mod_res = INIT_MODEL_B( 13 + 1, 2 );
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( g = 0; g < 10; g++ ) {
			mod_scf[ch][0][g] = INIT_MODEL_S( 2, 16, 2 );
			mod_scf[ch][1][g] = INIT_MODEL_S( 4, 16, 2 );
			mod_scf[ch][2][g] = INIT_MODEL_S( 8, 16, 2 );
			mod_scf[ch][3][g] = INIT_MODEL_S( 16, 16, 2 );
		}
		for ( flags = 0x0; flags < ( (j_coding&&ch) ? 0x8 : 0x2 ); flags++ ) {
			mod_abv[ch][flags][ 0] = NULL;
			mod_abv[ch][flags][ 1] = INIT_MODEL_S(  1 + 1, 16, 2 );
			mod_abv[ch][flags][ 2] = INIT_MODEL_S(  2 + 1, 16, 2 );
			mod_abv[ch][flags][ 3] = INIT_MODEL_S(  2 + 1, 16, 2 );
			mod_abv[ch][flags][ 4] = NULL;
			mod_abv[ch][flags][ 5] = INIT_MODEL_S(  3 + 1, 16, 2 );
			mod_abv[ch][flags][ 6] = INIT_MODEL_S(  3 + 1, 16, 2 );
			mod_abv[ch][flags][ 7] = INIT_MODEL_S(  5 + 1, 16, 2 );
			mod_abv[ch][flags][ 8] = INIT_MODEL_S(  5 + 1, 16, 2 );
			mod_abv[ch][flags][ 9] = INIT_MODEL_S(  5 + 1, 16, 2 );
			mod_abv[ch][flags][10] = INIT_MODEL_S(  7 + 1, 16, 2 );
			mod_abv[ch][flags][11] = INIT_MODEL_S(  7 + 1, 16, 2 );
			mod_abv[ch][flags][12] = INIT_MODEL_S(  7 + 1, 16, 2 );
			mod_abv[ch][flags][13] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][14] = NULL;
			mod_abv[ch][flags][15] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][16] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][17] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][18] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][19] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][20] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][21] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][22] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][23] = mod_abv[ch][flags][16];
			mod_abv[ch][flags][24] = INIT_MODEL_S( 15 + 1, 16, 2 );
			mod_abv[ch][flags][25] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][26] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][27] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][28] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][29] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][30] = mod_abv[ch][flags][24];
			mod_abv[ch][flags][31] = mod_abv[ch][flags][24];
			mod_asv[ch][flags][0] = INIT_MODEL_B( 16, 2 );
			mod_asv[ch][flags][1] = INIT_MODEL_B( 16, 2 );
			mod_sgn[ch][flags] = INIT_MODEL_B( 16, 2 );
		}
		mod_len[ch][0] = NULL;
		for ( i = 1; i <= 13; i++ )
			mod_len[ch][i] = INIT_MODEL_S( i + 1, 13 + 1, 1 );
	}
	
	
	// --- PRE-PROCESSING PREPARATIONS: ALLOCATION OF MEMORY ---
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// alloc memory
		scf_c[ch]       = ( unsigned char* ) calloc(  21, sizeof( char ) );
		scf_l_long[ch]  = ( unsigned char* ) calloc(  21, sizeof( char ) );
		scf_l_short[ch] = ( unsigned char* ) calloc(  21, sizeof( char ) );
		abs_c[ch]       = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		sgn_c[ch]       = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		len_c[ch]       = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		lbt_c[ch]      = ( unsigned short* ) calloc( 576, sizeof( short ) );
		absl_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		abss_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		sgnl_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		sgns_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		lenl_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		lens_ctx_h[ch]  = ( unsigned char* ) calloc( 578+1+1, sizeof( char ) );
		// check for problems
		if ( ( scf_c[ch] == NULL ) || ( scf_l_long[ch] == NULL ) || ( scf_l_short[ch] == NULL ) ||
			 ( abs_c[ch] == NULL ) || ( absl_ctx_h[ch] == NULL ) || ( abss_ctx_h[ch] == NULL ) ||
			 ( sgn_c[ch] == NULL ) || ( sgnl_ctx_h[ch] == NULL ) || ( sgns_ctx_h[ch] == NULL ) ||
			 ( len_c[ch] == NULL ) || ( lenl_ctx_h[ch] == NULL ) || ( lens_ctx_h[ch] == NULL ) ||
			 ( lbt_c[ch] == NULL ) ) {
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}	
	
	
	// --- MAIN PROCESSING LOOP: DECODING AND ENCODING ---
	for ( frame = firstframe; frame != NULL; frame = frame->next, bitp = 0 ) {
		// record main index
		frame->main_index = huffman->getpos();
		for ( gr = 0; gr < 2; gr++ ) {
			for ( ch = 0; ch < g_nchannels; ch++ ) {
				
				// --- MAIN DATA DECODING: PREPARATIONS ---
				// initialize shortcuts
				granule = frame->granules[ch][gr];
				sbl = ( granule->block_type == SHORT_BLOCK ) ? 1 : 0;
				slen = slength_table[ (int) granule->slength ];
				region_bounds = granule->region_bound;
				region_tables = granule->region_table;
				abs = abs_c[ch] + 1;
				sgn = sgn_c[ch] + 1;
				len = len_c[ch] + 1;
				lbt = lbt_c[ch];
				// set compression flags
				flags = ( !j_coding || ( ch == 0 ) ) ? sbl :
					( sbl << 0 ) | // long (0) or short (1) block
					( frame->stereo_ms << 1 ) | // stereo ms on/off
					( ( flags ^ sbl ) << 2 ); // ch0/ch1 block type diffs y/n
				// store sharing info
				if ( gr == 0 ) {
					shared[ch][0] = ( granule->share >> 3 ) & 0x1;
					shared[ch][1] = ( granule->share >> 2 ) & 0x1;
					shared[ch][2] = ( granule->share >> 1 ) & 0x1;
					shared[ch][3] = ( granule->share >> 0 ) & 0x1;
				}
				// reset counter
				huffman->reset_counter();
				// store bit reservoir state
				frame->bit_reservoir = bitres;
				
				
				// ---> SCALEFACTOR PROCESSING <---
				if ( !sbl ) {
					
					// --- SCALEFACTOR DECODING/WRITING: LONG BLOCKS ---
					scf = scf_c[ch];
					scf_prev = scf_l_long[ch];
					ctx_scf = 0;
					
					// decode/write 21 scalefactors with/without sharing
					for ( g = 0, p = 0; g < 4; g++ ) {
						sl = slen[ ( g < 2 ) ? 0 : 1 ];
						if ( ( gr ) & ( shared[ch][g] ) ) { // shared
							// loop invariant condition (-funswitch-loops)
							memcpy( scf + p, scf_prev + p, scf_width[ g ] );
							p = scf_bounds[ g ];
							ctx_scf = scf[p-1];
						} else if ( sl == 0 ) { // zero slength
							memset( scf + p, 0, scf_width[ g ] );
							p = scf_bounds[ g ];
							ctx_scf = 0;
						} else {
							mod_sfc = mod_scf[ch][sl-1][(shared[ch][g])?g|0x4:g];
							for ( ; p < scf_bounds[ g ]; p++ ) { // non shared
								shift_model( mod_sfc, ctx_scf, scf_prev[p] );
								scf[p] = decode_ari( dec, mod_sfc );
								huffman->write_bits( scf[p], sl );
								ctx_scf = scf[p];
							}
						}
					}
					
					// --- SCALEFACTORS FINISHED: SWAP DATA ---
					swap = scf_c[ch]; scf_c[ch] = scf_l_long[ch]; scf_l_long[ch] = swap;
					
				} else {
					
					// --- SCALEFACTOR DECODING/WRITING: SHORT BLOCKS ---
					// encode (only non shared)
					for ( i = 0; i < 3; i++ ) { // 3 subblocks
						scf = scf_c[ch];
						scf_prev = scf_l_short[ch];
						ctx_scf = 0;
						for ( g = 0, p = 0; g < 2; g++ ) { // lo&hi groups
							sl = slen[ g ];
							if ( sl == 0 ) { // zero slength
								memset( scf + p, 0, scf_lh_width_short[ g ] );
								p = scf_lh_bounds_short[ g ];
							} else {
								mod_sfc = mod_scf[ch][sl-1][g|0x8];
								for ( ; p < scf_lh_bounds_short[ g ]; p++ ) { // scfs
									shift_model( mod_sfc, ctx_scf, scf_prev[p] );
									scf[p] = decode_ari( dec, mod_sfc );
									huffman->write_bits( scf[p], sl );
									ctx_scf = scf[p];
								}
							}
						}
						// --- SWAP DATA ---
						swap = scf_c[ch]; scf_c[ch] = scf_l_short[ch]; scf_l_short[ch] = swap;
					}				
				}
				
				
				// ---> COEFFICIENTS PROCESSING <---
				
				// --- DECODING AND FIXING OF SV BOUND ---
				if ( !sbl ) {
					mod_svb->shift_context( ctx_svb[ch] );
				} else mod_svb->shift_context( 144 + 1 );
				c = decode_ari( dec, mod_svb );
				if ( c <= 144 ) { // for proper files
					granule->sv_bound = c << 2;
					if ( granule->region_bound[ 2 ] % 4 == 2 )
						granule->sv_bound += 2;
					if ( !sbl ) ctx_svb[ch] = c;
				} else { // for broken files
					c = decode_ari( dec, mod_bvf );
					granule->sv_bound = granule->region_bound[ 2 ] - ( c << 1 );
					// fix bounds
					granule->region_bound[ 2 ] = granule->sv_bound;
					for ( i = 1; i >= 0; i-- ) if ( granule->sv_bound < granule->region_bound[i] )
						granule->region_bound[i] = granule->sv_bound;
					else break;
				}
				
				
				// --- COEFFICIENT DECODING: PREPARATIONS ---
				if ( granule->block_type != SHORT_BLOCK ) {
					ctx_h_abs = absl_ctx_h[ch]+1;
					ctx_h_sgn = sgnl_ctx_h[ch]+1;
					ctx_h_len = lenl_ctx_h[ch]+1;
				} else {
					ctx_h_abs = abss_ctx_h[ch]+1;
					ctx_h_sgn = sgns_ctx_h[ch]+1;
					ctx_h_len = lens_ctx_h[ch]+1;
				}
				ctx_abs = 0; ctx_pat = 0;
				rlb = region_bounds[2];
				mod_sgc = mod_sgn[ch][flags];
				
				// --- COEFFICIENT DECODING: SMALL VALUES ---
				mod_asc = mod_asv[ch][flags][(int) granule->select_htabB];
				for ( p = granule->sv_bound-1; p >= rlb; p-- ) {
					shift_model( mod_asc, ctx_pat, ctx_h_abs[p] );
					abs[p] = decode_ari( dec, mod_asc ); // absolutes
					ctx_pat = ( (ctx_pat<<1) | abs[p] ) & 0xF;
					ctx_abs = ( 2 * abs[p] + ctx_abs + 2 ) / 3;
					if ( abs[p] == 1 ) {
						shift_model( mod_sgc, ctx_h_abs[p], ctx_h_sgn[p] );
						sgn[p] = decode_ari( dec, mod_sgc ); // signs
					}
				}
				
				// --- COEFFICIENT DECODING: BIG VALUES ---
				for ( r = 2; r >= 0; r-- ) {
					rlb = (r==0) ? 0 : region_bounds[ r-1 ];
					if ( p < rlb ) continue;
					if ( region_tables[ r ] == 0 ) { // tbl0 skipping
						memset( abs + rlb, 0, p - rlb );
						p = rlb; ctx_abs = 0;
						continue;
					}
					// set table and linbits
					bv_table = bv_enc_table + region_tables[ r ];
					linbits = bv_table->linbits;
					mod_abc = mod_abv[ch][flags][(int) region_tables[ r ]];
					mod_lnc = mod_len[ch][linbits];
					
					// decoding with/without linbits
					for ( ; p >= rlb; p-- ) {
						shift_model( mod_abc, ctx_abs, ctx_h_abs[p] );
						abs[p] = decode_ari( dec, mod_abc ); // absolutes
						ctx_abs = ( 2 * abs[p] + ctx_abs + 2 ) / 3;
						if ( abs[p] > 0 ) {
							shift_model( mod_sgc, ctx_h_abs[p], ctx_h_sgn[p] );
							sgn[p] = decode_ari( dec, mod_sgc ); // signs
							if ( linbits > 0 ) if ( abs[p] == 15 ) {
								// loop invariant condition (-funswitch-loops)
								mod_lnc->shift_context( ctx_h_len[p] );
								len[p] = decode_ari( dec, mod_lnc ); // bitlengths									
								if ( len[p] > 0 ) {
									for ( lbt[p] = 1, i = len[p] - 2; i >= 0; i-- ) {
										shift_model( mod_res, len[p], i ); // bit residuals
										lbt[p] = ( lbt[p] << 1 ) | decode_ari( dec, mod_res );
									}
								} else lbt[p] = 0;
							}
						}
					}
				}
				
				
				// --- COEFFICIENT ENCODING: BIG VALUES ---
				for ( p = 0, r = 0; r < 3; r++ ) {
					if ( region_tables[ r ] == 0 ) { // tbl0 skipping
						p = region_bounds[ r ];			
						continue;
					}
					// set table and linbits
					bv_table = bv_enc_table + region_tables[ r ];
					hcodes = bv_table->h;
					linbits = bv_table->linbits;
					
					// encoding with/without linbits
					while ( p < region_bounds[ r ] ) {
						huffman->encode_pair( hcodes, abs + p );
						for ( i = 0; i < 2; i++, p++ ) if ( abs[p] > 0 ) {
							if ( linbits > 0 ) // loop invariant condition (-unswitch-loops)
								if ( abs[p] == 15 ) huffman->write_bits( lbt[p], linbits );
							huffman->write_bit( sgn[p] );
						}
					}
				}
				
				// --- COEFFICIENT ENCODING: SMALL VALUES ---
				hcode = ( granule->select_htabB ) ? htabB_enc : htabA_enc;
				while ( p < granule->sv_bound ) {
					huffman->encode_quadruple( hcode, abs + p );
					for ( i = 0; i < 4; i++, p++ ) if ( abs[p] )
						huffman->write_bit( sgn[p] );
				}
				
				// --- COEFFICIENTS FINISHED: UPDATE CONTEXT ---
				if ( p < 578 ) memset( abs + p, 0, 578 - p );
				// loop invariant condition
				if ( !j_coding || ( ch == 0 ) ) {
					// channel 0 context
					for ( i = 578-1; i >= 0; i-- ) {
						ctx_h_abs[i] = ( 3 * abs[i] + abs[i-1] + abs[i+1] + 2 * ctx_h_abs[i] + 4 ) / 7;
						if ( abs[i] > 0 ) {
							ctx_h_sgn[i] = ( ( ctx_h_sgn[i] << 1 ) | ( sgn[i] & 0x1 ) ) & 0xF;
							if ( abs[i] == 15 ) ctx_h_len[i] = ( 2 * len[i] + ctx_h_len[i] + 2 ) / 3;
							else ctx_h_len[i] >>= 1;
						} else {
							ctx_h_sgn[i] = ( ctx_h_sgn[i] << 1 ) & 0xF;
							ctx_h_len[i] >>= 1;
						}
					}
					// loop invariant condition
					if ( j_coding ) {
						// channel 1 context
						if ( !sbl ) {
							ctx_h_abs = absl_ctx_h[1]+1;
							ctx_h_sgn = sgnl_ctx_h[1]+1;
							ctx_h_len = lenl_ctx_h[1]+1;
						} else {
							ctx_h_abs = abss_ctx_h[1]+1;
							ctx_h_sgn = sgns_ctx_h[1]+1;
							ctx_h_len = lens_ctx_h[1]+1;
						}
						if ( p < 578 ) {
							memset( ctx_h_abs + p, 0, 578 - p );
							memset( ctx_h_sgn + p, 2, 578 - p );
							memset( ctx_h_len + p, 0, 578 - p );
						}
						for ( i = p-1; i >= 0; i-- ) {
							ctx_h_abs[i] = abs[i];
							if ( abs[i] > 0 ) {
								ctx_h_sgn[i] = sgn[i];
								if ( abs[i] == 15 ) ctx_h_len[i] = len[i];
								else ctx_h_len[i] = 0;
							} else {
								ctx_h_sgn[i] = 2;
								ctx_h_len[i] = 0;
							}
						}
					}
				}
			
			
				// ---> RECONSTRUCTION INFORMATION: STUFFING BITS <---
				for ( n = 0, c = STUFFING_STEP; c == STUFFING_STEP; n += c ) {
					// find out # of stuffing bits
					mod_nst->shift_context( ctx_nst );
					c = decode_ari( dec, mod_nst );
					ctx_nst = c;
				}
				for ( ; n > 0; n-- ) {
					mod_bst->shift_context( ctx_bst );
					c = decode_ari( dec, mod_bst );
					huffman->write_bit( c );
					ctx_bst = ( (ctx_bst<<1) | c ) & 0xF;
				}
				
				
				// ---> GRANULE FINISHED! <---
				granule->main_data_bit = huffman->get_count();
				bitp += granule->main_data_bit;
				/*if ( ( granule->n >= 0 ) && ( granule->n <= 0 ) ) {
					fprintf( stderr, "\ngranule %i channel %i block_type: %i flags: %i\n", granule->n, ch, granule->block_type, flags );
					fprintf( stderr, "\ngranule %i channel %i nst: %i bst: %i aus: %i aux: %i pad: %i abs: %i\n", granule->n, ch, ctx_nst, ctx_bst, ctx_aux, ctx_aux, ctx_pad, ctx_abs );
					fprintf( stderr, "\ngranule %i channel %i bounds: %i/%i/%i/%i/%i\n", granule->n, ch, granule->region_bound[0], granule->region_bound[1], granule->region_bound[2], granule->sv_bound, granule->main_data_bit );
					fprintf( stderr, "\ngranule %i channel %i tables: %i/%i/%i/%i\n", granule->n, ch, granule->region_table[0], granule->region_table[1], granule->region_table[2], granule->select_htabB );
					fprintf( stderr, "\ngranule %i channel %i pos: %i\n", granule->n, ch, huffman->getpos() );
					fprintf( stderr, "\ngranule %i channel %i scfs: (%i/%i)\n", granule->n, ch, granule->share, granule->slength );
					for ( i = 0; i < 21; i++ ) fprintf( stderr, "%i, ", scf[i] );
					fprintf( stderr, "\n" );
					fprintf( stderr, "\ngranule %i channel %i coefs:\n", granule->n, ch );
					for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", abs[i] );
					fprintf( stderr, "\n" );
					for ( i = 0; i < 576; i++ ) if ( abs[i] == 15 ) fprintf( stderr, "%i, ", lbt[i] );
					fprintf( stderr, "\n" );
					for ( i = 0; i < 578; i++ ) if ( abs[i] > 0 ) fprintf( stderr, "%s", (sgn[i]) ? "+" : "-" );
					fprintf( stderr, "\n" );
					// fprintf( stderr, "\ngranule %i channel %i abs_ctx:\n", granule->n, ch );
					// for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", ctx_h_abs[i] );
					// fprintf( stderr, "\n" );
					// fprintf( stderr, "\ngranule %i channel %i sgn_ctx:\n", granule->n, ch );
					// for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", ctx_h_sgn[i] );
					// fprintf( stderr, "\n" );
					// fprintf( stderr, "\ngranule %i channel %i len_ctx:\n", granule->n, ch );
					// for ( i = 0; i < 578; i++ ) fprintf( stderr, "%i, ", ctx_h_len[i] );
					// fprintf( stderr, "\n" );
				}*/
			}
		}
		// --- STORE FRAME MAIN DATA SIZES ---
		frame->main_bits = bitp;
		frame->main_size = ( bitp + 7 ) / 8;
		
		
		// ---> DECODE BITRATE <---
		if ( i_bitrate == -1 ) {
			mod_btr->shift_context( bitrate_pred[ frame->main_size ] );
			frame->bits = decode_ari( dec, mod_btr );
		}
		// calculate size of frame - use the lookup table
		frame->frame_size = frame_size[(int)frame->bits];
		if ( frame->padding ) frame->frame_size++;
		
		
		// ---> RECONSTRUCTION INFORMATION: AUX DATA AND PADDING <---
		if ( i_bit_res != 0 ) {
			// preliminary next frame bit reservoir size
			bitres =
				frame->frame_size -
				frame->fixed_size +
				frame->bit_reservoir -
				frame->main_size;
			if ( frame != lastframe ) {
				n = ( bitres < 511 ) ? 0 : bitres - 511;
				for ( c = AUX_DATA_STEP; c == AUX_DATA_STEP; n += c ) {
					// byte size of aux data
					mod_aux->shift_context( ctx_aux );
					c = decode_ari( dec, mod_aux );
					ctx_aux = c;
				}
				frame->aux_size = n;
			} else frame->aux_size = bitres;
			// finalize size of bit reservoir
			bitres -= frame->aux_size;
		} else {
			bitres = 0;
			frame->aux_size = 
				frame->frame_size -
				frame->fixed_size +
				frame->bit_reservoir -
				frame->main_size;
		}
		// # of padding and aux data bits
		n = ( ( frame->main_size + frame->aux_size ) * 8 ) - bitp;
		// check if data can be predicted 100%
		if ( decode_ari( dec, mod_pap ) == 1 ) {
			// prediction matches !
			pna_c = pmp_predict_lame_anc( n, NULL );
			for ( ; n >= 8; n -= 8 ) huffman->write_bits( *(pna_c++), 8 );
			if ( n > 0 ) huffman->write_bits( *(pna_c) >> (8-n), n );
		} else {
			// prediction doesn't match (d'oh!)
			pna_c = pad_and_aux; *pna_c = 0;
			for ( ctx_pad = 0xFF, i = 1; i <= n; i++ ) {
				mod_pad->shift_context( ctx_pad );
				c = decode_ari( dec, mod_pad );
				huffman->write_bit( c );
				ctx_pad = ( (ctx_pad<<1) | c ) & 0xFF;
				*pna_c = (*pna_c<<1)|c;
				if ( i % 8 == 0 ) *(++pna_c) = 0;
			}
			if ( pmp_predict_lame_anc( n, pad_and_aux ) == NULL ) {
				sprintf( errormessage, "ancilary prediction error, please report" );
				errorlevel = 2;
				return false;
			}
		}
		// decode padding bits
		/*for ( c = 0xFF; n >= 8; n -= 8 ) {
			mod_pad->shift_context( c );
			c = decode_ari( dec, mod_pad );
			huffman->write_bits( c, 8 );
		}
		if ( n > 0 ) {
			mod_pad->shift_context( 0xFF + n );
			c = decode_ari( dec, mod_pad );
			huffman->write_bits( c, n );
		}*/		
		
		// ---> FRAME FINISHED! <----		
	}
	
	
	// ---> AFTER ENCODING: GRAB POINTER AND CLEAN UP <---
	
	// --- GRAB POINTER AND SIZE ---
	main_data = huffman->getptr();
	main_data_size = huffman->getpos();
	
	// --- CALCULATE MP3 FILE SIZE ---
	mp3filesize =
		data_before_size +
		data_after_size +
		main_data_size +
		( g_nframes * lastframe->fixed_size );
	
	// --- CLEAN UP: MODELS AND HUFFMAN CODER ---
	delete( huffman );
	delete( mod_svb );
	delete( mod_bvf );
	delete( mod_nst );
	delete( mod_bst );
	delete( mod_pad );
	delete( mod_pap );
	delete( mod_aux );
	delete( mod_btr );
	delete( mod_res );
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( sl = 0; sl < 4; sl++ )
			for ( g = 0; g < 6; g++ )
				delete( mod_scf[ch][sl][g] );
		for ( flags = 0x0; flags < ( (j_coding&&ch) ? 0x8 : 0x2 ); flags++ ) {
			delete( mod_abv[ch][flags][ 1] );
			delete( mod_abv[ch][flags][ 2] );
			delete( mod_abv[ch][flags][ 3] );
			delete( mod_abv[ch][flags][ 5] );
			delete( mod_abv[ch][flags][ 6] );
			delete( mod_abv[ch][flags][ 7] );
			delete( mod_abv[ch][flags][ 8] );
			delete( mod_abv[ch][flags][ 9] );
			delete( mod_abv[ch][flags][10] );
			delete( mod_abv[ch][flags][11] );
			delete( mod_abv[ch][flags][12] );
			delete( mod_abv[ch][flags][13] );
			delete( mod_abv[ch][flags][15] );
			delete( mod_abv[ch][flags][16] );
			delete( mod_abv[ch][flags][24] );
			delete( mod_asv[ch][flags][0] );
			delete( mod_asv[ch][flags][1] );
			delete( mod_sgn[ch][flags] );
		}
		for ( i = 0; i <= 13; i++ )
			delete( mod_len[ch][i] );
	}
	
	// --- CLEAN UP: MEMORY DEALLOCATION ---
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		free( scf_c[ch] ); free( scf_l_long[ch] ); free( scf_l_short[ch] );
		free( abs_c[ch] ); free( absl_ctx_h[ch] ); free( abss_ctx_h[ch] );
		free( sgn_c[ch] ); free( sgnl_ctx_h[ch] ); free( sgns_ctx_h[ch] );
		free( len_c[ch] ); free( lenl_ctx_h[ch] ); free( lens_ctx_h[ch] );
		free( lbt_c[ch] );
	}
	
	
	return true;
}


/* -----------------------------------------------
	store unmute data
	----------------------------------------------- */
INTERN inline bool pmp_store_unmute_data( iostream* str )
{
	// as simple as it gets...
	str->write( unmute_data, sizeof( char ), unmute_data_size );
	
	return true;
}


/* -----------------------------------------------
	unstore unmute data
	----------------------------------------------- */
INTERN inline bool pmp_unstore_unmute_data( iostream* str )
{
	unsigned char n;
	
	// read # of of muted frames, set data size
	str->read( &n, sizeof( char ), 1 );
	if ( str->chkeof() ) n = 0;
	unmute_data_size = 1 + n * ( 2 + ( g_nchannels * 2 * 4 ) );
	
	// alloc memory for unmute data
	unmute_data = (unsigned char*) calloc( unmute_data_size, sizeof( char ) );
	if ( unmute_data == NULL ) {
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// read unmute data
	*unmute_data = n;
	str->read( unmute_data + 1, sizeof( char ), unmute_data_size - 1 );
	
	// check for eof
	if ( str->chkeof() ) {
		sprintf( errormessage, "unexpected end of data" );
		errorlevel = 2;
		return false;
	}
	
	
	return true;
}


/* -----------------------------------------------
	build universal context from global gain
	----------------------------------------------- */
INTERN inline bool pmp_build_context( void )
{
	granuleInfo* granule;
	int count_gg[ 2 ][ 256 ];
	unsigned char* gg0;
	unsigned char* gg1;
	int ngr;
	
	int lbound;
	int inc0, inc1;
	int i, ch;
	
	
	// check and free gg context arrays where needed
	if ( gg_context[0] != NULL ) free ( gg_context[0] );
	if ( gg_context[1] != NULL ) free ( gg_context[1] );
	gg_context[0] = NULL;
	gg_context[1] = NULL;
	
	// number of granules (per channel)
	ngr = g_nframes * 2;
	
	// alloc memory for three global gain contexts
	// yup, this might be a waste for mono files
	gg_context[0] = (unsigned char*) calloc ( ngr, sizeof( char ) );
	gg_context[1] = (unsigned char*) calloc ( ngr, sizeof( char ) );
	if ( ( gg_context[0] == NULL ) || ( gg_context[1] == NULL ) ) {
		sprintf( errormessage, MEM_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// set everything zero
	memset( count_gg[0], 0, 256 * 4 );
	memset( count_gg[1], 0, 256 * 4 );
	
	// count channel 0 symbol occurences
	gg0 = gg_context[0];
	for ( granule = firstframe->granules[0][0]; granule != NULL; granule = granule->next ) {
		*gg0 = granule->global_gain;
		count_gg[0][*(gg0++)]++;
	}
	for ( i = 1; i < 256; i++ ) // accumulate counts 
		count_gg[0][i] += count_gg[0][i-1];
	
	// count channel 1 and diff symbol occurences
	if ( g_nchannels == 2 ) {
		gg0 = gg_context[0];
		gg1 = gg_context[1];
		for ( granule = firstframe->granules[1][0]; granule != NULL; granule = granule->next ) {
			*gg1 = granule->global_gain;
			count_gg[1][*(gg1++)]++;
			gg0++;
		}
		for ( i = 1; i < 256; i++ ) // accumulate counts
			count_gg[1][i] += count_gg[1][i-1];
	}
	
	// analyse and process gg_context
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// find the interval with the highest increase
		for ( lbound = 0, inc0 = 0, inc1 = 0, i = 0; i < 256 - GG_CONTEXT_SIZE; i++ ) {
			inc1 = count_gg[ch][i + GG_CONTEXT_SIZE] - count_gg[ch][i];
			if ( inc1 >= inc0 ) {
				inc0 = inc1;
				lbound = i;
			}
		}
		// process gg_context
		for ( i = 0, gg0 = gg_context[ch]; i < ngr; i++, gg0++ ) {
			if ( *gg0 >= lbound ) {
				*gg0 -= lbound;
				if ( *gg0 >= GG_CONTEXT_SIZE ) *gg0 = GG_CONTEXT_SIZE - 1;
			} else *gg0 = 0;
		}
	}
	
	
	return true;
}





/* -----------------------------------------------
	predict LAME ancillary data
	----------------------------------------------- */
INTERN inline unsigned char* pmp_predict_lame_anc( int nbits, unsigned char* ref )
{
	static unsigned char* pred = (unsigned char*) calloc( 2048, sizeof( char ) );
	static unsigned char* lame_str = (unsigned char*) calloc( 4 + 16, sizeof( char ) ); // !!!
	const unsigned char b01 = 0x55;
	const unsigned char b10 = 0xAA;
	const unsigned char b00 = 0x00;
	const unsigned char b11 = 0xFF;
	static int lame_str_len = 4;
	static int lame_bit = 0;
	static bool alt_pred = 0;
	int offset;
	int nbytes;
	int i;
	
	// fprintf( stderr, "state:%i/%i/%02X/%02X ", (alt_pred)?0:1, nbits, pred[0], (ref!=NULL)?ref[0]:0xFF );
	if ( nbits < 0 ) {
		// (re-)init prediction
		lame_bit = 0;
		lame_str_len = 4;
		memcpy( lame_str, "LAME", 4 );
		alt_pred = false;
	} else {
		offset = nbits % 8;
		nbytes = nbits / 8;
		// build prediction
		if ( !alt_pred ) { // option 0 - lame predictor
			for ( i = 0; ( nbits >=8 ) && ( i < 4 ); nbits -= 8, i++ )
				pred[i] = lame_str[i]; // 'LAME'
			if ( nbits >= 32 ) {
				for ( ; ( nbits >= 8 ) && ( i < lame_str_len ); nbits -= 8, i++ )
					pred[i] = lame_str[i]; // version number
			}
			for ( ; nbits > 0; nbits -= 8, i++ )
				pred[i] = ( lame_bit == 0 ) ? b01 : b10; // switching bits
			lame_bit ^= ( offset % 2 );
		} // else: keep the prediction as is
		if ( ref != NULL ) { // compare prediction with reference (if any)
			if ( ( memcmp( pred, ref, nbytes ) == 0 ) && // match
				( ref[nbytes] == pred[nbytes] >> (8-offset) ) ) return NULL;
			else { // no match - check alternatives / relearn version number
				if ( (nbytes >= 8 ) && ( memcmp( ref, lame_str, 4 ) == 0 ) ) { // 'LAME' match
					// careful !!! (ref has to be >=8 in size)
					// relearn version number (if any)
					for ( i = 4; ( i < 4 + 16 ) && ( i < nbytes ); i++ ) {
						if ( ref[i] == b01 ) {
							lame_bit = 0;
							break;
						} else if ( ref[i] == b10 ) {
							lame_bit = 1;
							break;
						}
						lame_str[i] = ref[i];
					}
					lame_str_len = i;
				} else if ( ( nbytes == 0 ) && ( offset > 1 ) ) {
					if ( (b00^ref[0]) >> (8-offset) == 0 ) {
						memset( pred, b00, 2048 );
						alt_pred = true;
					} else if ( (b11^ref[0]) >> (8-offset) == 0 ) {
						memset( pred, b11, 2048 );
						alt_pred = true;
					} else alt_pred = false;
				} else if ( nbytes >= 1 ) {
					if ( b00 == ref[0] ) {
						memset( pred, b00, 2048 );
						alt_pred = true;
					} else if ( b11 == ref[0] ) {
						memset( pred, b11, 2048 );
						alt_pred = true;
					} else alt_pred = false;
				}
			}
		}
		// else: no reference, so prediction is assumed correct
	}
	
	
	return pred;
}

/* ----------------------- End of PMP specific functions -------------------------- */

/* ----------------------- Begin of miscellaneous helper functions -------------------------- */


/* -----------------------------------------------
	displays progress bar on screen
	----------------------------------------------- */
#if !defined(BUILD_LIB)
INTERN inline void progress_bar( int current, int last )
{
	int barpos = ( ( current * BARLEN ) + ( last / 2 ) ) / last;
	int i;
	
	
	// generate progress bar
	fprintf( msgout, "[" );
	#if defined(_WIN32)
	for ( i = 0; i < barpos; i++ )
		fprintf( msgout, "\xFE" );
	#else
	for ( i = 0; i < barpos; i++ )
		fprintf( msgout, "X" );
	#endif
	for (  ; i < BARLEN; i++ )
		fprintf( msgout, " " );
	fprintf( msgout, "]" );
}
#endif

/* -----------------------------------------------
	creates filename, callocs memory for it
	----------------------------------------------- */
#if !defined(BUILD_LIB)
INTERN inline char* create_filename( const char* base, const char* extension )
{
	int len = strlen( base ) + ( ( extension == NULL ) ? 0 : strlen( extension ) + 1 ) + 1;	
	char* filename = (char*) calloc( len, sizeof( char ) );	
	
	// create a filename from base & extension
	strcpy( filename, base );
	set_extension( filename, extension );
	
	return filename;
}
#endif

/* -----------------------------------------------
	creates filename, callocs memory for it
	----------------------------------------------- */
#if !defined(BUILD_LIB)
INTERN inline char* unique_filename( const char* base, const char* extension )
{
	int len = strlen( base ) + ( ( extension == NULL ) ? 0 : strlen( extension ) + 1 ) + 1;	
	char* filename = (char*) calloc( len, sizeof( char ) );	
	
	// create a unique filename using underscores
	strcpy( filename, base );
	set_extension( filename, extension );
	while ( file_exists( filename ) ) {
		len += sizeof( char );
		filename = (char*) realloc( filename, len );
		add_underscore( filename );
	}
	
	return filename;
}
#endif

/* -----------------------------------------------
	changes extension of filename
	----------------------------------------------- */
#if !defined(BUILD_LIB)
INTERN inline void set_extension( const char* filename, const char* extension )
{
	char* extstr;
	
	// find position of extension in filename	
	extstr = ( strrchr( filename, '.' ) == NULL ) ?
		strrchr( filename, '\0' ) : strrchr( filename, '.' );
	
	// set new extension
	if ( extension != NULL ) {
		(*extstr++) = '.';
		strcpy( extstr, extension );
	}
	else
		(*extstr) = '\0';
}
#endif

/* -----------------------------------------------
	adds underscore after filename
	----------------------------------------------- */
#if !defined(BUILD_LIB)
INTERN inline void add_underscore( char* filename )
{
	char* tmpname = (char*) calloc( strlen( filename ) + 1, sizeof( char ) );
	char* extstr;
	
	// copy filename to tmpname
	strcpy( tmpname, filename );
	// search extension in filename
	extstr = strrchr( filename, '.' );
	
	// add underscore before extension
	if ( extstr != NULL ) {
		(*extstr++) = '_';
		strcpy( extstr, strrchr( tmpname, '.' ) );
	}
	else
		sprintf( filename, "%s_", tmpname );
		
	// free memory
	free( tmpname );
}
#endif

/* -----------------------------------------------
	checks if a file exists
	----------------------------------------------- */
INTERN inline bool file_exists( const char* filename )
{
	// needed for both, executable and library
	FILE* fp = fopen( filename, "rb" );
	
	if ( fp == NULL ) return false;
	else {
		fclose( fp );
		return true;
	}
}

/* ----------------------- End of miscellaneous helper functions -------------------------- */

/* ----------------------- Begin of developers functions -------------------------- */


#if !defined(BUILD_LIB) && defined(DEV_BUILD)
/* -----------------------------------------------
	Writes to file
	----------------------------------------------- */
INTERN bool write_file( const char* base, const char* ext, void* data, int bpv, int size )
{	
	FILE* fp;
	char* fn;
	
	// create filename
	fn = create_filename( base, ext );
	
	// open file for output
	fp = fopen( fn, "wb" );	
	if ( fp == NULL ) {
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// write & close
	fwrite( data, bpv, size, fp );
	fclose( fp );
	
	return true;
}


/* -----------------------------------------------
	Writes error info file
	----------------------------------------------- */
INTERN bool write_errfile( void )
{
	FILE* fp;
	char* fn;
	
	
	// return immediately if theres no error
	if ( errorlevel == 0 ) return true;
	
	// create filename based on errorlevel
	if ( errorlevel == 1 ) {
		fn = create_filename( filelist[ file_no ], "wrn.nfo" );
	}
	else {
		fn = create_filename( filelist[ file_no ], "err.nfo" );
	}
	
	// open file for output
	fp = fopen( fn, "w" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// write status and errormessage to file
	fprintf( fp, "--> error (level %i) in file \"%s\" <--\n", errorlevel, filelist[ file_no ] );
	fprintf( fp, "\n" );
	// write error specification to file
	fprintf( fp, " %s -> %s:\n", get_status( errorfunction ),
			( errorlevel == 1 ) ? "warning" : "error" );
	fprintf( fp, " %s\n", errormessage );
	
	// done, close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	Writes info to a specific csv file
	----------------------------------------------- */
INTERN bool write_file_analysis( void )
{
	static const char* fn = FILE_ANALYSIS_CSV;
	FILE* fp;	
	bool labels;
	
	mp3Frame* frame;
	granuleInfo* granule;
	int total_fixed = 0;
	int total_scf   = 0;
	int total_coef  = 0;
	int total_aux   = 0;
	int main_bits;
	int cur_scf;
	int share = 0;
	const int* slen;
	int ch, gr;
	
	
	// check if file exists
	labels = !file_exists( fn );
	
	// open file for output
	fp = fopen( fn, "a" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	
	// calculate sizes of scf/coef/aux/fixed
	for ( frame = firstframe; frame != NULL; frame = frame->next ) {
		main_bits = 0; cur_scf = 0;
		for ( ch = 0; ch < g_nchannels; ch++ ) for ( gr = 0; gr < 2; gr++ ) {
			granule = frame->granules[ch][gr];
			slen = slength_table[ (int) granule->slength ];
			main_bits += granule->main_data_bit;
			if ( granule->block_type != SHORT_BLOCK ) {
				if ( ( ( share >> 3 ) & 1 ) == 0 ) cur_scf += slen[ 0 ] * 6;
				if ( ( ( share >> 2 ) & 1 ) == 0 ) cur_scf += slen[ 0 ] * 5;
				if ( ( ( share >> 1 ) & 1 ) == 0 ) cur_scf += slen[ 1 ] * 5;
				if ( ( ( share >> 0 ) & 1 ) == 0 ) cur_scf += slen[ 1 ] * 5;
			} else {
				cur_scf += slen[ 0 ] * 6 * 3;
				cur_scf += slen[ 1 ] * 6 * 3;
			}			
			share = granule->share;
		}
		total_fixed += frame->fixed_size * 8;
		total_scf += cur_scf;
		total_coef += main_bits - cur_scf;
		total_aux += frame->aux_size * 8;
		if ( main_bits % 8 > 0 )
			total_aux += 8 - ( main_bits % 8 );
	}
	
	// write labels
	if ( labels ) {
		fprintf( fp, "name;" );
		fprintf( fp, "total_bits;" );
		fprintf( fp, "tag_bits;" );
		fprintf( fp, "fixed_bits;" );
		fprintf( fp, "scf_bits;" );
		fprintf( fp, "coef_bits;" );
		fprintf( fp, "aux_bits;" );
		fprintf( fp, "frames;" );
		fprintf( fp, "mpeg;" );
		fprintf( fp, "layer;" );
		fprintf( fp, "samples;" );
		fprintf( fp, "bitrate;" );
		fprintf( fp, "channels;" );
		fprintf( fp, "protection;" );
		fprintf( fp, "padding;" );
		fprintf( fp, "ms_stereo;" );
		fprintf( fp, "int_stereo;" );
		fprintf( fp, "private;" );
		fprintf( fp, "copyright;" );
		fprintf( fp, "original;" );
		fprintf( fp, "emphasis;" );
		fprintf( fp, "padbits;" );
		fprintf( fp, "bitres;" );
		fprintf( fp, "sharing;" );
		fprintf( fp, "switching;" );
		fprintf( fp, "mixed;" );
		fprintf( fp, "preemphasis;" );
		fprintf( fp, "coarse;" );
		fprintf( fp, "sbgain;" );
		fprintf( fp, "auxiliary_h;" );
		fprintf( fp, "sblock_diff;" );
		fprintf( fp, "bad_first" );
		fprintf( fp, "\n" );
	}
	
	// write data
	fprintf( fp, "%s;", mp3filename );
	fprintf( fp, "%i;", mp3filesize * 8 );
	fprintf( fp, "%i;", ( data_before_size + data_after_size ) * 8 );
	fprintf( fp, "%i;", total_fixed );
	fprintf( fp, "%i;", total_scf );
	fprintf( fp, "%i;", total_coef );
	fprintf( fp, "%i;", total_aux );
	fprintf( fp, "%i;", g_nframes );
	fprintf( fp, "%s;", mpeg_description[(int)i_mpeg] );
	fprintf( fp, "%s;", layer_description[(int)i_layer] );
	fprintf( fp, "%i;", g_samplerate );
	fprintf( fp, "%i;", g_bitrate * 1000 );
	fprintf( fp, "%s;", channels_description[(int)i_channels ] );
	fprintf( fp, "%i;", i_protection );
	fprintf( fp, "%i;", i_padding );
	fprintf( fp, "%i;", i_stereo_ms );
	fprintf( fp, "%i;", i_stereo_int );
	fprintf( fp, "%i;", i_privbit );
	fprintf( fp, "%i;", i_copyright );
	fprintf( fp, "%i;", i_original );
	fprintf( fp, "%i;", i_emphasis );
	fprintf( fp, "%i;", i_padbits );
	fprintf( fp, "%i;", i_bit_res);
	fprintf( fp, "%i;", i_share );
	fprintf( fp, "%i;", i_sblocks );
	fprintf( fp, "%i;", i_mixed );
	fprintf( fp, "%i;", i_preemphasis );
	fprintf( fp, "%i;", i_coarse );
	fprintf( fp, "%i;", i_sbgain );
	fprintf( fp, "%i;", i_aux_h );
	fprintf( fp, "%i;", i_sb_diff );
	fprintf( fp, "%i", n_bad_first );
	fprintf( fp, "\n" );
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	Writes block analysis to csv
	----------------------------------------------- */
INTERN bool write_block_analysis( void )
{
	const char* frm_ext = "frm.csv";
	const char* ch_ext[2] = { "ch0.csv", "ch1.csv" };	
	FILE* fp;
	char* fn;
	
	mp3Frame* frame;
	int ch, gr;
	
	
	/* --- frames analysis --- */
	
	// create filename
	fn = create_filename( filelist[ file_no ], frm_ext );
	
	// open file for output
	fp = fopen( fn, "w" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// write labels (header)
	fprintf( fp, "frame;" );
	fprintf( fp, "main_index;" );
	fprintf( fp, "frame_size;" );
	fprintf( fp, "fixed_size;" );
	fprintf( fp, "bit_reservoir;");
	fprintf( fp, "main_bits;" );
	fprintf( fp, "main_size;" );
	fprintf( fp, "aux_size;" );
	fprintf( fp, "mpeg;");
	fprintf( fp, "layer;");
	fprintf( fp, "samples;");
	fprintf( fp, "bitrate;");
	fprintf( fp, "channels;");
	fprintf( fp, "protection;");
	fprintf( fp, "padding;");
	fprintf( fp, "ms_stereo;");
	fprintf( fp, "int_stereo;");
	fprintf( fp, "private;");
	fprintf( fp, "copyright;");
	fprintf( fp, "original;");
	fprintf( fp, "emphasis;");
	fprintf( fp, "padbits");
	fprintf( fp, "\n" );
	
	// write frame data
	for ( frame = firstframe; frame != NULL; frame = frame->next ) {
		// header/frame-global info
		fprintf( fp, "%i;", frame->n );
		fprintf( fp, "%i;", frame->main_index );
		fprintf( fp, "%i;", frame->frame_size );
		fprintf( fp, "%i;", frame->fixed_size );
		fprintf( fp, "%i;", frame->bit_reservoir );
		fprintf( fp, "%i;", frame->main_bits );
		fprintf( fp, "%i;", frame->main_size );
		fprintf( fp, "%i;", frame->aux_size );
		fprintf( fp, "%s;", mpeg_description[(int)frame->mpeg ] );
		fprintf( fp, "%s;", layer_description[(int)frame->layer ] );
		fprintf( fp, "%i;", mp3_samplerate_table[(int)frame->samples] );
		fprintf( fp, "%i;", mp3_bitrate_table[(int)frame->bits] * 1000 );
		fprintf( fp, "%s;", channels_description[(int)frame->channels ] );
		fprintf( fp, "%i;", frame->protection );
		fprintf( fp, "%i;", frame->padding );
		fprintf( fp, "%i;", frame->stereo_ms );
		fprintf( fp, "%i;", frame->stereo_int );
		fprintf( fp, "%i;", frame->privbit );
		fprintf( fp, "%i;", frame->copyright );
		fprintf( fp, "%i;", frame->original );
		fprintf( fp, "%i;", frame->emphasis );
		fprintf( fp, "%i;", frame->padbits );
		fprintf( fp, "\n" );
	}
	
	// close file
	fclose( fp );
	
	
	/* --- granules analysis --- */
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// create filename
		fn = create_filename( filelist[ file_no ], ch_ext[ch] );
		
		// open file for output
		fp = fopen( fn, "w" );
		if ( fp == NULL ){
			sprintf( errormessage, FWR_ERRMSG );
			errorlevel = 2;
			return false;
		}
		free( fn );
		
		// labels (granules)
		fprintf( fp, "block;" );
		fprintf( fp, "ms_stereo;" );
		fprintf( fp, "int_stereo;" );
		fprintf( fp, "share;" );
		fprintf( fp, "main_bit;" );
		fprintf( fp, "big_vals;" );
		fprintf( fp, "gl_gain;" );
		fprintf( fp, "slength;" );
		fprintf( fp, "switching;" );
		fprintf( fp, "block_type;" );
		fprintf( fp, "mixed;" );
		fprintf( fp, "preemphasis;" );
		fprintf( fp, "coarse;" );
		fprintf( fp, "r0_tbl;" );
		fprintf( fp, "r1_tbl;" );
		fprintf( fp, "r2_tbl;" );
		fprintf( fp, "sm_tbl;" );
		fprintf( fp, "r0_size;" );
		fprintf( fp, "r1_size;" );
		fprintf( fp, "r0_bound;" );
		fprintf( fp, "r1_bound;" );
		fprintf( fp, "r2_bound;" );
		fprintf( fp, "sv_bound;" );
		fprintf( fp, "sb0_gain;" );
		fprintf( fp, "sb1_gain;" );
		fprintf( fp, "sb2_gain" );
		fprintf( fp, "\n" );
		
		// write granules data
		for ( frame = firstframe; frame != NULL; frame = frame->next ) if ( ch < frame->nchannels ) {
			for ( gr = 0; gr < 2; gr++ ) {
				fprintf( fp, "%i;", frame->granules[ch][gr]->n );
				fprintf( fp, "%i;", frame->stereo_ms );
				fprintf( fp, "%i;", frame->stereo_int );
				fprintf( fp, "0b%i%i%i%i;",
					BITN( frame->granules[ch][gr]->share, 3 ),
					BITN( frame->granules[ch][gr]->share, 2 ),
					BITN( frame->granules[ch][gr]->share, 1 ),
					BITN( frame->granules[ch][gr]->share, 0 ) );
				fprintf( fp, "%i;", frame->granules[ch][gr]->main_data_bit );
				fprintf( fp, "%i;", frame->granules[ch][gr]->big_val_pairs );
				fprintf( fp, "%i;", frame->granules[ch][gr]->global_gain );
				fprintf( fp, "%i/%i;", slength_table[ (int) frame->granules[ch][gr]->slength ][ 0 ], slength_table[ (int) frame->granules[ch][gr]->slength ][ 1 ] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->window_switching );
				fprintf( fp, "%s;", blocktype_description[(int)frame->granules[ch][gr]->block_type] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->mixed_flag );
				fprintf( fp, "%i;", frame->granules[ch][gr]->preemphasis );
				fprintf( fp, "%i;", frame->granules[ch][gr]->coarse_scalefactors );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region_table[0] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region_table[1] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region_table[2] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->select_htabB );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region0_size );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region1_size );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region_bound[0] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region_bound[1] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->region_bound[2] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->sv_bound );
				fprintf( fp, "%i;", frame->granules[ch][gr]->sb_gain[0] );
				fprintf( fp, "%i;", frame->granules[ch][gr]->sb_gain[1] );
				fprintf( fp, "%i", frame->granules[ch][gr]->sb_gain[2] );
				fprintf( fp, "\n" );
			}
		}
	
		// close file
		fclose( fp );
	}
	
	
	return true;
}


/* -----------------------------------------------
	Writes statistic info to a specific csv file
	----------------------------------------------- */
INTERN bool write_stat_analysis( void )
{
	static const char* fn = STAT_ANALYSIS_CSV;
	FILE* fp;
	bool labels;
	
	huffman_reader* decoder;
	mp3Frame* frame;
	granuleInfo* granule;
	granuleData*** frame_data;
	int ch, gr;
	int i;
	
	unsigned int coef_stats[ 16 ] = { 0 };
	unsigned int coefb_stats[ 15 ] = { 0 };
	unsigned int scf_stats[ 16 ] = { 0 };
	unsigned int bv_tbl_stats[ 32 ] = { 0 };
	unsigned int sv_tbl_stats[ 2 ] = { 0 };
	unsigned int blt_stats[ 4 ] = { 0 };
	unsigned int n_bad_bv_bound = 0;
	unsigned int n_bad_sv_bound = 0;
	int bv_bound_max_dist = 0;
	int sv_bound_max_dist = 0;
	int bv_bound_avrg = 0;
	int sv_bound_avrg = 0;
	int bad_code = 0;
	int r2bvs_avrg = 0;
	
	
	
	// check if file exists
	labels = !file_exists( fn );
	
	// open file for output
	fp = fopen( fn, "a" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// write labels
	if ( labels ) {
		fprintf( fp, "name;" );
		fprintf( fp, "n_frames;" );
		fprintf( fp, "muted;" );
		fprintf( fp, "bad_code;" );
		for ( i = 0; i < 15; i++ )
			fprintf( fp, "cf=%i;", i );
		fprintf( fp, "cf>=15;" );
		for ( i = 0; i < 15; i++ )
			fprintf( fp, "cb=%i;", i );
		for ( i = 0; i < 16; i++ )
			fprintf( fp, "sc=%i;", i );
		for ( i = 0; i < 32; i++ )
			fprintf( fp, "bv_tbl%i;", i );
		fprintf( fp, "sv_tblA;" );
		fprintf( fp, "sv_tblB;" );
		fprintf( fp, "long;" );
		fprintf( fp, "start;" );
		fprintf( fp, "short;" );
		fprintf( fp, "stop;" );
		fprintf( fp, "n_bad_bv;" );
		fprintf( fp, "bad_bv_lim;" );
		fprintf( fp, "bad_bv_avr;" );
		fprintf( fp, "n_bad_sv;" );
		fprintf( fp, "bad_sv_lim;" );
		fprintf( fp, "bad_sv_avr;" );
		fprintf( fp, "reg2_bv_avr" );
		fprintf( fp, "\n" );
	}
	
	// init decoder	
	decoder = new huffman_reader( main_data, main_data_size );
	
	// run statistical analysis
	for ( frame = firstframe; frame != NULL; frame = frame->next ) {
		// skip bad first frames
		if ( frame->n < n_bad_first ) continue;
		frame_data = mp3_decode_frame( decoder, frame );
		if ( frame_data == NULL ) bad_code++;
		for ( ch = 0; ch < frame->nchannels; ch++ ) {
			for ( gr = 0; gr < 2; gr++ ) {
				granule = frame->granules[ch][gr];
				// simple stuff first
				bv_tbl_stats[ (int) granule->region_table[0] ]++;
				bv_tbl_stats[ (int) granule->region_table[1] ]++;
				sv_tbl_stats[ (int) granule->select_htabB ]++;
				blt_stats[ (int) granule->block_type ]++;
				if ( !granule->window_switching )
					bv_tbl_stats[ (int) granule->region_table[2] ]++;
				if ( frame_data == NULL ) continue;
				// count coefficients and scalefactors
				if ( granule->block_type != SHORT_BLOCK ) {
					for ( i = 0; i < 21; i++ )
						scf_stats[ (int) frame_data[ch][gr]->scalefactors[i] ]++;
				} else {
					for ( i = 0; i < 36; i++ )
						scf_stats[ (int) frame_data[ch][gr]->scalefactors[i] ]++;
				}
				for ( i = 0; i < 576; i++ ) {
					if ( ABS(frame_data[ch][gr]->coefficients[i]) < 16 )
						coef_stats[ ABS(frame_data[ch][gr]->coefficients[i]) ]++;
					else coef_stats[ 15 ]++;
					coefb_stats[ BITLEN8224N(frame_data[ch][gr]->coefficients[i]) ]++;
				}
				// check for bad sv bounds
				for ( i = 0; i < granule->sv_bound; i++ )
					if ( frame_data[ch][gr]->coefficients[granule->sv_bound-i-1] != 0 ) break;
				i /= 4;
				if ( i > 0 ) {
					n_bad_sv_bound++;
					sv_bound_avrg += i;
					if ( sv_bound_max_dist < i ) sv_bound_max_dist = i;
				}
				// done if zero table
				// if 	( granule->window_switching ) if ( granule->region_table[2] == 0 ) continue;
				// else if ( granule->region_table[1] == 0 ) continue;
				// check for bad bv bounds
				for ( i = 0; i < granule->region_bound[2]; i++ )
					if ( ABS(frame_data[ch][gr]->coefficients[granule->region_bound[2]-i-1]) > 1 ) break;
				i /= 2;
				if ( i > 1 ) {
					n_bad_bv_bound++;
					bv_bound_avrg += i;
					if ( bv_bound_max_dist < i ) bv_bound_max_dist = i;
				}
				// count region 2 coefficients ABS > 2
				for ( i = granule->region_bound[(granule->window_switching)?0:1]; i < granule->region_bound[2]; i++ )
					if ( ABS(frame_data[ch][gr]->coefficients[i]) > 1 ) r2bvs_avrg++;
			}
		}
	}
	sv_bound_avrg = ( n_bad_sv_bound > 0 ) ? sv_bound_avrg / n_bad_sv_bound : 0;
	bv_bound_avrg = ( n_bad_bv_bound > 0 ) ? bv_bound_avrg / n_bad_bv_bound : 0;
	r2bvs_avrg = r2bvs_avrg / ( g_nframes * 2 * g_nchannels );
	
	// done analyzing, delete decoder
	delete( decoder );
	
	// write data
	fprintf( fp, "%s;", mp3filename );
	fprintf( fp, "%i;", g_nframes );
	fprintf( fp, "%i;", n_bad_first );
	fprintf( fp, "%i;", bad_code );
	for ( i = 0; i < 16; i++ )
		fprintf( fp, "%i;", (int) coef_stats[i] );
	for ( i = 0; i < 15; i++ )
		fprintf( fp, "%i;", (int) coefb_stats[i] );
	for ( i = 0; i < 16; i++ )
		fprintf( fp, "%i;", (int) scf_stats[i] );
	for ( i = 0; i < 32; i++ )
		fprintf( fp, "%i;", (int) bv_tbl_stats[i] );
	for ( i = 0; i < 2; i++ )
		fprintf( fp, "%i;", (int) sv_tbl_stats[i] );
	for ( i = 0; i < 4; i++ )
		fprintf( fp, "%i;", (int) blt_stats[i] );
	fprintf( fp, "%i;", (int) n_bad_bv_bound );
	fprintf( fp, "%i;", bv_bound_max_dist );
	fprintf( fp, "%i;", bv_bound_avrg );
	fprintf( fp, "%i;", (int) n_bad_sv_bound );
	fprintf( fp, "%i;", sv_bound_max_dist );
	fprintf( fp, "%i;", sv_bound_avrg );
	fprintf( fp, "%i", r2bvs_avrg );
	fprintf( fp, "\n" );
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	Make a PGM of header/sideinfo data
	----------------------------------------------- */
INTERN bool visualize_headers( void )
{
	static const int img_width = 1280; // must be divisible by 2
	static const bool inc_all = false;
	static unsigned char* line = (unsigned char*) calloc ( img_width, sizeof( char ) );
	mp3Frame* frame;
	mp3Frame* frame0 = firstframe;
	mp3Frame* frame1 = firstframe;
	int i;
	
	FILE* fp;
	char* fn;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "headers.pgm" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// write PGM header
	fprintf( fp, "P5\n" );
	fprintf( fp, "# created by %s v%i.%i%s (%s) by %s\n",
		appname, appversion / 10, appversion % 10, subversion, versiondate, author );
	fprintf( fp, "%i %i\n", img_width, ( ((g_nframes*2) + img_width - 1)  / img_width ) * ( inc_all ? 60 : 40 ) );
	fprintf( fp, "255\n" );
	
	do { // write data, line per line, 2x2 pixels for header stuff
		memset ( line, 0x0, img_width );
		frame0 = frame1;
		for ( i = 0; i < (img_width/2); i++ ) {
			frame1 = frame1->next;
			if ( frame1 == NULL ) break;
		}
		i = frame0->n;
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = frame->mpeg << 6;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 2 (MPEG)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -2
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = frame->layer << 6;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 4 (LAYER)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -4
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = frame->channels << 6;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 6 (CHANNELS)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -6
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = frame->samples << 6;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 8 (SAMPLES)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -8
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 ] = frame->bits << 4;
			line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
		} // 10 (BITRATE)
		fwrite( line, 1, img_width, fp );
		fwrite( line, 1, img_width, fp );
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = ( frame->protection == 0 ) ? 0 : 255;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 12 (PROTECTION)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -10
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 ] = ( frame->padding == 0 ) ? 0 : 255;
			line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
		} // 14 (PADDING)
		fwrite( line, 1, img_width, fp );
		fwrite( line, 1, img_width, fp );
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = ( frame->privbit == 0 ) ? 0 : 255;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 16 (PRIVATE)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -12
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = ( frame->copyright == 0 ) ? 0 : 255;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 18 (COPYRIGHT)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -14
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = ( frame->original == 0 ) ? 0 : 255;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 20 (ORIGINAL)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -16
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 ] = ( frame->stereo_ms == 0 ) ? 0 : 255;
			line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
		} // 22 (MS STEREO)
		fwrite( line, 1, img_width, fp );
		fwrite( line, 1, img_width, fp );
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = ( frame->stereo_int == 0 ) ? 0 : 255;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 24 (INT STEREO)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -18
		
		if ( inc_all ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 ] = frame->emphasis << 6;
				line[ (frame->n-i) * 2 + 1 ] = line[ (frame->n-i) * 2 ];
			} // 26 (EMPHASIS)
			fwrite( line, 1, img_width, fp );
			fwrite( line, 1, img_width, fp );
		} // -20
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->main_data_bit >> 4;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->main_data_bit >> 4;
		}
		fwrite( line, 1, img_width, fp ); // 27 (MAIN DATA BIT)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->main_data_bit >> 4;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->main_data_bit >> 4;
			}
		}
		fwrite( line, 1, img_width, fp ); // 28 (MAIN DATA BIT)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->big_val_pairs >> 1;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->big_val_pairs >> 1;
		}
		fwrite( line, 1, img_width, fp ); // 29 (BIG VALUES)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->big_val_pairs >> 1;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->big_val_pairs >> 1;
			}
		}
		fwrite( line, 1, img_width, fp ); // 30 (BIG VALUES)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->share << 4;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->share << 4;
		}
		fwrite( line, 1, img_width, fp ); // 31 (SHARING)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->share << 4;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->share << 4;
			}
		}
		fwrite( line, 1, img_width, fp ); // 32 (SHARING)
			
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->global_gain;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->global_gain;
		}
		fwrite( line, 1, img_width, fp ); // 33 (GLOBAL GAIN)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->global_gain;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->global_gain;
			}
		}
		fwrite( line, 1, img_width, fp ); // 34 (GLOBAL GAIN)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->slength << 4;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->slength << 4;
		}
		fwrite( line, 1, img_width, fp ); // 35 (SLENGTH)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->slength << 4;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->slength << 4;
			}
		}
		fwrite( line, 1, img_width, fp ); // 36 (SLENGTH)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->block_type << 6;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->block_type << 6;
		}
		fwrite( line, 1, img_width, fp ); // 37 (BLOCK TYPE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->block_type << 6;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->block_type << 6;
			}
		}
		fwrite( line, 1, img_width, fp ); // 38 (BLOCK TYPE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->preemphasis * 255;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->preemphasis * 255;
		}
		fwrite( line, 1, img_width, fp ); // 39 (PREEMPHASIS)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->preemphasis * 255;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->preemphasis * 255;
			}
		}
		fwrite( line, 1, img_width, fp ); // 40 (PREEMPHASIS)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->coarse_scalefactors * 255;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->coarse_scalefactors * 255;
		}
		fwrite( line, 1, img_width, fp ); // 41 (COARSE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->coarse_scalefactors * 255;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->coarse_scalefactors * 255;
			}
		}
		fwrite( line, 1, img_width, fp ); // 42 (COARSE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->region_table[0] << 3;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->region_table[0] << 3;
		}
		fwrite( line, 1, img_width, fp ); // 43 (R0 TABLE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->region_table[0] << 3;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->region_table[0] << 3;
			}
		}
		fwrite( line, 1, img_width, fp ); // 44 (R0 TABLE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->region_table[1] << 3;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->region_table[1] << 3;
		}
		fwrite( line, 1, img_width, fp ); // 45 (R1 TABLE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->region_table[1] << 3;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->region_table[1] << 3;
			}
		}
		fwrite( line, 1, img_width, fp ); // 46 (R1 TABLE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->region_table[2] << 3;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->region_table[2] << 3;
		}
		fwrite( line, 1, img_width, fp ); // 47 (R2 TABLE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->region_table[2] << 3;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->region_table[2] << 3;
			}
		}
		fwrite( line, 1, img_width, fp ); // 48 (R2 TABLE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->select_htabB * 255;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->select_htabB * 255;
		}
		fwrite( line, 1, img_width, fp ); // 49 (R4 TABLE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->select_htabB * 255;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->select_htabB * 255;
			}
		}
		fwrite( line, 1, img_width, fp ); // 50 (R4 TABLE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->region0_size << 4;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->region0_size << 4;
		}
		fwrite( line, 1, img_width, fp ); // 51 (R0 SIZE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->region0_size << 4;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->region0_size << 4;
			}
		}
		fwrite( line, 1, img_width, fp ); // 52 (R0 SIZE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->region1_size << 4;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->region1_size << 4;
		}
		fwrite( line, 1, img_width, fp ); // 53 (R1 SIZE)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->region1_size << 4;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->region1_size << 4;
			}
		}
		fwrite( line, 1, img_width, fp ); // 54 (R1 SIZE)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->sb_gain[0] << 5;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->sb_gain[0] << 5;
		}
		fwrite( line, 1, img_width, fp ); // 55 (SB0 GAIN)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->sb_gain[0] << 5;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->sb_gain[0] << 5;
			}
		}
		fwrite( line, 1, img_width, fp ); // 56 (SB0 GAIN)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->sb_gain[1] << 5;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->sb_gain[1] << 5;
		}
		fwrite( line, 1, img_width, fp ); // 57 (SB1 GAIN)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->sb_gain[1] << 5;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->sb_gain[1] << 5;
			}
		}
		fwrite( line, 1, img_width, fp ); // 58 (SB1 GAIN)
		
		for ( frame = frame0; frame != frame1; frame = frame->next ) {
			line[ (frame->n-i) * 2 + 0 ] = frame->granules[0][0]->sb_gain[2] << 5;
			line[ (frame->n-i) * 2 + 1 ] = frame->granules[0][1]->sb_gain[2] << 5;
		}
		fwrite( line, 1, img_width, fp ); // 59 (SB2 GAIN)
		if ( g_nchannels == 2 ) {
			for ( frame = frame0; frame != frame1; frame = frame->next ) {
				line[ (frame->n-i) * 2 + 0 ] = frame->granules[1][0]->sb_gain[2] << 5;
				line[ (frame->n-i) * 2 + 1 ] = frame->granules[1][1]->sb_gain[2] << 5;
			}
		}
		fwrite( line, 1, img_width, fp ); // 60 (SB2 GAIN)
	} while ( frame1 != NULL );
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	Make a PGM of coefs/scalefactors
	----------------------------------------------- */
INTERN bool visualize_decoded_data( void )
{
	static const int nfs = 2;
	static const int nfc = 9;
	static const unsigned char v_types[4][2] = {
		{ 0xFF, 0xFF }, { 0xFF, 0x00 },
		{ 0x00, 0x00 }, { 0x00, 0xFF }
	};
	static const unsigned char v_unk  = 0x7F;
	static const unsigned char v_zero = 0x00;
	
	FILE* fp[nfs+nfc];
	const char* ext[nfs+nfc] = {
		"scale.unt.pgm",
		"scale.nrm.pgm",
		"coefs.f15.pgm",
		"coefs.sgn.pgm",
		"coefs.l15.pgm",
		"coefs.len.pgm",
		"coefs.nzz.pgm",
		"coefs.reg.pgm",
		"coefs.tbl.pgm",
		"coefs.bsb.pgm",
		"coefs.nnz.pgm"
	};
	char* fn;
	
	int total_cf, total_sc;
	int width_cf, width_sc;
	int height_cf, height_sc;
	mp3Frame* frame;
	granuleInfo* granule;
	granuleData*** frame_data;
	huffman_reader* decoder;
	unsigned char* data_cf[nfc];
	unsigned char* data_sc[nfs];
	unsigned char* temp_cf;
	unsigned char* temp_sc;	
	int temp_reg[15];
	int tmp;
	int gr, ch;
	int blt;
	int n_sc, n_cf;
	int i, r;
	
	
	// open files
	for ( i = 0; i < (nfs+nfc); i++ ) {
		fn = create_filename( filelist[ file_no ], ext[i] );
		// open file for output
		fp[i] = fopen( fn, "wb" );
		free( fn );
		if ( fp[i] == NULL ) {
			for ( i--; i >= 0; i-- ) fclose( fp[i] );
			sprintf( errormessage, FWR_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}
	
	// decide widths for both (try reaching 4:3 ratio)
	total_cf = g_nframes * g_nchannels * 2 * ( 576 + 2 + 2 + 2 );
	total_sc = g_nframes * g_nchannels * 2 * (  32 + 2 + 2 + 2 );
	height_cf = ( 576 + 2 + 2 + 2 ) * 2;
	for ( i = 2; ( 582 * 582 * i * i ) <= total_cf; i += 2 )
		height_cf = ( 576 + 2 + 2 + 2 ) * i;
	height_sc = (  32 + 2 + 2 + 2 ) * 2;
	for ( i = 2; (  38 *  38 * i * i ) <= total_sc; i += 2 )
		height_sc = (  32 + 2 + 2 + 2 ) * i;
	width_cf = ( total_cf + height_cf - 1 ) / height_cf;
	width_sc = ( total_sc + height_sc - 1 ) / height_sc;
	if ( width_cf % 2 == 1 ) width_cf++;
	if ( width_sc % 2 == 1 ) width_sc++;
	
	// write pgm headers
	for ( i = 0; i < (nfs+nfc); i++ ) {
		fprintf( fp[i], "P5\n" );
		fprintf( fp[i], "# created by %s v%i.%i%s (%s) by %s\n",
			appname, appversion / 10, appversion % 10, subversion, versiondate, author );
		fprintf( fp[i], "%i %i\n", (i>=nfs)?width_cf:width_sc, (i>=nfs)?height_cf:height_sc );
		fprintf( fp[i], "255\n" );
	}
	
	// alloc memory
	temp_sc = ( unsigned char* ) calloc( 32, sizeof( char ) );
	temp_cf = ( unsigned char* ) calloc( 576, sizeof( char ) );
	for ( i = 0; i < nfs; i++ ) {
		data_sc[i] = ( unsigned char* ) calloc( width_sc * g_nchannels * 38, sizeof( char ) );
		if ( data_sc[i] == NULL ) {
			free( temp_sc ); free( temp_cf );
			for ( i = (nfs+nfc) - 1; i >= 0; i-- ) fclose( fp[i] );
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}
	for ( i = 0; i < nfc; i++ ) {
		data_cf[i] = ( unsigned char* ) calloc( width_cf * g_nchannels * 582, sizeof( char ) );
		if ( data_cf[i] == NULL ) {
			free( temp_sc ); free( temp_cf );
			for ( i = (nfs+nfc) - 1; i >= 0; i-- ) fclose( fp[i] );
			sprintf( errormessage, MEM_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}
	
	
	// init decoder	
	decoder = new huffman_reader( main_data, main_data_size );
	
	// main processing loop
	for ( n_sc = 0, n_cf = 0, frame = firstframe; frame != NULL; frame = frame->next ) {
		frame_data = ( frame->n >= n_bad_first ) ? mp3_decode_frame( decoder, frame ) : NULL;
		for ( gr = 0; gr < 2; gr++, n_sc++, n_cf++ ) {
			if ( frame_data != NULL ) for ( ch = 0; ch < g_nchannels; ch++ ) {
				granule = frame->granules[ch][gr];
				// block type
				blt = frame->granules[ch][gr]->block_type;
				
				// scalefactors visualization
				for ( i = 0; i < nfs; i++ ) {
					switch ( i ) {
						case 0: // untouched scalefactors
							for ( r = ( blt == SHORT_BLOCK ) ? 31 : 20; r >= 0; r-- )
								temp_sc[r] = frame_data[ch][gr]->scalefactors[ r ] * 16;
							break;
						case 1: // normalized scalefactors
							for ( r = ( blt == SHORT_BLOCK ) ? 31 : 20; r >= 0; r-- ) {
								temp_sc[r] = frame_data[ch][gr]->scalefactors[ r ];
								if ( granule->preemphasis && ( granule->block_type != SHORT_BLOCK ) )
									temp_sc[r] += preemphasis_table[ i ];
								if ( granule->coarse_scalefactors )
									temp_sc[r] <<= 1;
								temp_sc[r] *= 6;
							}
							break;
					}
					data_sc[i][ ( (  0 + (ch*40) ) * width_sc ) + n_sc ] = v_types[blt][0];
					data_sc[i][ ( (  1 + (ch*40) ) * width_sc ) + n_sc ] = v_types[blt][1];
					data_sc[i][ ( ( 34 + (ch*40) ) * width_sc ) + n_sc ] = v_types[blt][0];
					data_sc[i][ ( ( 35 + (ch*40) ) * width_sc ) + n_sc ] = v_types[blt][1];
					if ( ch == 0 ) for ( r = ( blt == SHORT_BLOCK ) ? 31 : 20; r >= 0; r-- )
						data_sc[i][ ( ( 2 + 31 - r ) * width_sc ) + n_sc ] = temp_sc[r];
					else for ( r = ( blt == SHORT_BLOCK ) ? 31 : 20; r >= 0; r-- )
						data_sc[i][ ( ( 40 + 2 + r ) * width_sc ) + n_sc ] = temp_sc[r];
					if ( g_nchannels == 1 ) {
						data_sc[i][ ( 36 * width_sc ) + n_sc ] = granule->global_gain;
						data_sc[i][ ( 37 * width_sc ) + n_sc ] = granule->global_gain;
					} else if ( ch == 0 ) {
						data_sc[i][ ( 36 * width_sc ) + n_sc ] = granule->global_gain;
						data_sc[i][ ( 37 * width_sc ) + n_sc ] = frame->stereo_ms * 255;
					} else {
						data_sc[i][ ( 38 * width_sc ) + n_sc ] = frame->stereo_ms * 255;
						data_sc[i][ ( 39 * width_sc ) + n_sc ] = granule->global_gain;
					}
				}				
				
				
				// coefficients visualization
				for ( i = 0; i < nfc; i++ ) {
					switch ( i ) {
						case 0: // fixed 4 bit part
							for ( r = 0; r < 576; r++ )	temp_cf[r] =
								( frame_data[ch][gr]->coefficients[ r ] >  15 ) ? 15 * 16:
								( frame_data[ch][gr]->coefficients[ r ] < -15 ) ? 15 * 16:
								( frame_data[ch][gr]->coefficients[ r ] >=  0 ) ? 
									 frame_data[ch][gr]->coefficients[ r ] * 16:
									-frame_data[ch][gr]->coefficients[ r ] * 16;
							break;
						case 1: // sign
							for ( r = 0; r < 576; r++ ) temp_cf[r] = 
								( frame_data[ch][gr]->coefficients[ r ] == 0 ) ? 128 :
									( frame_data[ch][gr]->coefficients[ r ] > 0 ) ? 255 : 0;
							break;
						case 2: // 4 bit truncated bitlength
							for ( r = 0; r < 576; r++ ) temp_cf[r] =
								( frame_data[ch][gr]->coefficients[ r ] > 15 ) ?
									BITLEN8192P( ( frame_data[ch][gr]->coefficients[ r ] - 15 ) ) * 18 :
								( frame_data[ch][gr]->coefficients[ r ] < -15 ) ?
									BITLEN8192P( ( -frame_data[ch][gr]->coefficients[ r ] - 15 ) ) * 18 : 0;
							break;
						case 3: // full bitlength
							for ( r = 0; r < 576; r++ ) temp_cf[r] = 
								BITLEN8224N( frame_data[ch][gr]->coefficients[ r ] ) * 19;
							break;
						case 4: // zeroes - non-zeroes
							for ( r = 0; r < 576; r++ ) temp_cf[r] = 
								( frame_data[ch][gr]->coefficients[ r ] == 0 ) ? 0 : 128;
							break;
						case 5: // regions
							for ( r = 0; r < 576; r++ ) temp_cf[r] = 
								( r < granule->region_bound[0] ) ? 255 :
								( r < granule->region_bound[1] ) ? 191 :
								( r < granule->region_bound[2] ) ? 131 :
								( r < granule->sv_bound ) ? 63 : 0;
							break;
						case 6: // tables
							for ( r = 0; r < 576; r++ ) temp_cf[r] =
								( r < granule->region_bound[0] ) ? granule->region_table[0] * 7 + 21 :
								( r < granule->region_bound[1] ) ? granule->region_table[1] * 7 + 21 :
								( r < granule->region_bound[2] ) ? granule->region_table[2] * 7 + 21 :
								( r < granule->sv_bound ) ? granule->select_htabB * 7 + 7 : 0;
							break;
						case 7: // only bv/sv region diffs
							for ( r = 0; r < 576; r++ ) temp_cf[r] = 
								( r < granule->sv_bound - granule->region_bound[2] ) ? 255 : 0;
							break;
						case 8: // numbers of non-zeroes
							memset( temp_reg, 0x00, 16 * sizeof(int) );
							for ( tmp = 0; tmp < 16; tmp++ ) for ( r = 0; r < 576; r++ )
								if ( ABS( frame_data[ch][gr]->coefficients[ r ] ) >= tmp ) temp_reg[tmp]++;
							for ( tmp = 0, r = 0; tmp < 16; tmp++ ) for ( ; r < temp_reg[15-tmp]; r++ )
								temp_cf[r] = (15-tmp) * 16;
							break;
					}					
					data_cf[i][ ( (   0 + (ch*584) ) * width_cf ) + n_cf ] = v_types[blt][0];
					data_cf[i][ ( (   1 + (ch*584) ) * width_cf ) + n_cf ] = v_types[blt][1];
					data_cf[i][ ( ( 578 + (ch*584) ) * width_cf ) + n_cf ] = v_types[blt][0];
					data_cf[i][ ( ( 579 + (ch*584) ) * width_cf ) + n_cf ] = v_types[blt][1];
					if ( ch == 0 ) for ( r = 576 - 1; r >= 0; r-- )
						data_cf[i][ ( ( 2 + 575 - r ) * width_cf ) + n_cf ] = temp_cf[ r ];
					else for ( r = 0; r < 576; r++ )
						data_cf[i][ ( ( 584 + 2 + r ) * width_cf ) + n_cf ] = temp_cf[ r ];
					if ( g_nchannels == 1 ) {
						data_cf[i][ ( 580 * width_cf ) + n_cf ] = granule->global_gain;
						data_cf[i][ ( 581 * width_cf ) + n_cf ] = granule->global_gain;
					} else if ( ch == 0 ) {
						data_cf[i][ ( 580 * width_cf ) + n_cf ] = granule->global_gain;
						data_cf[i][ ( 581 * width_cf ) + n_cf ] = frame->stereo_ms * 255;
					} else {
						data_cf[i][ ( 582 * width_cf ) + n_cf ] = frame->stereo_ms * 255;
						data_cf[i][ ( 583 * width_cf ) + n_cf ] = granule->global_gain;
					}
				}
			} else {
				// no scalefactors
				for ( i = 0; i < nfs; i++ ) {
					data_sc[i][ (  0 * width_sc ) + n_sc ] = v_unk;
					data_sc[i][ (  1 * width_sc ) + n_sc ] = v_unk;
					data_sc[i][ ( 34 * width_sc ) + n_sc ] = v_unk;
					data_sc[i][ ( 35 * width_sc ) + n_sc ] = v_unk;
					if ( g_nchannels == 2 ) {
						data_sc[i][ ( 36 * width_sc ) + n_sc ] = v_unk;
						data_sc[i][ ( 37 * width_sc ) + n_sc ] = v_unk;
						data_sc[i][ ( 70 * width_sc ) + n_sc ] = v_unk;
						data_sc[i][ ( 71 * width_sc ) + n_sc ] = v_unk;
					}
				}
				// no coefficients
				for ( i = 0; i < nfc; i++ ) {
					data_cf[i][ (    0 * width_cf ) + n_cf ] = v_unk;
					data_cf[i][ (    1 * width_cf ) + n_cf ] = v_unk;
					data_cf[i][ (  578 * width_cf ) + n_cf ] = v_unk;
					data_cf[i][ (  579 * width_cf ) + n_cf ] = v_unk;
					if ( g_nchannels == 2 ) {
						data_cf[i][ (  580 * width_cf ) + n_cf ] = v_unk;
						data_cf[i][ (  581 * width_cf ) + n_cf ] = v_unk;
						data_cf[i][ ( 1158 * width_cf ) + n_cf ] = v_unk;
						data_cf[i][ ( 1159 * width_cf ) + n_cf ] = v_unk;
					}
				}
			}
		}
		
		if ( n_sc >= width_sc ) { // flush scfs
			for ( i = 0; i < nfs; i++ ) {
				fwrite( data_sc[i], 1, width_sc * g_nchannels * 38, fp[i] );
				memset( data_sc[i], v_zero, width_sc * g_nchannels * 38 );
			}
			n_sc = 0;
		}
		
		if ( n_cf >= width_cf ) { // flush coefs
			for ( i = 0; i < nfc; i++ ) {
				fwrite( data_cf[i], 1, width_cf * g_nchannels * 582, fp[nfs+i] );
				memset( data_cf[i], v_zero, width_cf * g_nchannels * 582 );
			}
			n_cf = 0;
		}
	}
	if ( n_sc > 0 ) { // flush scfs
		for ( i = 0; i < nfs; i++ )
			fwrite( data_sc[i], 1, width_sc * g_nchannels * 38, fp[i] );
		n_sc = 0;
	}
	if ( n_cf > 0 ) { // flush coefs
		for ( i = 0; i < nfc; i++ )
			fwrite( data_cf[i], 1, width_cf * g_nchannels * 582, fp[nfs+i] );
		n_cf = 0;
	}
	
	
	// close file pointers / dealloc memory
	free( temp_cf );
	free( temp_sc );
	for ( i = 0; i < (nfs+nfc); i++ ) fclose( fp[i] );
	for ( i = 0; i < nfs; i++ ) free( data_sc[i] );
	for ( i = 0; i < nfc; i++ ) free( data_cf[i] );
	
	
	// delete decoder
	delete( decoder );
	
	
	return true;
}


/* -----------------------------------------------
	dump main data size
	----------------------------------------------- */
INTERN bool dump_main_sizes( void )
{	
	FILE* fp;
	char* fn;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "main_size.u2b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// dump data & close file
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
		fwrite( &(frame->main_size), 2, 1, fp );
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump aux data size
	----------------------------------------------- */
INTERN bool dump_aux_sizes( void )
{
	FILE* fp;
	char* fn;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "aux_size.u2b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// dump data & close file
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
		fwrite( &(frame->aux_size), 2, 1, fp );
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump bitrates
	----------------------------------------------- */
INTERN bool dump_bitrates( void )
{
	FILE* fp;
	char* fn;
	
	
	// only output for vbr files
	if ( i_bitrate != -1 ) return true;
	
	// create filename
	fn = create_filename( filelist[ file_no ], "bitrate.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// dump data & close file
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
		fwrite( &(frame->bits), 1, 1, fp );
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump stereo ms bit
	----------------------------------------------- */
INTERN bool dump_stereo_ms( void )
{
	FILE* fp;
	char* fn;
	
	
	// check if there is any ms stereo
	if ( i_stereo_ms == 0 ) return true;
	
	// create filename
	fn = create_filename( filelist[ file_no ], "stereo_ms.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// dump data & close file
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
		fwrite( &(frame->stereo_ms), 1, 1, fp );
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump padding bit
	----------------------------------------------- */
INTERN bool dump_padding( void )
{
	FILE* fp;
	char* fn;
	
	
	// check if there is any padding
	if ( i_padding == 0 ) return true;
	
	// create filename
	fn = create_filename( filelist[ file_no ], "padding.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );
	
	// dump data & close file
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
		fwrite( &(frame->padding), 1, 1, fp );
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump main data bit counts
	----------------------------------------------- */
INTERN bool dump_main_data_bits( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "main_bits.u2b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->main_data_bit), 2, 1, fp );
		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump big values
	----------------------------------------------- */
INTERN bool dump_big_value_ns( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "big_value_pairs.u2b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->big_val_pairs), 2, 1, fp );
		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump global gains
	----------------------------------------------- */
INTERN bool dump_global_gain( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "global_gain.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->global_gain), 1, 1, fp );
		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump slength
	----------------------------------------------- */
INTERN bool dump_slength( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "slength.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->slength), 1, 1, fp );
		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump block types
	----------------------------------------------- */
INTERN bool dump_block_types( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "block_types.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->block_type), 1, 1, fp );
		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump sharing bits
	----------------------------------------------- */
INTERN bool dump_sharing( void )
{
	FILE* fp;
	char* fn;
	int ch;
	int c;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "sharing.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
			c = BITN( frame->granules[ch][0]->share, 3 ); fwrite( &c, 1, 1, fp );
			c = BITN( frame->granules[ch][0]->share, 2 ); fwrite( &c, 1, 1, fp );
			c = BITN( frame->granules[ch][0]->share, 1 ); fwrite( &c, 1, 1, fp );
			c = BITN( frame->granules[ch][0]->share, 0 ); fwrite( &c, 1, 1, fp );
		}		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump preemphasis setting
	----------------------------------------------- */
INTERN bool dump_preemphasis( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "preemphasis.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->preemphasis), 1, 1, fp );
		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump coarse setting
	----------------------------------------------- */
INTERN bool dump_coarse( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "coarse_scalefactors.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->coarse_scalefactors), 1, 1, fp );
		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump huffman table selections
	----------------------------------------------- */
INTERN bool dump_htable_sel( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "htable_selection.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( int i = 0; i < 3; i++ ) {
			for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
				for ( gr = 0; gr < 2; gr++ )
					fwrite( &(frame->granules[ch][gr]->region_table[i]), 1, 1, fp );
		}
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->select_htabB), 1, 1, fp );
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump region sizes
	----------------------------------------------- */
INTERN bool dump_region_sizes( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "region_sizes.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->region0_size), 1, 1, fp );
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next )
			for ( gr = 0; gr < 2; gr++ )
				fwrite( &(frame->granules[ch][gr]->region1_size), 1, 1, fp );		
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump subblock gains
	----------------------------------------------- */
INTERN bool dump_subblock_gains( void )
{
	FILE* fp;
	char* fn;
	int ch, gr;
	
	
	// check if there is any subblock gain
	if ( i_sbgain == 0 ) return true;
	
	// create filename
	fn = create_filename( filelist[ file_no ], "subblock_gain.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
			for ( gr = 0; gr < 2; gr++ ) if ( frame->granules[ch][gr]->block_type == SHORT_BLOCK ) {
				fwrite( &(frame->granules[ch][gr]->sb_gain[0]), 1, 1, fp );
				fwrite( &(frame->granules[ch][gr]->sb_gain[1]), 1, 1, fp );
				fwrite( &(frame->granules[ch][gr]->sb_gain[2]), 1, 1, fp );
			}
		}
	}
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump data files
	----------------------------------------------- */
INTERN bool dump_data_files( void )
{
	FILE* fp_aux;
	FILE* fp_huf;
	char* fn;
	
	
	// --- simple stuff ---
	if ( main_data_size > 0 )
		if ( !write_file( filelist[ file_no ], "main_data.s1b", main_data, 1, main_data_size ) )
			return false;
	if ( data_before_size > 0 )
		if ( !write_file( filelist[ file_no ], "data_before.s1b", data_before, 1, data_before_size ) )
			return false;
	if ( data_after_size > 0 )
		if ( !write_file( filelist[ file_no ], "data_after.s1b", data_after, 1, data_after_size ) )
			return false;
	
	// --- huffman and aux data files ---
	
	// open huffman file
	fn = create_filename( filelist[ file_no ], "huff_data.s1b" );
	fp_huf = fopen( fn, "wb" );
	free( fn );	
	// open aux file
	fn = create_filename( filelist[ file_no ], "aux_data.s1b" );
	fp_aux = fopen( fn, "wb" );
	free( fn );
	// check for problems
	if ( ( fp_huf == NULL ) || ( fp_aux == NULL ) ) {
		if ( fp_huf != NULL ) fclose( fp_huf );
		if ( fp_aux != NULL ) fclose( fp_aux );
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// dump data
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
		if ( frame->n < n_bad_first ) continue;
		fwrite( main_data + frame->main_index, 1, frame->main_size, fp_huf );
		fwrite( main_data + frame->main_index + frame->main_size, 1, frame->aux_size, fp_aux );
	}
	
	// close files
	fclose( fp_huf );
	fclose( fp_aux );
	
	
	return true;
}


/* -----------------------------------------------
	dump global gain context
	----------------------------------------------- */
INTERN bool dump_gg_ctx( void )
{	
	FILE* fp;
	char* fn;
	
	
	// build the context (even if it is already built)
	pmp_build_context();
	
	
	// create filename
	fn = create_filename( filelist[ file_no ], "gg_ctx.u1b" );
	
	// open file for output
	fp = fopen( fn, "wb" );
	if ( fp == NULL ){
		sprintf( errormessage, FWR_ERRMSG );
		errorlevel = 2;
		return false;
	}
	free( fn );	
	
	// dump data
	fwrite( gg_context[0], 1, 2 * g_nframes, fp );
	if ( g_nchannels == 2 )
		fwrite( gg_context[1], 1, 2 * g_nframes, fp );
	
	// close file
	fclose( fp );
	
	
	return true;
}


/* -----------------------------------------------
	dump coefficients
	----------------------------------------------- */
INTERN bool dump_decoded_data( void )
{
	static const unsigned char zeroes[32] = { 0 };
	FILE* fp_cf[2];
	FILE* fp_sc[2];
	const char* ext_cf[2] = { "coefs.ch0.s2b", "coefs.ch1.s2b" };
	const char* ext_sc[2] = { "scale.ch0.u1b", "scale.ch1.u1b" };
	char* fn;
	int ch, gr;
	
	granuleData*** frame_data;
	huffman_reader* decoder;
	
	
	// open 4 files (max)
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		// coeficients file
		// create filename
		fn = create_filename( filelist[ file_no ], ext_cf[ch] );
		// open file for output
		fp_cf[ch] = fopen( fn, "wb" );
		free( fn );
		// scalefactors file
		// create filename
		fn = create_filename( filelist[ file_no ], ext_sc[ch] );
		// open file for output
		fp_sc[ch] = fopen( fn, "wb" );
		free( fn );
		// error checking and file pointer rollback
		if ( ( fp_cf[ch] == NULL ) || ( fp_sc[ch] == NULL ) ) {
			for ( ; ch >= 0; ch-- ) { 
				if ( fp_cf[ch] != NULL ) fclose( fp_cf[ch] );
				if ( fp_sc[ch] != NULL ) fclose( fp_sc[ch] );
			}
			sprintf( errormessage, FWR_ERRMSG );
			errorlevel = 2;
			return false;
		}
	}
	
	// init decoder	
	decoder = new huffman_reader( main_data, main_data_size );
	
	// dump data
	for ( mp3Frame* frame = firstframe; frame != NULL; frame = frame->next ) {
		// skip bad first frames
		if ( frame->n < n_bad_first ) continue;
		frame_data = mp3_decode_frame( decoder, frame );
		if ( frame_data == NULL ) continue;
		for ( gr = 0; gr < 2; gr++ ) {
			for ( ch = 0; ch < g_nchannels; ch++ ) {
				fwrite( frame_data[ch][gr]->coefficients, sizeof( short ), 576, fp_cf[ch] );
				if ( frame->granules[ch][gr]->block_type != SHORT_BLOCK ) {
					fwrite( frame_data[ch][gr]->scalefactors, sizeof( char ), 21, fp_sc[ch] );
					fwrite( zeroes, sizeof( char ), 11, fp_sc[ch] );
				} else fwrite( frame_data[ch][gr]->scalefactors, sizeof( char ), 32, fp_sc[ch] );
			}
		}
	}
	
	// close files
	for ( ch = 0; ch < g_nchannels; ch++ ) {
		fclose( fp_cf[ch] );
		fclose( fp_sc[ch] );
	}
	
	// delete decoder
	delete( decoder );
	
	
	return true;
}
#endif

/* ----------------------- End of developers functions -------------------------- */

/* ----------------------- End of file -------------------------- */
