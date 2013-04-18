#include "log.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "utlist.h"
#include "sha256.h"

int check_hash(unsigned char hash[], unsigned char filehash[])
{
  for(int i=0;i<32;i++)
  {
    if (hash[i] != filehash[i])
    {
      fprintf(stderr, "[FATAL] SHA-1 Checksum mismatch. Data corruption detected.\n");
      return -1;
    }
  }

  return 0;
}

int cmp_offset(log_header_t *a, log_header_t *b)
{
    return a->offset - b->offset;
}

static int has_transaction(rvm_data_t *rvm, int transaction_id)
{
  for(int i=0;i<rvm->log_info->count;i++)
    if (rvm->log_info->trans_ids[i] == transaction_id)
      return 1;

  return 0;
}

static int current_transaction_id(rvm_data_t *rvm)
{
  if (rvm->log_info->count == 0)
    return -1;
  return rvm->log_info->trans_ids[rvm->log_info->count - 1];
}

void log_init(rvm_data_t *rvm)
{
  char fName[256];
  int id = -1, maxID = -1;

  memset(rvm->log_info, 0, sizeof(log_t));
  rvm->log_info->next_trans_id = 1;
  rvm->log_info->count = 0;

  sprintf(fName, "%s/transaction.log", rvm->directoryName);
  if (access(fName, F_OK) == -1)
    return;

  FILE *file = fopen(fName, "r");
  while(!feof(file))
  {
    fread(&id, sizeof(int), 1, file);
    if (id > maxID)
      maxID = id;
    rvm->log_info->trans_ids[rvm->log_info->count] = id;
    rvm->log_info->count++;
  }
  fclose(file);

  if (maxID > rvm->log_info->next_trans_id)
    rvm->log_info->next_trans_id = maxID;
}

void log_read(rvm_data_t *rvm, rvm_seg_t *segment)
{
  unsigned char hash[32], filehash[32];
  int offset, size, trans_id;

  FILE *file = fopen(segment->filePath, "r");
  if (file == NULL)
  {
    fprintf(stderr, "[FATAL] Unable to open log file. Bail out.\n");
    exit(-1);
  }

  while(true)
  {
    if ( (fread(&trans_id, sizeof(int), 1, file) == 0) && feof(file) )
      break;
    fread(&offset, sizeof(int), 1, file);
    fread(&size, sizeof(int), 1, file);
    #ifdef DEBUG
    fprintf(stderr, "[Log] read: trans_id=%d, segment=%s, offset=%d, size=%d\n", trans_id, segment->name, offset, size);
    #endif
    if ((offset >= segment->size) || (offset+size > segment->size))
    {
      fprintf(stderr, "[FATAL] Offest/Size out of range. Bail out.\n");
      fprintf(stderr, "[FATAL] Read (%d, %d), seg size=%d\n", offset, size, segment->size);
      exit(-1);
    }
    if (!has_transaction(rvm, trans_id))
    {
      fprintf(stderr, "[WARN] Unknown transaction ID: %d, skipped\n", trans_id);
      fseek(file, size, SEEK_CUR);
      continue;
    }

    int c = fread((char*)segment->segbase + offset, 1, size, file);
    if (c != size)
    {
      fprintf(stderr, "[WARN] Expected %d bytes of data, read %d bytes\n", size, c);
      exit(-1);
    }
    fread(filehash, 1, 32, file);
    sha256_do((unsigned char*)segment->segbase + offset, size, hash);
    if (check_hash(hash, filehash) != 0)
    {
      fclose(file);
      return;
    }
  }

  fclose(file);
}

void log_start(rvm_data_t *rvm)
{
#ifdef DEBUG
  printf("[LOG] Starting transaction #%d\n", rvm->log_info->next_trans_id);
#endif
  rvm->log_info->trans_ids[rvm->log_info->count] = rvm->log_info->next_trans_id;
  rvm->log_info->next_trans_id++;
  rvm->log_info->count++;
}

