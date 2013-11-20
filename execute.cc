#include "mipsim.hpp"
#include <cstdlib>
#include <stdio.h>
#include <iostream>

Stats stats;
Caches caches(0);

unsigned int signExtend16to32ui(short i) {
   return static_cast<unsigned int>(static_cast<int>(i));
}

void execute() {
   static unsigned int pctemp;
   static bool wasLoad = false;
   static bool doJump = false;
   static int count = 0;
   Data32 instr = imem[pc];
   GenericType rg(instr);
   RType rt(instr);
   IType ri(instr);
   JType rj(instr);
   unsigned int pctarget = pc + 4;
   unsigned int addr;
   
   if(doJump) {
      //cout << "newLoc: " << pctemp << endl;
      stats.numRegWrites++;
      stats.numRegReads++;
      pc = pctemp;
      doJump = false;
   } else {
      pc = pctarget;
   }
   
   if(instr.data_uint() == 0U) {
      wasLoad = false;
      return;
   } else {
      stats.loadHasLoadUseStall += wasLoad;
      wasLoad = false;
      stats.instrs++;
   }
   
   switch(rg.op) {
   case OP_SPECIAL:
      switch(rg.func) {
      case SP_ADDU:
         stats.numRType++;
         stats.numRegWrites++;
         stats.numRegReads += 2; //+= 2?
         
         rf.write(rt.rd, rf[rt.rs] + rf[rt.rt]);
         break;
      case SP_SLL:
         stats.numRType++;
         stats.numRegWrites++;
         stats.numRegReads++;
         
         rf.write(rt.rd, rf[rt.rt] << rt.sa);
         break;
      case SP_JR: //IMPLEMENT ME!!!
         printf("blah\n");
         stats.numRType++;
         stats.numRegReads++;
         
         pctemp = rf[rt.rs];
         doJump = true;
         break;
      case SP_JALR: //IMPLEMENT ME!!
         printf("blah2\n");
         stats.numRType++;
         stats.numRegReads++;
         stats.numRegWrites++;
         
         rf.write(31, pc + 4);
         pctemp = rf[rt.rs];
         doJump = true;
         break;
      default:
         cout << "Unsupported instruction: ";
         instr.printI(instr);
         exit(1);
         break;
      }
      break;
   case OP_ADDIU:
      stats.numIType++;
      stats.numRegWrites++;
      stats.numRegReads++;

      rf.write(ri.rt, rf[ri.rs] + signExtend16to32ui(ri.imm));

      break;
   case OP_SW:
      stats.numIType++;
      stats.numMemWrites++;
      stats.numRegReads += 2;
      
      addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
      caches.access(addr);
      dmem.write(addr, rf[ri.rt]);
      break;
   case OP_LW: 
      stats.numIType++;
      stats.numMemReads++;
      stats.numRegReads++;
      stats.numRegWrites++;
      
      wasLoad = true;
      addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
      caches.access(addr);
      rf.write(ri.rt, dmem[addr]);
      break;
   case OP_SLTI: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads++;
      stats.numRegWrites++;

      //printf("STLI: comp, ri.rt(%d): %d < %d\n", ri.rt, rf[ri.rt].data_uint(), signExtend16to32ui(ri.imm));
      if(rf[ri.rs] < signExtend16to32ui(ri.imm)) {
         rf.write(ri.rt, 1U);
         //printf("STLI: TRUE, value of ri.rt(%d): %d\n", ri.rt, rf[ri.rt].data_uint());
      } else {
         rf.write(ri.rt, 0U);
         //printf("STLI: TRUE, value of ri.rt(%d): %d\n", ri.rt, rf[ri.rt].data_uint());
      }
      break;
   case OP_LUI: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegWrites++;

      rf.write(ri.rt, ri.imm << 16);
      break;
   case OP_LBU: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegWrites++;
      stats.numRegReads++;
      stats.numMemReads++;
      
      rf.write(ri.rt, dmem[rf[ri.rs] + ri.imm]);
      break;
   case OP_BEQ: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads += 2;
      
      if(rf[ri.rs] == rf[ri.rt]) {
         pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
         doJump = true;
      }
      break;
   case OP_BNE: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads += 2;

      //printf("BNE: ri.rs(%d):%d != ri.rt(%d):%d\n", ri.rs, rf[ri.rs].data_uint(), ri.rt, rf[ri.rt].data_uint());
      if(rf[ri.rs] != rf[ri.rt]) {
         pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
         doJump = true;
      }
      
      break;
   case OP_J: //IMPLEMENT ME!!! (done possibly)
      stats.numJType++;
      
      pctemp = (pc & 0xf0000000) | (rj.target << 2);
      doJump = true;

      break;
   case OP_JAL: //IMPLEMENT ME!!!
      stats.numJType++;
      stats.numRegWrites++;
      
      rf.write(31, pc + 4);
      pctemp = (pc & 0xf0000000) | (rj.target << 2);
      doJump = true;

      break;
   default:
      cout << "Unsupported instruction: ";
      instr.printI(instr);
      exit(1);
      break;
   }
}
