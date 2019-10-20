cmake_minimum_required(VERSION 3.2)

project(precomp)

set(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OBJDIR ${CMAKE_CURRENT_BINARY_DIR})

if (UNIX)
  set(CMAKE_C_FLAGS "-DLINUX -DUNIX -DHAVE_STDBOOL_H")
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_CXX_FLAGS "-DUNIX")
  set(CMAKE_CXX_STANDARD 11)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()


include_directories(AFTER ${SRCDIR})

function(add_stem2file VAR pattern stem)
    set(TMPV ${${VAR}})
    foreach(IND ${stem})
        string(REGEX REPLACE "^[ ]*([0-9a-zA-Z_%]+)[ ]*$" \\1 IND ${IND})
        string(REPLACE "%STEM%" ${IND} NEXT ${pattern})
        set(TMPV "${TMPV};${NEXT}")
    endforeach()
    set(${VAR} "${TMPV}" PARENT_SCOPE)
endfunction()

set(GIF_SRC "")
add_stem2file(GIF_SRC "${SRCDIR}/contrib/giflib/%STEM%.c" 
              "gifalloc;gif_err;dgif_lib_gcc;egif_lib_gcc")

set(BZIP_SRC "")
add_stem2file(BZIP_SRC "${SRCDIR}/contrib/bzip2/%STEM%.c"
              "bzlib;blocksort;crctable;compress;decompress;huffman;randtable")

set(ZLIB_SRC "")
add_stem2file(ZLIB_SRC "${SRCDIR}/contrib/zlib/%STEM%.c" 
              "adler32;crc32;zutil;trees;inftrees;inffast;inflate;deflate")

set(PACKARI_SRC "")
add_stem2file(PACKARI_SRC "${SRCDIR}/contrib/packjpg/%STEM%.cpp" 
              "aricoder;bitops")

set(PACKJPG_SRC "")
add_stem2file(PACKJPG_SRC "${SRCDIR}/contrib/packjpg/%STEM%.cpp" 
              "packjpg")
              
set(PACKMP3_SRC "")
add_stem2file(PACKMP3_SRC "${SRCDIR}/contrib/packmp3/%STEM%.cpp" 
              "huffmp3;packmp3")
add_definitions(-DBUILD_LIB)

set(LZMA_SRC "")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/common/%STEM%.c" 
              "tuklib_physmem;tuklib_cpucores;common;block_util;easy_preset;\
               filter_common;hardware_physmem;index;stream_flags_common;\
               vli_size;alone_encoder;block_buffer_encoder;block_encoder;\
               block_header_encoder;easy_buffer_encoder;easy_encoder;\
               easy_encoder_memusage;filter_buffer_encoder;filter_encoder;\
               filter_flags_encoder;index_encoder;stream_buffer_encoder;\
               stream_encoder;stream_flags_encoder;vli_encoder;\
               hardware_cputhreads;outqueue;stream_encoder_mt;alone_decoder;\
               auto_decoder;block_buffer_decoder;block_decoder;block_header_decoder;\
               easy_decoder_memusage;filter_buffer_decoder;filter_decoder;\
               filter_flags_decoder;index_decoder;index_hash;stream_decoder;\
               stream_buffer_decoder;stream_flags_decoder;vli_decoder")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/check/%STEM%.c" 
              "check;crc32_table;crc32_fast;crc64_table;crc64_fast;sha256")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/lz/%STEM%.c" 
              "lz_encoder;lz_encoder_mf;lz_decoder")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/lzma/%STEM%.c" 
              "lzma_encoder;lzma_encoder_presets;lzma_encoder_optimum_fast;\
               lzma_encoder_optimum_normal;fastpos_table;lzma_decoder;\
               lzma2_encoder;lzma2_decoder")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/rangecoder/%STEM%.c" 
              "price_table")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/delta/%STEM%.c" 
              "delta_common;delta_encoder;delta_decoder")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/simple/%STEM%.c" 
              "simple_coder;simple_encoder;simple_decoder;x86;powerpc;ia64;arm;armthumb;sparc")
add_stem2file(LZMA_SRC "${SRCDIR}/contrib/liblzma/%STEM%.cpp" 
              "compress_easy_mt")

