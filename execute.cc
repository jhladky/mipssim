#include "mipsim.hpp"
#include <iostream>

Stats stats;
Caches caches(0);

unsigned int signExtend16to32ui(short i) {
   return static_cast<unsigned int>(static_cast<int>(i));
}

void execute() {
   Data32 instr = imem[pc];
   GenericType rg(instr);
   RType rt(instr);
   IType ri(instr);
   JType rj(instr);
   unsigned int pctarget = pc + 4;
   unsigned int addr;
   stats.instrs++;
   pc = pctarget;
   switch(rg.op) {
   case OP_SPECIAL:
      switch(rg.func) {
      case SP_ADDU:
         stats.numRType++;
         stats.numRegWrites++;
         stats.numRegReads++; //+= 2?
         
         rf.write(rt.rd, rf[rt.rs] + rf[rt.rt]);
         break;
      case SP_SLL:
         stats.numRType++;
         stats.numRegWrites++;
         stats.numRegReads++;
         
         rf.write(rt.rd, rf[rt.rt] << rt.sa);
         break;
      case SP_JR: //IMPLEMENT ME!!!
         stats.numRType++;
         stats.numRegReads++;
         
         pctarget = rf[rt.rd];
         break;
      case SP_JALR: //IMPLEMENT ME!!
         stats.numRType++;
         stats.numRegReads++;
         stats.numRegWrites++;

         rf.write(31, pc);
         pctarget = rf[rt.rd];
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
      stats.numRegReads++;
      
      addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
      caches.access(addr);
      dmem.write(addr, rf[ri.rt]);
      break;
   case OP_LW: 
      stats.numIType++;
      stats.numMemReads++;
      stats.numRegReads++;
      
      addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
      caches.access(addr);
      rf.write(ri.rt, dmem[addr]);
      break;
   case OP_SLTI: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads++;
      stats.numRegWrites++;
      
      if(rf[ri.rt] < signExtend16to32ui(ri.imm)) {
         rf.write(ri.rs, 1);
      } else {
         rf.write(ri.rs, 0);
      }
      
   case OP_LUI: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegWrites++;

      rf.write(ri.rt, ri.imm << 16);
      break;
   case OP_BEQ: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads += 2;
      
      if(rf[ri.rs] == rf[ri.rt]) {
         pctarget += (ri.imm << 2);
      }
      break;
   case OP_BNE: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads += 2;
      
      if(rf[ri.rs] != rf[ri.rt]) {
         pctarget += (ri.imm << 2);
      }
      break;
   case OP_J: //IMPLEMENT ME!!! (done possibly)
      stats.numJType++;
      
      pctarget = (pc & 0xf0000000) | rj.target << 2;
      break;
   case OP_JAL: //IMPLEMENT ME!!!
      stats.numJType++;
      stats.numRegWrites++;
      
      rf.write(31, pc + 8);
      pctarget = (pc & 0xf0000000) | rj.target << 2;
      break;
   default:
      cout << "Unsupported instruction: ";
      instr.printI(instr);
      exit(1);
      break;
   }
}
