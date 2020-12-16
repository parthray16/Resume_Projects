#include "thumbsim.hpp"
#include <list>
#include <iostream>
#include <iterator>
#include <map>
#include <stdio.h>

// These are just the register NUMBERS
#define PC_REG 15  
#define LR_REG 14
#define SP_REG 13

// These are the contents of those registers
#define PC rf[PC_REG]
#define LR rf[LR_REG]
#define SP rf[SP_REG]

Stats stats;
Caches caches(0);


unsigned int bitcount(unsigned short n){
    unsigned int count = 0;
    while(n){
        count += n & 1;
        n >>= 1;
    }
    return count;
}
//  1 strong_taken
//  2 weak_taken
//  3 weak_nottaken
//  4 strong_nottaken
typedef struct BTAC {
    int prediction;
    int branch_target;
} BTAC;

typedef struct HazardData{
  bool defs; // does the instruction define any registers
  unsigned int defreg; //what reg defined
  bool load; //is this a load
} HazardData;

list <HazardData> HazardList;
map<unsigned int, BTAC> btacmap;

void checkLoadUse(list<HazardData>* HazardList, unsigned int rt, unsigned int rm){
  int i = 1;
  if(HazardList->empty()){
    stats.cycles++;
    return;
  }
  list<HazardData> :: iterator it;
  for (it = HazardList->begin(); i <= HazardList->size(); it++){
    if(it->defs){
      if((rt == it->defreg || rm == it->defreg) && it->load){
        stats.cycles += 4 - i;
        it->defs = false;
        return;
      }
    }
    i++;
  }
  stats.cycles++;
  return;
}

void checkEarlyReg(list<HazardData>* HazardList, unsigned int rt, unsigned int rm){
  if(HazardList->empty()){
    return;
  }
  if(HazardList->front().defs){
      if((rt == HazardList->front().defreg || rm == HazardList->front().defreg) && !(HazardList->front().load)){
        stats.cycles++;
        return;
      }  
  }
  return;
}

// CPE 315: you'll need to implement a custom sign-extension function
// in addition to the ones given below, specifically for the unconditional
// branch instruction, which has an 11-bit immediate field
unsigned int signExtend16to32ui(short i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}

unsigned int signExtend8to32ui(char i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}

unsigned int signExtend11to32ui(unsigned short i){
  unsigned int mask0 = 0;
  unsigned int mask1 = 4294965248;
  unsigned int mask = 1024;
  if ((i & mask) == 0){
    return i + mask0;
  }
  return i + mask1;
}

// This is the global object you'll use to store condition codes N,Z,V,C
// Set these bits appropriately in execute below.
ASPR flags;

// CPE 315: You need to implement a function to set the Negative and Zero
// flags for each instruction that does that. It only needs to take
// one parameter as input, the result of whatever operation is executing
void setZeroNeg(int num){
  flags.N = 0;
  flags.Z = 0;
  if(num < 0){
    flags.N = 1;
  }
  if(num == 0){
    flags.Z = 1;
  }
}
// This function is complete, you should not have to modify it
void setCarryOverflow (int num1, int num2, OFType oftype) {
  switch (oftype) {
    case OF_ADD:
      if (((unsigned long long int)num1 + (unsigned long long int)num2) ==
          ((unsigned int)num1 + (unsigned int)num2)) {
        flags.C = 0;
      }
      else {
        flags.C = 1;
      }
      if (((long long int)num1 + (long long int)num2) ==
          ((int)num1 + (int)num2)) {
        flags.V = 0;
      }
      else {
        flags.V = 1;
      }
      break;
    case OF_SUB:
      if (num1 >= num2) {
        flags.C = 1;
      }
      else if ((long long int)((unsigned long long int)num1 - (unsigned long long int)num2) ==
          (int)((unsigned int)num1 - (unsigned int)num2)) {
        flags.C = 0;
      }
      else {
        flags.C = 1;
      }
      if (((num1==0) && (num2==0)) ||
          (((long long int)num1 - (long long int)num2) ==
           ((int)num1 - (int)num2))) {
        flags.V = 0;
      }
      else {
        flags.V = 1;
      }
      break;
    case OF_SHIFT:
      // C flag unaffected for shifts by zero
      if (num2 != 0) {
        if (((unsigned long long int)num1 << (unsigned long long int)num2) ==
            ((unsigned int)num1 << (unsigned int)num2)) {
          flags.C = 0;
        }
        else {
          flags.C = 1;
        }
      }
      // Shift doesn't set overflow
      break;
    default:
      cerr << "Bad OverFlow Type encountered." << __LINE__ << __FILE__ << endl;
      exit(1);
  }
}

