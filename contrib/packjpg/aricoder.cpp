#include <stdlib.h>
#include "bitops.h"
#include "aricoder.h"

#define ERROR_EXIT { error = true; exit( 0 ); }


/* -----------------------------------------------
	constructor for aricoder class
	----------------------------------------------- */

aricoder::aricoder( iostream* stream, int iomode )
{	
	// iomode (i/o mode)
	// 0 -> reading
	// 1 -> writing
	
	int i;
	
	// set initial values
	ccode	= 0;
	clow	= 0;
	chigh	= CODER_LIMIT100 - 1;
	cstep	= 0;
	bbyte	= 0;
	cbit	= 0;
	nrbits	= 0;
	
	// store pointer to iostream for reading/writing
	sptr = stream;
	
	// store i/o mode
	mode = iomode;
	
	if ( mode == 0 ) { // mode is reading / decoding
		// code buffer has to be filled before starting decoding
		for ( i = 0; i < CODER_USE_BITS; i++ )
			ccode = ( ccode << 1 ) | read_bit();
	} // mode is writing / encoding otherwise
}

/* -----------------------------------------------
	destructor for aricoder class
	----------------------------------------------- */

aricoder::~aricoder( void )
{
	if ( mode == 1 ) { // mode is writing / encoding
		// due to clow < CODER_LIMIT050, and chigh >= CODER_LIMIT050
		// there are only two possible cases
		if ( clow < CODER_LIMIT025 ) { // case a.) 
			write_bit( 0 );
			// write remaining bits
			write_bit( 1 );
			while ( nrbits-- > 0 )
				write_bit( 1 );
		}
		else { // case b.), clow >= CODER_LIMIT025
			write_bit( 1 );
		} // done, zeroes are auto-read by the decoder
		
		// pad code with zeroes
		while ( cbit > 0 ) write_bit( 0 );
	}
}

/* -----------------------------------------------
	arithmetic encoder function
	----------------------------------------------- */
	
void aricoder::encode( symbol* s )
{	
	// update steps, low count, high count
	cstep = ( ( chigh - clow ) + 1 ) / s->scale;
	chigh = clow + ( cstep * s->high_count ) - 1;
	clow  = clow + ( cstep * s->low_count );
	
	// e3 scaling is performed for speed and to avoid underflows
	// if both, low and high are either in the lower half or in the higher half
	// one bit can be safely shifted out
	while ( ( clow >= CODER_LIMIT050 ) || ( chigh < CODER_LIMIT050 ) ) {		
		if ( chigh < CODER_LIMIT050 ) {	// this means both, high and low are below, and 0 can be safely shifted out
			// write 0 bit
			write_bit( 0 );
			// shift out remaing e3 bits
			for ( ; nrbits > 0; nrbits-- )
				write_bit( 1 );
		}
		else { // if the first wasn't the case, it's clow >= CODER_LIMIT050
			// write 1 bit
			write_bit( 1 );
			clow  &= CODER_LIMIT050 - 1;
			chigh &= CODER_LIMIT050 - 1;
			// shift out remaing e3 bits
			for ( ; nrbits > 0; nrbits-- )
				write_bit( 0 );
		}
		clow  <<= 1;
		chigh <<= 1;
		chigh++;
	}
	
	// e3 scaling, to make sure that theres enough space between low and high
	while ( ( clow >= CODER_LIMIT025 ) && ( chigh < CODER_LIMIT075 ) ) {
		nrbits++;
		clow  &= CODER_LIMIT025 - 1;
		chigh ^= CODER_LIMIT025 + CODER_LIMIT050;
		// clow  -= CODER_LIMIT025;
		// chigh -= CODER_LIMIT025;
		clow  <<= 1;
		chigh <<= 1;
		chigh++;
	}
}

/* -----------------------------------------------
	arithmetic decoder get count function
	----------------------------------------------- */
	
