#include "rvm.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
unsigned long globalTID = 0;
global_trans_t* globalTransHead = NULL;

int cmp_segbase(rvm_seg_t *a, rvm_seg_t *b)
{
    long segbase_a = (long)a->segbase;
    long segbase_b = (long)b->segbase;

    return (int)(segbase_a - segbase_b);
}

int cmp_segname(rvm_seg_t *a, rvm_seg_t *b)
{
    return strcmp(a->name, b->name);
}

int is_seg_exist(rvm_t rvm, const char *segname)
{
    rvm_seg_t *seg, target;

    target.name = strdup(segname);
    LL_SEARCH(rvm->segments, seg, &target, cmp_segbase);
    free(target.name);

    return seg != NULL;
}

rvm_t rvm_init(const char *directory)
{
    rvm_t rvm = (rvm_t)malloc(sizeof(rvm_data_t));
    memset(rvm, 0, sizeof(rvm));
    rvm->directoryName = strdup(directory);
    rvm->segments = NULL;
    rvm->transactions = NULL;
    rvm->log_info = (log_t*)malloc(sizeof(log_t));

    struct stat sb;
    if (!(stat(directory, &sb) == 0 && S_ISDIR(sb.st_mode)))
    {
        if (mkdir(directory, 0755) != 0)
        {
            fprintf(stderr, "[FATAL] Unable to create directory: %s\n", directory);
            exit(-1);
        }
    }

    log_init(rvm);
    return rvm;
}

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
    char fName[256];

    if (is_seg_exist(rvm, segname))
        return NULL;

    rvm_seg_t *seg = (rvm_seg_t*)malloc(sizeof(rvm_seg_t));
    memset(seg, 0, sizeof(seg));
    seg->name = strdup(segname);
    seg->segbase = (void*)malloc(size_to_create);
    memset(seg->segbase, 0, size_to_create);
    seg->state = MAPPED;
    seg->log_entries_count = 0;
    sprintf(fName, "%s/%s.log", rvm->directoryName, seg->name);
    seg->filePath = strdup(fName);
    seg->size = size_to_create;

    if (access(fName, F_OK) != -1)
        log_read(rvm, seg);
    else
        log_write_header(rvm, seg);

    LL_APPEND(rvm->segments, seg);

    #ifdef DEBUG
    fprintf(stderr, "[RVM] Map %s with size %d to 0x%lx\n", seg->name, size_to_create, (long)seg->segbase);
    #endif

    return seg->segbase;
}

void rvm_unmap(rvm_t rvm, void *segbase)
{
    rvm_seg_t *seg, target;

    target.segbase = segbase;
    LL_SEARCH(rvm->segments, seg, &target, cmp_segbase);
    seg->state = UNMAPPED;
    free(seg->segbase);
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
    char fName[256];
    rvm_seg_t *seg, target;

    target.name = strdup(segname);
    LL_SEARCH(rvm->segments, seg, &target, cmp_segbase);
    if (seg)
    {
        if (seg->state != UNMAPPED)
        {
            fprintf(stderr, "[WARN] Segment: %s is mapped. Unable to destroy it.\n", segname);
            return;
        }
        LL_DELETE(rvm->segments, seg);
        unlink(seg->filePath);
        free(seg->filePath);
        free(seg);
    }
    else
    {
        sprintf(fName, "%s/%s.log", rvm->directoryName, segname);
        unlink(fName);
    }

    free(target.name);
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
                    return (trans_t)-1; //return error -1, no segment should be in other transaction
            }
        }
        transPtr = transPtr->next;
    }

    //Add transaction entry into rvm_data_t
    transPtr = (rvm_trans_t*)malloc( sizeof(rvm_trans_t) );
    transPtr->id = globalTID++;
    transPtr->numsegs = numsegs;
    transPtr->segbases = (void**)malloc(sizeof(void*)*numsegs);
    for(i=0;i<numsegs;i++)
        transPtr->segbases[i] = segbases[i];
    transPtr->segModify = (bool*)calloc(numsegs, sizeof(bool));
    transPtr->next = NULL;
    LL_APPEND(rvmPtr->transactions, transPtr);

    //Add transaction into global transaction list
    globalTransPtr = (global_trans_t*) malloc( sizeof(global_trans_t) );
    globalTransPtr->trans = transPtr;
    globalTransPtr->next = NULL;
    globalTransPtr->rvmEntry = rvmPtr;
    LL_APPEND(globalTransHead, globalTransPtr);

    #ifdef DEBUG
    fprintf(stderr, "Create transaction %d with %d segs: ", transPtr->id, transPtr->numsegs);
    for(i=0;i<transPtr->numsegs;i++)
        fprintf(stderr, "0x%08lx ", (long)transPtr->segbases[i]);
    fprintf(stderr, "\n");
    #endif

    return transPtr->id;
}

