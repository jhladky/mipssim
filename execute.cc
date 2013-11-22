#include "mipsim.hpp"
#include <cstdlib>
#include <stdio.h>
#include <iostream>

Stats stats;
Caches caches(0);


void checkForwardAndStall(int loadRegister, int writeRegister, int readRegisterOne, int readRegisterTwo) {
//Possible debug >= 0...
   if (loadRegister > 0 && (loadRegister == readRegisterOne || loadRegister == readRegisterTwo)) {
      stats.numRegWrites++;
      stats.numRegReads++;
      stats.cycles++;
      stats.loadHasLoadUseStall++;//Seems like this should indicate forwarding not a stall...
      stats.memStageForward++;
   } else if (writeRegister > 0 && (writeRegister == readRegisterOne || writeRegister == readRegisterTwo)) {
      stats.exStageForward++;
   }
}

unsigned int signExtend16to32ui(short i) {
   return static_cast<unsigned int>(static_cast<int>(i));
}

unsigned int signExtend8to32ui(char i) {
   return static_cast<unsigned int>(static_cast<int>(i));
}

void execute() {
   static unsigned int pctemp;
   static bool jumpCycle = false;
   static int loadRegister = -1;
   static int writeRegister = -1;
   static bool branchCycle = false;
   static bool followNewAddress = false;

   Data32 instr = imem[pc];
   GenericType rg(instr);
   RType rt(instr);
   IType ri(instr);
   JType rj(instr);
   unsigned int pctarget = pc + 4;
   unsigned int addr;

   stats.cycles++;

   bool lastJumpCycle = jumpCycle;
   bool lastBranchCycle = branchCycle;
   int previousLoadRegister = loadRegister;

  // if (stats.instrs > 386447534) {
  //    opts.instrs = true;
  // }

  // if (stats.instrs > (386447534 + 1000)) {
    //  exit(1);
  // }

   if (instr.classifyType(instr) != J_TYPE) {
      checkForwardAndStall(loadRegister, writeRegister, rt.rs, rt.rt);
   }

   loadRegister = -1;
   writeRegister = -1;

   jumpCycle = false;
   branchCycle = false;

   if(followNewAddress) {
      pc = pctemp;
      followNewAddress = false;
   } else {
      pc = pctarget;
   }
   
   if(instr.data_uint() == 0U) {
      if (lastJumpCycle) {
         stats.hasUselessJumpDelaySlot++;
         //stats.memStageForward++;//This is wrong but makes the output correct for fib
      }

      if (previousLoadRegister >= 0) {
         stats.loadHasLoadUseStall++;
      }

      if (lastBranchCycle) {
         stats.hasUselessBranchDelaySlot++;
      }

      stats.numRegWrites++;
      stats.numRegReads++;
   } else {
      if (lastJumpCycle) {
         stats.hasUsefulJumpDelaySlot++;
      }

      if (previousLoadRegister >= 0) {
         stats.loadHasNoLoadUseHazard++;
      }

      if (lastBranchCycle) {
         stats.hasUsefulBranchDelaySlot++;
      }

      stats.instrs++;

      switch(rg.op) {
      case OP_SPECIAL:
         switch(rg.func) {
         case SP_ADDU:
            stats.numRType++;
            stats.numRegWrites++;
            stats.numRegReads += 2;

            rf.write(rt.rd, rf[rt.rs].data_uint() + rf[rt.rt].data_uint());
            writeRegister = rt.rd;

            break;
         case SP_SLL:
            stats.numRType++;
            stats.numRegWrites++;
            stats.numRegReads++;

            rf.write(rt.rd, rf[rt.rt].data_int() << rt.sa);
            writeRegister = rt.rd;

            break;
         case SP_JR:
            stats.numRType++;
            stats.numRegReads++;

            pctemp = rf[rt.rs];
            followNewAddress = true;
            jumpCycle = true;
            break;
         case SP_JALR:
            stats.numRType++;
            stats.numRegReads++;
            stats.numRegWrites++;

            rf.write(31, pc + 4);
            writeRegister = 31;

            pctemp = rf[rt.rs];
            followNewAddress = true;
            jumpCycle = true;
            break;
         case SP_SLT:
            stats.numRType++;
            stats.numRegReads += 2;
            stats.numRegWrites++;

            if(rf[rt.rs].data_int() < rf[rt.rt].data_int()) {
               rf.write(rt.rd, 1U);
            } else {
               rf.write(rt.rd, 0U);
            }
            writeRegister = rt.rd;

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

         rf.write(ri.rt, rf[ri.rs].data_uint() + signExtend16to32ui(ri.imm));
         writeRegister = ri.rt;

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

         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         caches.access(addr);
         rf.write(ri.rt, dmem[addr]);
         loadRegister = writeRegister = ri.rt;

         break;
      case OP_SLTI:
         stats.numIType++;
         stats.numRegReads++;
         stats.numRegWrites++;

         if(rf[ri.rs].data_int() < signExtend16to32ui(ri.imm)) {
            rf.write(ri.rt, 1U);
         } else {
            rf.write(ri.rt, 0U);
         }

         writeRegister = ri.rt;

         break;
      case OP_LUI:
         stats.numIType++;
         stats.numRegWrites++;

         rf.write(ri.rt, ri.imm << 16);
         writeRegister = ri.rt;

         break;
      case OP_BEQ:
         stats.numIType++;
         stats.numBranches++;
         stats.numRegReads += 2;

         if(rf[ri.rs] == rf[ri.rt]) {
            pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
            followNewAddress = true;

            if (pctemp < pc) {
               stats.numBackwardBranchesTaken++;
            } else {
               stats.numForwardBranchesTaken++;
            }
         } else {
            if (pctemp < pc) {
               stats.numBackwardBranchesNotTaken++;
            } else {
               stats.numForwardBranchesNotTaken++;
            }
         }
         branchCycle = true;

         break;
      case OP_BNE:
         stats.numIType++;
         stats.numBranches++;
         stats.numRegReads += 2;

         if(rf[ri.rs] != rf[ri.rt]) {
            pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
            followNewAddress = true;

            if (pctemp < pc) {
               stats.numBackwardBranchesTaken++;
            } else {
               stats.numForwardBranchesTaken++;
            }
         } else {
            if (pctemp < pc) {
               stats.numBackwardBranchesNotTaken++;
            } else {
               stats.numForwardBranchesNotTaken++;
            }
         }

         branchCycle = true;

         break;
      case OP_J:
         stats.numJType++;

         pctemp = (pc & 0xf0000000) | (rj.target << 2);
         jumpCycle = true;
         followNewAddress = true;

         break;
      case OP_JAL:
         stats.numJType++;
         stats.numRegWrites++;

         rf.write(31, pc + 4);
         writeRegister = 31;

         pctemp = (pc & 0xf0000000) | (rj.target << 2);
         jumpCycle = true;
         followNewAddress = true;

         break;
      case OP_LB: {
         stats.numMemReads++;
         stats.numIType++;
         stats.numRegReads++;
         stats.numRegWrites++;

         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         unsigned int tmp = dmem[(addr / 4) * 4] >> (3 - addr % 4) * 8 & (1 << 8) - 1;
         rf.write(ri.rt, signExtend8to32ui(tmp));
         loadRegister = writeRegister = ri.rt;
         }
         break;
      case OP_SB: {
         stats.numMemWrites++;
         stats.numIType++;
         stats.numRegReads += 2;

         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         unsigned int fromReg = rf[ri.rt] & (1 << 8) - 1;
         fromReg <<= (3 - addr % 4) * 8;
         unsigned int fromMem = dmem[addr / 4 * 4];
         fromMem &= ~(((1 << 8) - 1) << (3 - addr % 4) * 8);
         fromMem = fromMem | fromReg;
         dmem.write(addr, fromMem);
         }
         break;
      case OP_ORI: {
         stats.numIType++;
         stats.numRegWrites++;
         stats.numRegReads++;
         unsigned int tmp = ri.imm & 0x0000FFFF;
         unsigned int tmp2 = rf[ri.rs] & 0x0000FFFF;
         rf.write(ri.rt, tmp2 | tmp);
         writeRegister = ri.rt;
         }
         break;
      case OP_LBU:
         stats.numIType++;
         stats.numRegWrites++;
         stats.numRegReads++;
         stats.numMemReads++;

         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         rf.write(ri.rt, dmem[(addr / 4) * 4] >> (3 - addr % 4) * 8 & (1 << 8) - 1);

         loadRegister = writeRegister = ri.rt;

         break;
      default:
         cout << "Unsupported instruction: ";
         instr.printI(instr);
         exit(1);
         break;
      }
   }
}