unsigned int aricoder::decode_count( symbol* s )
{
	// update cstep, which is needed to remove the symbol from the stream later
	cstep = ( ( chigh - clow ) + 1 ) / s->scale;
	
	// return counts, needed to decode the symbol from the statistical model
	return ( ccode - clow ) / cstep;
}

/* -----------------------------------------------
	arithmetic decoder function
	----------------------------------------------- */
	
void aricoder::decode( symbol* s )
{
	// no actual decoding takes place, as this has to happen in the statistical model
	// the symbol has to be removed from the stream, though
	
	// alread have steps updated from decoder_count
	// update low count and high count
	chigh = clow + ( cstep * s->high_count ) - 1;
	clow  = clow + ( cstep * s->low_count );
	
	// e3 scaling is performed for speed and to avoid underflows
	// if both, low and high are either in the lower half or in the higher half
	// one bit can be safely shifted out
	while ( ( clow >= CODER_LIMIT050 ) || ( chigh < CODER_LIMIT050 ) ) {
		if ( clow >= CODER_LIMIT050 ) {
			clow  &= CODER_LIMIT050 - 1;
			chigh &= CODER_LIMIT050 - 1;
			ccode &= CODER_LIMIT050 - 1;
		} // if the first wasn't the case, it's chigh < CODER_LIMIT050
		clow  <<= 1;
		chigh <<= 1;
		chigh++;
		ccode <<= 1;
		ccode |= read_bit();
		nrbits = 0;
	}
	
	// e3 scaling, to make sure that theres enough space between low and high
	while ( ( clow >= CODER_LIMIT025 ) && ( chigh < CODER_LIMIT075 ) ) {
		nrbits++;
		clow  &= CODER_LIMIT025 - 1;
		chigh ^= CODER_LIMIT025 + CODER_LIMIT050;
		// clow  -= CODER_LIMIT025;
		// chigh -= CODER_LIMIT025;
		ccode -= CODER_LIMIT025;
		clow  <<= 1;
		chigh <<= 1;
		chigh++;
		ccode <<= 1;
		ccode |= read_bit();
	}	
}

/* -----------------------------------------------
	bit writer function
	----------------------------------------------- */
	
void aricoder::write_bit( unsigned char bit )
{
	// add bit at last position
	bbyte = ( bbyte << 1 ) | bit;
	// increment bit position
	cbit++;
	
	// write bit if done
	if ( cbit == 8 ) {
		sptr->write( (void*) &bbyte, 1, 1 );
		cbit = 0;
	}
}

/* -----------------------------------------------
	bit reader function
	----------------------------------------------- */
	
unsigned char aricoder::read_bit( void )
{
	// read in new byte if needed
	if ( cbit == 0 ) {
		if ( sptr->read( &bbyte, 1, 1 ) == 0 ) // read next byte if available
			bbyte = 0; // if no more data is left in the stream
		cbit = 8;
	}
	
	// decrement current bit position
	cbit--;	
	// return bit at cbit position
	return BITN( bbyte, cbit );
}


/* -----------------------------------------------
	universal statistical model for arithmetic coding
	----------------------------------------------- */
	
