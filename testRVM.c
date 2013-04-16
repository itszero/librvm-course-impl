#include <stdio.h>
#include <stdlib.h>
#include "rvm.h"

int* data1;
long* data2;
char* data3;

int main(int argc, char *argv[]) {
    int i;
    trans_t transID;
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


    transID = rvm_begin_trans(rvmPtr, 3, logSegbases);
    rvm_about_to_modify(transID, logSegbases[0], 0, 16*sizeof(int));
    rvm_about_to_modify(transID, logSegbases[1], 0, 16*sizeof(long));
    rvm_about_to_modify(transID, logSegbases[2], 0, 16*sizeof(char));
   

    for(i=0;i<16;i++)
        data1[i] = 0xAA;
    for(i=0;i<16;i++)
        data2[i] = 0xBB;
    for(i=0;i<16;i++)
        data3[i] = '9';
    rvm_commit_trans(transID);

    return 0;
}