// CPE 315: You're given the code for evaluating BEQ, and you'll need to 
// complete the rest of these conditions. See Page 208 of the armv7 manual
static int checkCondition(unsigned short cond) {
  switch(cond) {
    case EQ:
      if (flags.Z == 1) {
        return TRUE;
      }
      break;
    case NE:
      if (flags.Z == 0) {
        return TRUE;
      }
      break;
    case CS:
      if (flags.C == 1) {
        return TRUE;
      }
      break;
    case CC:
      if (flags.C == 0) {
        return TRUE;
      }
      break;
    case MI:
      if (flags.N == 1) {
        return TRUE;
      }
      break;
    case PL:
      if (flags.N == 0) {
        return TRUE;
      }
      break;
    case VS:
      if (flags.V == 1) {
        return TRUE;
      }
      break;
    case VC:
      if (flags.V == 0) {
        return TRUE;
      }
      break;
    case HI:
      if (flags.C == 1 && flags.Z == 0) {
        return TRUE;
      }
      break;
    case LS:
      if (flags.C == 0 || flags.Z == 1) {
        return TRUE;
      }
      break;
    case GE:
      if (flags.N == flags.V) {
        return TRUE;
      }
      break;
    case LT:
      if (flags.N != flags.V) {
        return TRUE;
      }
      break;
    case GT:
      if (flags.Z == 0 && (flags.N == flags.V)) {
        return TRUE;
      }
      break;
    case LE:
      if (flags.Z == 1 || (flags.N != flags.V)) {
        return TRUE;
      }
      break;
    case AL:
      return TRUE;
      break;
  }
  return FALSE;
}

