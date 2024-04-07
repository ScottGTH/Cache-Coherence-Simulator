/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
    printf("===== 506 Personal information =====\n");
    printf("Tzu-Ching Yeh\n");
    printf("tyeh6\n");
    printf("ECE406 Students? NO\n");

    



    ifstream fin;
    FILE * pFile;

    if(argv[1] == NULL){
         printf("input format: ");
         printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
         exit(0);
        }

    ulong cache_size     = atoi(argv[1]);
    ulong cache_assoc    = atoi(argv[2]);
    ulong blk_size       = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol       = atoi(argv[5]); /* 0:MODIFIED_MSI 1:DRAGON*/
    char *fname        = (char *) malloc(20);
    fname              = argv[6];


    printf("===== 506 SMP Simulator configuration =====\n");
    
    cout << "L1_SIZE:                " << cache_size << endl;
    cout << "L1_ASSOC:               " << cache_assoc << endl;
    cout << "L1_BLOCKSIZE:           " << blk_size << endl;
    cout << "NUMBER OF PROCESSORS:   " << num_processors << endl;

    if(protocol == 0){
        printf("COHERENCE PROTOCOL:     MSI\n");
    }else if(protocol == 1){
        printf("COHERENCE PROTOCOL:     Dragon\n");
    }
    printf("TRACE FILE:             %s\n",fname);
    
    // print out simulator configuration here
    
    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache **) malloc(num_processors * sizeof(Cache));
    for(ulong i = 0; i < num_processors; i++) {
        //if(protocol == 0) {
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
        //}
    }

    pFile = fopen (fname,"r");
    if(pFile == 0)
    {   
        printf("Trace file problem\n");
        exit(0);
    }
    
    ulong proc;
    char op;
    ulong addr;
    //bool C = false;

    int line = 1;
    while(fscanf(pFile, "%lu %c %lx", &proc, &op, &addr) != EOF)
    {
/*#ifdef _DEBUG
        printf("%d\n", line);
#endif*/
        // propagate request down through memory hierarchy
        // by calling cachesArray[processor#]->Access(...)

    if(proc == 0){
        if(protocol == 0){    // MSI protocal
            cacheArray[0]->Access(addr, op);
            ulong STA = cacheArray[0]->getStatus();
        
            cacheArray[1]->MSI_Snooper(addr, op, STA);
            cacheArray[2]->MSI_Snooper(addr, op, STA);
            cacheArray[3]->MSI_Snooper(addr, op, STA);
        }else{


            /*if(cacheArray[1]->havecopy(addr, op) ){
                C = true;
            }else if(cacheArray[2]->havecopy(addr, op)){
                C = true;
            }else if(cacheArray[3]->havecopy(addr, op)){
                C = true;
            }*/

            cacheArray[0]->D_Access(0,num_processors,addr, op, cacheArray);
            ulong STA = cacheArray[0]->hit;


            if(STA == 3){
                cacheArray[1]->D_Snooper(addr, op, 0);
                cacheArray[2]->D_Snooper(addr, op, 0);
                cacheArray[3]->D_Snooper(addr, op, 0);
                cacheArray[1]->D_Snooper(addr, op, 1);
                cacheArray[2]->D_Snooper(addr, op, 1);
                cacheArray[3]->D_Snooper(addr, op, 1);
            }else{
                cacheArray[1]->D_Snooper(addr, op, STA);
                cacheArray[2]->D_Snooper(addr, op, STA);
                cacheArray[3]->D_Snooper(addr, op, STA);
            }
            
            

        }
    }else if(proc == 1){
        if(protocol == 0){    // MSI protocal
            cacheArray[1]->Access(addr, op);
            ulong STA = cacheArray[1]->getStatus();
        
            cacheArray[0]->MSI_Snooper(addr, op, STA);
            cacheArray[2]->MSI_Snooper(addr, op, STA);
            cacheArray[3]->MSI_Snooper(addr, op, STA);
        }else if(protocol == 1){

            
            /*if(cacheArray[0]->havecopy(addr, op) ){
                C = true;
            }else if(cacheArray[2]->havecopy(addr, op)){
                C = true;
            }else if(cacheArray[3]->havecopy(addr, op)){
                C = true;
            }*/



            cacheArray[1]->D_Access(1,num_processors,addr, op, cacheArray);
            ulong STA = cacheArray[1]->hit;
        
            if(STA == 3){
                cacheArray[0]->D_Snooper(addr, op, 0);
                cacheArray[2]->D_Snooper(addr, op, 0);
                cacheArray[3]->D_Snooper(addr, op, 0);
                cacheArray[0]->D_Snooper(addr, op, 1);
                cacheArray[2]->D_Snooper(addr, op, 1);
                cacheArray[3]->D_Snooper(addr, op, 1);
            }else{
                cacheArray[0]->D_Snooper(addr, op, STA);
                cacheArray[2]->D_Snooper(addr, op, STA);
                cacheArray[3]->D_Snooper(addr, op, STA);
            }
        }
    }else if(proc == 2){
        if(protocol == 0){    // MSI protocal
            cacheArray[2]->Access(addr, op);
            ulong STA = cacheArray[2]->getStatus();

            cacheArray[0]->MSI_Snooper(addr, op, STA);
            cacheArray[1]->MSI_Snooper(addr, op, STA);
            cacheArray[3]->MSI_Snooper(addr, op, STA);
        }else{


            /*if(cacheArray[0]->havecopy(addr, op) ){
                C = true;
            }else if(cacheArray[1]->havecopy(addr, op)){
                C = true;
            }else if(cacheArray[3]->havecopy(addr, op)){
                C = true;
            }*/

            cacheArray[2]->D_Access(2,num_processors,addr, op, cacheArray);
            ulong STA = cacheArray[2]->hit;
        
            if(STA == 3){
                cacheArray[0]->D_Snooper(addr, op, 0);
                cacheArray[1]->D_Snooper(addr, op, 0);
                cacheArray[3]->D_Snooper(addr, op, 0);
                cacheArray[0]->D_Snooper(addr, op, 1);
                cacheArray[1]->D_Snooper(addr, op, 1);
                cacheArray[3]->D_Snooper(addr, op, 1);
            }else{
                cacheArray[0]->D_Snooper(addr, op, STA);
                cacheArray[1]->D_Snooper(addr, op, STA);
                cacheArray[3]->D_Snooper(addr, op, STA);
            }

        }
    }else if(proc == 3){
        if(protocol == 0){    // MSI protocal
            cacheArray[3]->Access(addr, op);
            ulong STA = cacheArray[3]->getStatus();

            cacheArray[0]->MSI_Snooper(addr, op, STA);
            cacheArray[1]->MSI_Snooper(addr, op, STA);
            cacheArray[2]->MSI_Snooper(addr, op, STA);
        }else{
 

            /*if(cacheArray[0]->havecopy(addr, op) ){
                C = true;
            }else if(cacheArray[2]->havecopy(addr, op)){
                C = true;
            }else if(cacheArray[1]->havecopy(addr, op)){
                C = true;
            }*/

            cacheArray[3]->D_Access(3,num_processors,addr, op, cacheArray);
            ulong STA = cacheArray[3]->hit;
        
            if(STA == 3){
                cacheArray[0]->D_Snooper(addr, op, 0);
                cacheArray[2]->D_Snooper(addr, op, 0);
                cacheArray[1]->D_Snooper(addr, op, 0);
                cacheArray[0]->D_Snooper(addr, op, 1);
                cacheArray[2]->D_Snooper(addr, op, 1);
                cacheArray[1]->D_Snooper(addr, op, 1);
            }else{
                cacheArray[0]->D_Snooper(addr, op, STA);
                cacheArray[2]->D_Snooper(addr, op, STA);
                cacheArray[1]->D_Snooper(addr, op, STA);
            }

        }
    }



        line++;
    }

    fclose(pFile);

    //********************************//
    //print out all caches' statistics //
    //********************************//
    

    if(protocol == 0){
        cacheArray[0]->printStats(0);    
        cacheArray[1]->printStats(1);
        cacheArray[2]->printStats(2);
        cacheArray[3]->printStats(3);
    }else{
        cacheArray[0]->D_printStats(0);    
        cacheArray[1]->D_printStats(1);
        cacheArray[2]->D_printStats(2);
        cacheArray[3]->D_printStats(3);
    }
}
