#include <stdio.h>
#include <stdlib.h>
#include "rvm.h"
#include "log.h"

int* data1;
long* data2;
char* data3;

int main(int argc, char *argv[]) {
    int i;
    trans_t transID=0;
    trans_t transID2=0;
    void* logSegbases[4];
    printf("Test program starts...\n");
    rvm_t rvmPtr = rvm_init( "TestRVM.log" );

    logSegbases[0] = rvm_map(rvmPtr, "Data1", sizeof(int)*1024);
    logSegbases[1] = rvm_map(rvmPtr, "Data2", sizeof(long)*16);
    logSegbases[2] = rvm_map(rvmPtr, "Data3", sizeof(char)*64);


    data1 = (int*) logSegbases[0];
    data2 = (long*) logSegbases[1];
    data3 = (char*) logSegbases[2];

    //Initialize data
    for(i=0;i<1024;i++)
        data1[i] = i;
    for(i=0;i<16;i++)
        data2[i] = i*i*i;
    sprintf(data3, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    printf("Before\n");
    for(i=0;i<16;i++)
      printf("%c", data3[i]);
    printf("\n");
    transID = rvm_begin_trans(rvmPtr, 3, logSegbases);
    transID2 = rvm_begin_trans(rvmPtr, 3, logSegbases);
    printf("transID:%d, transID2:%d\n", transID, transID2);
    rvm_about_to_modify(transID, logSegbases[0], 0, 64*sizeof(int));
    rvm_about_to_modify(transID, logSegbases[0], 32, 16*sizeof(int));
    rvm_about_to_modify(transID, logSegbases[0], 64, 64*sizeof(int));
    rvm_about_to_modify(transID, logSegbases[1], 0, 16*sizeof(long));
    rvm_about_to_modify(transID, logSegbases[2], 0, 16*sizeof(char));
  

    for(i=0;i<16;i++)
        data1[i] = 0xAA;
    for(i=0;i<16;i++)
        data2[i] = 0xBB;
    for(i=0;i<16;i++)
        data3[i] = '9';

    rvm_unmap(rvmPtr, logSegbases[0]);
//    rvm_abort_trans(transID2);
    rvm_commit_trans(transID);
    rvm_commit_trans(transID);

    printf("After\n");
    for(i=0;i<16;i++)
      printf("%c", data3[i]);
    printf("\n");
//    log_write(rvmPtr->segments, 0, 16);
//    log_write(rvmPtr->segments, 16, 32);

    rvm_truncate_log(rvmPtr);

    return 0;
}
