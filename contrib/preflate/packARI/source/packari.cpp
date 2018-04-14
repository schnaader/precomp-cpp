#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctime>

#include "bitops.h"
#include "aricoder.h"
#include "paritbl.h"

#if defined BUILD_DLL // define BUILD_LIB from the compiler options if you want to compile a DLL!
	#define BUILD_LIB
#endif

#if defined BUILD_LIB // define BUILD_LIB from the compiler options if you want to compile a library!
	#include "packarilib.h"
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
INTERN bool decide_model_order( void );
INTERN bool encode_file( void );
INTERN bool decode_file( void );


/* -----------------------------------------------
	function declarations: other coding functions
	----------------------------------------------- */

INTERN inline bool check_par_header( void );
INTERN inline bool read_par_header( void );
INTERN inline bool write_par_header( void );
INTERN inline bool endecode_file( bool encoding );
INTERN inline double cost_count_ari( model_s* model, int c );

	
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
// in any way to compress or decompress files
#if !defined(BUILD_LIB) && defined(DEV_BUILD)
INTERN bool write_errfile( void );
#endif

/* -----------------------------------------------
	global variables: library only variables
	----------------------------------------------- */
#if defined(BUILD_LIB)
INTERN int lib_in_type  = -1;
INTERN int lib_out_type = -1;
#endif


/* -----------------------------------------------
	global variables: info about files
	----------------------------------------------- */
	
INTERN char*  cmpfilename = NULL;	// name of compressed file
INTERN char*  uncfilename = NULL;	// name of uncompressed file
INTERN int    cmpfilesize;			// size of compressed file
INTERN int    uncfilesize;			// size of uncompressed file
INTERN int    filetype;				// type of current file
INTERN iostream* str_in  = NULL;	// input stream
INTERN iostream* str_out = NULL;	// output stream

#if !defined( BUILD_LIB )
INTERN iostream* str_str = NULL;	// storage stream

INTERN char** filelist = NULL; 		// list of files to process 
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
INTERN int  verbosity  = -1;	// level of verbosity
INTERN bool overwrite  = false;	// overwrite files yes / no
INTERN bool wait_exit  = true;	// pause after finished yes / no
INTERN bool force_enc  = false;	// force encoding even for already compressed files yes / no
INTERN bool store_name = true;	// store file name in compressed files yes / no
INTERN int  verify_lv  = 0;		// verification level ( none (0), simple (1), detailed output (2) )
INTERN int  err_tol    = 1;		// error threshold ( proceed on warnings yes (2) / no (1) )

INTERN bool developer  = false;	// allow developers functions yes/no
INTERN int  action     = A_COMPRESS; // what to do with files

INTERN FILE*  msgout   = stdout;	// stream for output of messages
INTERN bool   pipe_on  = false;	// use stdin/stdout instead of filelist
#else
INTERN bool force_enc  = false;	// force encoding even for already compressed files yes / no
INTERN bool store_name = false;	// store file name in compressed files yes / no
INTERN int  err_tol    = 1;		// error threshold ( proceed on warnings yes (2) / no (1) )
INTERN int  action     = A_COMPRESS; // what to do with files
#endif

/* -----------------------------------------------
	global variables: coding parameters
	----------------------------------------------- */

INTERN int model_order = -1; // order of statistical model (decided by analysis)
INTERN int stored_order = -1; // stored model order


/* -----------------------------------------------
	global variables: info about program
	----------------------------------------------- */

