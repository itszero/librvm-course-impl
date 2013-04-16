#ifndef __LOG_H__
#define __LOG_H__

#include "rvm.h"

typedef struct log_header_t {
  int offset;
  int size;
  struct log_header_t *next;
} log_header_t;

void log_read(rvm_seg_t *segment);
void log_write(rvm_seg_t *segment, int offset, int size);
void log_truncate(rvm_seg_t *segment);

#endif  //__LOG_H__
