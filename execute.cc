#include "mipsim.hpp"
#include <cstdlib>
#include <stdio.h>
#include <iostream>

Stats stats;
Caches caches(0);


void checkForward(int memReg, int exReg, int readRegisterOne, int readRegisterTwo) {
   if (exReg > 0 && exReg == readRegisterOne) {
      stats.exStageForward++;
   } else if (memReg > 0 && memReg == readRegisterOne) {
      stats.memStageForward++;
   }

   if (exReg > 0 && exReg == readRegisterTwo) {
      stats.exStageForward++;
   } else if (memReg > 0 && memReg == readRegisterTwo) {
      stats.memStageForward++;
   }
}

unsigned int signExtend16to32ui(short i) {
   return static_cast<unsigned int>(static_cast<int>(i));
}

void execute() {
   static unsigned int pctemp;
   static bool jumpCycle = false;
   static int writeRegisters[] = {-1, -1, -1};
   static bool branchCycle = false;
   static bool loadCycle = false;
   static bool followNewAddress = false;

   Data32 instr = imem[pc];
   GenericType rg(instr);
   RType rt(instr);
   IType ri(instr);
   JType rj(instr);
   unsigned int pctarget = pc + 4;
   unsigned int addr;
   const unsigned int mask = (1 << 8) - 1;
   unsigned int tmp;
   Data32 tmp2(0);

   stats.cycles++;

   bool lastJumpCycle = jumpCycle;
   bool lastBranchCycle = branchCycle;
   bool lastLoadCycle = loadCycle;
   writeRegisters[2] = writeRegisters[1];
   writeRegisters[1] = writeRegisters[0];
   writeRegisters[0] = -1;

   jumpCycle = false;
   branchCycle = false;
   loadCycle = false;

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
         stats.loadHasNoLoadUseHazard++;
      }

      if (lastBranchCycle) {
         stats.hasUselessBranchDelaySlot++;
      }

      writeRegisters[0] = 0;
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

            checkForward(writeRegisters[2], writeRegisters[1], rt.rs, rt.rt);
            rf.write(rt.rd, rf[rt.rs].data_uint() + rf[rt.rt].data_uint());
            writeRegisters[0] = rt.rd;

            break;
         case SP_SLL:
            stats.numRType++;
            stats.numRegWrites++;
            stats.numRegReads++;

            checkForward(writeRegisters[2], writeRegisters[1], -1, rt.rt);
            rf.write(rt.rd, rf[rt.rt].data_int() << rt.sa);
            writeRegisters[0] = rt.rd;

            break;
         case SP_JR:
            stats.numRType++;
            stats.numRegReads++;

            checkForward(writeRegisters[2], writeRegisters[1], rt.rs, -1);
            pctemp = rf[rt.rs];
            followNewAddress = true;
            jumpCycle = true;
            break;
         case SP_JALR:
            stats.numRType++;
            stats.numRegReads++;
            stats.numRegWrites++;

            rf.write(31, pc + 4);
            writeRegisters[0] = 31;

            checkForward(writeRegisters[2], writeRegisters[1], rt.rs, -1);
            pctemp = rf[rt.rs];
            followNewAddress = true;
            jumpCycle = true;
            break;
         case SP_SLT:
            stats.numRType++;
            stats.numRegReads += 2;
            stats.numRegWrites++;

            checkForward(writeRegisters[2], writeRegisters[1], rt.rs, rt.rt);
            if(rf[rt.rs].data_int() < rf[rt.rt].data_int()) {
               rf.write(rt.rd, 1U);
            } else {
               rf.write(rt.rd, 0U);
            }
            writeRegisters[0] = rt.rd;


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

         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, -1);
         rf.write(ri.rt, rf[ri.rs].data_uint() + signExtend16to32ui(ri.imm));
         writeRegisters[0] = ri.rt;

         break;
      case OP_SW:
         stats.numIType++;
         stats.numMemWrites++;
         stats.numRegReads += 2;

         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, ri.rt);
         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         caches.access(addr);
         dmem.write(addr, rf[ri.rt]);

         break;
      case OP_LW:
         stats.numIType++;
         stats.numMemReads++;
         stats.numRegReads++;
         stats.numRegWrites++;

         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, -1);
         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         caches.access(addr);
         rf.write(ri.rt, dmem[addr]);
         writeRegisters[0] = ri.rt;
         loadCycle = true;

         break;
      case OP_SLTI:
         stats.numIType++;
         stats.numRegReads++;
         stats.numRegWrites++;

         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, -1);
         if(rf[ri.rs].data_int() < signExtend16to32ui(ri.imm)) {
            rf.write(ri.rt, 1U);

         } else {
            rf.write(ri.rt, 0U);
         }
         writeRegisters[0] = ri.rt;


         break;
      case OP_LUI:
         stats.numIType++;
         stats.numRegWrites++;

         rf.write(ri.rt, ri.imm << 16);
         writeRegisters[0] = ri.rt;

         break;
      case OP_BEQ:
         stats.numIType++;
         stats.numBranches++;
         stats.numRegReads += 2;

         pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, ri.rt);
         if(rf[ri.rs] == rf[ri.rt]) {
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

         pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, ri.rt);
         if(rf[ri.rs] != rf[ri.rt]) {
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
      case OP_BLEZ:
         stats.numIType++;
         stats.numBranches++;
         stats.numRegReads += 2; //Is right
         //stats.numRegReads++; //Should be right

         pctemp = pc + (signExtend16to32ui(ri.imm) << 2);
         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, -1);
         if(rf[ri.rs] <= 0) {
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
         writeRegisters[0] = 31;

         pctemp = (pc & 0xf0000000) | (rj.target << 2);
         jumpCycle = true;
         followNewAddress = true;

         break;
      case OP_LB:
         stats.numMemReads++;
         stats.numIType++;
         stats.numRegReads++;
         stats.numRegWrites++;
         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, -1);
         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         tmp2 = dmem[addr];
         rf.write(ri.rt, signExtend16to32ui(tmp2.data_ubyte4(0)));
         writeRegisters[0] = ri.rt;
         loadCycle = true;

         break;
      case OP_SB: {
         stats.numMemWrites++;
         stats.numIType++;
         stats.numRegReads += 2;
         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, ri.rt);
         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         tmp = rf[ri.rt] & mask;

         tmp2 = dmem[addr];
         tmp2.set_data_ubyte4(0, tmp);
         dmem.write(addr, tmp2);

         break;
      }
      case OP_ORI:
         stats.numIType++;
         stats.numRegWrites++;
         stats.numRegReads++;

         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, -1);
         tmp = ri.imm & ((1<<16) - 1);
         rf.write(ri.rt, rf[ri.rs] | tmp);
         writeRegisters[0] = ri.rt;

         break;
      case OP_LBU:
         stats.numIType++;
         stats.numRegWrites++;
         stats.numRegReads++;
         stats.numMemReads++;

         checkForward(writeRegisters[2], writeRegisters[1], ri.rs, -1);
         addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
         tmp2 = dmem[addr];
         rf.write(ri.rt, tmp2.data_ubyte4(0));
         writeRegisters[0] = ri.rt;
         loadCycle = true;

         break;
      default:
         cout << "Unsupported instruction: ";
         instr.printI(instr);
         exit(1);
         break;
      }
   }
}
