int def(FILE *source, FILE *dest, int level, int windowbits, int memlevel);
long long def_compare(FILE *compfile, int level, int windowbits, int memlevel, long long & decompressed_bytes_used, long long decompressed_bytes_total, bool in_memory);
long long def_compare_bzip2(FILE *source, FILE *compfile, int level, long long& decompressed_bytes_used);
int def_part(FILE *source, FILE *dest, int level, int windowbits, int memlevel, long long stream_size_in, long long stream_size_out);
int def_part_skip(FILE *source, FILE *dest, int level, int windowbits, int memlevel, long long stream_size_in, long long stream_size_out, int bmp_width);
int inf(FILE *source, int windowbits, long long& compressed_stream_size, long long& decompressed_stream_size, bool& in_memory);
void zerr(int ret);
#ifndef PRECOMPDLL
#ifndef COMFORT
int init(int argc, char* argv[]);
#else
int init_comfort(int argc, char* argv[]);
#endif
#endif
void denit_compress();
void denit_decompress();
void denit();
bool intense_mode_is_active();
bool brute_mode_is_active();
int inf_bzip2(FILE *source, FILE *dest, long long& compressed_stream_size, long long& decompressed_stream_size);
int def_bzip2(FILE *source, FILE *dest, int level);
long long file_recompress(FILE* origfile, int compression_level, int windowbits, int memlevel, long long& decompressed_bytes_used, long long decomp_bytes_total, bool in_memory);
long long file_recompress_bzip2(FILE* origfile, int level, long long& decompressed_bytes_used, long long& decompressed_bytes_total);
void write_decompressed_data(long long byte_count, char* decompressed_file_name = tempfile1);
void write_decompressed_data_io_buf(long long byte_count, bool in_memory, char* decompressed_file_name = tempfile1);
unsigned long long compare_files(FILE* file1, FILE* file2, unsigned int pos1, unsigned int pos2);
long long compare_file_mem_penalty(FILE* file1, unsigned char* input_bytes2, long long pos1, long long bytecount, long long& total_same_byte_count, long long& total_same_byte_count_penalty, long long& rek_same_byte_count, long long& rek_same_byte_count_penalty, long long& rek_penalty_bytes_len, long long& local_penalty_bytes_len, bool& use_penalty_bytes);
long long compare_files_penalty(FILE* file1, FILE* file2, long long pos1, long long pos2);
void start_uncompressed_data();
void end_uncompressed_data();
void try_decompression_pdf(int windowbits, int pdf_header_length, int img_width, int img_height, int img_bpc);
void try_decompression_zip(int zip_header_length);
void try_decompression_gzip(int gzip_header_length);
void try_decompression_png(int windowbits);
void try_decompression_png_multi(int windowbits);
void try_decompression_gif(unsigned char version[5]);
void try_decompression_jpg(long long jpg_length, bool progressive_jpg);
void try_decompression_mp3(long long mp3_length);
void try_decompression_zlib(int windowbits);
void try_decompression_brute();
void try_decompression_swf(int windowbits);
void try_decompression_bzip2(int compression_level);
void try_decompression_base64(int gzip_header_length);

// helpers for try_decompression functions

void init_decompression_variables();
unsigned char base64_char_decode(unsigned char c);
void base64_reencode(FILE* file_in, FILE* file_out, int line_count, unsigned int* base64_line_len, long long max_in_count = 0x7FFFFFFFFFFFFFFF, long long max_byte_count = 0x7FFFFFFFFFFFFFFF);

