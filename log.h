#ifndef __LOG_H__
#define __LOG_H__

struct log_t;
typedef struct log_t log_t;

#include "rvm.h"

#define MAX_TRANS 256

struct log_t {
  int trans_ids[MAX_TRANS];
  int next_trans_id;
  int count;
};

typedef struct log_header_t {
  int offset;
  int size;
  struct log_header_t *next;
} log_header_t;

void log_init(rvm_data_t *rvm);
void log_read(rvm_data_t *rvm, rvm_seg_t *segment);
void log_start(rvm_data_t *rvm);
void log_write_header(rvm_data_t *rvm, rvm_seg_t *segment);
void log_write(rvm_data_t *rvm, rvm_seg_t *segment, int offset, int size);
void log_commit(rvm_data_t *rvm);
void log_truncate(rvm_data_t *rvm);

#endif  //__LOG_H__