model_s::model_s( int max_s, int max_c, int max_o, int c_lim )
{
	// boundaries of this model:
	// max_s (maximum symbol) -> 1 <= max_s <= 1024 (???)
	// max_c (maximum context) -> 1 <= max_c <= 1024 (???)
	// max_o (maximum order) -> -1 <= max_o <= 4
	// c_lim (maximum count) -> 2 <= c_lim <= 4096 (???)
	// WARNING: this can be memory intensive, so don't overdo it
	// max_s == 256; max_c == 256; max_o == 4 would be way too much

	table_s* null_table;
	table_s* start_table;
	int i;
	
	
	// set error false
	error = false;	
	
	// copy settings into model
	max_symbol  = max_s;
	max_context = max_c;
	max_order   = max_o;
	max_count   = c_lim;
	
	
	// alloc memory for totals table
	// totals = ( unsigned short* ) calloc( max_symbol + 2, sizeof( short ) );
	totals = ( unsigned int* ) calloc( max_symbol + 2, sizeof( int ) );
	
	// alloc memory for scoreboard, set sb0_count
	scoreboard = ( char* ) calloc( max_symbol, sizeof( char ) );
	sb0_count = max_symbol;
	
	// set current order
	current_order = max_order;
	
	
	// set up null table
	null_table = ( table_s* ) calloc( 1, sizeof( table_s ) );
	if ( null_table == NULL ) ERROR_EXIT;	
	null_table->counts = ( unsigned short* ) calloc( max_symbol, sizeof( short ) );
	if ( null_table->counts == NULL ) ERROR_EXIT;
	for ( i = 0; i < max_symbol; i++ )
		null_table->counts[ i ] = 1; // set all probabilities
	// set up internal counts
	null_table->max_count = 1;
	null_table->max_symbol = max_symbol;
	
	// set up start table
	start_table = ( table_s* ) calloc( 1, sizeof( table_s ) );
    if ( start_table == NULL ) ERROR_EXIT;	
	start_table->links = ( table_s** ) calloc( max_context, sizeof( table_s* ) );
	if ( start_table->links == NULL ) ERROR_EXIT;
	// set up internal counts
	start_table->max_count = 0;
	start_table->max_symbol = 0;
	
	// build links for start table & null table
	start_table->lesser = null_table;
	null_table->links = ( table_s** ) calloc( max_context, sizeof( table_s* ) );
	if ( null_table->links == NULL ) ERROR_EXIT;
	for ( i = 0; i < max_context; i++ )
		null_table->links[ i ] = start_table;
	
	// alloc memory for storage & contexts
	storage = ( table_s** ) calloc( max_order + 3, sizeof( table_s* ) );
	if ( storage == NULL ) ERROR_EXIT;
	contexts = storage + 1;
	
	// integrate tables into contexts
	contexts[ -1 ] = null_table;
	contexts[  0 ] = start_table;
	
	// build initial 'normal' tables
	for ( i = 1; i <= max_order; i++ ) {
		// set up current order table
		contexts[ i ] = ( table_s* ) calloc( 1, sizeof( table_s ) );
	    if ( contexts[ i ] == NULL ) ERROR_EXIT;
		contexts[ i ]->max_count  = 0;
		contexts[ i ]->max_symbol = 0;
		// build forward and backward links
		contexts[ i ]->lesser = contexts[ i - 1 ];
		if ( i < max_order ) {
			contexts[ i ]->links = ( table_s** ) calloc( max_context, sizeof( table_s* ) );
			if ( contexts[ i ]->links == NULL ) ERROR_EXIT;
		}
		else {
			contexts[ i ]->links = NULL;
		}
		contexts[ i - 1 ]->links[ 0 ] = contexts[ i ];
	}
}


/* -----------------------------------------------
	model class destructor - recursive cleanup of memory is done here
	----------------------------------------------- */

model_s::~model_s( void )
{
	table_s* context;
	
	
	// clean up each 'normal' table
	context = contexts[ 0 ];
	recursive_cleanup ( context );
	
	// clean up null table
	context = contexts[ -1 ];	
	if ( context->links  != NULL )
		free( context->links  );
	if ( context->counts != NULL ) free( context->counts );
	free ( context );
	
	// free everything else
	free( storage );
	free( totals );
	free( scoreboard );
}


/* -----------------------------------------------
	updates statistics for a specific symbol / resets to highest order
	----------------------------------------------- */