void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
    global_trans_t* globalTransPtr;
    rvm_trans_t* transPtr;
    bool transFound = false;
    void* buffer;
    rvm_undo_t* undoPtr;
    int i;
    rvm_seg_t* segPtr;

    if(tid==-1) {
        fprintf(stderr, "[rvm_about_to_modify] ERROR! Illegal transaction id %d, ignore!\n", tid);
        return;
    }


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
        fprintf(stderr, "[rvm_about_to_modify] ERROR! Did not find transaction %d. Ignore and return.\n", (int)tid);
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
    segPtr = globalTransPtr->rvmEntry->segments;
    while(segPtr!=NULL) {
        if(segPtr->segbase==segbase) {
            break;
        }
        segPtr = segPtr->next;
    }
    if(segPtr==NULL) {
        fprintf(stderr, "[rvm_about_to_modify] Cannot find segment! Ignore and return.\n");
        return;
    }

    undoPtr->segment = segPtr;
    undoPtr->offset = offset;
    undoPtr->size = size;
    buffer = (void*) malloc( size );
    bcopy((char*)segbase+offset, buffer, size );
    undoPtr->backupData = buffer;
    LL_APPEND(transPtr->undologs, undoPtr);

    #ifdef DEBUG
    fprintf(stderr, "Backup transaction %d, segment %s(0x%lx), data:", tid, segPtr->name, (long)segPtr->segbase);
    char* charPtr = (char*)undoPtr->backupData;
    for(i=0;i<size;i++)
        fprintf(stderr, "%x", charPtr[i]);
    fprintf(stderr, "\n");
    #endif
}

void rvm_commit_trans(trans_t tid)
{
    rvm_trans_t* transPtr;
    global_trans_t* globalTransPtr;
    bool transFound = false;
    rvm_undo_t* undoPtr;
    rvm_undo_t* prevUndoPtr;
    rvm_seg_t *seg = NULL;
    rvm_data_t* rvmPtr;

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
        fprintf(stderr, "[rvm_abort_trans] ERROR! Did not find transaction %d. Ignore and return.\n", (int)tid);
        exit(1);
    }

    //Write the diff into log file
    log_start(globalTransPtr->rvmEntry);
    transPtr = globalTransPtr->trans;
    rvmPtr = globalTransPtr->rvmEntry;
    undoPtr = transPtr->undologs;
    while(undoPtr!=NULL) {
        LL_SEARCH(rvmPtr->segments, seg, undoPtr->segment, cmp_segbase);
        if(seg==NULL) {
            printf("[rvm_commit_trans] ERROR! Cannot find segment %s! Ignore and return.\n", undoPtr->segment->name);
            return;
        }
        else if(seg->state != MAPPED) {
            printf("[rvm_commit_trans] ERROR! Segment %s is in state %d(0:UNMAPPED, 1:MAPPED), ignore commit for this segment\n", undoPtr->segment->name, seg->state);
        }
        else {
            #ifdef DEBUG
            printf("[rvm_commit_trans] Committing segment %s\n", undoPtr->segment->name);
            #endif
            log_write(globalTransPtr->rvmEntry, undoPtr->segment, undoPtr->offset, undoPtr->size);
        }
        prevUndoPtr = undoPtr;
        undoPtr = undoPtr->next;

        LL_DELETE(globalTransPtr->trans->undologs, prevUndoPtr);    //Does it free the entry prevUndoPtr?
        free( prevUndoPtr );
    }
    globalTransPtr->trans->undologs = NULL;
    log_commit(globalTransPtr->rvmEntry);

    //Remove this transaction
    LL_DELETE(globalTransPtr->rvmEntry->transactions, globalTransPtr->trans);
    LL_DELETE(globalTransHead, globalTransPtr);

    //Free transaction entries
    free( transPtr->segbases );
    free( transPtr->segModify );
    free( transPtr );
    free( globalTransPtr );
}

void rvm_abort_trans(trans_t tid)
{
    global_trans_t* globalTransPtr;
    bool transFound = false;
    rvm_undo_t* undoPtr;
    rvm_undo_t* prevUndoPtr;
    rvm_trans_t* transPtr;
    rvm_data_t* rvmPtr;
    rvm_seg_t *seg = NULL;

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
        fprintf(stderr, "[rvm_abort_trans] ERROR! Did not find transaction %d\n", (int)tid);
        exit(1);
    }

    //Undo all modifications
    transPtr = globalTransPtr->trans;
    undoPtr = globalTransPtr->trans->undologs;
    rvmPtr = globalTransPtr->rvmEntry;
    while(undoPtr!=NULL) {
        LL_SEARCH(rvmPtr->segments, seg, undoPtr->segment, cmp_segbase);
        if(seg==NULL) {
            printf("[rvm_abort_trans] ERROR! Cannot find segment %s. It has never been mapped. THIS SHOULD NOT HAPPEN!\n", undoPtr->segment->name);
            exit(1);
        }
        else if(seg->state != MAPPED) {
            printf("[rvm_abort_trans] ERROR! Segment %s is in state %d(0:UNMAPPED, 1:MAPPED), ignore commit for this segment\n", undoPtr->segment->name, seg->state);
        }
        else {
            #ifdef DEBUG
            printf("[rvm_abort_trans] Aborting segment %s\n", undoPtr->segment->name);
            #endif
            bcopy(undoPtr->backupData, (char*)undoPtr->segment->segbase+undoPtr->offset, undoPtr->size );
        }
        prevUndoPtr = undoPtr;
        undoPtr = undoPtr->next;

        LL_DELETE(globalTransPtr->trans->undologs, prevUndoPtr);    //Does it free the entry prevUndoPtr?
        free( prevUndoPtr );
    }
    globalTransPtr->trans->undologs = NULL;

    //Remove this transaction
    LL_DELETE(globalTransPtr->rvmEntry->transactions, globalTransPtr->trans);
    LL_DELETE(globalTransHead, globalTransPtr);

    //Free trans entry and globalTransEntry
    free( transPtr->segbases );
    free( transPtr->segModify );
    free( transPtr );
    free( globalTransPtr );
}

void rvm_truncate_log(rvm_t rvm)
{
    log_truncate(rvm);
}

