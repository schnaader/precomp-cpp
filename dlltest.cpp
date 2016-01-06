#include <stdio.h>
#include "precomp.h"

__declspec (dllimport)
void get_copyright_msg(char* msg);

__declspec (dllimport)
bool precompress_file(char* in_file, char* out_file, char* msg, Switches switches);
__declspec (dllimport)
bool recompress_file(char* in_file, char* out_file, char* msg, Switches switches);

int main(int argc, char* argv[]) {
  char msg[256];

  if ((argc != 2) && (argc != 3)) {
    printf("\nSyntax: dlltest [d] input_file\n");
    return -1;
  }

  printf("\nCopyright message:\n");
  get_copyright_msg(msg);
  printf("%s\n", msg);

  Switches switches; //Precomp switches

  if (argc == 2) {
    if (!precompress_file(argv[1], "~temp.pcf", msg, switches)) {
      printf("%s\n", msg);
    } else {
      printf("File %s was precompressed successfully to ~temp.pcf.\n", argv[1]);
    }
  } else {
    if (!recompress_file(argv[2], "out.dat", msg, switches)) {
      printf("%s\n", msg);
    } else {
      printf("File %s was recompressed successfully to out.dat.\n", argv[2]);
    }
  }

  return 0;
}
