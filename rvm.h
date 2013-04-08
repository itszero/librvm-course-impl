#ifndef __LRVM_H__
#define __LRVM_H__

#include "utlist.h"
/* Doc for utlist: http://troydhanson.github.com/uthash/utlist.html */

unsigned long globalTID = 0;

typedef struct rvm_seg_t {
  char *name;
  long size;
  void *ptr;
  struct rvm_seg_t *next;
} rvm_seg_t;

typedef struct rvm_undo_t {
  struct rvm_seg_t segment;
  void *backupPtr;
} rvm_undo_t;

typedef int trans_t;

typedef struct rvm_trans_t {
  trans_t id;
  int numsegs;
  void **segbases;
  rvm_undo_t *undologs;      /* A utlist singly-linked-list */
  struct rvm_trans_t* next;
} rvm_trans_t;

typedef struct rvm_data_t {
  char *directoryName;
  rvm_seg_t *segments;       /* A utlist singly-linked-list */
  rvm_trans_t *transactions; /* as above */
} rvm_data_t;

typedef struct rvm_data_t* rvm_t;

extern rvm_t rvm_init(const char *directory);
extern void *rvm_map(rvm_t rvm, const char *segname, int size_to_create);
extern void rvm_unmap(rvm_t rvm, void *segbase);
extern void rvm_destroy(rvm_t rvm, const char *segname);
extern trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases);
extern void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size);
extern void rvm_commit_trans(trans_t tid);
extern void rvm_abort_trans(trans_t tid);
extern void rvm_truncate_log(rvm_t rvm);

#endif