void model_s::update_model( int symbol )
{
	// use -1 if you just want to reset without updating statistics
	
	table_s* context;
	unsigned short* counts;
	int local_order;
	int i;
	
	
	// only contexts, that were actually used to encode
	// the symbol get their counts updated
	if ( symbol >= 0 ) {
		for ( local_order = ( current_order < 0 ) ? 0 : current_order;
				local_order <= max_order; local_order++ ) {
			context = contexts[ local_order ];
			counts = context->counts + symbol;
			// update count for specific symbol & scale
			(*counts)++;
			// store side information for totalize_table
			if ( (*counts) > context->max_count ) context->max_count = (*counts);
			if ( symbol >= context->max_symbol ) context->max_symbol = symbol+1;
			// if counts for that symbol have gone above the maximum count
			// the table has to be resized (scale factor 2)
			if ( (*counts) >= max_count )
				rescale_table( context, 1 );
		}
	}
	
	// reset scoreboard and current order
	current_order = max_order;
	for ( i = 0; i < max_symbol; i++ )
		scoreboard[ i ] = 0;
	sb0_count = max_symbol;
}


/* -----------------------------------------------
	shift in one context (max no of contexts is max_c)
	----------------------------------------------- */
	
void model_s::shift_context( int c )
{
	table_s* context;
	int i;
	
	// shifting is not possible if max_order is below 1
	// or context index is negative
	if ( ( max_order < 1 ) || ( c < 0 ) ) return;
	
	// shift each orders' context
	for ( i = max_order; i > 0; i-- ) {
		// this is the new current order context
		context = contexts[ i - 1 ]->links[ c ];
		
		// check if context exists, build if needed
		if ( context == NULL ) {
			// reserve memory for next table_s
			context = ( table_s* ) calloc( 1, sizeof( table_s ) );
			if ( context == NULL ) ERROR_EXIT;			
			// set counts NULL
			context->counts = NULL;
			// setup internal counts
			context->max_count  = 0;
			context->max_symbol = 0;
			// link lesser context later if not existing, this is done below
			context->lesser = contexts[ i - 2 ]->links[ c ];
			// finished here if this is a max order context
			if ( i == max_order )
				context->links = NULL;
			else {
				// build links to higher order tables otherwise
				context->links = ( table_s** ) calloc( max_context, sizeof( table_s* ) );
				if ( context->links == NULL ) ERROR_EXIT;
				// add lesser link for higher context (see above)
				contexts[ i + 1 ]->lesser = context;
			}
			// put context to its right place
			contexts[ i - 1 ]->links[ c ] = context;
		}
		
		// switch context
		contexts[ i ] = context;
	}
}


/* -----------------------------------------------
	flushes the whole model by diviging through a specific scale factor
	----------------------------------------------- */
	
void model_s::flush_model( int scale_factor )
{
	recursive_flush( contexts[ 0 ], scale_factor );
}


/* -----------------------------------------------
	exclude specific symbols using this function
	----------------------------------------------- */
	
void model_s::exclude_symbols( char rule, int c )
{
	// exclusions are back to normal after update_model is used	
	// modify scoreboard according to rule and value
	switch ( rule )
	{
		case 'a':
			// above rule
			// every symbol above c is excluded
			for ( c = c + 1; c < max_symbol; c++ ) {
				if ( scoreboard[ c ] == 0 ) {
					scoreboard[ c ] = 1;
					sb0_count--;
				}
			}
			break;
		
		case 'b':
			// below rule
			// every symbol below c is excluded
			for ( c = c - 1; c >= 0; c-- ) {
				if ( scoreboard[ c ] == 0 ) {
					scoreboard[ c ] = 1;
					sb0_count--;
				}
			}
			break;
		
		case 'e':
			// equal rule
			// only c is excluded
			if ( scoreboard[ c ] == 0 ) {
				scoreboard[ c ] = 1;
				sb0_count--;
			}
			break;
		
		default:
			// unknown rule
			// do nothing
			break;
	}
}


/* -----------------------------------------------
	converts an int to a symbol, needed only when encoding
	----------------------------------------------- */
	