void packjpg_mp3_dll_msg();
bool is_valid_mp3_frame(unsigned char* frame_data, unsigned char header2, unsigned char header3, int protection);
inline unsigned short mp3_calc_layer3_crc(unsigned char header2, unsigned char header3, unsigned char* sideinfo, int sidesize);
bool recompress_gif(FILE* srcfile, FILE* dstfile, unsigned char block_size, GifCodeStruct* g, GifDiffStruct* gd);
bool decompress_gif(FILE* srcfile, FILE* dstfile, long long src_pos, int& gif_length, int& decomp_length, unsigned char& block_size, GifCodeStruct* g);
void sort_comp_mem_levels();
void show_used_levels();
bool compress_file(float min_percent = 0, float max_percent = 100);
void decompress_file();
void convert_file();
long long try_to_decompress(FILE* file, int windowbits, long long& compressed_stream_size, bool& in_memory);
long long try_to_decompress_bzip2(FILE* file, int compression_level, long long& compressed_stream_size);
void try_recompress(FILE* origfile, int comp_level, int mem_level, int windowbits, long long& compressed_stream_size, long long decomp_bytes_total, bool in_memory);
void try_recompress_bzip2(FILE* origfile, int level, long long& compressed_stream_size);
void write_header();
void read_header();
void convert_header();
void fast_copy(FILE* file1, FILE* file2, long long bytecount, bool update_progress = false);
void fast_copy(FILE* file, unsigned char* mem, long long bytecount);
void fast_copy(unsigned char* mem, FILE* file, long long bytecount);
size_t own_fwrite(const void *ptr, size_t size, size_t count, FILE* stream, bool final_byte = false, bool update_lzma_progress = false);
size_t own_fread(void *ptr, size_t size, size_t count, FILE* stream);
void seek_64(FILE* f, unsigned long long pos);
unsigned long long tell_64(FILE* f);
bool file_exists(char* filename);
#ifdef COMFORT
  bool check_for_pcf_file();
  void wait_for_key();
#endif
void error(int error_nr);
FILE* tryOpen(const char* filename, const char* mode);
long long fileSize64(char* filename);
void print64(long long i64);
void init_temp_files();
long long get_time_ms();
void printf_time(long long t);
char get_char_with_echo();
void safe_fclose(FILE** f);
void print_work_sign(bool with_backspace);
void print_debug_percent();
void show_progress(float percent, bool use_backspaces, bool check_time);
void ctrl_c_handler(int sig);

struct recursion_result {
  bool success;
  char* file_name;
  long long file_length;
  FILE* frecurse;
};

class zLibMTF{
  struct MTFItem{
    int Next, Previous;
  };
  alignas(16) MTFItem List[81];
  int Root, Index;
public:
  zLibMTF(): Root(0), Index(0) {
    for (int i=0;i<81;i++){
      List[i].Next = i+1;
      List[i].Previous = i-1;
    }
    List[80].Next = -1;
  }
  inline int First(){
    return Index=Root;
  }
  inline int Next(){
    return (Index>=0)?Index=List[Index].Next:Index;
  }
  inline void Update(){
    if (Index==Root) return;
    
    List[ List[Index].Previous ].Next = List[Index].Next;
    if (List[Index].Next>=0)
      List[ List[Index].Next ].Previous = List[Index].Previous;
    List[Root].Previous = Index;
    List[Index].Next = Root;
    List[Root=Index].Previous = -1;
  }
};

void write_ftempout_if_not_present(long long byte_count, bool in_memory, bool leave_open = false);
recursion_result recursion_compress(long long compressed_bytes, long long decompressed_bytes);
recursion_result recursion_decompress(long long recursion_data_length);

// compression-on-the-fly
enum {OTF_NONE = 0, OTF_BZIP2 = 1, OTF_XZ_MT = 2}; // uncompressed, bzip2, lzma2 multithreaded
void own_fputc(char c, FILE* f);
unsigned char fin_fgetc();
long long fin_fget64();
void fout_fputc(char c);
void fout_fput16(int v);
void fout_fput24(int v);
void fout_fput32_little_endian(int v);
void fout_fput32(int v);
void fout_fput32(unsigned int v);
void fout_fput64(long long v);
void fout_fput64(unsigned long long v);
void init_compress_otf();
void denit_compress_otf();
void init_decompress_otf();
void denit_decompress_otf();
int auto_detected_thread_count();
int lzma_max_memory_default();
