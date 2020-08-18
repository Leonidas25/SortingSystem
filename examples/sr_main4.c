#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bf.h"
#include "sort_file.h"


#define CALL_OR_DIE(call)     \
  {                           \
    SR_ErrorCode code = call; \
    if (code != SR_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

int main() {
  BF_Init(LRU);
  int fd;

  CALL_OR_DIE(SR_Init());

  CALL_OR_DIE(SR_OpenFile("temp22.db", &fd));

  CALL_OR_DIE(SR_PrintAllEntries(fd));

  CALL_OR_DIE(SR_CloseFile(fd));
  BF_Close();
}
