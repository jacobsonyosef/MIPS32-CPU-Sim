/*
* Simulates a pipelined CPU using the MIPS32 architecture. Includes various functions which simulate
* different elements/functional groups and their execution within the CPU.
*
* Author: Yosef Jacobson
* NetID: yjacobson@email.arizona.edu
* Instructor: Tyler Conklin
*/

#include "sim5.h"
#include <stdio.h>

// Function header for a helper function.
void setControls(int ALUsrc, int ALUop, int bNegate,
	int memRead, int memWrite, int memToReg,
	int regDst, int regWrite,
	int extra1,
	ID_EX* idex);

/*
* Fills all of the instructionFields fields with the proper bits from the instruction.
*/
void extract_instructionFields(WORD instruction, InstructionFields* fieldsOut) {
	int opcode = (instruction >> 26) & 0x3f,
		rs = (instruction >> 21) & 0x1f,
		rt = (instruction >> 16) & 0x1f,
		rd = (instruction >> 11) & 0x1f,
		shamt = (instruction >> 6) & 0x1f,
		funct = instruction & 0x3f,
		imm16 = instruction & 0xffff,
		imm32 = signExtend16to32(imm16),
		address = instruction & 0x3ffffff;

	fieldsOut->opcode = opcode;
	fieldsOut->rs = rs;
	fieldsOut->rt = rt;
	fieldsOut->rd = rd;
	fieldsOut->shamt = shamt;
	fieldsOut->funct = funct;
	fieldsOut->imm16 = imm16;
	fieldsOut->imm32 = imm32;
	fieldsOut->address = address;
}

/*
* Determines if a stall is required for the current instruction. Returns 1 if a stall is necessary, 0 otherwise.
*/
int IDtoIF_get_stall(InstructionFields* fields,
	ID_EX* old_idex, EX_MEM* old_exmem) {

	int opcode = fields->opcode;

	if (opcode == 43) {
		// sw is current instruction, possible hazard involving rt
		int inst1Hazard = 0, inst2Hazard = 0;

		int compReg = old_idex->regDst ? old_idex->rd : old_idex->rt;

		if (old_idex->regWrite && compReg == fields->rt) {
			inst1Hazard = 1;
		}

		if (old_exmem->regWrite && old_exmem->writeReg == fields->rt) {
			inst2Hazard = 1;
		}

		// if both instructions have a hazard, it will be resolved through forwarding; so this is the only stall condition.
		if (inst1Hazard == 0 && inst2Hazard == 1) {
			return 1;
		}

		return 0;
	}

	else if (old_idex->memRead) {
		// lw in EX phase, possible hazard
		if (opcode == 0) {
			// R-type: any matching rs or rt registers will be a hazard
			if (old_idex->rt == fields->rs || old_idex->rt == fields->rt) {
				return 1;
			}

			return 0;
		}

		else if (opcode == 4 || opcode == 5) {
			// Branches: rs is a destination register, so only rt will cause hazards
			if (old_idex->rt == fields->rt) {
				return 1;
			}

			return 0;
		}

		else if ((opcode > 7 && opcode < 15) || opcode > 31) {
			// I-type instructions in which rt is a destination register, so only rs will cause hazards
			if (old_idex->rt == fields->rs) {
				return 1;
			}

			return 0;
		}

		else {
			// other instructions shouldn't cause a hazard
			return 0;
		}
	}
}

/*
* Determines how to calculate next PC. Returns 0 for PC+4, 1 for a branch jump, and 2 for an unconditional jump.
*/
int IDtoIF_get_branchControl(InstructionFields* fields, WORD rsVal, WORD rtVal) {
	int opcode = fields->opcode;

	if (opcode == 2) {
		// J instruction
		return 2;
	}

	if ((opcode == 4 && rsVal == rtVal) || (opcode == 5 && rsVal != rtVal)) {
		// Branch instruction
		return 1;
	}

	// everything else
	return 0;
}

