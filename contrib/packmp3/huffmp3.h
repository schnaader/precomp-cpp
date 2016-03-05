/* -----------------------------------------------
	struct declarations
	----------------------------------------------- */
	
struct huffman_code {
	unsigned char len;
	unsigned int code;
};

struct huffman_enc_table {
	huffman_code** h;
	int linbits;
	int max;
};

struct huffman_conv {
	unsigned char len;
	int v0;
	int v1;
	int ext;
};

struct huffman_conv_set {
	huffman_conv* h;
	int hwidth;
};

struct huffman_dec_table {
	huffman_conv_set* h;
	int linbits;
	int max;
};


/* -----------------------------------------------
	class for huffman decoding/data reading
	----------------------------------------------- */
	
class huffman_reader
{
	public:
	huffman_reader( unsigned char* data, int size );
	~huffman_reader( void );
	// public functions
	void decode_pair( huffman_conv_set* table, unsigned char* vals );
	void decode_quadruple( huffman_conv_set* table, unsigned char* vals );
	unsigned int read_bits( int n );
	// not the fastest method to read single bits/bytes
	// luckily, we won't need it that often
	unsigned char read_bit( void );
	void reset_counter( void );
	int get_count( void );
	void rewind_bits( int n );
	void setpos( int pbyte, int pbit );
	int getpos();
	
	private:
	// utility functions
	inline void advance_bitstream( int n );
	inline void advance_bitstream_1( void );
	
	// storage
	abitreader* bit_reader;
	unsigned int bit_buffer;
	int count;
};


/* -----------------------------------------------
	class for huffman encoding/data writer
	----------------------------------------------- */
	
class huffman_writer
{
	public:
	huffman_writer( int adds );
	~huffman_writer( void );
	// public functions
	void encode_pair( huffman_code** hcodes, unsigned char* vals );
	void encode_quadruple( huffman_code* hcode, unsigned char* vals );
	void write_bits( unsigned int bits, int n );
	void write_bit( unsigned char bit );
	void reset_counter( void );
	int get_count( void );
	unsigned char* getptr( void );
	int getpos();
	
	private:
	// storage
	abitwriter* bit_writer;
	int count;
};
