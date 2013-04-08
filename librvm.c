#include "rvm.h"
#include <stdlib.h>
#include <string.h>

rvm_t rvm_init(const char *directory)
{
  rvm_t rvm = (rvm_t)malloc(sizeof(rvm_data_t));
  rvm->directoryName = strdup(directory);
  rvm->segments = NULL;
  rvm->transactions = NULL;
}

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
}

void rvm_unmap(rvm_t rvm, void *segbase)
{
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases)
{
    int i, j;
    rvm_data_t* rvmPtr = (rvm_data_t*)rvm;    
    rvm_trans_t* transPtr = NULL;

    // Check if any segments are in a transaction
    transPtr = rvmPtr->transactions;
    while(transPtr!=NULL) {
        for(i=0; i<numsegs; i++) {
            for(j=0; j<transPtr->numsegs; j++) {
                if(segbases[i] == transPtr->segbases[j])
                    return (trans_t)-1;
            }
        }
        transPtr = transPtr->next;
    }

    transPtr = (rvm_trans_t*)malloc( sizeof(rvm_trans_t) );
    transPtr->id = globalTID++;
    transPtr->numsegs = numsegs;
    transPtr->segbases = segbases;  //Can I do this? Or do I need to copy the input content?!
    transPtr->next = NULL;
    LL_APPEND(rvmPtr->transactions, transPtr); 
}

void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
}

void rvm_commit_trans(trans_t tid)
{
}

void rvm_abort_trans(trans_t tid)
{
}

void rvm_truncate_log(rvm_t rvm)
{
}