INTERN const unsigned char appversion = 6;
INTERN const char*  subversion   = "f";
INTERN const char*  apptitle     = "packARI";
INTERN const char*  appname      = "packARI";
INTERN const char*  versiondate  = "01/22/2016";
INTERN const char*  author       = "Matthias Stirner";
#if !defined(BUILD_LIB)
INTERN const char*  website      = "http://packjpg.encode.ru/";
INTERN const char*  email        = "packjpg (at) matthiasstirner.com";
INTERN const char*	copyright    = "2012-2016 Matthias Stirner & HTW Aalen";
INTERN const char*  cmp_ext      = "par";
INTERN const char*  unc_ext      = "unc";
#endif
INTERN const char   cmp_magic[] = { 'A', 'S' };


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
	
	double acc_uncsize = 0;
	double acc_cmpsize = 0;
	
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
			acc_uncsize += uncfilesize;
			acc_cmpsize += cmpfilesize;
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
	if ( ( file_cnt > error_cnt ) && ( verbosity != 0 ) &&
	 ( action == A_COMPRESS ) ) {
		acc_uncsize /= 1024.0; acc_cmpsize /= 1024.0;
		total = (double) ( end - begin ) / CLOCKS_PER_SEC; 
		kbps  = ( total > 0 ) ? ( acc_uncsize / total ) : acc_uncsize;
		cr    = ( acc_uncsize > 0 ) ? ( 100.0 * acc_cmpsize / acc_uncsize ) : 0;
		
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
EXPORT bool parlib_convert_stream2stream( char* msg )
{
	// process in main function
	return parlib_convert_stream2mem( NULL, NULL, msg ); 
}
#endif


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */

#if defined(BUILD_LIB)
EXPORT bool parlib_convert_file2file( char* in, char* out, char* msg )
{
	// init streams
	parlib_init_streams( (void*) in, 0, 0, (void*) out, 0 );
	
	// process in main function
	return parlib_convert_stream2mem( NULL, NULL, msg ); 
}
#endif


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT bool parlib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg )
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
			if ( filetype == F_UNK ) {
				if ( file_exists( cmpfilename ) ) remove( cmpfilename );
			} else if ( filetype == F_PAR ) {
				if ( file_exists( uncfilename ) ) remove( uncfilename );
			}
		}
		if ( msg != NULL ) strcpy( msg, errormessage );
		return false;
	}
	
	// get compression info
	total = (int) ( (double) (( end - begin ) * 1000) / CLOCKS_PER_SEC );
	cr    = ( uncfilesize > 0 ) ? ( 100.0 * cmpfilesize / uncfilesize ) : 0;
	
	// write success message else
	if ( msg != NULL ) {
		switch( filetype )
		{
			case F_UNK:
				sprintf( msg, "Compressed to %s (%.2f%%) in %ims",
					cmpfilename, cr, ( total >= 0 ) ? total : -1 );
				break;
			case F_PAR:
				sprintf( msg, "Decompressed to %s (%.2f%%) in %ims",
					uncfilename, cr, ( total >= 0 ) ? total : -1 );
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
EXPORT void parlib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type )
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
	
	unsigned char buffer[ 3 ];
	
	
	// (re)set errorlevel
	filetype = F_UNK;
	errorfunction = NULL;
	errorlevel = 0;
	uncfilesize = 0;
	cmpfilesize = 0;
	
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
	if ( uncfilename != NULL ) free( uncfilename ); uncfilename = NULL;
	if ( cmpfilename != NULL ) free( cmpfilename ); cmpfilename = NULL;
	
	// check input stream
	if ( !force_enc ) {
		str_in->read( buffer, 1, 3 );
		if ( (buffer[0] == cmp_magic[0]) && (buffer[1] == cmp_magic[1]) ) {
			// file might be PAR - check version and header
			if ( check_par_header() ) {
				filetype = F_PAR;
				if ( buffer[2] != appversion ) {
					sprintf( errormessage, "incompatible file, use %s v%i.%i",
						appname, buffer[2] / 10, buffer[2] % 10 );
					errorlevel = 2;
					return;
				}
				// copy filenames
				cmpfilename = (char*) calloc( (  in_type == 0 ) ? strlen( (char*) in_src   ) + 1 : 32, sizeof( char ) );
				uncfilename = (char*) calloc( ( out_type == 0 ) ? strlen( (char*) out_dest ) + 1 : 32, sizeof( char ) );
				strcpy( cmpfilename, (  in_type == 0 ) ? (char*) in_src   : "PAR in memory" );
				strcpy( uncfilename, ( out_type == 0 ) ? (char*) out_dest : "ANY in memory" );
			}
		}
		str_in->rewind();
	}
	
	if ( filetype == F_UNK ) {
		// file is any other file type
		filetype = F_UNK;
		// copy filenames
		uncfilename = (char*) calloc( (  in_type == 0 ) ? strlen( (char*) in_src   ) + 1 : 32, sizeof( char ) );
		cmpfilename = (char*) calloc( ( out_type == 0 ) ? strlen( (char*) out_dest ) + 1 : 32, sizeof( char ) );
		strcpy( uncfilename, (  in_type == 0 ) ? (char*) in_src   : "PAR in memory" );
		strcpy( cmpfilename, ( out_type == 0 ) ? (char*) out_dest : "ANY in memory" );
		// get uncompressed file size
		uncfilesize = str_in->getsize();
		str_in->rewind();
	}
	
	// store types of in-/output
	lib_in_type  = in_type;
	lib_out_type = out_type;
}
#endif