include_directories(AFTER "${SRCDIR}" "${SRCDIR}/contrib/liblzma"
                          "${SRCDIR}/contrib/liblzma/api" "${SRCDIR}/contrib/liblzma/check"
                          "${SRCDIR}/contrib/liblzma/common"
                          "${SRCDIR}/contrib/liblzma/delta" "${SRCDIR}/contrib/liblzma/lz"
                          "${SRCDIR}/contrib/liblzma/lzma" "${SRCDIR}/contrib/liblzma/rangecoder"
                          "${SRCDIR}/contrib/liblzma/simple")
add_definitions(-DHAVE_BOOL)
if (MSVC)
  include_directories(AFTER "msinttypes")
  add_definitions(-D_UNICODE -DUNICODE)
endif (MSVC)

set(BRUNSLI_SRC "")
add_stem2file(BRUNSLI_SRC "${SRCDIR}/contrib/brunsli/c/common/%STEM%.cc"
              "ans_params;constants;context;huffman_tree;lehmer_code;platform;\
               predict;quant_matrix")
add_stem2file(BRUNSLI_SRC "${SRCDIR}/contrib/brunsli/c/dec/%STEM%.cc"
              "ans_decode;bit_reader;brunsli_decode;context_map_decode;histogram_decode;\
               huffman_decode;huffman_table;jpeg_data_writer")
add_stem2file(BRUNSLI_SRC "${SRCDIR}/contrib/brunsli/c/enc/%STEM%.cc"
              "ans_encode;brunsli_encode;context_map_encode;histogram_encode;jpeg_data_reader;\
               jpeg_huffman_decode;write_bits")
include_directories(AFTER "${SRCDIR}/contrib/brunsli/c/common"
                          "${SRCDIR}/contrib/brunsli/c/dec"
                          "${SRCDIR}/contrib/brunsli/c/enc"
                          "${SRCDIR}/contrib/brunsli/c/include")

set(BROTLI_SRC "")
add_stem2file(BROTLI_SRC "${SRCDIR}/contrib/brotli/c/common/%STEM%.c"
              "dictionary;transform")
add_stem2file(BROTLI_SRC "${SRCDIR}/contrib/brotli/c/dec/%STEM%.c"
              "bit_reader;decode;huffman;state")
add_stem2file(BROTLI_SRC "${SRCDIR}/contrib/brotli/c/enc/%STEM%.c"
              "backward_references;backward_references_hq;bit_cost;block_splitter;\
               brotli_bit_stream;cluster;compress_fragment;compress_fragment_two_pass;\
               dictionary_hash;encode;encoder_dict;entropy_encode;histogram;literal_cost;\
               memory;metablock;static_dict;utf8_util")
include_directories(AFTER "${SRCDIR}/contrib/brotli/c/include")

if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
  add_definitions(-DBIT64)
endif ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")


set(PREFLATE_SRC "")
add_stem2file(PREFLATE_SRC "${SRCDIR}/contrib/preflate/preflate_%STEM%.cpp" 
              "block_decoder;block_reencoder;block_trees;\
               complevel_estimator;constants;decoder;hash_chain;info;\
               parameter_estimator;parser_config;predictor_state;reencoder;\
               seq_chain;statistical_codec;statistical_model;token;\
               token_predictor;tree_predictor")
add_stem2file(PREFLATE_SRC "${SRCDIR}/contrib/preflate/support/%STEM%.cpp"
              "arithmetic_coder;array_helper;bit_helper;bitstream;const_division;\
               filestream;huffman_decoder;huffman_encoder;huffman_helper;memstream;\
               outputcachestream;task_pool")
include_directories(AFTER "${SRCDIR}/contrib/preflate")

set(PRECOMP_SRC "${SRCDIR}/precomp.cpp")

set(PRECOMP_HDR "${SRCDIR}/precomp.h")

add_executable(precomp ${GIF_SRC} ${BZIP_SRC} ${ZLIB_SRC} ${PACKARI_SRC}
                       ${PACKJPG_SRC} ${PACKMP3_SRC} ${LZMA_SRC} ${PREFLATE_SRC}
                       ${BRUNSLI_SRC} ${BROTLI_SRC} ${PRECOMP_SRC} ${PRECOMP_HDR})

if (UNIX)
  target_link_libraries(precomp Threads::Threads)
endif()

install(TARGETS precomp DESTINATION bin)