#include "log.h"

void log_read(rvm_seg_t *segment)
{
  int count, offset, size;

  fseek(segment->file, 0, SEEK_SET);
  fread(&count, sizeof(int), 1, segment->file);
  while(count-->0)
  {
    fread(&offset, sizeof(int), 1, segment->file);
    fread(&size, sizeof(int), 1, segment->file);
    fread((char*)segment->segbase + offset, sizeof(char), size, segment->file);
  }
}

void log_write(rvm_seg_t *segment, int offset, int size)
{
  fseek(segment->file, -1, SEEK_END);
  fwrite(&offset, sizeof(int), 1, segment->file);
  fwrite(&size, sizeof(int), 1, segment->file);
  fwrite((char*)segment->segbase + offset, size, 1, segment->file);
  segment->log_entries_count++;
  fseek(segment->file, 0, SEEK_SET);
  fwrite(&segment->log_entries_count, sizeof(int), 1, segment->file);
}
