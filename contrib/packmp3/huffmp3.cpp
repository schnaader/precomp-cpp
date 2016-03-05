#include <stdlib.h>
#include <string.h>

#include "../packjpg/bitops.h"
#include "huffmp3.h"

// bit buffer size - careful changing this!
// should not be smaller than 8 or HWIDTH_MAX
#define BIT_BUFFER_SIZE	16


/* -----------------------------------------------
	small values quadruple conversion table (abs)
	----------------------------------------------- */
static const unsigned char sv_qconv_abs[ 16 ][ 4 ] = {
	{  0,  0,  0,  0 }, // 0000 -> 0/0/0/0
	{  0,  0,  0,  1 }, // 0001 -> 0/0/0/1
	{  0,  0,  1,  0 }, // 0010 -> 0/0/1/0
	{  0,  0,  1,  1 }, // 0011 -> 0/0/1/1
	{  0,  1,  0,  0 }, // 0100 -> 0/1/0/0
	{  0,  1,  0,  1 }, // 0101 -> 0/1/0/1
	{  0,  1,  1,  0 }, // 0110 -> 0/1/1/0
	{  0,  1,  1,  1 }, // 0111 -> 0/1/1/1
	{  1,  0,  0,  0 }, // 1000 -> 1/0/0/0
	{  1,  0,  0,  1 }, // 1001 -> 1/0/0/1
	{  1,  0,  1,  0 }, // 1010 -> 1/0/1/0
	{  1,  0,  1,  1 }, // 1011 -> 1/0/1/1
	{  1,  1,  0,  0 }, // 1100 -> 1/1/0/0
	{  1,  1,  0,  1 }, // 1101 -> 1/1/0/1
	{  1,  1,  1,  0 }, // 1110 -> 1/1/1/0
	{  1,  1,  1,  1 }, // 1111 -> 1/1/1/1
};


/* -----------------------------------------------
	constructor for huffman reader class
	----------------------------------------------- */
	
huffman_reader::huffman_reader( unsigned char* data, int size )
{
	// WARNING: NO error checks in this class!
	// init bitreader
	bit_reader = new abitreader( data, size );
	// fill the bit buffer for the first time
	bit_buffer = bit_reader->read( BIT_BUFFER_SIZE );
	// set count zero
	count = 0;
}


/* -----------------------------------------------
	destructor for huffman reader class
	----------------------------------------------- */
	
huffman_reader::~huffman_reader( void )
{
	// close bitreader
	delete( bit_reader );
}


/* -----------------------------------------------
	decode one big value pair
	----------------------------------------------- */
	
void huffman_reader::decode_pair( huffman_conv_set* table, unsigned char* vals )
{
	huffman_conv* conv;
	int ext;
	
	
	for ( ext = 0; ext != -1; ext = conv->ext ) {
		conv = (table+ext)->h + ( bit_buffer >> ( BIT_BUFFER_SIZE - (table+ext)->hwidth ) );
		advance_bitstream( conv->len );
	}
	
	vals[ 0 ] = conv->v0;
	vals[ 1 ] = conv->v1;
}


/* -----------------------------------------------
	decode one small value quadruple
	----------------------------------------------- */
	
void huffman_reader::decode_quadruple( huffman_conv_set* table, unsigned char* vals )
{
	huffman_conv* conv;
	
	conv = table->h + ( bit_buffer >> ( BIT_BUFFER_SIZE - table->hwidth ) );
	advance_bitstream( conv->len );
	
	memcpy( vals, sv_qconv_abs[ conv->v0 ], sizeof( char ) * 4 );
}


/* -----------------------------------------------
	bit reader function for n bit
	----------------------------------------------- */
	
unsigned int huffman_reader::read_bits( int n )
{
	unsigned int bits;
	
	// read bits
	bits = ( n <= BIT_BUFFER_SIZE ) ?
		bit_buffer >> (BIT_BUFFER_SIZE-n) :
		( bit_buffer << (n-BIT_BUFFER_SIZE) ) | bit_reader->read( (n-BIT_BUFFER_SIZE) );
	// refill the buffer
	advance_bitstream( n );
	
	return bits;
}


/* -----------------------------------------------
	bit reader function for one bit
	----------------------------------------------- */
	
unsigned char huffman_reader::read_bit( void )
{
	unsigned char bit;
	
	// read one bit
	bit = bit_buffer >> (BIT_BUFFER_SIZE-1);
	// refill the buffer
	advance_bitstream_1();
	
	return bit;
}


/* -----------------------------------------------
	reset the internal bit counter
	----------------------------------------------- */
	
void huffman_reader::reset_counter( void )
{
	count = 0;
}


/* -----------------------------------------------
	get the internal bit count
	----------------------------------------------- */
	
