#include "log.h"

void log_read(rvm_seg_t *segment)
{
  int count, offset, size;
  FILE *file = fopen(segment->filePath, "r");
  if (file == NULL)
  {
    printf("[FATAL] Unable to open log file. Bail out.\n");
    exit(-1);
  }

  rewind(file);
  while(!feof(file))
  {
    fread(&offset, sizeof(int), 1, file);
    fread(&size, sizeof(int), 1, file);
    fread((char*)segment->segbase + offset, sizeof(char), size, file);

    #ifdef DEBUG
    int i;
    unsigned char* charPtr = (unsigned char*)segment->segbase + offset;
    printf("[log_read] Count:%d, Base:0x%x, Offset:%d, size:%d, data:\n", count, segment->segbase, offset, size);
    for(i=0;i<size;i++)
      printf("%02x ", charPtr[i]);
    printf("\n");
    #endif
  }
}

void log_write(rvm_seg_t *segment, int offset, int size)
{
  FILE *file = fopen(segment->filePath, "a");
  if (file == NULL)
  {
    printf("[FATAL] Unable to open log file. Bail out.\n");
    exit(-1);
  }

  fseek(file, -1, SEEK_END);
  fwrite(&offset, sizeof(int), 1, file);
  fwrite(&size, sizeof(int), 1, file);
  fwrite((char*)segment->segbase + offset, size, 1, file);

  #ifdef DEBUG
  int i;
  unsigned char* charPtr = (unsigned char*)segment->segbase + offset;
  printf("[log_write] Base:0x%x, Offset:%d, size:%d, data:\n", segment->segbase, offset, size);
  for(i=0;i<size;i++)
    printf("%02x ", charPtr[i]);
  printf("\n");
  #endif
}


