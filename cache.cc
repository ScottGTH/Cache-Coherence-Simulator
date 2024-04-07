/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <inttypes.h>
#include <tuple>
#include <iomanip>
#include <list>
#include <string.h>
#include <vector>
#include <bitset>
#include <string>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   mem = inval = flush = RdX = interv = Upd = 0;
   status = pos = v_block_way = 0;
   miss_rate = 0;
   hit = 4;
   
   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
   
   if(assoc > 0){
      is_asso = true;
   }else{
      is_asso = false;
   }


   
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;   /// index_extract
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }

   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
         cache[i][j].invalidate();
         cache[i][j].Lru = j;
         cache[i][j].is_valid = false;
         cache[i][j].is_dirty = false;
         cache[i][j].Dtag = 0;
      }
   }      
   
}

void Cache::WA(ulong set_pos, ulong tag_content, ulong way_pos){
   cache[set_pos][way_pos].Dtag = tag_content;
   cache[set_pos][way_pos].is_valid = true;
   cache[set_pos][way_pos].is_dirty = false;
}

void Cache::lru_update(ulong set_pos, ulong way_pos) {
    if(is_asso) {
        for (unsigned int i = 0; i < assoc; i++) {
            if (i != way_pos) {
                if (cache[set_pos][i].Lru < cache[set_pos][way_pos].Lru) {
                    cache[set_pos][i].Lru++;
                }
            }
        }
    }
    cache[set_pos][way_pos].Lru = 0;
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::D_Access(ulong pro_order, ulong n_pro,ulong addr,uchar op, Cache** cacheArray){


   currentCycle++;
   if(op == 'w') writes++;
   else          reads++;

   cacheLine * line = findLine(addr);
   cacheLine * line_other= NULL;
   bool copy_exists = false;
   for(ulong i=0;i<n_pro;i++)
   {
      if(pro_order != i)
      {
         line_other = cacheArray[i]->findLine(addr);
         
         if((line_other!=NULL))
         {
            copy_exists = true;
            break;
         }
      }        
   }

   if(line == NULL)/*miss*/
   {
      if(op == 'w') {
         writeMisses++;
         mem++;
      }
      else {
         readMisses++;
         mem++;
      }
      cacheLine *newline = D_fillLine(addr);

      

      if(op == 'w'){         
         newline->is_dirty = true;
         if(copy_exists){
            newline->setD_Flags(3);
            hit = 3;   // RD + Upd
            
            Upd++;
            setStatus(3);  /// SM
         }else{
            newline->setD_Flags(4);
            hit = 0;
            setStatus(4);  /// M
         }
      }else{
         if(copy_exists){
            newline->setD_Flags(2);
            hit = 0;
            setStatus(2);  /// SC
         }else{
            newline->setD_Flags(1);
            hit = 0;
            setStatus(1);  /// E
         }
      }
   }
   else
   {
      updateLRU(line);
      if(op == 'w'){
         line->is_dirty = true;
         if(line->getD_Flags() == 1){ // E
            line->setD_Flags(4);
            setStatus(4);
            hit = 2;   /// do nothing
         }else if(line->getD_Flags() == 2){
            hit = 1;
            Upd++;
            if(copy_exists){
              // Upd++;
               line->setD_Flags(3);
               setStatus(3);  /// SM
            }else{
               line->setD_Flags(4);
               setStatus(4);  /// M
            }
         }else if(line->getD_Flags() == 3){
            hit = 1;
            Upd++;
            
            if(copy_exists){
               //Upd++;
               line->setD_Flags(3);
               setStatus(3);  /// SM
            }else{
               
               line->setD_Flags(4);
               setStatus(4);  /// M
            }
         }else if(line->getD_Flags() == 4){
            hit = 2;
         }
      }else {
         if(line->getD_Flags() == 1){ // E
            line->setD_Flags(1);
            setStatus(1);
            hit = 2;   /// do nothing
         }else if(line->getD_Flags() == 2){
            hit = 2;
            line->setD_Flags(2);
            setStatus(2);  /// M
            
         }else if(line->getD_Flags() == 3){
            hit = 2;
               line->setD_Flags(3);
               setStatus(3);  /// SM
         }else if(line->getD_Flags() == 4){
            line->setD_Flags(4);
            setStatus(4);  /// M
            hit = 2;
         }
      }
   }
}

bool Cache::havecopy(ulong addr, uchar op){

   
   cacheLine * line = findLine(addr);
   bool judge = false;
   if(line == NULL){   // miss, invalid state
      return judge;
   }else{
      //updateLRU(line);
      judge = true;
      return judge;
   }
}

void Cache::Get_V_Block_Way(ulong set_pos){
   v_block_way = 0;
    if(is_asso){
        for(unsigned int i=1; i < assoc; i++){
            if(cache[set_pos][i].Lru > cache[set_pos][v_block_way].Lru){
               v_block_way = i;
            }
        }
    }
}

void Cache::D_Snooper(ulong addr, uchar op, ulong signal){
   /*ulong B_addr = addr >> log2Blk;
   ulong set_pos = B_addr & tagMask; 
   ulong tag_content = B_addr >> log2Sets;*/


   cacheLine * line = findLine(addr);
   if(line == NULL){   // miss, invalid state
      return;
   }else{
      //lru_update(set_pos, pos);
      if(signal == 0){   // bus read
         if(line->getD_Flags() == 1){
            line->setD_Flags(2);
            interv++;
            return;
         }else if(line->getD_Flags() == 2){
            return;
         }else if(line->getD_Flags() == 3){
            //writeBacks++;
            flush++;
            return;
         }else if(line->getD_Flags() == 4){
            //writeBacks++;
            line->setD_Flags(3);
            interv++;
            flush++;
            return;
         }
      }else if(signal == 1){   //upd
         
         if(line->getD_Flags() == 3){
            //Upd++;
            line->setD_Flags(2);
            return;
         }else if(line->getD_Flags() == 2){
            //Upd++;
            line->setD_Flags(2);
            return;
         }

      }else if(signal == 3){
         if(line->getD_Flags() == 1){
            line->setD_Flags(2);
            interv++;
            return;
         }else if(line->getD_Flags() == 3){
            //Upd++;
            line->setD_Flags(2);
            flush++;
            return;
         }else if(line->getD_Flags() == 4){
            //writeBacks++;
            interv++;
            flush++;
            line->setD_Flags(3);
            return;
         }else {
            return;
         }
      }
   }
   return;
}

void Cache::Access(ulong addr,uchar op)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
         
   if(op == 'w') writes++;
   else          reads++;

   setStatus(0);
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      if(op == 'w') writeMisses++;
      else readMisses++;

      mem++;

      cacheLine *newline = fillLine(addr);
      setStatus(1);
      if(op == 'w') 
      {
         newline->setFlags(DIRTY);
         RdX++;    
         setStatus(2);
      }
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w') {
         if(line->getFlags() == 1){
            
            //mem++;
         }
         line->setFlags(DIRTY);
         setStatus(2);
      }
   }
}

