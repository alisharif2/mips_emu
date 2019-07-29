#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <bitset>
#include <array>

struct instructionInfo {
	int opcode;
	int rs, rt, rd;
	int shamt;
	int funct;
	int address;
	int imm;
};

template <int N>
class bitmask {
	unsigned long mask = 0;

public:
	bitmask() {
		mask = (~0U) >> (sizeof(unsigned long) * 8 - N);
	}

	operator unsigned long() {
		return mask;
	}
};

auto decode_bin_line(std::bitset<32> bin_line) {
	struct instructionInfo i;

	i.opcode = (bin_line >> 26).to_ulong() & bitmask<6>();
	i.rs = (bin_line >> 21).to_ulong() & bitmask<5>();
	i.rt = (bin_line >> 16).to_ulong() & bitmask<5>();
	i.rd = (bin_line >> 11).to_ulong() & bitmask<5>();
	i.shamt = (bin_line >> 6).to_ulong() & bitmask<5>();
	i.funct = (bin_line >> 0).to_ulong() & bitmask<6>();
	i.imm = (bin_line >> 0).to_ulong() & bitmask<16>();
	i.address = (bin_line >> 0).to_ulong() & bitmask<26>();

	return i;
}

template <int N>
int sign_extend(int v) {
	std::bitset<N> container = v;
	if (container[N - 1] == 1) {
		return -v;
	}
	else {
		return v;
	}
}

enum funct_type {
	f_SLL = 0,
	f_SRL = 2,
	f_SRA = 3,
	f_SLLV = 4,
	f_SRLV = 6,
	f_SRAV = 7,
	f_JR = 8,
	f_JALR = 9,
	f_MFHI = 16,
	f_MTHI = 17,
	f_MFLO = 18,
	f_MTLO = 19,
	f_MULT = 24,
	f_MULTU = 25,
	f_DIV = 26,
	f_DIVU = 27,
	f_ADD = 32,
	f_ADDU = 33,
	f_SUB = 34,
	f_SUBU = 35,
	f_AND = 36,
	f_OR = 37,
	f_XOR = 38,
	f_NOR = 39,
	f_SLT = 42,
	f_SLTU = 43
};

enum opcode_type {
	op_BEQ = 4,
	op_BNE = 5,
	op_BLEZ = 6,
	op_BGTZ = 7,
	op_ADDI = 8,
	op_ADDIU = 9,
	op_SLTI = 10,
	op_SLTIU = 11,
	op_ANDI = 12,
	op_ORI = 13,
	op_XORI = 14,
	op_LUI = 15,
	op_LB = 32,
	op_LH = 33,
	op_LW = 34,
	op_LBU = 36,
	op_LHU = 37,
	op_SB = 40,
	op_SH = 41,
	op_SW = 43
};

// processor state information
struct {
	std::array<int, 32> register_file;
	std::map<int, std::bitset<32>> memory_file;
	std::vector<std::bitset<32>> instruction_memory;
	long pc;

	auto get_instruction() {
		register_file.at(0) = 0;
		return instruction_memory.at(pc++);
	}

	auto& r(int n) {
		return register_file.at(n);
	}

	auto& m(int n) {
		return memory_file[n];
	}

} mips_processor;


int main(int argc, char* argv[]) {

	if (argc != 2) {
		std::cout << "Arguments not supported. Please pass the path to a MIPS binary" << std::endl;
		exit(1);
	}

	std::string bin_filename(argv[1]);
	std::ifstream ifs(bin_filename);

	if (!ifs.is_open()) {
		std::cout << "Could not open MIPS binary: " << bin_filename << std::endl;
		exit(1);
	}

	// initialized our processor
	mips_processor.register_file.fill(0);
	mips_processor.memory_file.clear();
	mips_processor.instruction_memory.clear();
	mips_processor.pc = 0;

	// load up instruction memory
	std::string line;
	while (std::getline(ifs, line)) {
		std::bitset<32> bin_line(line);
		mips_processor.instruction_memory.push_back(bin_line);
	}

	auto& proc = mips_processor;
	int instruction_count = proc.instruction_memory.size();

	// read and process instructions
	while (proc.pc < instruction_count) {
		auto data = decode_bin_line(proc.get_instruction());
		bool is_i_type = false, is_r_type = false;

		if (data.opcode == 0) {
			is_r_type = true;
		}
		else {
			is_i_type = true;
		}

		int rs, rt, rd;
		rs = data.rs;
		rd = data.rd;
		rt = data.rt;

		switch (data.opcode) {
		case op_BEQ:
			break;
		case op_BNE:
			break;
		case op_BLEZ:
			break;
		case op_BGTZ:
			break;
		case op_ADDI:
			proc.r(rt) = proc.r(rs) + sign_extend<16>(data.imm);
			break;
		case op_ADDIU:
			break;
		case op_SLTI:
			break;
		case op_SLTIU:
			break;
		case op_ANDI:
			break;
		case op_ORI:
			break;
		case op_XORI:
			break;
		case op_LUI:
			break;
		case op_LB:
			break;
		case op_LH:
			break;
		case op_LW:
			break;
		case op_LBU:
			break;
		case op_LHU:
			break;
		case op_SB:
			break;
		case op_SH:
			break;
		case op_SW:
			break;

		case 0:
		default:
			// Go to funct because it is r_type
			break;
		};


		if (!is_r_type) {
			continue;
		}

		switch (data.funct) {
		case f_SLL:
			break;
		case f_SRL:
			break;
		case f_SRA:
			break;
		case f_SLLV:
			break;
		case f_SRLV:
			break;
		case f_SRAV:
			break;
		case f_JR:
			break;
		case f_JALR:
			break;
		case f_MFHI:
			break;
		case f_MTHI:
			break;
		case f_MFLO:
			break;
		case f_MTLO:
			break;
		case f_MULT:
			break;
		case f_MULTU:
			break;
		case f_DIV:
			break;
		case f_DIVU:
			break;
		case f_ADD:
			proc.r(rd) = proc.r(rs) + proc.r(rt);
			break;
		case f_ADDU:
			break;
		case f_SUB:
			break;
		case f_SUBU:
			break;
		case f_AND:
			break;
		case f_OR:
			break;
		case f_XOR:
			break;
		case f_NOR:
			break;
		case f_SLT:
			break;
		case f_SLTU:
			break;
		}
	}

	return 0;
}