/*
* Calculates the next address using a branch jump.
*/
WORD calc_branchAddr(WORD pcPlus4, InstructionFields* fields) {
	int imm32 = fields->imm32;
	return (WORD)(pcPlus4 + (4 * imm32));
}

/*
* Calculates the next address using an unconditonal jump.
*/
WORD calc_jumpAddr(WORD pcPlus4, InstructionFields* fields) {
	int address = fields->address << 2;
	int topBits = pcPlus4 & 0xf0000000;
	address = address | topBits;
	return address;
}

/*
* Sets all control bits for this instruction using the fields of the instruction. Also handles stalls/NOPs.
*/
int execute_ID(int IDstall,
	InstructionFields* fieldsIn,
	WORD pcPlus4,
	WORD rsVal, WORD rtVal,
	ID_EX* new_idex) {

	int opcode = fieldsIn->opcode;
	int funct = fieldsIn->funct;

	// filling/forwarding instruction-independent fields of ID/EX reg
	new_idex->imm16 = fieldsIn->imm16;
	new_idex->imm32 = fieldsIn->imm32;
	new_idex->rd = fieldsIn->rd;
	new_idex->rs = fieldsIn->rs;
	new_idex->rt = fieldsIn->rt;
	new_idex->rsVal = rsVal;
	new_idex->rtVal = rtVal;

	int returnCode = 0;

	if (IDstall) {
		// Stall
		returnCode = 1;

		new_idex->imm16 = 0;
		new_idex->imm32 = 0;
		new_idex->rd = 0;
		new_idex->rs = 0;
		new_idex->rt = 0;
		new_idex->rsVal = 0;
		new_idex->rtVal = 0;

		setControls(0, 0, 0,		// ALU
			0, 0, 0,				// Memory
			0, 0,					// Registers
			0,						// "extra1" (used as controls for andi, lui, and bne)
			new_idex);
	}

	else if (opcode == 0) {
		// R-format instruction
		switch (funct) {
		case 0:
			// sll, but only implementing as NOP
			returnCode = 1;

			setControls(0, 5, 0,		// ALU
				0, 0, 0,				// Memory
				1, 1,					// Registers
				0,						// "extra1" (used as controls for andi, lui, and bne)
				new_idex);
			break;

		case 32: case 33:
			// add, addu
			returnCode = 1;

			setControls(0, 2, 0,		// ALU
				0, 0, 0,				// Memory
				1, 1,					// Registers
				0,						// "extra1" (used as controls for andi, lui, and bne)
				new_idex);
			break;

		case 34: case 35:
			// sub, subu
			returnCode = 1;

			setControls(0, 2, 1,
				0, 0, 0,
				1, 1,
				0,
				new_idex);
			break;

		case 36:
			// and
			returnCode = 1;

			setControls(0, 0, 0,
				0, 0, 0,
				1, 1,
				0,
				new_idex);
			break;

		case 37:
			// or
			returnCode = 1;

			setControls(0, 1, 0,
				0, 0, 0,
				1, 1,
				0,
				new_idex);
			break;

		case 38:
			// xor
			returnCode = 1;

			setControls(0, 4, 0,
				0, 0, 0,
				1, 1,
				0,
				new_idex);
			break;

		case 39:
			// nor
			returnCode = 1;

			setControls(0, 4, 0,
				0, 0, 0,
				1, 1,
				3,
				new_idex);
			break;

		case 42:
			// slt
			returnCode = 1;

			setControls(0, 3, 1,
				0, 0, 0,
				1, 1,
				0,
				new_idex);
			break;
		}
	}

	// I-format instruction or j
	else if (opcode == 2) {
		// j
		returnCode = 1;

		new_idex->rd = 0;
		new_idex->rs = 0;
		new_idex->rt = 0;
		new_idex->rsVal = 0;
		new_idex->rtVal = 0;

		setControls(0, 0, 0,
			0, 0, 0,
			0, 0,
			0,
			new_idex);
	}

	else if (opcode == 4) {
		// beq
		returnCode = 1;

		new_idex->rd = 0;
		new_idex->rs = 0;
		new_idex->rt = 0;
		new_idex->rsVal = 0;
		new_idex->rtVal = 0;

		setControls(0, 0, 0,
			0, 0, 0,
			0, 0,
			0,
			new_idex);
	}

	else if (opcode == 5) {
		// bne
		returnCode = 1;

		new_idex->rd = 0;
		new_idex->rs = 0;
		new_idex->rt = 0;
		new_idex->rsVal = 0;
		new_idex->rtVal = 0;

		setControls(0, 0, 0,
			0, 0, 0,
			0, 0,
			0,
			new_idex);
	}

	else if (opcode == 8 || opcode == 9) {
		// addi, addiu
		returnCode = 1;

		setControls(1, 2, 0,
			0, 0, 0,
			0, 1,
			0,
			new_idex);
	}

	else if (opcode == 10) {
		// slti
		returnCode = 1;

		setControls(1, 3, 1,
			0, 0, 0,
			0, 1,
			0,
			new_idex);
	}

	else if (opcode == 12) {
		// andi
		returnCode = 1;

		setControls(2, 0, 0,
			0, 0, 0,
			0, 1,
			0,
			new_idex);
	}

	else if (opcode == 13) {
		// ori
		returnCode = 1;
		setControls(2, 1, 0,
			0, 0, 0,
			0, 1,
			0,
			new_idex);
	}

	else if (opcode == 15) {
		// lui
		returnCode = 1;

		setControls(2, 0, 0,
			0, 0, 0,
			0, 1,
			2,
			new_idex);
	}

	else if (opcode == 35) {
		// lw
		returnCode = 1;

		setControls(1, 2, 0,
			1, 0, 1,
			0, 1,
			0,
			new_idex);
	}

	else if (opcode == 43) {
		// sw
		returnCode = 1;

		setControls(1, 2, 0,
			0, 1, 0,
			0, 0,
			0,
			new_idex);
	}

	return returnCode;
}

