
// Switches class
class Switches {
  public:
    Switches();

    int compression_method;        //compression method to use (default: none)
    //byte positions to ignore (default: none)
    long long* ignore_list;
    int ignore_list_len;

    bool slow_mode;                //slow mode (default: off)
    bool fast_mode;                //fast mode (default: off)
    bool brute_mode;               //brute mode (default: off)
    bool pdf_bmp_mode;             //wrap BMP header around PDF images
                                   //  (default: off);
    bool prog_only;                //recompress progressive JPGs only
                                   //  (default: off);
    bool use_mjpeg;                //insert huffman table for MJPEG recompression
                                   //  (default: on);
    bool debug_mode;               //debug mode (default: off)

    unsigned int min_ident_size;   //minimal identical bytes (default: 4)

    //(p)recompression types to use (default: all)
    bool use_pdf;
    bool use_zip;
    bool use_gzip;
    bool use_png;
    bool use_gif;
    bool use_jpg;
    bool use_swf;
    bool use_base64;
    bool use_bzip2;

    bool level_switch;            //level switch used? (default: no)
    bool use_zlib_level[81];      //compression levels to use (default: all)
};

//Switches constructor
Switches::Switches() {
  compression_method = 0;
  ignore_list = NULL;
  ignore_list_len = 0;
  slow_mode = false;
  fast_mode = false;
  brute_mode = false;
  pdf_bmp_mode = false;
  prog_only = false;
  use_mjpeg = true;
  debug_mode = false;
  min_ident_size = 4;
  use_pdf = true;
  use_zip = true;
  use_gzip = true;
  use_png = true;
  use_gif = true;
  use_jpg = true;
  use_swf = true;
  use_base64 = true;
  use_bzip2 = true;
  level_switch = false;
  for (int i = 0; i < 81; i++) {
    use_zlib_level[i] = true;
  }
}

DLL void get_copyright_msg(char* msg);
DLL bool precompress_file(char* in_file, char* out_file, char* msg, Switches switches);
DLL bool recompress_file(char* in_file, char* out_file, char* msg, Switches switches);