void Cache::MSI_Snooper(ulong addr, uchar op, ulong flag){   /// if is original MSI need to use flag, modifed MSI quite simple so do not need it
   cacheLine * line = findLine(addr);


   ////////////////////
   /// original MSI ///
   ////////////////////
   /*if(line == NULL){   // miss, invalid state
      return;
   }else{
      if(line->getFlags() == 1){   // have the copy, share state
         if(flag == 1){  // read, share --> share
            return;
         }else{    // readX, share --> invalid
            line->invalidate();
            return;
         }
      }else if(line->getFlags() == 2){  // have the copy, modify state
         if(flag == 1){  // read, M --> s
            line->setFlags(VALID);
            return;
         }else{    // readX, M --> invalid
            line->invalidate();
            /////// may need to add a code to flush the dirty value to MEM ///////
            return;
         }
      }else{
         return;
      }*/



   //// modified MSI ////
   if(line == NULL){   // miss, invalid state
      return;
   }else{
      if(line->getFlags() == 1){   // have the copy, share state
         line->invalidate();
         inval++;
         return;
      }else if(line->getFlags() == 2){  // have the copy, modify state
         line->invalidate();
         flush++;
         /////// may need to add a code to flush the dirty value to MEM ///////
         writeBacks++;
         inval++;
         return;
      }else{
         return;
      }
   }
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, Mpos;
   
   Mpos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         Mpos = j; 
         break; 
      }
   }
   if(Mpos == assoc) {
      return NULL;
   }
   else {
      return &(cache[i][Mpos]); 
   }
}

