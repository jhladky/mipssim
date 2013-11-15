#include "mipsim.hpp"
#include <cstdlib>
#include <iostream>

Stats stats;
Caches caches(0);

unsigned int signExtend16to32ui(short i) {
   return static_cast<unsigned int>(static_cast<int>(i));
}

void execute() {
   static unsigned int pctemp;
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
      cout << "newLoc: " << pctemp << endl;
      pc = pctemp;
      doJump = false;
   } else {
      pc = pctarget;
   }
   
   stats.instrs++;
   if(count++ == 200) {
      exit(1);
   }
   switch(rg.op) {
   case OP_SPECIAL:
      switch(rg.func) {
      case SP_ADDU:
         stats.numRType++;
         stats.numRegWrites++;
         stats.numRegReads += 2; //+= 2?
         
         cout << "Value of RS in ADDU: " << rt.rs << endl;
         cout << "Value of RT in ADDU: " << rt.rt << endl;
         cout << "Arg RS to ADDU: " << rf[rt.rs] << endl;
         cout << "Arg immed to ADDU: " << rf[rt.rt] << endl;
         rf.write(rt.rd, rf[rt.rs] + rf[rt.rt]);
         cout << "Result of ADDU: " << rf[rt.rd] << endl;
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
         
         pctemp = rf[rt.rd];
         doJump = true;
         break;
      case SP_JALR: //IMPLEMENT ME!!
         stats.numRType++;
         stats.numRegReads++;
         stats.numRegWrites++;

         rf.write(31, pc + 4);
         pctemp = rf[rt.rd];
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

      cout << "Value of RS in ADDIU: " << ri.rs << endl;
      cout << "Value of RT in ADDIU: " << ri.rt << endl;
      cout << "Arg RS to ADDIU: " << rf[ri.rs] << endl;
      cout << "Arg immed to ADDIU: " << signExtend16to32ui(ri.imm) << endl;
      rf.write(ri.rt, rf[ri.rs] + signExtend16to32ui(ri.imm));
      cout << "Result of ADDIU: " << rf[ri.rt] << endl;
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
      

      if(rf[ri.rs] < signExtend16to32ui(ri.imm)) {
         rf.write(ri.rt, 1U);
         cout << "SLTI: TRUE, ri.rt is: " <<  rf[ri.rt] << endl << endl;
      } else {
         rf.write(ri.rt, 0U);
         cout << "SLTI: FALSE, ri.rt is: " <<  rf[ri.rt] << endl << endl;
      }
      
   case OP_LUI: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegWrites++;

      rf.write(ri.rt, ri.imm << 16);
      break;
   case OP_LBU: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegWrites++;
      
      rf.write(ri.rt, dmem[rf[ri.rs] + ri.imm]);
      break;
   case OP_BEQ: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads += 2;
      
      if(rf[ri.rs] == rf[ri.rt]) {
         pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
         cout << "OP_BEQ setting new pc to: " << pctemp << endl;
         doJump = true;
      }
      break;
   case OP_BNE: //IMPLEMENT ME!!!
      stats.numIType++;
      stats.numRegReads += 2;

      cout << "Value of RS in BNE: " << ri.rs << endl;
      cout << "Value of RT in BNE: " << ri.rt << endl;
      cout << "Arg RS to BNE: " << rf[ri.rs] << endl;
      cout << "Arg RT to BNE: " << rf[ri.rt] << endl;
      if(rf[ri.rs] != rf[ri.rt]) {
         pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
         cout << "OP_BNE setting new pc to: "  << pctemp << endl;
         doJump = true;
      }
      
      break;
   case OP_J: //IMPLEMENT ME!!! (done possibly)
      stats.numJType++;
      
      pctemp = (pc & 0xf0000000) | (rj.target << 2);
      cout << "OP_J setting new pc to: "  << pctemp << endl;
      
      doJump = true;
      break;
   case OP_JAL: //IMPLEMENT ME!!!
      stats.numJType++;
      stats.numRegWrites++;
      
      rf.write(31, pc + 4);
      pctemp = (pc & 0xf0000000) | (rj.target << 2);
      cout << "OP_JAL setting new pc to: "  << pctemp << endl;
      doJump = true;
      break;
   default:
      cout << "Unsupported instruction: ";
      instr.printI(instr);
      exit(1);
      break;
   }
}


//Executing: 400188: op: OP_J target: 10006f
/*
  24 bits, 
we need 32 bits

 */
