#ifndef __SIM5_H__INCLUDED__
#define __SIM5_H__INCLUDED__



typedef int WORD;


typedef struct InstructionFields
{
	int opcode;
	int rs;
	int rt;
	int rd;
	int shamt;
	int funct;
	int imm16, imm32;
	int address;		// this is the 26 bit field from the J format
} InstructionFields;



typedef struct ID_EX
{
	// Register numbers, mapped directly from the instruction fields.
	int rs,rt,rd;

	// Register values, which were read from the register file.
	WORD rsVal, rtVal;

	// save the immediate fields for use in the EX phase
	int imm16, imm32;


	// Control bits
	int ALUsrc;
	struct {
		int bNegate;
		int op;
	} ALU;

	int memRead;
	int memWrite;
	int memToReg;

	int regDst;
	int regWrite;

	WORD extra1, extra2, extra3;
} ID_EX;



typedef struct EX_MEM
{
	int  rt;         // necessary for forwarding into SW instructions
	WORD rtVal;      // carries the data-to-write for SW

	int memRead, memWrite, memToReg;      // 1 bit each

	int writeReg;   // 5 bits
	int regWrite;   // 1 bit

	WORD aluResult;

	// Fields for extra instructions
	WORD extra1, extra2, extra3;
} EX_MEM;



typedef struct MEM_WB
{
	int  memToReg;
	WORD aluResult;
	WORD memResult;

	int writeReg;    // 5 bits
	int regWrite;    // 1 bit

	WORD extra1, extra2, extra3;
} MEM_WB;




// ------------------ EXECUTE METHODS -----------------------


void extract_instructionFields(WORD instruction, InstructionFields *fieldsOut);

int IDtoIF_get_stall(InstructionFields *fields,
                     ID_EX  *old_idex, EX_MEM *old_exmem);
int IDtoIF_get_branchControl(InstructionFields *fields, WORD rsVal, WORD rtVal);

WORD calc_branchAddr(WORD pcPlus4, InstructionFields *fields);
WORD calc_jumpAddr  (WORD pcPlus4, InstructionFields *fields);

int execute_ID(int IDstall,
               InstructionFields *fieldsIn,
               WORD pcPlus4,
               WORD rsVal, WORD rtVal,
               ID_EX *new_idex);

WORD EX_getALUinput1(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb);
WORD EX_getALUinput2(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb);

void execute_EX(ID_EX *in, WORD input1, WORD input2,
                EX_MEM *new_exMem);

void execute_MEM(EX_MEM *in, MEM_WB *old_memWb,
                 WORD *mem, MEM_WB *new_memwb);

void execute_WB (MEM_WB *in, WORD *regs);



// Helper function

static inline WORD signExtend16to32(int val16)
{
	if (val16 & 0x8000)
		return val16 | 0xffff0000;
	else
		return val16;
}


#endif