int model_s::convert_int_to_symbol( int c, symbol *s )
{
	// search the symbol c in the current context table_s,
	// return scale, low- and high counts
	
	table_s* context;
	

	// totalize table for the current context
	context = contexts[ current_order ];
	totalize_table( context );
	
	// finding the scale is easy
	s->scale = totals[ 0 ];
	
	// check if that symbol exists in the current table. send escape otherwise
	if ( c >= 0 ) {
		if ( context->counts[ c ] > 0 ) {
			// return high and low count for the current symbol
			s->low_count  = totals[ c + 2 ];
			s->high_count = totals[ c + 1 ];
			return 0;
		}
	}
	
	// return high and low count for the escape symbol
	s->low_count  = totals[ 1 ];
	s->high_count = totals[ 0 ];
	current_order--;
	return 1;
}


/* -----------------------------------------------
	returns the current context scale needed only when decoding
	----------------------------------------------- */
	
void model_s::get_symbol_scale( symbol *s )
{
	// getting the scale is easy: totalize the table_s, use accumulated count -> done
	totalize_table( contexts[ current_order ] );
	s->scale = totals[ 0 ];
}


/* -----------------------------------------------
	converts a count to an int, called after get_symbol_scale
	----------------------------------------------- */
	
int model_s::convert_symbol_to_int( int count, symbol *s )
{
	// seek the symbol that matches the count,
	// also, set low- and high count for the symbol - it has to be removed from the stream
	
	int c;
	
	// go through the totals table, search the symbol that matches the count
	for ( c = 1; count < (signed) totals[ c ]; c++ );	
	// set up the current symbol
	s->low_count  = totals[ c ];
	s->high_count = totals[ c - 1 ];
	// send escape if escape symbol encountered
	if ( c == 1 ) {
		current_order--;
		return ESCAPE_SYMBOL;
	}
	
	// return symbol value
	return ( c - 2 );
}


/* -----------------------------------------------
	totals are calculated by accumulating counts in the current table_s
	----------------------------------------------- */

void model_s::totalize_table( table_s *context )
{
	// update exclusion is used, so this has to be done each time
	// escape probability calculation also takes place here
	
	// accumulated counts must never exceed CODER_MAXSCALE
	// as CODER_MAXSCALE is big enough, though, (2^29), this shouldn't happen and is not checked

	unsigned short* counts;
	signed int      local_symb;
	unsigned int    curr_total;
	unsigned int    curr_count;
	unsigned int    esc_prob;
	int i;
	
	// make a local copy of the pointer
	counts = context->counts;
	
	// check counts
	if ( counts != NULL ) {	// if counts are already set
		// locally store current fill/symbol count
		local_symb = sb0_count;
		
		// set the last symbol of the totals table_s zero
		i = context->max_symbol - 1;
		totals[ i + 2 ]	= 0;
		// (re)set current total
		curr_total = 0;
		
		// go reverse though the whole counts table and accumulate counts
		// leave space at the beginning of the table for the escape symbol
		for ( ; i >= 0; i-- ) {			
			// only count probability if the current symbol is not 'scoreboard - excluded'
			if ( scoreboard[ i ] == 0 ) {
				curr_count = counts[ i ];
				if ( curr_count > 0 ) {
					// add counts for the current symbol
					curr_total = curr_total + curr_count;
					// exclude symbol from scoreboard
					scoreboard[ i ] = 1;
					sb0_count--;
				}
			}
			totals[ i + 1 ] = curr_total;
		}		
		// here the escape calculation needs to take place
		if ( local_symb == sb0_count )
			esc_prob = 1;
		else if ( sb0_count == 0 )
			esc_prob = 0;
		else {
			// esc_prob = 1;
			esc_prob  =  sb0_count * ( local_symb - sb0_count );
			esc_prob /= ( local_symb * context->max_count );
			esc_prob++;
		}
		// include escape probability in totals table
		totals[ 0 ] = totals[ 1 ] + esc_prob;
	}
	else { // if counts are not already set
		// setup counts for current table
		context->counts = ( unsigned short* ) calloc( max_symbol, sizeof( short ) );
		if ( context->counts == NULL ) ERROR_EXIT;
		// set totals table -> only escape probability included
		totals[ 0 ] = 1;
		totals[ 1 ] = 0;
	}	
}


