#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <bitset>
#include <array>

// bitset is pretty incomplete so add my own
// handles two's complement signage
class mips_word : public std::bitset<32> {
private:
	const int width = 32;

	mips_word complement() const {
		return mips_word(~(*this)) + mips_word(1U);
	}

	bool sign() const {
		return (*this)[width - 1];
	}

public:
	mips_word() : std::bitset<32>(0) { };
	mips_word(std::bitset<32> b) : std::bitset<32>(b) { };
	mips_word(unsigned int v) : std::bitset<32>(v) { };

	mips_word operator+(const mips_word& other) const {
		mips_word sum;
		for (int i = 0; i < width; ++i) {
			int inner_sum = (int)(*this)[i] + (int)other[i];
			
			// handle carry out
			if (inner_sum > 1) {
				if(i != width - 1) sum[i + 1] = 1; // except for last bit
				inner_sum = 0;
			}

			sum[i] = inner_sum;
		}
		return sum;
	}

	mips_word operator-(const mips_word& other) const {
		return *this + other.complement();
	}

	bool operator<(const mips_word& other) const {
		if (other == *this) return false;
		return (*this - other).sign();
	}

	bool operator>(const mips_word& other) const {
		if (other == *this) return false;
		return (other - *this).sign();
	}

	bool operator<=(const mips_word& other) const {
		return other == *this || *this < other;
	}

	bool operator>=(const mips_word& other) const {
		return other == *this || *this > other;
	}

	mips_word& operator=(const mips_word& other) {
		for (int i = 0; i < width; ++i) {
			(*this)[i] = other[i];
		}
		return *this;
	}
};


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
std::bitset<32> sign_extend(std::bitset<N> v) {
	int sign = v[N - 1];
	v[N - 1] = 0;
	std::bitset<32> value(v.to_string());
	return value | std::bitset<32>(sign) << 31;

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
	mips_word register_file[32] = { };
	std::map<int, mips_word> memory_file = { };
	std::vector<std::bitset<32>> instruction_memory = { };
	long pc = 0;

	auto get_instruction() {
		register_file[0] = 0;
		return instruction_memory.at(pc++);
	}

	auto& r(int n) {
		return register_file[n];
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

		// used to index the register file
		int rs, rt, rd;
		rs = data.rs;
		rd = data.rd;
		rt = data.rt;

		// sign extended to 32 bits
		auto imm_16 = sign_extend<16>(data.imm);
		auto imm_18 = sign_extend<18>(data.imm << 2);

		// new value of pc for branch instruction
		long new_pc = proc.pc + 1 + imm_18.to_ulong();

		switch (data.opcode) {
		case op_BEQ:
			if (proc.r(rs) == proc.r(rt)) proc.pc = new_pc;
			break;
		case op_BNE:
			if (proc.r(rs) != proc.r(rt)) proc.pc = new_pc;
			break;
		case op_BLEZ:
			if (proc.r(rs) <= 0) proc.pc = new_pc;
			break;
		case op_BGTZ:
			if (proc.r(rs) > 0) proc.pc = new_pc;
			break;
		case op_ADDI:
		case op_ADDIU:
			proc.r(rt) = proc.r(rs) + imm_16;
			break;
		case op_SLTI:
		case op_SLTIU:
			proc.r(rt) = proc.r(rs) < imm_16 ? 1 : 0;
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