int huffman_reader::get_count( void )
{
	return count;
}


/* -----------------------------------------------
	rewind n bits
	----------------------------------------------- */
	
void huffman_reader::rewind_bits( int n )
{	
	// rewind n bits
	bit_reader->rewind_bits( n + BIT_BUFFER_SIZE );
	// fill the bit buffer
	bit_buffer = bit_reader->read( BIT_BUFFER_SIZE );
	// set back the counter
	count = ( count > n ) ? count - n : 0;
}


/* -----------------------------------------------
	set position in stream
	----------------------------------------------- */
	
void huffman_reader::setpos( int pbyte, int pbit )
{
	// reposition the bit reader
	bit_reader->setpos( pbyte, pbit );
	// fill the bit buffer
	bit_buffer = bit_reader->read( BIT_BUFFER_SIZE );
}


/* -----------------------------------------------
	return current byte position
	----------------------------------------------- */
	
int  huffman_reader::getpos( void )
{
	// ugly, but it works!
	return bit_reader->getpos() - ( BIT_BUFFER_SIZE + ( 8 - bit_reader->getbitp() ) ) / 8;
}


/* -----------------------------------------------
	refill bit buffer utility function
	----------------------------------------------- */
	
inline void huffman_reader::advance_bitstream( int n )
{
	// refill the buffer
	bit_buffer = ( n >= BIT_BUFFER_SIZE ) ?
		bit_reader->read( BIT_BUFFER_SIZE ) :
		( ( bit_buffer << n ) | bit_reader->read( n ) ) & ( ( 1 << BIT_BUFFER_SIZE ) - 1 );
	
	// record # of bits read
	count += n;
}


/* -----------------------------------------------
	refill bit buffer utility function (1 bit)
	----------------------------------------------- */
	
inline void huffman_reader::advance_bitstream_1( void )
{
	// refill the buffer
	bit_buffer = ( ( bit_buffer << 1 ) | bit_reader->read_bit() ) &
		( ( 1 << BIT_BUFFER_SIZE ) - 1 );
	
	// increment counter
	count++;
}


/* -----------------------------------------------
	constructor for huffman writer class
	----------------------------------------------- */
	
huffman_writer::huffman_writer( int adds )
{
	// WARNING: NO error checks in this class!
	// init bitwriter - recommended value: 5MB
	if ( adds == 0 ) adds = 5 * 1024 * 1024;
	bit_writer = new abitwriter( adds );
	// set count zero
	count = 0;
}


/* -----------------------------------------------
	destructor for huffman writer class
	----------------------------------------------- */
	
huffman_writer::~huffman_writer( void )
{
	// close bitwriter
	delete( bit_writer );
}


/* -----------------------------------------------
	encode one big value pair
	----------------------------------------------- */
	
void huffman_writer::encode_pair( huffman_code** hcodes, unsigned char* vals )
{
	huffman_code* hcode;	
	
	// find correct code, encode absvals and signs
	hcode = &hcodes[vals[0]][vals[1]];
	write_bits( hcode->code, hcode->len );
}


/* -----------------------------------------------
	encode one small value quadruple
	----------------------------------------------- */
	
void huffman_writer::encode_quadruple( huffman_code* hcode, unsigned char* vals )
{
	int bits = 0;
	int i;
	
	
	// build bits unit
	for ( i = 0; i < 4; i++ ) {
		bits = bits << 1;
		if ( vals[i] ) bits |= 1;
	}
	
	// encode vals
	hcode += bits;
	write_bits( hcode->code, hcode->len );
}


/* -----------------------------------------------
	bit writer function for n bit
	----------------------------------------------- */
	
void huffman_writer::write_bits( unsigned int bits, int n )
{
	// write bits
	bit_writer->write( bits, n );
	// take count
	count += n;
}


/* -----------------------------------------------
	bit writer function for 1 bit
	----------------------------------------------- */
	
void huffman_writer::write_bit( unsigned char bit )
{
	// write bit
	bit_writer->write_bit( bit );
	// increment counter
	count++;
}


/* -----------------------------------------------
	reset the internal bit counter
	----------------------------------------------- */
	
void huffman_writer::reset_counter( void )
{
	count = 0;
}


/* -----------------------------------------------
	get the internal bit count
	----------------------------------------------- */
	
int huffman_writer::get_count( void )
{
	return count;
}


/* -----------------------------------------------
	return data pointer (when finished)
	----------------------------------------------- */
	
unsigned char* huffman_writer::getptr( void )
{
	return bit_writer->getptr();
}


/* -----------------------------------------------
	return current byte position
	----------------------------------------------- */
	
int huffman_writer::getpos( void )
{
	return bit_writer->getpos();
}