/* -----------------------------------------------
	resizes one table by bitshifting each count using a specific value
	----------------------------------------------- */
	
inline void model_s::rescale_table( table_s* context, int scale_factor )
{
	unsigned short* counts = context->counts;
	int lst_symbol = context->max_symbol;
	int i;
	
	// return now if counts not set
	if ( counts == NULL ) return;
	
	// now scale the table by bitshifting each count
	for ( i = 0; i < lst_symbol; i++ ) {
		if ( counts[ i ] > 0 )
			counts[ i ] >>= scale_factor;
	}
		
	// also rescale tables max count
	context->max_count >>= scale_factor;
	
	// seek for new last symbol
	for ( i = lst_symbol - 1; i >= 0; i-- )
		if ( counts[ i ] > 0 ) break;
	context->max_symbol = i + 1;
}


/* -----------------------------------------------
	a recursive function to go through each context and rescale the counts
	----------------------------------------------- */
	
inline void model_s::recursive_flush( table_s* context, int scale_factor )
{
	int i;

	// go through each link != NULL
	if ( context->links != NULL )
		for ( i = 0; i < max_context; i++ )
			if ( context->links[ i ] != NULL )
				recursive_flush( context->links[ i ], scale_factor );
    
	// rescale specific table
	rescale_table( context, scale_factor );
}


/* -----------------------------------------------
	frees all memory for all contexts starting at a given table_s
	----------------------------------------------- */

inline void model_s::recursive_cleanup( table_s *context )
{
	// be careful not to cut any link too early!
	
	int i;

	// go through each link != NULL
	if ( context->links != NULL ) {
		for ( i = 0; i < max_context; i++ )
			if ( context->links[ i ] != NULL )
				recursive_cleanup( context->links[ i ] );
		free ( context->links );
	}
	
	// clean up table	
	if ( context->counts != NULL ) free ( context->counts );	
	free( context );
}


/* -----------------------------------------------
	special version of model_s for binary coding
	----------------------------------------------- */

model_b::model_b( int max_c, int max_o, int c_lim )
{
	// boundaries of this model:
	// ... (maximum symbol) -> 2 (0 or 1 )
	// max_c (maximum context) -> 1 <= max_c <= 1024 (???)
	// max_o (maximum order) -> -1 <= max_o <= 4

	table* null_table;
	table* start_table;
	int i;
	
	
	// set error false
	error = false;	
	
	// copy settings into model
	max_context = max_c;
	max_order   = max_o;
	max_count   = c_lim;
	
	
	// set up null table
	null_table = ( table* ) calloc( 1, sizeof( table ) );
	if ( null_table == NULL ) ERROR_EXIT;
	
	null_table->counts = ( unsigned short* ) calloc( 2, sizeof( short ) );
	if ( null_table->counts == NULL ) ERROR_EXIT;
	null_table->counts[ 0 ] = 1;
	null_table->counts[ 1 ] = 1;
	null_table->scale = 2;
	
	// set up start table
	start_table = ( table* ) calloc( 1, sizeof( table ) );
    if ( start_table == NULL ) ERROR_EXIT;	
	start_table->links = ( table** ) calloc( max_context, sizeof( table* ) );
	if ( start_table->links == NULL ) ERROR_EXIT;
	start_table->scale = 0;
	
	// build links for start table & null table
	start_table->lesser = null_table;
	null_table->links = ( table** ) calloc( max_context, sizeof( table* ) );
	if ( null_table->links == NULL ) ERROR_EXIT;
	for ( i = 0; i < max_context; i++ )
		null_table->links[ i ] = start_table;
	
	// alloc memory for storage & contexts
	storage = ( table** ) calloc( max_order + 3, sizeof( table* ) );
	if ( storage == NULL ) ERROR_EXIT;
	contexts = storage + 1;
	
	// integrate tables into contexts
	contexts[ -1 ] = null_table;
	contexts[  0 ] = start_table;
	
	// build initial 'normal' tables
	for ( i = 1; i <= max_order; i++ ) {
		// set up current order table
		contexts[ i ] = ( table* ) calloc( 1, sizeof( table ) );
	    if ( contexts[ i ] == NULL ) ERROR_EXIT;
		contexts[ i ]->scale = 0;
		// build forward and backward links
		contexts[ i ]->lesser = contexts[ i - 1 ];
		if ( i < max_order ) {
			contexts[ i ]->links = ( table** ) calloc( max_context, sizeof( table* ) );
			if ( contexts[ i ]->links == NULL ) ERROR_EXIT;
		}
		else {
			contexts[ i ]->links = NULL;
		}
		contexts[ i - 1 ]->links[ 0 ] = contexts[ i ];
	}
}