void log_write(rvm_data_t *rvm, rvm_seg_t *segment, int offset, int size)
{
  unsigned char hash[32];
  int trans_id = current_transaction_id(rvm);
  FILE *file = fopen(segment->filePath, "a");
  if (file == NULL)
  {
    fprintf(stderr, "[FATAL] Unable to open log file. Bail out.\n");
    exit(-1);
  }

  fwrite(&trans_id, sizeof(int), 1, file);
  fwrite(&offset, sizeof(int), 1, file);
  fwrite(&size, sizeof(int), 1, file);
  fwrite((char*)segment->segbase + offset, size, 1, file);
  sha256_do((unsigned char*)segment->segbase + offset, size, hash);
  fwrite(hash, 32, 1, file);

  #ifdef DEBUG
  fprintf(stderr, "[Log] write: trans_id=%d, segment=%s, offset=%d, size=%d\n", trans_id, segment->name, offset, size);
  #endif

  fclose(file);
}

void log_commit(rvm_data_t *rvm)
{
  char fName[256];
  int trans_id = current_transaction_id(rvm);
  sprintf(fName, "%s/transaction.log", rvm->directoryName);
  FILE *file = fopen(fName, "a");
  fseek(file, -1, SEEK_END);
  fwrite(&trans_id, sizeof(int), 1, file);
  fclose(file);

  if (rvm->log_info->count == MAX_TRANS)
  {
    #ifdef DEBUG
    fprintf(stderr, "[INFO] # transactions reaches max(%d), truncating log.\n", MAX_TRANS);
    #endif
    log_truncate(rvm);
  }
}

static void log_truncate_segment(rvm_data_t *rvm, rvm_seg_t *segment)
{
  unsigned char hash[32], filehash[32];
  int new_trans_id = 1, trans_id;
  FILE *file = fopen(segment->filePath, "r");
  if (file == NULL)
    return;

  int offset, size;
  log_header_t *logs = NULL, *e, *tmp;

  // read everything out into a separate buffer
  void *data = (void*)malloc(segment->size);

  while(true)
  {
    if ( (fread(&trans_id, sizeof(int), 1, file) == 0) && feof(file) )
      break;
    fread(&offset, sizeof(int), 1, file);
    fread(&size, sizeof(int), 1, file);
    if (!has_transaction(rvm, trans_id))
    {
      fprintf(stderr, "[WARN] Unknown transaction ID: %d, skipped\n", trans_id);
      fseek(file, size, SEEK_CUR);
      continue;
    }
    fread((char*)data + offset, 1, size, file);
    fread(filehash, 1, 32, file);
    sha256_do((unsigned char*)data + offset, size, hash);
    check_hash(hash, filehash);
    fprintf(stderr, "[Log] truncate_read: trans_id=%d, segment=%s, offset=%d, size=%d\n", trans_id, segment->name, offset, size);
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
      int newsz = (int)((long)e->offset + e->size - (long)last->offset);
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
    fprintf(stderr, "[FATAL] Unable to open log file for writing. Bail out.\n");
    exit(-1);
  }

  LL_FOREACH(logs, e) {
    fwrite(&new_trans_id, sizeof(int), 1, file);
    fwrite(&e->offset, sizeof(int), 1, file);
    fwrite(&e->size, sizeof(int), 1, file);
    fwrite((unsigned char*)data + e->offset, 1, e->size, file);
    sha256_do((unsigned char*)data + offset, size, hash);
    fwrite(hash, 1, 32, file);
    fprintf(stderr, "[Log] truncate_write: trans_id=%d, segment=%s, offset=%d, size=%d\n", new_trans_id, segment->name, e->offset, e->size);
  }
  fclose(file);

  // cleanup
  LL_FOREACH_SAFE(logs, e, tmp) {
    LL_DELETE(logs, e);
    free(e);
  }

  free(data);
}

void log_truncate(rvm_data_t *rvm)
{
  char fName[256];
  int trans_id = 1;

  rvm_seg_t *seg;
  LL_FOREACH(rvm->segments, seg) {
    #ifdef DEBUG
    fprintf(stderr, "[LOG] Truncating log for segment: %s\n", seg->name);
    #endif
    log_truncate_segment(rvm, seg);
  }

  // clear transaction log and add transaction id 1 for consolidated log
  sprintf(fName, "%s/transaction.log", rvm->directoryName);
  memset(rvm->log_info, 0, sizeof(log_t));
  rvm->log_info->next_trans_id = 2;
  rvm->log_info->count = 1;
  rvm->log_info->trans_ids[0] = 1;

  FILE *file = fopen(fName, "w");
  fwrite(&trans_id, sizeof(int), 1, file);
  fclose(file);

}
