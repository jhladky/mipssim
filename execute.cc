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
   static bool jumpCycle = false;
   static bool loadCycle = false;
   static int loadRegister = 0;
   static bool branchCycle = false;
   static bool followNewAddress = false;

   static int count = 0;
   Data32 instr = imem[pc];
   GenericType rg(instr);
   RType rt(instr);
   IType ri(instr);
   JType rj(instr);
   unsigned int pctarget = pc + 4;
   unsigned int addr;

   stats.cycles++;

   bool lastJumpCycle = jumpCycle;
   bool lastLoadCycle = loadCycle;
   bool lastBranchCycle = branchCycle;
   int lastLoadRegister = loadRegister;

   jumpCycle = false;
   loadCycle = false;
   branchCycle = false;
   loadRegister = 0;

   if (lastLoadCycle) {
      if (lastLoadRegister == ri.rt) {//Pretend we had a stall
      /*
      This works for now but probably should be fixed and implemented better
      We're not checking to see if ri.rt is actually being written to (in all cases with fib it is)
      Also something could be getting written to a different register
      */
         stats.numRegWrites++;
         stats.numRegReads++;
         stats.cycles++;
         stats.loadHasLoadUseStall++;
      }
   }

   if(followNewAddress) {
      pc = pctemp;
      followNewAddress = false;
   } else {
      pc = pctarget;
   }
   
   if(instr.data_uint() == 0U) {
      if (lastJumpCycle) {
         stats.hasUselessJumpDelaySlot++;
      }

      if (lastLoadCycle) {
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

      if (lastLoadCycle) {
         stats.loadHasNoLoadUseHazard++;
      }

      if (lastBranchCycle) {
         stats.hasUselessBranchDelaySlot++;
      }

      stats.instrs++;

      switch(rg.op) {
      case OP_SPECIAL:
         switch(rg.func) {
         case SP_ADDU:
            stats.numRType++;
            stats.numRegWrites++;
            stats.numRegReads += 2;

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

            pctemp = rf[rt.rs];
            followNewAddress = true;
            jumpCycle = true;
            break;
         case SP_JALR: //IMPLEMENT ME!!
            stats.numRType++;
            stats.numRegReads++;
            stats.numRegWrites++;

            rf.write(31, pc + 4);
            pctemp = rf[rt.rs];
            followNewAddress = true;
            jumpCycle = true;
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

         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         caches.access(addr);
         loadCycle = true;
         loadRegister = ri.rt;
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

         loadCycle = true;
         loadRegister = ri.rt;
         rf.write(ri.rt, dmem[rf[ri.rs] + ri.imm]);
         break;
      case OP_BEQ: //IMPLEMENT ME!!!
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
      case OP_BNE: //IMPLEMENT ME!!!
         stats.numIType++;
         stats.numBranches++;
         stats.numRegReads += 2;

         //printf("BNE: ri.rs(%d):%d != ri.rt(%d):%d\n", ri.rs, rf[ri.rs].data_uint(), ri.rt, rf[ri.rt].data_uint());
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
      case OP_J: //IMPLEMENT ME!!! (done possibly)
         stats.numJType++;

         pctemp = (pc & 0xf0000000) | (rj.target << 2);
         jumpCycle = true;
         followNewAddress = true;

         break;
      case OP_JAL: //IMPLEMENT ME!!!
         stats.numJType++;
         stats.numRegWrites++;

         rf.write(31, pc + 4);
         pctemp = (pc & 0xf0000000) | (rj.target << 2);
         jumpCycle = true;
         followNewAddress = true;

         break;
      default:
         cout << "Unsupported instruction: ";
         instr.printI(instr);
         exit(1);
         break;
      }
   }
   

}
