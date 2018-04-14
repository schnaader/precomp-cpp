CC            = g++
COPT          = -x c -std=c99 -O3 -funroll-loops -ffast-math -fomit-frame-pointer -DZ_SOLO -DNO_GZIP
CCOPT         = -std=c++11 -O3 -Wall -Wno-misleading-indentation -pedantic -funroll-loops -ffast-math -fomit-frame-pointer
DEMONAME      = preflate_demo.exe
OBJDIR        = objs
BINDIR        = bin

PREFLATE_LIB_FILEROOTS = block_decoder block_reencoder block_trees complevel_estimator \
                         constants decoder hash_chain info parameter_estimator parser_config \
                         predictor_state reencoder statistical_codec statistical_model \
                         token_predictor token tree_predictor
SUPPORT_LIB_FILEROOTS = arithmetic_coder array_helper bit_helper bitstream const_division \
                        filestream huffman_decoder huffman_encoder huffman_helper \
                        memstream outputcachestream support_tests task_pool
PACKARI_FILEROOTS = aricoder bitops
PREFLATE_DEMO_FILEROOTS = main preflate_checker preflate_dumper preflate_unpack
ZLIB_FILEROOTS = adler32 inffast inflate inftrees trees zutil
DEMO_FILEROOTS = main checker dumper unpack

PREFLATE_LIB_SRC_FILES = $(addprefix preflate_, $(addsuffix .cpp, $(PREFLATE_LIB_FILEROOTS)))
PREFLATE_LIB_OBJ_FILES = $(addprefix $(OBJDIR)/preflate_, $(addsuffix .o, $(PREFLATE_LIB_FILEROOTS)))
SUPPORT_LIB_SRC_FILES  = $(addprefix support/, $(addsuffix .cpp, $(SUPPORT_LIB_FILEROOTS)))
SUPPORT_LIB_OBJ_FILES  = $(addprefix $(OBJDIR)/prefsup_, $(addsuffix .o, $(SUPPORT_LIB_FILEROOTS)))
PACKARI_SRC_FILES      = $(addprefix packARI/, $(addsuffix .cpp, $(PACKARI_FILEROOTS)))
PACKARI_OBJ_FILES      = $(addprefix $(OBJDIR)/packari_, $(addsuffix .o, $(PACKARI_FILEROOTS)))
ZLIB_SRC_FILES         = $(addprefix zlib1.2.11.dec/, $(addsuffix .cpp, $(ZLIB_FILEROOTS)))
ZLIB_OBJ_FILES         = $(addprefix $(OBJDIR)/zlib_, $(addsuffix .o, $(ZLIB_FILEROOTS)))
DEMO_OBJ_FILES         = $(addprefix $(OBJDIR)/prefdemo_, $(addsuffix .o, $(DEMO_FILEROOTS)))

.PHONY: all
all: dirs bin/$(DEMONAME)

.PHONY: clean
clean:
	rm -f objs/*.o

dirs: objs bin lib

objs bin lib:
	mkdir $@

lib/libpreflate.a: $(PREFLATE_LIB_OBJ_FILES) $(SUPPORT_LIB_OBJ_FILES) $(PACKARI_OBJ_FILES)
	ar r $@ $?
  
bin/$(DEMONAME): $(ZLIB_OBJ_FILES) $(DEMO_OBJ_FILES) lib/libpreflate.a
	$(CC) $(CCOPT) -s -o $@ $?

$(OBJDIR)/preflate_%.o : preflate_%.cpp
	$(CC) $(CCOPT) -Werror -o $@ -c -Isupport -IpackARI $<
$(OBJDIR)/prefsup_%.o : support/%.cpp
	$(CC) $(CCOPT) -Werror -o $@ -c -Isupport -IpackARI $<
$(OBJDIR)/packari_%.o : packARI/source/%.cpp
	$(CC) $(CCOPT) -o $@ -c $<
$(OBJDIR)/zlib_%.o : zlib1.2.11.dec/%.c
	$(CC) $(COPT) -o $@ -c -Izlib1.2.13.dec $<
$(OBJDIR)/prefdemo_main.o : main.cpp
	$(CC) $(CCOPT) -Werror -o $@ -c -Isupport -IpackARI -Izlib1.2.13.dec $<
$(OBJDIR)/prefdemo_checker.o : preflate_checker.cpp
	$(CC) $(CCOPT) -Werror -o $@ -c  -Isupport $<
$(OBJDIR)/prefdemo_dumper.o : preflate_dumper.cpp
	$(CC) $(CCOPT) -Werror -o $@ -c  -Isupport -Izlib1.2.13.dec $<
$(OBJDIR)/prefdemo_unpack.o : preflate_unpack.cpp
	$(CC) $(CCOPT) -Werror -o $@ -c  -Isupport -Izlib1.2.13.dec $<