/* -----------------------------------------------
	DLL export force encoding setting
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT void parlib_force_encoding( bool setting )
{
	// adopt the setting
	force_enc = setting;
}
#endif


/* -----------------------------------------------
	DLL export version information
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT const char* parlib_version_info( void )
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
EXPORT const char* parlib_short_name( void )
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
		else if ( strcmp((*argv), "-f" ) == 0 ) {
			force_enc = true;
		}
		#if defined(DEV_BUILD)
		else if ( strcmp((*argv), "-dev") == 0 ) {
			developer = true;
		}
		else if ( strcmp((*argv), "-test") == 0 ) {
			verify_lv = 2;
		}
		else if ( strcmp((*argv), "-nfn") == 0 ) {
			store_name = false;
		}
		else if ( sscanf( (*argv), "-mo%i", &tmp_val ) == 1 ) {
			tmp_val = ( tmp_val < 0 ) ? 0 : tmp_val;
			tmp_val = ( tmp_val > MAX_ORDER ) ? MAX_ORDER : tmp_val;
			stored_order = tmp_val;
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
	uncfilesize = 0;
	cmpfilesize = 0;	
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
		switch ( action ) {
			case A_COMPRESS: ( filetype == F_UNK ) ? actionmsg = "Compressing" : actionmsg = "Decompressing";
				break;
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
		if ( filetype == F_UNK ) {
			if ( file_exists( cmpfilename ) ) remove( cmpfilename );
		} else if ( filetype == F_PAR ) {
			if ( file_exists( uncfilename ) ) remove( uncfilename );
		}
	}
	
	end = clock();	
	
	// speed and compression ratio calculation
	total = (int) ( (double) (( end - begin ) * 1000) / CLOCKS_PER_SEC );
	bpms  = ( total > 0 ) ? ( uncfilesize / total ) : uncfilesize;
	cr    = ( uncfilesize > 0 ) ? ( 100.0 * cmpfilesize / uncfilesize ) : 0;

	
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
	} else if ( function == *decide_model_order ) {
		return "Deciding model order";
	} else if ( function == *encode_file ) {
		return "Compressing file";
	} else if ( function == *decode_file ) {
		return "Decompressing file";
	} else if ( function == *swap_streams ) {
		return "Swapping input/output streams";
	} else if ( function == *compare_output ) {
		return "Verifying output stream";
	} else if ( function == *reset_buffers ) {
		return "Resetting program";
	}
	#if defined(DEV_BUILD)
	// no specific developer functions!
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
	fprintf( msgout, " [-f]     force encoding for compressed files\n" );
	fprintf( msgout, " [-ver]   verify files after processing\n" );
	fprintf( msgout, " [-v?]    set level of verbosity (max: 2) (def: 0)\n" );
	fprintf( msgout, " [-np]    no pause after processing files\n" );
	fprintf( msgout, " [-o]     overwrite existing files\n" );
	fprintf( msgout, " [-p]     proceed on warnings\n" );
	#if defined(DEV_BUILD)
	if ( developer ) {
	fprintf( msgout, "\n" );
	fprintf( msgout, " [-mo?]   force model order for coding (0...%i)\n", MAX_ORDER );
	fprintf( msgout, " [-nfn]   don't store filenames\n" );
	}
	#endif
	fprintf( msgout, "\n" );
	fprintf( msgout, "Examples: \"%s -v1 -o image??.%s\"\n", appname, cmp_ext );
	fprintf( msgout, "          \"%s -p *.txt\"\n", appname );	
}
#endif


/* -----------------------------------------------
	processes one file
	----------------------------------------------- */

