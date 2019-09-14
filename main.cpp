#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <bitset>
#include <array>
#include <unordered_map>

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

	mips_word abs() {
		return this->sign() ? this->complement() : (*this);
	}

	signed long to_slong() {
		signed long v = static_cast<signed long>(this->abs().to_ulong());
		return this->sign() ? -v : v;
	}

	mips_word signed_shift(int shamt) {
		int sign = this->sign();
		mips_word a = this->abs();
		if (shamt > 0) {
			a <<= shamt;
		}
		else {
			a >>= shamt;
		}
		return mips_word(sign) << 31 | a;
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


/**
 * Produces a constant expression bitmask
 * the bitmask is right aligned so you can
 * shift it to get it into any position required
*/
template <int N>
class bitmask {
	unsigned long mask = (~0U) >> (sizeof(unsigned long) * 8 - N);

public:
	constexpr operator unsigned long() {
		return mask;
	}
};


/** 
 * Unpacks a 32 bit number into a set of values
 * according to the MIPS instruction format.
 * The actual type of the instruction is not taken
 * into consideration.
*/
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

/** 
 * Takes an N sized binary number and extends it
 * to a 32 bit sized number while maintaining the 
 * sign bit. Assuming the sign bit it at the Nth 
 * bit
 */
template <int N>
std::bitset<32> sign_extend(std::bitset<N> v) {
	int sign = v[N - 1];
	v[N - 1] = 0;
	std::bitset<32> value(v.to_ulong());
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
	op_SW = 43,

	op_J = 2,
	op_JAL = 3
};

// processor state information
struct {
	mips_word register_file[32];
	std::map<mips_word, std::bitset<8>> memory_file;
	std::unordered_map<int, std::bitset<32>> instruction_memory;

	// program counter
	mips_word pc = 0;

	// HI and LO registers
	mips_word hi, lo;

	// Advance by one clock cycle
	auto step_proc() {
		// r0 is always zero
		register_file[0] = 0;

		return instruction_memory.at(pc.to_ulong());
	}

	// Accessor for registor file
	auto& r(int n) {
		return register_file[n];
	}

	// Accessor for memory file
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
	{
		int n = 0;
		std::string line;
		while (std::getline(ifs, line)) {
			std::bitset<32> bin_line(line);
			mips_processor.instruction_memory.insert({ n, bin_line });
			n += 4;
		}
	}

	auto& proc = mips_processor;
	int n_instruction_bytes = proc.instruction_memory.size() * 4;

	// Main loop
	// read and process instructions
	while (proc.pc.abs() < n_instruction_bytes) {
		auto data = decode_bin_line(proc.step_proc());
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
		mips_word imm_16 = sign_extend<16>(data.imm);
		mips_word imm_18 = sign_extend<18>(data.imm << 2);
		mips_word uimm = data.imm;

		// new value of pc for branch instruction
		mips_word new_pc = proc.pc + 4 + imm_18;

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
			proc.r(rt) = proc.r(rs).to_slong() < imm_16.to_slong() ? 1 : 0;
		case op_SLTIU:
			proc.r(rt) = proc.r(rs) < imm_16 ? 1 : 0;
			break;
		case op_ANDI:
			proc.r(rt) = proc.r(rs) & uimm;
			break;
		case op_ORI:
			proc.r(rt) = proc.r(rs) | uimm;
			break;
		case op_XORI:
			proc.r(rt) = proc.r(rs) ^ uimm;
			break;
		case op_LUI:
			proc.r(rt) = uimm << 16;
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
		case op_J:
			proc.pc = data.address << 2;
			break;
		case op_JAL:
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
			proc.r(rd) = proc.r(rt) << data.shamt;
			break;
		case f_SRL:
			proc.r(rd) = proc.r(rt) >> data.shamt;
			break;
		case f_SRA:
			proc.r(rd) = proc.r(rt).signed_shift(-data.shamt);
			break;
		case f_SLLV:
			proc.r(rd) = proc.r(rt) << proc.r(rs).to_ulong();
			break;
		case f_SRLV:
			proc.r(rd) = proc.r(rt) >> proc.r(rs).to_ulong();
			break;
		case f_SRAV:
			proc.r(rd) = proc.r(rt).signed_shift(-static_cast<signed long>(proc.r(rs).to_ulong()));
			break;
		case f_JR:
			proc.pc = proc.r(rs).to_ulong() / 4;
			break;
		case f_JALR:
			proc.r(rd) = proc.pc + 2;
			proc.pc = proc.r(rs).to_ulong() / 4;
			break;
		case f_MFHI:
			proc.r(rd) = proc.hi;
			break;
		case f_MTHI:
			proc.hi = proc.r(rs);
			break;
		case f_MFLO:
			proc.r(rd) = proc.lo;
			break;
		case f_MTLO:
			proc.lo = proc.r(rs);
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