cacheLine * Cache::D_findLine(ulong set_pos, ulong tag_content)
{
   ulong j;
   pos = assoc;

   for(j=0; j<assoc; j++){
      if(cache[set_pos][j].is_valid) {
         if(cache[set_pos][j].Dtag == tag_content)
         {
            pos = j; 
            break; 
         }
      }
   }
   if(pos == assoc) {
      return NULL;
   }
   else {
      return &(cache[set_pos][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) { 
         return &(cache[i][j]); 
      }   
   }

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].getSeq() <= min) { 
         victim = j; 
         min = cache[i][j].getSeq();}
   } 

   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

cacheLine * Cache::DgetLRU(ulong set_pos)
{
   ulong victim = v_block_way;
   return &(cache[set_pos][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if(victim->getFlags() == DIRTY) {
      writeBack(addr);
   }

   
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

cacheLine *Cache::D_fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if(victim->getD_Flags() == 4 || victim->getD_Flags() == 3) {
      writeBack(addr);
   }

   
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   //victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(ulong num)
{ 
   miss_rate = (double) (readMisses + writeMisses) / (double) (reads + writes)*100;
   mem = mem + writeBacks;
   if(num == 0){
      printf("============ Simulation results (Cache 0) ============\n");
   }else if(num == 1){
      printf("============ Simulation results (Cache 1) ============\n");
   }else if(num == 2){
      printf("============ Simulation results (Cache 2) ============\n");
   }else if(num == 3){
      printf("============ Simulation results (Cache 3) ============\n");
   }
   
   /****print out the rest of statistics here.****/
   /****follow the ouput file format**************/
   cout << "01. number of reads:                            " << reads << endl;
	cout << "02. number of read misses:                      " << readMisses << endl;
   cout << "03. number of writes:                           " << writes << endl;
	cout << "04. number of write misses:                     " << writeMisses << endl;
   cout << "05. total miss rate:                            " << fixed << setprecision(2) << miss_rate << "%" << endl;
	cout << "06. number of writebacks:                       " << writeBacks << endl;
   cout << "07. number of memory transactions:              " << mem << endl;
	cout << "08. number of invalidations:                    " << inval << endl;
   cout << "09. number of flushes:                          " << flush << endl;
	cout << "10. number of BusRdX:                           " << RdX << endl;
}

void Cache::D_printStats(ulong num)
{ 
   miss_rate = (double) (readMisses + writeMisses) / (double) (reads + writes)*100;
   writeBacks = writeBacks + flush;
   mem = mem + writeBacks;
   if(num == 0){
      printf("============ Simulation results (Cache 0) ============\n");
   }else if(num == 1){
      printf("============ Simulation results (Cache 1) ============\n");
   }else if(num == 2){
      printf("============ Simulation results (Cache 2) ============\n");
   }else if(num == 3){
      printf("============ Simulation results (Cache 3) ============\n");
   }
   /****print out the rest of statistics here.****/
   /****follow the ouput file format**************/
   cout << "01. number of reads:                            " << reads << endl;
	cout << "02. number of read misses:                      " << readMisses << endl;
   cout << "03. number of writes:                           " << writes << endl;
	cout << "04. number of write misses:                     " << writeMisses << endl;
   cout << "05. total miss rate:                            " << fixed << setprecision(2) << miss_rate << "%" << endl;
	cout << "06. number of writebacks:                       " << writeBacks << endl;
   cout << "07. number of memory transactions:              " << mem << endl;
	cout << "08. number of interventions:                    " << interv << endl;
   cout << "09. number of flushes:                          " << flush << endl;
	cout << "10. number of Bus Transactions(BusUpd):         " << Upd << endl;
}