/*
* Determines the ALU's first input. Checks if a hazard exists, and properly forwards data if so.
*/
WORD EX_getALUinput1(ID_EX* in, EX_MEM* old_exMem, MEM_WB* old_memWb) {
	if (old_exMem->regWrite && old_exMem->writeReg == in->rs) {
		return old_exMem->aluResult;
	}

	if (old_memWb->regWrite && old_memWb->writeReg == in->rs && !old_memWb->memToReg) {
		return old_memWb->aluResult;
	}

	if (old_memWb->regWrite && old_memWb->writeReg == in->rs && old_memWb->memToReg) {
		return old_memWb->memResult;
	}

	return in->rsVal;
}

/* 
* Determines the ALU's second input. Checks if a hazard exists, and properly forwards data if so.
* Otherwise, chooses between rtVal, a zero-extended immediate, and a sign-extended immediate depending on the instruction.
*/
WORD EX_getALUinput2(ID_EX* in, EX_MEM* old_exMem, MEM_WB* old_memWb) {
	if (in->ALUsrc == 1) {
		// i instruction
		return (WORD)in->imm32;
	}

	if (in->ALUsrc == 2) {
		// immediate should be 0 extended instead of sign extended
		WORD out = (WORD)in->imm16;
		return out;
	}

	if (old_exMem->regWrite && old_exMem->writeReg == in->rt) {
		return old_exMem->aluResult;
	}

	if (old_memWb->regWrite && old_memWb->writeReg == in->rt && !old_memWb->memToReg) {
		return old_memWb->aluResult;
	}

	if (old_memWb->regWrite && old_memWb->writeReg == in->rt && old_memWb->memToReg) {
		return old_memWb->memResult;
	}

	return in->rtVal;
}