/* -----------------------------------------------
	model class destructor - recursive cleanup of memory is done here
	----------------------------------------------- */
	
model_b::~model_b( void )
{
	table* context;
	
	
	// clean up each 'normal' table
	context = contexts[ 0 ];
	recursive_cleanup ( context );
	
	// clean up null table
	context = contexts[ -1 ];	
	if ( context->links  != NULL )
		free( context->links  );
	if ( context->counts != NULL ) free( context->counts );
	free ( context );
	
	// free everything else
	free( storage );
}


/* -----------------------------------------------
	updates statistics for a specific symbol / resets to highest order
	----------------------------------------------- */
	
void model_b::update_model( int symbol )
{
	// use -1 if you just want to reset without updating statistics
	
	table* context = contexts[ max_order ];
	
	// only contexts, that were actually used to encode
	// the symbol get their counts updated
	if ( ( symbol >= 0 ) && ( max_order >= 0 ) ) {
		// update count for specific symbol & scale
		context->counts[ symbol ]++;
		context->scale++;
		// if counts for that symbol have gone above the maximum count
		// the table has to be resized (scale factor 2)
		if ( context->counts[ symbol ] >= max_count )
			rescale_table( context, 1 );
	}
}


/* -----------------------------------------------
	shift in one context (max no of contexts is max_c)
	----------------------------------------------- */
	
void model_b::shift_context( int c )
{
	table* context;
	int i;
	
	// shifting is not possible if max_order is below 1
	// or context index is negative
	if ( ( max_order < 1 ) || ( c < 0 ) ) return;
	
	// shift each orders' context
	for ( i = max_order; i > 0; i-- ) {
		// this is the new current order context
		context = contexts[ i - 1 ]->links[ c ];
		
		// check if context exists, build if needed
		if ( context == NULL ) {
			// reserve memory for next table
			context = ( table* ) calloc( 1, sizeof( table ) );
			if ( context == NULL ) ERROR_EXIT;			
			// set internal counts NULL
			context->counts = NULL;
			context->scale  = 0;	
			// link lesser context later if not existing, this is done below
			context->lesser = contexts[ i - 2 ]->links[ c ];
			// finished here if this is a max order context
			if ( i == max_order ) {
				context->links = NULL;
			}
			else {
				// build links to higher order tables otherwise
				context->links = ( table** ) calloc( max_context, sizeof( table* ) );
				if ( context->links == NULL ) ERROR_EXIT;
				// add lesser link for higher context (see above)
				contexts[ i + 1 ]->lesser = context;
			}
			// put context to its right place
			contexts[ i - 1 ]->links[ c ] = context;
		}
		
		// switch context
		contexts[ i ] = context;
	}
}


/* -----------------------------------------------
	flushes the whole model by dividing through a specific scale factor
	----------------------------------------------- */
	
