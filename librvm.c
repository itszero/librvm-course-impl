#include "rvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned long globalTID = 0;
global_trans_t* globalTransHead = NULL;


rvm_t rvm_init(const char *directory)
{
  char buf[1024];
  rvm_t rvm = (rvm_t)malloc(sizeof(rvm_data_t));
  rvm->directoryName = strdup(directory);
  rvm->segments = NULL;
  rvm->transactions = NULL;

  snprintf(buf, sizeof buf, "%s.log", directory);
  rvm->logFilePtr = fopen(buf, "r");
  if(rvm->logFilePtr==NULL) {
    printf("[rvm_init] %s log file does not exist, create one!", directory);
    rvm->logFilePtr = fopen(buf, "w");
  }
  else {
    printf("[rvm_init] %s log file exist!", directory);
    fclose(rvm->logFilePtr);
    rvm->logFilePtr = fopen(buf, "a");
  }

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
    global_trans_t* globalTransPtr;

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

    //Add transaction entry into rvm_data_t
    transPtr = (rvm_trans_t*)malloc( sizeof(rvm_trans_t) );
    transPtr->id = globalTID++;
    transPtr->numsegs = numsegs;
    transPtr->segbases = segbases;  //Can I do this? Or do I need to copy the input content?!
    transPtr->segModify = (bool*)calloc(numsegs, sizeof(bool));
    transPtr->next = NULL;
    LL_APPEND(rvmPtr->transactions, transPtr); 

    //Add transaction into global transaction list
    globalTransPtr = (global_trans_t*) malloc( sizeof(global_trans_t) );
    globalTransPtr->trans = transPtr;
    globalTransPtr->next = NULL;
    globalTransPtr->rvmEntry = rvmPtr;
    LL_APPEND(globalTransHead, globalTransPtr);
}

void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
    global_trans_t* globalTransPtr;
    rvm_trans_t* transPtr;
    bool transFound = false; 
    void* buffer;
    rvm_undo_t* undoPtr;
    int i;

    //Find the transaction entry in the global transaction list
    globalTransPtr = globalTransHead;    
    while( globalTransPtr!=NULL ) {
        if(globalTransPtr->trans->id==tid) {
            transFound = true;
            break;
        }
        else
            globalTransPtr = globalTransPtr->next;
    }
    if(!transFound) {
        printf("[rvm_about_to_modify] ERROR! Did not find transaction %d\n", (int)tid);
        exit(1);
    }
    
    //Create undo copy and set the segModify flag
    transPtr = globalTransPtr->trans;
    for(i=0;i<transPtr->numsegs;i++) {
        if(transPtr->segbases[i] == segbase) {
            transPtr->segModify[i] = true;
        }
    }
    undoPtr = (rvm_undo_t*) malloc( sizeof(rvm_undo_t) );
    undoPtr->segment.size = size;
    undoPtr->segment.offset = offset;
    undoPtr->segment.segbase = segbase;
    buffer = (void*) malloc( size ); 
    bcopy( segbase+offset, buffer, size );        
    undoPtr->backupData = buffer; 
    LL_APPEND(transPtr->undologs, undoPtr);
}

void rvm_commit_trans(trans_t tid)
{
    rvm_trans_t* transPtr;
    global_trans_t* globalTransPtr;
    bool transFound = false;
    rvm_undo_t* undoPtr;
    rvm_undo_t* prevUndoPtr;
    FILE* filePtr;
    int i;

    //Find the transaction entry in the global transaction list
    globalTransPtr = globalTransHead;
    while( globalTransPtr!=NULL ) { 
        if(globalTransPtr->trans->id==tid) {
            transFound = true;
            break;
        }   
        else
            globalTransPtr = globalTransPtr->next;
    }   
    if(!transFound) {
        printf("[rvm_abort_trans] ERROR! Did not find transaction %d\n", (int)tid);
        exit(1);
    }   
   
    //Write the diff into log file
    transPtr = globalTransPtr->trans;
    undoPtr = transPtr->undologs;
    filePtr = globalTransPtr->rvmEntry->logFilePtr;
    while(undoPtr!=NULL) {
        /*Create the log file!*/
        fprintf(filePtr, "%d %d ", transPtr->id, transPtr->numsegs ); 
        for(i=0;i<transPtr->numsegs;i++)
            fprintf(filePtr, "%d ", transPtr->segbases[i]);

        /******************/
        /* NOT DONE YET!! */
        /******************/        
 
        prevUndoPtr = undoPtr;
        undoPtr = undoPtr->next;
       
        LL_DELETE(globalTransPtr->trans->undologs, prevUndoPtr);    //Does it free the entry prevUndoPtr?
    }   
    globalTransPtr->trans->undologs = NULL;    

   //Remove this transaction
    LL_DELETE(globalTransPtr->rvmEntry->transactions, globalTransPtr->trans);
    LL_DELETE(globalTransHead, globalTransPtr);
}

void rvm_abort_trans(trans_t tid)
{
    global_trans_t* globalTransPtr;
    bool transFound = false;
    rvm_undo_t* undoPtr;
    rvm_undo_t* prevUndoPtr;

    //Find the transaction entry in the global transaction list
    globalTransPtr = globalTransHead;
    while( globalTransPtr!=NULL ) {
        if(globalTransPtr->trans->id==tid) {
            transFound = true;
            break;
        }
        else
            globalTransPtr = globalTransPtr->next;
    }
    if(!transFound) {
        printf("[rvm_abort_trans] ERROR! Did not find transaction %d\n", (int)tid);
        exit(1);
    }
   
    //Undo all modifications
    undoPtr = globalTransPtr->trans->undologs;
    while(undoPtr!=NULL) {
        bcopy(undoPtr->backupData, undoPtr->segment.segbase+undoPtr->segment.offset, undoPtr->segment.size );

        prevUndoPtr = undoPtr;
        undoPtr = undoPtr->next;
        
        LL_DELETE(globalTransPtr->trans->undologs, prevUndoPtr);    //Does it free the entry prevUndoPtr?
    }
    globalTransPtr->trans->undologs = NULL;

    //Remove this transaction
    LL_DELETE(globalTransPtr->rvmEntry->transactions, globalTransPtr->trans);
    LL_DELETE(globalTransHead, globalTransPtr);

}

void rvm_truncate_log(rvm_t rvm)
{
}

