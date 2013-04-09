#include <stdio.h>
#include <stdlib.h>
#include "rvm.h"

int data1[1024];
long data2[16];
char* data3;

int main(int argc, char *argv[]) {
    int i;
    void* localSegbases[3];
    trans_t transID;

    printf("Test program starts...\n");
    rvm_t rvmPtr = rvm_init( "TestRVM" );

    localSegbases[0] = rvm_map(rvmPtr, "Data1", sizeof(int)*1024);
    localSegbases[1] = rvm_map(rvmPtr, "Data2", sizeof(long)*16);
    localSegbases[2] = rvm_map(rvmPtr, "Data3", sizeof(char)*64);
 
    //Initialize data
    for(i=0;i<1024;i++)
        data1[i] = i;
    for(i=0;i<16;i++)
        data2[i] = i*i*i;
    data3 = (char*) malloc(sizeof(char)*64);
    data3 = "Please don't change me!";

    transID = rvm_begin_trans(rvmPtr, 3, localSegbases);
    rvm_about_to_modify(transID, localSegbases[0], 0, 1024);
    rvm_about_to_modify(transID, localSegbases[1], 4, 32);
    rvm_about_to_modify(transID, localSegbases[2], 0, 8);
    rvm_commit_trans(transID);

    
    return 0;
}