void model_b::flush_model( int scale_factor )
{
	recursive_flush( contexts[ 0 ], scale_factor );
}


/* -----------------------------------------------
	converts an int to a symbol, needed only when encoding
	----------------------------------------------- */
	
int model_b::convert_int_to_symbol( int c, symbol *s )
{
	table* context = contexts[ max_order ];
	
	// check if counts are available
	check_counts( context );
	
	// finding the scale is easy
	s->scale = context->scale;
	
	// return high and low count for current symbol
	if ( c == 0 ) { // if 0 is to be encoded
		s->low_count  = 0;
		s->high_count = context->counts[ 0 ];
	}
	else { // if 1 is to be encoded
		s->low_count  = context->counts[ 0 ];
		s->high_count = context->scale;
	}
	
	return 1;
}


/* -----------------------------------------------
	returns the current context scale needed only when decoding
	----------------------------------------------- */
	
void model_b::get_symbol_scale( symbol *s )
{
	table* context = contexts[ max_order ];
	
	// check if counts are available
	check_counts( context );
	
	// getting the scale is easy
	s->scale = context->scale;
}


/* -----------------------------------------------
	converts a count to an int, called after get_symbol_scale
	----------------------------------------------- */
	
int model_b::convert_symbol_to_int( int count, symbol *s )
{
	table* context = contexts[ max_order ];
	unsigned short counts0 = context->counts[ 0 ];
	
	// set up the current symbol
	if ( count < counts0 ) {
		s->low_count  = 0;
		s->high_count = counts0;
		return 0;
	}
	else {
		s->low_count  = counts0;
		s->high_count = s->scale;
		return 1;
	}
}


/* -----------------------------------------------
	this function checks if counts exist, and, if they exist and are below max
	----------------------------------------------- */
	
inline void model_b::check_counts( table *context )
{
	unsigned short* counts = context->counts;
	
	// check if counts are available
	if ( counts == NULL ) {
		// setup counts for current table
		counts = ( unsigned short* ) calloc( 2, sizeof( short ) );
		if ( counts == NULL ) ERROR_EXIT;
		counts[ 0 ] = 1;
		counts[ 1 ] = 1;
		// set scale
		context->counts = counts;
		context->scale = 2;
	}
}


/* -----------------------------------------------
	resizes one table by bitshifting each count using a specific value
	----------------------------------------------- */
	
inline void model_b::rescale_table( table* context, int scale_factor )
{
	unsigned short* counts = context->counts;
	
	// return now if counts not set
	if ( counts == NULL ) return;
	
	// now scale the table by bitshifting each count, be careful not to set any count zero
	counts[ 0 ] >>= scale_factor;
	counts[ 1 ] >>= scale_factor;
	if ( counts[ 0 ] == 0 ) counts[ 0 ] = 1;
	if ( counts[ 1 ] == 0 ) counts[ 1 ] = 1;
	context->scale = counts[ 0 ] + counts[ 1 ];
}


/* -----------------------------------------------
	a recursive function to go through each context and rescale the counts
	----------------------------------------------- */
	
inline void model_b::recursive_flush( table* context, int scale_factor )
{
	int i;

	// go through each link != NULL
	if ( context->links != NULL )
		for ( i = 0; i < max_context; i++ )
			if ( context->links[ i ] != NULL )
				recursive_flush( context->links[ i ], scale_factor );
    
	// rescale specific table
	rescale_table( context, scale_factor );
}


/* -----------------------------------------------
	frees all memory for all contexts starting at a given table
	----------------------------------------------- */
	
inline void model_b::recursive_cleanup( table *context )
{
	int i;

	// go through each link != NULL
	if ( context->links != NULL ) {
		for ( i = 0; i < max_context; i++ )
			if ( context->links[ i ] != NULL )
				recursive_cleanup( context->links[ i ] );
		free ( context->links );
	}
	
	// clean up table	
	if ( context->counts != NULL ) free ( context->counts );	
	free( context );
}