void execute() {
  Data16 instr = imem[PC];
  Data16 instr2;
  Data32 temp(0); // Use this for STRB instructions
  Thumb_Types itype;
  // the following counts as a read to PC
  unsigned int pctarget = PC + 2;
  stats.numRegReads++;
  unsigned int addr;
  int i, n, offset;
  unsigned int list1, mask;
  int num1, num2, result, BitCount;
  unsigned int bit;
  HazardData saveInstr;
  list<HazardData> :: iterator it; 
  map<unsigned int, BTAC> :: iterator itmap; 
  unsigned int btacidx;
  BTAC entry;

  /* Convert instruction to correct type */
  /* Types are described in Section A5 of the armv7 manual */
  BL_Type blupper(instr);
  ALU_Type alu(instr);
  SP_Type sp(instr);
  DP_Type dp(instr);
  LD_ST_Type ld_st(instr);
  MISC_Type misc(instr);
  COND_Type cond(instr);
  UNCOND_Type uncond(instr);
  LDM_Type ldm(instr);
  STM_Type stm(instr);
  LDRL_Type ldrl(instr);
  ADD_SP_Type addsp(instr);

  BL_Ops bl_ops;
  ALU_Ops add_ops;
  DP_Ops dp_ops;
  SP_Ops sp_ops;
  LD_ST_Ops ldst_ops;
  MISC_Ops misc_ops;

  // This counts as a write to the PC register
  rf.write(PC_REG, pctarget);
  stats.numRegWrites++;

  itype = decode(ALL_Types(instr));

  // CPE 315: The bulk of your work is in the following switch statement
  // All instructions will need to have stats and cache access info added
  // as appropriate for that instruction.
  
  switch(itype) {
    case ALU:
      add_ops = decode(alu);
      switch(add_ops) {
        case ALU_LSLI:
          setCarryOverflow(rf[alu.instr.lsli.rm], alu.instr.lsli.imm, OF_SHIFT);
          setZeroNeg(rf[alu.instr.lsli.rm] << alu.instr.lsli.imm);
          rf.write(alu.instr.lsli.rd, rf[alu.instr.lsli.rm] << alu.instr.lsli.imm);
          stats.numRegWrites++;
          stats.numRegReads++;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.lsli.rm, 60);
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.lsli.rd;
          saveInstr.load = false;
          break;
        case ALU_ADDR:
          setCarryOverflow(rf[alu.instr.addr.rn], rf[alu.instr.addr.rm], OF_ADD);
          setZeroNeg(rf[alu.instr.addr.rn] + rf[alu.instr.addr.rm]);
          rf.write(alu.instr.addr.rd, rf[alu.instr.addr.rn] + rf[alu.instr.addr.rm]);
          stats.numRegWrites++;
          stats.numRegReads += 2;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.addr.rn, alu.instr.addr.rm);
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.addr.rd;
          saveInstr.load = false;
          break;
        case ALU_SUBR:
          setCarryOverflow(rf[alu.instr.subr.rn], rf[alu.instr.subr.rm], OF_SUB);
          setZeroNeg(rf[alu.instr.subr.rn] - rf[alu.instr.subr.rm]);        
          rf.write(alu.instr.subr.rd, rf[alu.instr.subr.rn] - rf[alu.instr.subr.rm]);
          stats.numRegWrites++;
          stats.numRegReads += 2;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.subr.rn, alu.instr.subr.rm);
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.subr.rd;
          saveInstr.load = false;
          break;
        case ALU_ADD3I:
          setCarryOverflow(rf[alu.instr.add3i.rn], alu.instr.add3i.imm, OF_ADD);
          setZeroNeg(rf[alu.instr.add3i.rn] + alu.instr.add3i.imm);
          rf.write(alu.instr.add3i.rd, rf[alu.instr.add3i.rn] + alu.instr.add3i.imm);
          stats.numRegWrites++;
          stats.numRegReads += 1;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.add3i.rn, 60);
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.add3i.rd;
          saveInstr.load = false;
          break;
        case ALU_SUB3I:
          setCarryOverflow(rf[alu.instr.sub3i.rn], alu.instr.sub3i.imm, OF_SUB);
          setZeroNeg(rf[alu.instr.sub3i.rn] - alu.instr.sub3i.imm);
          rf.write(alu.instr.sub3i.rd, rf[alu.instr.sub3i.rn] - alu.instr.sub3i.imm);
          stats.numRegWrites++;
          stats.numRegReads += 1;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.add3i.rn, 60);
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.sub3i.rd;
          saveInstr.load = false;
          break;
        case ALU_MOV:
          setZeroNeg(alu.instr.mov.imm);
          rf.write(alu.instr.mov.rdn, alu.instr.mov.imm);
          stats.numRegWrites++;
          stats.instrs++;
          stats.cycles++;
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.mov.rdn;
          saveInstr.load = false;
          break;
        case ALU_CMP:
          setCarryOverflow(rf[alu.instr.cmp.rdn], alu.instr.cmp.imm, OF_SUB);
          setZeroNeg(rf[alu.instr.cmp.rdn] - alu.instr.cmp.imm);
          stats.numRegReads += 1;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.cmp.rdn, 60);
          saveInstr.defs = false;
          saveInstr.defreg = -1;
          saveInstr.load = false;
          break;
        case ALU_ADD8I:
          setCarryOverflow(rf[alu.instr.add8i.rdn], alu.instr.add8i.imm, OF_ADD);
          setZeroNeg(rf[alu.instr.add8i.rdn] + alu.instr.add8i.imm);
          rf.write(alu.instr.add8i.rdn, rf[alu.instr.add8i.rdn] + alu.instr.add8i.imm);
          stats.numRegWrites++;
          stats.numRegReads += 1;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.add8i.rdn, 60);
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.add8i.rdn;
          saveInstr.load = false;
          break;
        case ALU_SUB8I:
          setCarryOverflow(rf[alu.instr.sub8i.rdn], alu.instr.sub8i.imm, OF_SUB);
          setZeroNeg(rf[alu.instr.sub8i.rdn] - alu.instr.sub8i.imm);
          rf.write(alu.instr.sub8i.rdn, rf[alu.instr.sub8i.rdn] - alu.instr.sub8i.imm);
          stats.numRegWrites++;
          stats.numRegReads += 1;
          stats.instrs++;
          checkLoadUse(&HazardList, alu.instr.sub8i.rdn, 60);
          saveInstr.defs = true;
          saveInstr.defreg = alu.instr.sub8i.rdn;
          saveInstr.load = false;
          break;
        default:
          cout << "instruction not implemented" << endl;
          exit(1);
          break;
      }
      break;
    case BL: 
      // This instruction is complete, nothing needed here
      stats.instrs++;
      bl_ops = decode(blupper);
      if (bl_ops == BL_UPPER) {
        // PC has already been incremented above
        instr2 = imem[PC];
        BL_Type bllower(instr2);
        if (blupper.instr.bl_upper.s) {
          addr = static_cast<unsigned int>(0xff<<24) | 
            ((~(bllower.instr.bl_lower.j1 ^ blupper.instr.bl_upper.s))<<23) |
            ((~(bllower.instr.bl_lower.j2 ^ blupper.instr.bl_upper.s))<<22) |
            ((blupper.instr.bl_upper.imm10)<<12) |
            ((bllower.instr.bl_lower.imm11)<<1);
        }
        else {
          addr = ((blupper.instr.bl_upper.imm10)<<12) |
            ((bllower.instr.bl_lower.imm11)<<1);
        }
        // return address is 4-bytes away from the start of the BL insn
        rf.write(LR_REG, PC + 2);
        // Target address is also computed from that point
        rf.write(PC_REG, PC + 2 + addr);

        stats.numRegReads += 1;
        stats.numRegWrites += 2;
        stats.cycles++;
        saveInstr.defs = false;
        saveInstr.defreg = -1;
        saveInstr.load = false;
      }
      else {
        cerr << "Bad BL format." << endl;
        exit(1);
      }
      break;
    case DP:
      dp_ops = decode(dp);
      switch(dp_ops) {
        case DP_CMP:
          // need to implement
          setCarryOverflow(rf[dp.instr.DP_Instr.rdn], rf[dp.instr.DP_Instr.rm], OF_SUB);
          setZeroNeg(rf[dp.instr.DP_Instr.rdn] - rf[dp.instr.DP_Instr.rm]);
          stats.numRegReads += 2;
          stats.instrs++;
          checkLoadUse(&HazardList, dp.instr.DP_Instr.rdn, dp.instr.DP_Instr.rm);
          saveInstr.defs = false;
          saveInstr.defreg = -1;
          saveInstr.load = false;
          break;
        case DP_MUL:
          setZeroNeg(rf[dp.instr.DP_Instr.rdn] * rf[dp.instr.DP_Instr.rm]);
          rf.write(dp.instr.DP_Instr.rdn, rf[dp.instr.DP_Instr.rdn] * rf[dp.instr.DP_Instr.rm]);
          stats.numRegReads+=2;
          stats.numRegWrites+=1;
          stats.instrs++;
          saveInstr.defs = true;
          saveInstr.defreg = dp.instr.DP_Instr.rdn;
          saveInstr.load = true;
          i = 0;
          if(HazardList.empty()){
            stats.cycles++;
          }
          else{
            for (it = HazardList.begin(); i < HazardList.size(); it++){
              if(it->defs){
                if((dp.instr.DP_Instr.rdn == it->defreg || dp.instr.DP_Instr.rm == it->defreg) && it->load){
                  stats.cycles += 4 - i;
                  it->defs = false;
                  break;
                }
              }
              i++;
            }
            if (i == HazardList.size()){
              stats.cycles++;
            }
          }  
      }
      break;
    case SPECIAL:
      sp_ops = decode(sp);
      switch(sp_ops) {
        case SP_MOV:
          // needs stats and flags
          setZeroNeg(rf[sp.instr.mov.rm]);
          rf.write((sp.instr.mov.d << 3 ) | sp.instr.mov.rd, rf[sp.instr.mov.rm]);
          stats.numRegWrites++;
          stats.numRegReads++;
          stats.instrs++;
          checkLoadUse(&HazardList, sp.instr.mov.rm, 60);
          saveInstr.defs = true;
          saveInstr.defreg = (sp.instr.mov.d << 3 ) | sp.instr.mov.rd;
          saveInstr.load = false;
          break;
        case SP_ADD:
          setCarryOverflow(rf[(sp.instr.add.d << 3 ) | sp.instr.add.rd], rf[sp.instr.add.rm], OF_ADD);
          setZeroNeg(rf[(sp.instr.add.d << 3 ) | sp.instr.add.rd] + rf[sp.instr.add.rm]);
          rf.write((sp.instr.add.d << 3 ) | sp.instr.add.rd, rf[(sp.instr.add.d << 3 ) | sp.instr.add.rd] + rf[sp.instr.add.rm]);
          stats.numRegWrites++;
          stats.numRegReads += 2;
          stats.instrs++;
          checkLoadUse(&HazardList, (sp.instr.add.d << 3 ) | sp.instr.add.rd, sp.instr.add.rm);
          saveInstr.defs = true;
          saveInstr.defreg = (sp.instr.add.d << 3 ) | sp.instr.add.rd;
          saveInstr.load = false;
          break;
        case SP_CMP:
          // need to implement these
          setCarryOverflow(rf[(sp.instr.cmp.d << 3 ) | sp.instr.cmp.rd], rf[sp.instr.cmp.rm], OF_SUB);
          setZeroNeg(rf[(sp.instr.cmp.d << 3 ) | sp.instr.cmp.rd] - rf[sp.instr.cmp.rm]);
          stats.numRegReads += 2;
          stats.instrs++;
          checkLoadUse(&HazardList, (sp.instr.cmp.d << 3 ) | sp.instr.cmp.rd, sp.instr.cmp.rm);
          saveInstr.defs = false;
          saveInstr.defreg = -1;
          saveInstr.load = false;
          break;
      }
      break;
    case LD_ST:
      // You'll want to use these load and store models
      // to implement ldrb/strb, ldm/stm and push/pop
      ldst_ops = decode(ld_st);
      switch(ldst_ops) {
        case STRI:
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;
          dmem.write(addr, rf[ld_st.instr.ld_st_imm.rt]);
          caches.access(addr);
          stats.numMemWrites++;
          stats.numRegReads += 2;
          stats.instrs++;
          checkLoadUse(&HazardList, ld_st.instr.ld_st_imm.rt, 60);
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_imm.rn, 60);
          saveInstr.defs = false;
          saveInstr.defreg = -1;
          saveInstr.load = false;
          break;
        case LDRI:
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;
          rf.write(ld_st.instr.ld_st_imm.rt, dmem[addr]);
          caches.access(addr);
          stats.numRegWrites++;
          stats.numMemReads++;
          stats.numRegReads++;
          stats.instrs++;
          saveInstr.defs = true;
          saveInstr.defreg = ld_st.instr.ld_st_imm.rt;
          saveInstr.load = true;
          i = 0;
          if(HazardList.empty()){
            stats.cycles++;
          }
          else{
            for (it = HazardList.begin(); i < HazardList.size(); it++){
              if(it->defs){
                if(ld_st.instr.ld_st_imm.rn == it->defreg && it->load){
                  stats.cycles += 4 - i;
                  it->defs = false;
                  break;
                }
              }
              i++;
            }
            if (i == HazardList.size()){
              stats.cycles++;
            }
          }
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_imm.rn, 60);
          break;
        case STRR:
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          dmem.write(addr, rf[ld_st.instr.ld_st_reg.rt]);
          caches.access(addr);
          stats.numMemWrites++;
          stats.numRegReads += 3;
          stats.instrs++;
          checkLoadUse(&HazardList, ld_st.instr.ld_st_reg.rt, 60);
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm);
          saveInstr.defs = false;
          saveInstr.defreg = -1;
          saveInstr.load = false;
          break;
        case LDRR:
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          rf.write(ld_st.instr.ld_st_reg.rt, dmem[addr]);
          caches.access(addr);
          stats.numRegWrites++;
          stats.numMemReads++;
          stats.numRegReads += 2;
          stats.instrs++;
          saveInstr.defs = true;
          saveInstr.defreg = ld_st.instr.ld_st_reg.rt;
          saveInstr.load = true;
          i = 0;
          if(HazardList.empty()){
            stats.cycles++;
          }
          else{
            for (it = HazardList.begin(); i < HazardList.size(); it++){
              if(it->defs){
                if((ld_st.instr.ld_st_reg.rn == it->defreg || ld_st.instr.ld_st_reg.rm == it->defreg) && it->load){
                  stats.cycles += 4 - i;
                  it->defs = false;
                  break;
                }
              }
              i++;
            }
            if (i == HazardList.size()){
              stats.cycles++;
            }
          }
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm);
          break;
        case STRBI:
          // functionally complete, needs stats
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm;
          temp = dmem[addr];
          temp.set_data_ubyte4(0, rf[ld_st.instr.ld_st_imm.rt]&0xFF);
          dmem.write(addr, temp);
          caches.access(addr);
          stats.numMemWrites++;
          stats.numRegReads += 2;
          stats.instrs++;
          checkLoadUse(&HazardList, ld_st.instr.ld_st_imm.rt, 60);
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_imm.rn, 60);
          saveInstr.defs = false;
          saveInstr.defreg = -1;
          saveInstr.load = false;
          break;
        case LDRBI:
          // need to implement
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm;
          temp = dmem[addr];
          rf.write(ld_st.instr.ld_st_imm.rt, temp.data_ubyte4(0));
          caches.access(addr);
          stats.numRegWrites++;
          stats.numMemReads++;
          stats.numRegReads++;
          stats.instrs++;
          saveInstr.defs = true;
          saveInstr.defreg = ld_st.instr.ld_st_imm.rt;
          saveInstr.load = true;
          i = 0;
          if(HazardList.empty()){
            stats.cycles++;
          }
          else{
            for (it = HazardList.begin(); i < HazardList.size(); it++){
              if(it->defs){
                if(ld_st.instr.ld_st_imm.rn == it->defreg && it->load){
                  stats.cycles += 4 - i;
                  it->defs = false;
                  break;
                }
              }
              i++;
            }
            if (i == HazardList.size()){
              stats.cycles++;
            }
          }
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_imm.rn, 60);
          break;
        case STRBR:
          // need to implement
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          temp = dmem[addr];
          temp.set_data_ubyte4(0, rf[ld_st.instr.ld_st_reg.rt]&0xFF);
          dmem.write(addr, temp);
          caches.access(addr);
          stats.numMemWrites++;
          stats.numRegReads += 3;
          stats.instrs++;
          checkLoadUse(&HazardList, ld_st.instr.ld_st_reg.rt, 60);
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm);
          saveInstr.defs = false;
          saveInstr.defreg = -1;
          saveInstr.load = false;
          break;
        case LDRBR:
          // need to implement
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          temp = dmem[addr];
          rf.write(ld_st.instr.ld_st_reg.rt, temp.data_ubyte4(0));
          caches.access(addr);
          stats.numRegWrites++;
          stats.numMemReads++;
          stats.numRegReads += 2;
          stats.instrs++;
          saveInstr.defs = true;
          saveInstr.defreg = ld_st.instr.ld_st_reg.rt;
          saveInstr.load = true;
          i = 0;
          if(HazardList.empty()){
            stats.cycles++;
          }
          else{
            for (it = HazardList.begin(); i < HazardList.size(); it++){
              if(it->defs){
                if((ld_st.instr.ld_st_reg.rn == it->defreg || ld_st.instr.ld_st_reg.rm == it->defreg) && it->load){
                  stats.cycles += 4 - i;
                  it->defs = false;
                  break;
                }
              }
              i++;
            }
            if (i == HazardList.size()){
              stats.cycles++;
            }
          }
          checkEarlyReg(&HazardList, ld_st.instr.ld_st_reg.rn, ld_st.instr.ld_st_reg.rm);
          break;
      }
      break;
    case MISC:
      misc_ops = decode(misc);
      switch(misc_ops) {
        case MISC_PUSH:
            BitCount = bitcount(misc.instr.push.reg_list);
            if (misc.instr.push.m == 1){
              BitCount++;
            }
            addr = SP - (4 * BitCount);
            stats.numRegReads++;
            for (int i = 0; i <= 7; i++){
                if (((misc.instr.push.reg_list & (1 << i)) >> i) == 1){
                    dmem.write(addr, rf[i]);
                    caches.access(addr);
                    addr += 4;
                    stats.numMemWrites++;
                    stats.numRegReads++;
                }
            }
            if (misc.instr.push.m == 1){
              dmem.write(addr, LR);
              caches.access(addr);
              addr += 4;
              stats.numMemWrites++;
              stats.numRegReads++;
            }
            rf.write(SP_REG, SP - (4 * BitCount));
            stats.numRegWrites++;
            stats.instrs++;
            stats.cycles++;
            saveInstr.defs = false;
            saveInstr.defreg = -1;
            saveInstr.load = false;
          break;
        case MISC_POP:
            addr = SP;
            stats.numRegReads++;
            for (int i = 0; i <= 7; i++){
                if (((misc.instr.push.reg_list & (1 << i)) >> i) == 1){
                    rf.write(i, dmem[addr]);
                    caches.access(addr);
                    addr += 4;
                    stats.numRegWrites++;
                    stats.numMemReads++;
                }
            }
            if (misc.instr.pop.m == 1){
                rf.write(PC_REG, dmem[addr]);
                caches.access(addr);
                addr += 4;
                stats.numRegWrites++;
                stats.numMemReads++;
            }
            rf.write(SP_REG, addr);
            stats.numRegWrites++;
            stats.instrs++;
            stats.cycles++;
            saveInstr.defs = false;
            saveInstr.defreg = -1;
            saveInstr.load = false;
          break;
        case MISC_SUB:
          // functionally complete, needs stats
          rf.write(SP_REG, SP - (misc.instr.sub.imm*4));
          stats.numRegWrites++;
          stats.numRegReads++;
          stats.instrs++;
          checkLoadUse(&HazardList, SP_REG, 60);
          saveInstr.defs = true;
          saveInstr.defreg = SP_REG;
          saveInstr.load = false;
          break;
        case MISC_ADD:
          // functionally complete, needs stats
          rf.write(SP_REG, SP + (misc.instr.add.imm*4));
          stats.numRegWrites++;
          stats.numRegReads++;
          stats.instrs++;
          checkLoadUse(&HazardList, SP_REG, 60);
          saveInstr.defs = true;
          saveInstr.defreg = SP_REG;
          saveInstr.load = false;
          break;
        case MISC_UXTB:
          rf.write(misc.instr.uxtb.rd, rf[misc.instr.uxtb.rm]);
          stats.numRegWrites++;
          stats.numRegReads++;
          stats.instrs++;
          checkLoadUse(&HazardList, misc.instr.uxtb.rm, 60);
          saveInstr.defs = true;
          saveInstr.defreg = misc.instr.uxtb.rd;
          saveInstr.load = false; 
      }
      break;
    case COND:
      decode(cond);
      // Once you've completed the checkCondition function,
      // this should work for all your conditional branches.
      // needs stats
      if (checkCondition(cond.instr.b.cond)){
        rf.write(PC_REG, PC + 2 * signExtend8to32ui(cond.instr.b.imm) + 2);
        stats.numRegWrites++;
        stats.numRegReads++;
        if (((int)signExtend8to32ui(cond.instr.b.imm)) > 0){
          stats.numForwardBranchesTaken++;
        }
        else{
          stats.numBackwardBranchesTaken++;
        }
      }
      else{
        if (((int)signExtend8to32ui(cond.instr.b.imm)) > 0){
          stats.numForwardBranchesNotTaken++;
        }
        else{
          stats.numBackwardBranchesNotTaken++;
        }
      }
      stats.instrs++;
      saveInstr.defs = false;
      saveInstr.defreg = -1;
      saveInstr.load = false;
      btacidx = ((cond.instr.b.cond & 2048) << 7) + cond.instr.b.imm;
      btacidx = (btacidx << 2) >> 5;
      itmap = btacmap.find(btacidx);
      if (itmap == btacmap.end()){
         entry.branch_target = PC + 2 * signExtend8to32ui(cond.instr.b.imm) + 2;
         if (checkCondition(cond.instr.b.cond)){
            entry.prediction = 2;
         }
         else{
            entry.prediction = 4;
         }
         btacmap[btacidx] = entry;
         stats.cycles += 4;
      }
      else if (itmap != btacmap.end()){
        if (checkCondition(cond.instr.b.cond)){ 
            if (itmap->second.prediction == 3 || itmap->second.prediction == 4){
                stats.cycles += 6;
                itmap->second.prediction--;
            }
            else{
                if (itmap->second.prediction == 2){
                    itmap->second.prediction--;
                }
                stats.cycles += 1;
            }
        }
        else{
            if (itmap->second.prediction == 1 || itmap->second.prediction == 2){
                stats.cycles += 6;
                itmap->second.prediction++;
            }
            else{
                if (itmap->second.prediction == 3){
                    itmap->second.prediction++;
                }
                stats.cycles += 1;
                }
        }
      }
      break;
    case UNCOND:
      // Essentially the same as the conditional branches, but with no
      // condition check, and an 11-bit immediate field
      decode(uncond);
      rf.write(PC_REG, PC + 2 * ((int)signExtend11to32ui(uncond.instr.b.imm)) + 2);
      stats.numRegWrites++;
      stats.numRegReads++;
      stats.instrs++;
      saveInstr.defs = false;
      saveInstr.defreg = -1;
      saveInstr.load = false;
      btacidx = (uncond.instr.b.imm << 1) >> 4;
      itmap = btacmap.find(btacidx);
      if (itmap == btacmap.end()){
        entry.branch_target = PC + 2 * ((int)signExtend11to32ui(uncond.instr.b.imm)) + 2;
        entry.prediction = 2;
        btacmap[btacidx] = entry;
        stats.cycles += 4;
      }
      else if (itmap != btacmap.end()){
        if (itmap->second.prediction > 1){
            itmap->second.prediction--;
        }
        stats.cycles += 1;
      }
      break;
    case LDM:
      decode(ldm);
      // need to implement
      addr = rf[ldm.instr.ldm.rn];
      stats.numRegReads++;
      for (int i = 0; i <= 7; i++){
        if (((ldm.instr.ldm.reg_list & (1 << i)) >> i) == 1){
          rf.write(i, dmem[addr]);
          caches.access(addr);
          addr += 4;
          stats.numMemReads++;
          stats.numRegWrites++;
        }
      }
      stats.instrs++;
      stats.cycles++;
      saveInstr.defs = false;
      saveInstr.defreg = -1;
      saveInstr.load = false;
      break;
    case STM:
      decode(stm);
      // need to implement
      addr = rf[stm.instr.stm.rn];
      stats.numRegReads++;
      for (int i = 0; i <= 7; i++){
        if (((stm.instr.stm.reg_list & (1 << i)) >> i) == 1){
          dmem.write(addr, rf[i]);
          caches.access(addr);
          addr += 4;
          stats.numRegReads++;
          stats.numMemWrites++;
        }
      }
      stats.instrs++;
      stats.cycles++;
      saveInstr.defs = false;
      saveInstr.defreg = -1;
      saveInstr.load = false;
      break;
    case LDRL:
      // This instruction is complete, nothing needed
      decode(ldrl);
      // Need to check for alignment by 4
      if (PC & 2) {
        addr = PC + 2 + (ldrl.instr.ldrl.imm)*4;
      }
      else {
        addr = PC + (ldrl.instr.ldrl.imm)*4;
      }
      // Requires two consecutive imem locations pieced together
      temp = imem[addr] | (imem[addr+2]<<16);  // temp is a Data32
      rf.write(ldrl.instr.ldrl.rt, temp);
      // One write for updated reg
      stats.numRegWrites++;
      // One read of the PC
      stats.numRegReads++;
      // One mem read, even though it's imem, and there's two of them
      stats.numMemReads++;
      stats.cycles++;
      stats.instrs++;
      break;
    case ADD_SP:
      // needs stats
      decode(addsp);
      rf.write(addsp.instr.add.rd, SP + (addsp.instr.add.imm*4));
      stats.numRegWrites++;
      stats.numRegReads++;
      stats.instrs++;
      checkLoadUse(&HazardList, SP_REG, 60);
      saveInstr.defs = true;
      saveInstr.defreg = addsp.instr.add.rd;
      saveInstr.load = false;
      break;
    default:
      cout << "[ERROR] Unknown Instruction to be executed" << endl;
      exit(1);
      break;
  }
  if(HazardList.size() == 4){
   HazardList.pop_back();
  }
  HazardList.push_front(saveInstr);
}
