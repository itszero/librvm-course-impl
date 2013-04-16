#include "log.h"
#include <stdlib.h>

int cmp_offset(log_header_t *a, log_header_t *b)
{
    return a->offset - b->offset;
}

void log_read(rvm_seg_t *segment)
{
  int offset, size;
  FILE *file = fopen(segment->filePath, "r");
  if (file == NULL)
  {
    printf("[FATAL] Unable to open log file. Bail out.\n");
    exit(-1);
  }

  while(!feof(file))
  {
    fread(&offset, sizeof(int), 1, file);
    fread(&size, sizeof(int), 1, file);
    if ((offset >= segment->size) || (offset+size > segment->size))
    {
      printf("[FATAL] Offest/Size out of range. Bail out.\n");
      printf("[FATAL] Read (%d, %d), seg size=%d\n", offset, size, segment->size);
      exit(-1);
    }
    fread((char*)segment->segbase + offset, sizeof(char), size, file);
    #ifdef DEBUG
    printf("[Log] read: segment=%s, offset=%d, size=%d\n", segment->name, offset, size);
    #endif
  }

  fclose(file);
}

void log_write(rvm_seg_t *segment, int offset, int size)
{
  FILE *file = fopen(segment->filePath, "a");
  if (file == NULL)
  {
    printf("[FATAL] Unable to open log file. Bail out.\n");
    exit(-1);
  }

  fwrite(&offset, sizeof(int), 1, file);
  fwrite(&size, sizeof(int), 1, file);
  fwrite((char*)segment->segbase + offset, size, 1, file);

  #ifdef DEBUG
  printf("[Log] write: segment=%s, offset=%d, size=%d\n", segment->name, offset, size);
  #endif

  fclose(file);
}

void log_truncate(rvm_seg_t *segment)
{
  FILE *file = fopen(segment->filePath, "r");
  if (file == NULL)
    return;

  int offset, size;
  log_header_t *logs = NULL, *e, *tmp;

  // read everything out into a separate buffer
  void *data = (void*)malloc(segment->size);

  while(!feof(file))
  {
    fread(&offset, sizeof(int), 1, file);
    fread(&size, sizeof(int), 1, file);
    fread((char*)data + offset, sizeof(char), size, file);
    printf("[Log] truncate_read: segment=%s, offset=%d, size=%d\n", segment->name, offset, size);

    log_header_t *header = (log_header_t*)malloc(sizeof(log_header_t));
    header->offset = offset;
    header->size = size;
    LL_APPEND(logs, header);
  }
  fclose(file);

  // merge blocks (offset, size pair)
  LL_SORT(logs, cmp_offset);

  log_header_t *last = NULL;
  LL_FOREACH_SAFE(logs, e, tmp) {
    if (last == NULL)
    {
      last = e;
      continue;
    }

    if ( (last->offset <= e->offset) && (e->offset <= last->offset + last->size) )
    {
      int newsz = (char*)e->offset + e->size - (char*)last->offset;
      if (newsz > last->size)
      {
        last->size = newsz;
      }
      LL_DELETE(logs, e);
      continue;
    }

    last = e;
  }

  // write consolidated log file
  file = fopen(segment->filePath, "w");
  if (file == NULL)
  {
    printf("[FATAL] Unable to open log file for writing. Bail out.\n");
    exit(-1);
  }

  LL_FOREACH(logs, e) {
    fwrite(&e->offset, sizeof(int), 1, file);
    fwrite(&e->size, sizeof(int), 1, file);
    fwrite((char*)data + offset, size, 1, file);
    printf("[Log] truncate_write: segment=%s, offset=%d, size=%d\n", segment->name, e->offset, e->size);
  }

  // cleanup
  LL_FOREACH_SAFE(logs, e, tmp) {
    LL_DELETE(logs, e);
    free(e);
  }

  free(data);
}