INTERN void process_file( void )
{	
	if ( filetype == F_UNK ) {
		switch ( action ) {
			case A_COMPRESS:
				execute( decide_model_order );
				execute( encode_file );
				#if !defined(BUILD_LIB)	
				if ( verify_lv > 0 ) { // verifcation
					execute( reset_buffers );
					execute( swap_streams );
					execute( decode_file );
					execute( compare_output );
				}
				#endif
				break;
				
			#if !defined(BUILD_LIB) && defined(DEV_BUILD)
			// no developers functionality!
			#else
			default:
				break;
			#endif
		}
	}
	else if ( filetype == F_PAR )	{
		switch ( action )
		{
			case A_COMPRESS:
				execute( decode_file );
				#if !defined(BUILD_LIB)
				// this does not work yet!
				// and it's not even needed
				/*if ( verify_lv > 0 ) { // verifcation
					execute( reset_buffers );
					execute( swap_streams );
					execute( decide_model_order );
					execute( encode_file );
					execute( compare_output );
				}*/
				#endif
				break;
				
			#if !defined(BUILD_LIB) && defined(DEV_BUILD)
			// no developers functionality!
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
	unsigned char fileid[ 3 ] = { 0, 0, 0 };
	const char* filename = filelist[ file_no ];
	
	// preset file type
	filetype = F_UNK;
	
	// open input stream, check for errors
	str_in = new iostream( (void*) filename, ( !pipe_on ) ? 0 : 2, 0, 0 );
	if ( str_in->chkerr() ) {
		sprintf( errormessage, FRD_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// free memory from filenames if needed
	if ( uncfilename != NULL ) free( uncfilename ); uncfilename = NULL;
	if ( cmpfilename != NULL ) free( cmpfilename ); cmpfilename = NULL;
	
	// check if this is a compressed par file
	while ( !force_enc ) {
		// read file id and version number
		if ( str_in->read( fileid, 1, 3 ) != 3 ) break;
		// check magic number
		if ( ( fileid[0] != cmp_magic[0] ) || ( fileid[1] != cmp_magic[1] ) ) break;
		// check par header, extract file name
		if ( !check_par_header() ) break;
		// alright: this is a .par file!
		// compare version number
		if ( fileid[2] != appversion ) {
			sprintf( errormessage, "incompatible file, use %s v%i.%i",
				appname, fileid[2] / 10, fileid[2] % 10 );
			errorlevel = 2;
			return false;
		}
		// file is .par and compatible
		filetype = F_PAR;
		// create filenames
		if ( !pipe_on ) {
			cmpfilename = (char*) calloc( strlen( filename ) + 1, sizeof( char ) );
			strcpy( cmpfilename, filename );
			if ( uncfilename != NULL ) {
				// ugly fix for path names in drag and drop
				if ( strrchr( uncfilename, '.' ) != NULL ) {
					uncfilename = ( overwrite ) ?
						create_filename( filename, strrchr( uncfilename, '.' ) + 1 ) :
						unique_filename( filename, strrchr( uncfilename, '.' ) + 1 );
				} else {
					uncfilename = ( overwrite ) ?
						create_filename( filename, NULL ) :
						unique_filename( filename, NULL );
				}
				// also, making full file name redundant
				/*if ( !overwrite ) while ( file_exists( uncfilename ) ) {
					uncfilename = (char*) realloc( uncfilename, strlen( uncfilename ) + 2 );
					add_underscore( uncfilename );
				}*/
			} else {
				uncfilename = ( overwrite ) ?
				create_filename( filename, (char*) unc_ext ) :
				unique_filename( filename, (char*) unc_ext );
			}
		}
		else {
			free( uncfilename );
			uncfilename = create_filename( "STDOUT", NULL );
			cmpfilename = create_filename( "STDIN", NULL );
		}
		// open output stream, check for errors
		str_out = new iostream( (void*) uncfilename, ( !pipe_on ) ? 0 : 2, 0, 1 );
		if ( str_out->chkerr() ) {
			sprintf( errormessage, FWR_ERRMSG );
			errorlevel = 2;
			return false;
		}
		break;
	}
	
	// filetype is unknown, or compression is forced
	if ( filetype == F_UNK ) {
		// free uncompressed file name again
		if ( uncfilename != NULL ) free( uncfilename ); uncfilename = NULL;
		// create filenames
		if ( !pipe_on ) {
			uncfilename = (char*) calloc( strlen( filename ) + 1, sizeof( char ) );
			strcpy( uncfilename, filename );
			cmpfilename = ( overwrite ) ?
				create_filename( filename, (char*) cmp_ext ) :
				unique_filename( filename, (char*) cmp_ext );
		}
		else {
			uncfilename = create_filename( "STDIN", NULL );
			cmpfilename = create_filename( "STDOUT", NULL );
		}
		// open output stream, check for errors
		str_out = new iostream( (void*) cmpfilename, ( !pipe_on ) ? 0 : 2, 0, 1 );
		if ( str_out->chkerr() ) {
			sprintf( errormessage, FWR_ERRMSG );
			errorlevel = 2;
			return false;
		}
		// get uncompressed file size
		uncfilesize = str_in->getsize();
	}
	
	// rewind (need to start from the beginning)
	if ( str_in->rewind() != 0 ) {
		sprintf( errormessage, FRD_ERRMSG );
		errorlevel = 2;
		return false;
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
#if !defined( BUILD_LIB )
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
	/*if ( str_out->getsize() != dsize ) {
		sprintf( errormessage, "file sizes do not match" );
		errorlevel = 2;
		return false;
	}*/
	
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
	// --- restore original settings ---
	model_order = stored_order;
	
	
	return true;
}


/* -----------------------------------------------
	decide order for statistical model
	----------------------------------------------- */

INTERN bool decide_model_order( void )
{
	double cost[ MAX_ORDER + 1 ];
	unsigned char lbyte[ MAX_ORDER + 1 ];
	unsigned char byte;
	model_s* model;
	int o, p;
	int i;
	
	
	// calculate cost for CHECK_BYTES and each order
	for ( o = 0; o <= MAX_ORDER; o++ ) {
		// preparations
		model = INIT_MODEL_S( 256, 256, o );
		memset( lbyte, 0, MAX_ORDER + 1 );
		str_in->rewind();
		cost[ o ] = 0;
		// main testing routine
		for ( p = 0; p < CHECK_BYTES; p++ ) {
			if ( str_in->read( &byte, 1, 1 ) != 1 ) break;
			// context shifting
			for ( i = o-1; i >= 0; i-- )
				model->shift_context( lbyte[i] );
			// cost count
			cost[ o ] += cost_count_ari( model, byte );
			// context update
			for ( i = 1; i < o; i++ )
				lbyte[i] = lbyte[i-1];
			lbyte[0] = byte;
		}
		// clean up
		delete( model );
	}
	
	// rewind (need to start from the beginning)
	if ( str_in->rewind() != 0 ) {
		sprintf( errormessage, FRD_ERRMSG );
		errorlevel = 2;
		return false;
	}
	
	// find the best one
	if ( stored_order == -1 ) for ( model_order = 0, o = 1; o <= MAX_ORDER; o++ )
		if ( cost[ model_order ] > cost[ o ] ) model_order = o;
		
	// check for incompressible files
	if ( (cost[ model_order ]/8) >= p ) {
		sprintf( errormessage, "file seems to be incompressible" );
		errorlevel = 1;
	}
	
	
	return true;
}


/* -----------------------------------------------
	encode one file
	----------------------------------------------- */

INTERN bool encode_file( void )
{
	// --- write PAR header ---
	str_out->write( (void*) cmp_magic, 1, 2 );
	str_out->write( (void*) &appversion, 1, 1 );
	if ( !write_par_header() ) return false;
	
	// --- encode file ---
	if ( !endecode_file( true ) ) return false;
	
	// --- get compressed file size ---
	cmpfilesize = str_out->getsize();
	
	
	return true;
}


/* -----------------------------------------------
	decode one file
	----------------------------------------------- */

INTERN bool decode_file( void )
{
	unsigned char byte;
	
	// --- get compressed file size ---
	cmpfilesize = str_in->getsize();
	
	// --- read PAR header again ---
	str_in->read( (void*) &byte, 1, 1 ); // skip byte 0
	str_in->read( (void*) &byte, 1, 1 ); // skip byte 1
	str_in->read( (void*) &byte, 1, 1 ); // skip byte 2
	if ( !read_par_header() ) {
		sprintf( errormessage, "this should not have happened :-(" );
		errorlevel = 2;
		return false;
	}
	
	// --- decode file ---
	if ( !endecode_file( false ) ) return false;
	
	
	return true;
}

/* ----------------------- End of main functions -------------------------- */

/* ----------------------- Begin of other coding functions -------------------------- */


/* -----------------------------------------------
	check par header, extract uncompressed name
	----------------------------------------------- */
INTERN inline bool check_par_header( void )
{
	unsigned char hash = 0x00;
	unsigned char byte;
	char* fn;
	int i;
	
	
	for ( i = 0; i < 5; i++ ) {
		if ( str_in->read( (void*) &byte, 1, 1 ) != 1 ) return false;
		hash = pearson_hash[ byte ^ hash ];
	}
	
	// fetch file name, followed by 0x00, do hashing
	if ( str_in->read( (void*) &byte, 1, 1 ) != 1 ) return false;
	if ( byte != 0x00 ) {
		uncfilename = ( char* ) calloc( 256 + 1, sizeof( char ) ); // !!!
		fn = uncfilename; hash = pearson_hash[ byte ^ hash ];
		for ( (*fn++) = byte, i = 1; i < 256; i++, fn++ ) {
			if ( str_in->read( (void*) fn, 1, 1 ) != 1 ) return false;
			hash = pearson_hash[ (*fn) ^ hash ];
			if ( (*fn) == 0x00 ) break;
		}
		if ( i == 256 ) return false;
	} else hash = pearson_hash[ hash ];
	
	// check hash
	if ( str_in->read( (void*) &byte, 1, 1 ) != 1 ) return false;
	if ( hash != byte ) return false;
	
	
	// success!
	return true;
}


/* -----------------------------------------------
	read/check header of compressed file
	----------------------------------------------- */
INTERN inline bool read_par_header( void )
{
	unsigned char nbyte[4] = { 0 };
	unsigned char hash = 0x00;
	unsigned char byte;
	int i;
	
	
	// read model_order, do hashing
	model_order = 0;
	if ( str_in->read( &model_order, 1, 1 ) != 1 ) return false;
	hash = pearson_hash[ model_order ^ hash ];
	
	// read number of bytes, do hashing
	if ( str_in->read( (void*) nbyte, 1, 4 ) != 4 ) return false;
	for ( i = 0; i < 4; i++ ) hash = pearson_hash[ nbyte[i] ^ hash ];
	
	// only check file name followed by 0x00, do hashing
	for ( i = 0; i < 256; i++ ) {
		if ( str_in->read( (void*) &byte, 1, 1 ) != 1 ) return false;
		hash = pearson_hash[ byte ^ hash ];
		if ( byte == 0x00 ) break;
	}
	if ( i == 256 ) return false;
	
	// check hash
	if ( str_in->read( (void*) &byte, 1, 1 ) != 1 ) return false;
	if ( hash != byte ) return false;
	
	// convert number of bytes from little endian
	uncfilesize  = 0;
	uncfilesize |= nbyte[0] << 24;
	uncfilesize |= nbyte[1] << 16;
	uncfilesize |= nbyte[2] <<  8;
	uncfilesize |= nbyte[3] <<  0;

	
	// success!
	return true;
}


/* -----------------------------------------------
	write header for compressed file
	----------------------------------------------- */
INTERN inline bool write_par_header( void )
{
	unsigned char nbyte[4] = { 0 };
	unsigned char hash = 0x00;
	unsigned char zero = 0x00;
	char* fn;
	int i;
	
	
	// convert number of bytes to little endian
	nbyte[0] = ( uncfilesize >> 24 ) & 0xFF;
	nbyte[1] = ( uncfilesize >> 16 ) & 0xFF;
	nbyte[2] = ( uncfilesize >>  8 ) & 0xFF;
	nbyte[3] = ( uncfilesize >>  0 ) & 0xFF;
	
	// write coding parameters, do hashing
	str_out->write( &model_order, 1, 1 );
	hash = pearson_hash[ model_order ^ hash ];
	
	// write number of bytes, do hashing
	str_out->write( (void*) nbyte, 1, 4 );
	for ( i = 0; i < 4; i++ ) hash = pearson_hash[ nbyte[i] ^ hash ];
	
	// write uncompressed file name, do hashing
	if ( store_name ) {
		// get filename without path
		if ( strrchr( uncfilename, '/' ) != NULL ) {
			fn = strrchr( uncfilename, '/' ) + 1;
			if ( strrchr( fn, '\\' ) != NULL )
				fn = strrchr( fn, '\\' ) + 1;
		} else if ( strrchr( uncfilename, '\\' ) != NULL )
			fn = strrchr( uncfilename, '\\' ) + 1;
		else fn = uncfilename;
		// store filename, do hashing
		str_out->write( (void*) fn, 1, strlen( fn ) + 1 );
		for ( ; (*fn) != 0x00; fn++ )
			hash = pearson_hash[ (*fn) ^ hash ];
	} else str_out->write( (void*) &zero, 1, 1 );
	hash = pearson_hash[ hash ];
	
	// write hash
	str_out->write( (void*) &hash, 1, 1 );
	
	
	// success!
	return true;
}


/* -----------------------------------------------
	encode or decode file
	----------------------------------------------- */
INTERN bool endecode_file( bool encoding ) {
	// this will either encode or decode one file
	// decision is based on the file type
	aricoder* coder;
	model_s* model;
	unsigned char lbyte[ MAX_ORDER + 1 ];
	unsigned char byte;
	int p, i;
		
	
	// --- preparations ---
	
	// init aricoder for encoding/decoding
	coder = ( encoding ) ?
		new aricoder( str_out, 1 ) : new aricoder( str_in, 0 );
	
	// init statistical model
	model = INIT_MODEL_S( 256, 256, model_order );
	
	// memset last byte
	memset( lbyte, 0, MAX_ORDER + 1 );
	
	
	// --- main encoding/decoding routine ---
	
	// main routine
	for ( p = 0; p < uncfilesize; p++ ) {
		// context shifting
		for ( i = model_order-1; i >= 0; i-- )
			model->shift_context( lbyte[i] );
			
		// actual encoding / decoding
		if ( encoding ) { // encoding / loop independent condition
			str_in->read( &byte, 1, 1 );		
			encode_ari( coder, model, byte );
		} else { // decoding
			byte = decode_ari( coder, model );
			str_out->write( &byte, 1, 1 );
		}
		
		// context update
		for ( i = 1; i < model_order; i++ )
			lbyte[i] = lbyte[i-1];
		lbyte[0] = byte;
	}
	
	
	// --- cleanup ---
	
	// free coder and models
	delete( coder );
	delete( model );
	
	
	return true;
}

/* -----------------------------------------------
	model_s cost count
	----------------------------------------------- */
INTERN inline double cost_count_ari( model_s* model, int c )
{
	static symbol s;
	static int esc;
	double cost = 0;
	
	do {		
		esc = model->convert_int_to_symbol( c, &s );
		cost -= log2( (double) (s.high_count - s.low_count) / (double) s.scale );
	} while ( esc );
	model->update_model( c );
	
	return cost;
}

/* ----------------------- End of other coding functions -------------------------- */

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
#endif

/* ----------------------- End of developers functions -------------------------- */

/* ----------------------- End of file -------------------------- */