/*
* Simulates the ALU. Reads the input words and the control bits from ID/EX, performs all ALU operations, and returns the result
* corresponding to the input ALU controls.
*/
void execute_EX(ID_EX* in, WORD input1, WORD input2,
	EX_MEM* new_exMem) {

	if (in->ALUsrc) {
		new_exMem->writeReg = in->rt;
	}

	else {
		new_exMem->writeReg = in->rd;
	}

	// forwarding values to EX/MEM register
	new_exMem->regWrite = in->regWrite;
	new_exMem->memRead = in->memRead;
	new_exMem->memToReg = in->memToReg;
	new_exMem->memWrite = in->memWrite;
	new_exMem->rt = in->rt;
	new_exMem->rtVal = in->rtVal;
	new_exMem->extra1 = in->extra1;
	new_exMem->extra2 = in->imm16; // preserving imm16 for use in lui instruction in WB phase

	int op = in->ALU.op,
		bNegate = in->ALU.bNegate;

	if (bNegate) {
		input2 = -input2;
	}

	int res0 = input1 & input2,
		res1 = input1 | input2,
		res2 = input1 + input2,
		res3 = res2 < 0,
		res4 = input1 ^ input2;


	if (in->extra1 == 3) {
		// nor
		new_exMem->aluResult = ~res1;
	}

	else if (op == 0) {
		// and
		new_exMem->aluResult = res0;
	}

	else if (op == 1) {
		// or
		new_exMem->aluResult = res1;
	}

	else if (op == 2) {
		// add/subtract
		new_exMem->aluResult = res2;
	}

	else if (op == 3) {
		// less than
		new_exMem->aluResult = res3;
	}

	else if (op == 4) {
		// xor
		new_exMem->aluResult = res4;
	}

	else if (op == 5) {
		// nop
		new_exMem->aluResult = 0;
	}
}

/*
* Simulates Data memory and its operations. Gets an address from EX/MEM, and either reads the corresponding word from memory,
* or writes an input word to memory at the indicated address. Also checks for hazards and, if found, properly forwards data to be written.
*/
void execute_MEM(EX_MEM* in, MEM_WB* old_memWb,
	WORD* mem, MEM_WB* new_memwb) {

	WORD address = in->aluResult;
	int memIndex = address / 4; // converting address to word number in memory

	new_memwb->memResult = 0; // setting to default value
	new_memwb->memToReg = in->memToReg;
	new_memwb->regWrite = in->regWrite;
	new_memwb->writeReg = in->writeReg;
	new_memwb->aluResult = in->aluResult;
	new_memwb->extra1 = in->extra1;
	new_memwb->extra2 = in->extra2;

	int writeVal;

	if (old_memWb->memToReg && old_memWb->writeReg == in->rt) {
		writeVal = old_memWb->memResult;
	}

	else if (old_memWb->regWrite && old_memWb->writeReg == in->rt) {
		writeVal = old_memWb->aluResult;
	}

	else {
		writeVal = in->rtVal;
	}

	if (in->memRead) {
		WORD readVal = mem[memIndex];
		new_memwb->memResult = readVal;
	}

	else if (in->memWrite) {
		mem[memIndex] = writeVal;
	}
}

/*
* Writes a value to a register at the end of a cycle, if indicated by the control bits.
*/
void execute_WB(MEM_WB* in, WORD* regs) {

	if (in->regWrite) {
		int regNum = in->writeReg;

		if (in->extra1 == 2) {
			// lui command; write shifted imm16 val to reg
			regs[regNum] = (WORD)(in->extra2 << 16);
		}

		else if (in->memToReg) {
			regs[regNum] = in->memResult;
		}

		else {
			regs[regNum] = in->aluResult;
		}
	}
}

/* HELPER FUNCTIONS */

/*
* Sets the control bits of ID_EX register using the input control bits.
*/
void setControls(int ALUsrc, int ALUop, int bNegate,
	int memRead, int memWrite, int memToReg,
	int regDst, int regWrite,
	int extra1,
	ID_EX* idex) {

	idex->ALUsrc = ALUsrc;
	idex->ALU.op = ALUop;
	idex->ALU.bNegate = bNegate;

	idex->memRead = memRead;
	idex->memWrite = memWrite;
	idex->memToReg = memToReg;

	idex->regDst = regDst;
	idex->regWrite = regWrite;

	idex->extra1 = extra1;
}