#include "Cpu.h"
#include "Nes.h"
#include "OpCodeTable.h"
#include "MemoryMap.h"
#include "Serializer.h"
#include "Debugger.h"
#include <android/log.h>

#define FAIL_ON_STACK_OVERFLOW 0

// Android logging
#define LOG_TAG "NES_CPU"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{
	OpCodeEntry** g_opCodeTable = GetOpCodeTable();

	FORCEINLINE uint16 GetPageAddress(uint16 address)
	{
		return (address & 0xFF00);
	}

	FORCEINLINE uint8 CalcNegativeFlag(uint16 v)
	{
		return (v & 0x0080) != 0;
	}

	FORCEINLINE uint8 CalcNegativeFlag(uint8 v)
	{
		return (v & 0x80) != 0;
	}

	FORCEINLINE uint8 CalcZeroFlag(uint16 v)
	{
		return (v & 0x00FF) == 0;
	}

	FORCEINLINE uint8 CalcZeroFlag(uint8 v)
	{
		return v == 0;
	}

	FORCEINLINE uint8 CalcCarryFlag(uint16 v)
	{
		return (v & 0xFF00) != 0;
	}

	FORCEINLINE uint8 CalcCarryFlag(uint8 v);

	FORCEINLINE uint8 CalcOverflowFlag(uint8 a, uint8 b, uint16 r)
	{
		return ((uint16)a ^ r) & ((uint16)b ^ r) & 0x0080;
	}

	FORCEINLINE uint8 CalcOverflowFlag(uint8 a, uint8 b, uint8 r);
}

Cpu::Cpu()
	: m_cpuMemoryBus(nullptr)
	, m_apu(nullptr)
	, m_opCodeEntry(nullptr)
{
}

void Cpu::Initialize(CpuMemoryBus& cpuMemoryBus, Apu& apu)
{
	m_cpuMemoryBus = &cpuMemoryBus;
	m_apu = &apu;

	m_controllerPorts.Initialize();
}

void Cpu::Reset()
{
	A = X = Y = 0;
	SP = 0xFF; 
	
	P.ClearAll();
	P.Set(StatusFlag::IrqDisabled);

	PC = Read16(CpuMemory::kResetVector);

	m_cycles = 0;
	m_totalCycles = 0;	 
	m_pendingNmi = m_pendingIrq = false;

	m_controllerPorts.Reset();
}

void Cpu::Serialize(class Serializer& serializer)
{
	SERIALIZE(PC);
	SERIALIZE(SP);
	SERIALIZE(A);
	SERIALIZE(X);
	SERIALIZE(Y);
	SERIALIZE(P);
	SERIALIZE(m_cycles);
	SERIALIZE(m_totalCycles);
	SERIALIZE(m_pendingNmi);
	SERIALIZE(m_pendingIrq);
	SERIALIZE(m_spriteDmaRegister);
	serializer.SerializeObject(m_controllerPorts);
}

void Cpu::Nmi()
{
	m_pendingNmi = true;
}

void Cpu::Irq()
{
	if (!P.Test(StatusFlag::IrqDisabled))
		m_pendingIrq = true;
}

void Cpu::Execute(uint32& cpuCyclesElapsed)
{
	m_cycles = 0;
	
	ExecutePendingInterrupts(); 
	
	const uint8 opCode = Read8(PC);
	m_opCodeEntry = g_opCodeTable[opCode];

	if (m_opCodeEntry == nullptr)
	{
        // FIX: Prevent crash by logging and skipping
		LOGE("Unknown opcode: %02X at PC: %04X", opCode, PC);
        
        // Treat as 1-byte NOP (2 cycles) to advance PC and prevent infinite loop on same address
        PC++;
        m_cycles += 2;
        
        cpuCyclesElapsed = m_cycles;
	    m_totalCycles += m_cycles;
        return; 
	}

	UpdateOperandAddress();

	Debugger::PreCpuInstruction();
	ExecuteInstruction();
	ExecutePendingInterrupts(); 
	Debugger::PostCpuInstruction();		

	cpuCyclesElapsed = m_cycles;
	m_totalCycles += m_cycles;
}

uint8 Cpu::HandleCpuRead(uint16 cpuAddress)
{
	uint8 result = 0;

	switch (cpuAddress)
	{
	case CpuMemory::kSpriteDmaReg: // $4014
		result = m_spriteDmaRegister;
		break;

	case CpuMemory::kControllerPort1: // $4016
	case CpuMemory::kControllerPort2: // $4017
		result = m_controllerPorts.HandleCpuRead(cpuAddress);
		break;

	default:
		result = m_apu->HandleCpuRead(cpuAddress);
		break;
	}
	
	return result;
}

void Cpu::HandleCpuWrite(uint16 cpuAddress, uint8 value)
{
	switch (cpuAddress)
	{
	case CpuMemory::kSpriteDmaReg: // $4014
		{
			static auto SpriteDmaTransfer = [&] (uint16 cpuAddress)
			{
				for (uint16 i = 0; i < 256; ++i) 
				{
					const uint8 value = m_cpuMemoryBus->Read(cpuAddress + i);
					m_cpuMemoryBus->Write(CpuMemory::kPpuSprRamIoReg, value);
				}
				m_cycles += 512;
			};

			m_spriteDmaRegister = value;
			const uint16 srcCpuAddress = m_spriteDmaRegister * 0x100;
			SpriteDmaTransfer(srcCpuAddress);
			return;
		}
		break;

	case CpuMemory::kControllerPort1: // $4016
		m_controllerPorts.HandleCpuWrite(cpuAddress, value);
		break;

	case CpuMemory::kControllerPort2: // $4017 
	default:
		m_apu->HandleCpuWrite(cpuAddress, value);
		break;
	}
}

uint8 Cpu::Read8(uint16 address) const
{
	return m_cpuMemoryBus->Read(address);
}

uint16 Cpu::Read16(uint16 address) const
{
	return TO16(m_cpuMemoryBus->Read(address)) | (TO16(m_cpuMemoryBus->Read(address + 1)) << 8);
}

void Cpu::Write8(uint16 address, uint8 value)
{
	m_cpuMemoryBus->Write(address, value);
}

void Cpu::UpdateOperandAddress()
{
#if CONFIG_DEBUG
	m_operandAddress = 0; 
#endif

	m_operandReadCrossedPage = false;

	switch (m_opCodeEntry->addrMode)
	{
	case AddressMode::Immedt:
		m_operandAddress = PC + 1; 
		break;

	case AddressMode::Implid:
		break;

	case AddressMode::Accumu:
		break;

	case AddressMode::Relatv: 
		{
			const int8 offset = Read8(PC+1); 
			m_operandAddress = PC + m_opCodeEntry->numBytes + offset;
		}
		break;

	case AddressMode::ZeroPg:
		m_operandAddress = TO16(Read8(PC+1));
		break;

	case AddressMode::ZPIdxX:
		m_operandAddress = TO16((Read8(PC+1) + X)) & 0x00FF; 
		break;

	case AddressMode::ZPIdxY:
		m_operandAddress = TO16((Read8(PC+1) + Y)) & 0x00FF; 
		break;

	case AddressMode::Absolu:
		m_operandAddress = Read16(PC+1);
		break;

	case AddressMode::AbIdxX:
		{
			const uint16 baseAddress = Read16(PC+1);
			const uint16 basePage = GetPageAddress(baseAddress);
			m_operandAddress = baseAddress + X;
			m_operandReadCrossedPage = basePage != GetPageAddress(m_operandAddress);
		}
		break;

	case AddressMode::AbIdxY:
		{
			const uint16 baseAddress = Read16(PC+1);
			const uint16 basePage = GetPageAddress(baseAddress);
			m_operandAddress = baseAddress + Y;
			m_operandReadCrossedPage = basePage != GetPageAddress(m_operandAddress);
		}
		break;

	case AddressMode::Indrct: 
		{
			uint16 low = Read16(PC+1);
			uint16 high = (low & 0xFF00) | ((low + 1) & 0x00FF);
			m_operandAddress = TO16(Read8(low)) | TO16(Read8(high)) << 8;
		}
		break;

	case AddressMode::IdxInd:
		{
			uint16 low = TO16((Read8(PC+1) + X)) & 0x00FF; 
			uint16 high = TO16(low + 1) & 0x00FF; 
			m_operandAddress = TO16(Read8(low)) | TO16(Read8(high)) << 8;
		}
		break;

	case AddressMode::IndIdx:
		{
			const uint16 low = TO16(Read8(PC+1)); 
			const uint16 high = TO16(low + 1) & 0x00FF; 
			const uint16 baseAddress = (TO16(Read8(low)) | TO16(Read8(high)) << 8);
			const uint16 basePage = GetPageAddress(baseAddress);
			m_operandAddress = baseAddress + Y;
			m_operandReadCrossedPage = basePage != GetPageAddress(m_operandAddress);
		}
		break;

	default:
		LOGE("Invalid addressing mode at PC: %04X", PC);
		break;
	}
}

void Cpu::ExecuteInstruction()
{
	using namespace OpCodeName;
	using namespace StatusFlag;

	uint16 nextPC = PC + m_opCodeEntry->numBytes;
	
	bool branchTaken = false;

	switch (m_opCodeEntry->opCodeName)
	{
	case ADC: 
		{
			const uint8 value = GetMemValue();
			const uint16 result = TO16(A) + TO16(value) + TO16(P.Test01(Carry));
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			P.Set(Carry, CalcCarryFlag(result));
			P.Set(Overflow, CalcOverflowFlag(A, value, result));
			A = TO8(result);
		}
		break;

	case AND: 
		A &= GetMemValue();
		P.Set(Negative, CalcNegativeFlag(A));
		P.Set(Zero, CalcZeroFlag(A));
		break;

	case ASL: 
		{
			const uint16 result = TO16(GetAccumOrMemValue()) << 1;
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			P.Set(Carry, CalcCarryFlag(result));
			SetAccumOrMemValue(TO8(result));
		}
		break;

	case BCC: 
		if (!P.Test(Carry))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case BCS: 
		if (P.Test(Carry))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case BEQ: 
		if (P.Test(Zero))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case BIT: 
		{
			uint8 memValue = GetMemValue();
			uint8 result = A & GetMemValue();
			P.SetValue( (P.Value() & 0x3F) | (memValue & 0xC0) ); 
			P.Set(Zero, CalcZeroFlag(result));
		}
		break;

	case BMI: 
		if (P.Test(Negative))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case BNE:  
		if (!P.Test(Zero))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case BPL: 
		if (!P.Test(Negative))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case BRK: 
		{
			uint16 returnAddr = PC + 2;
			Push16(returnAddr);
			PushProcessorStatus(true);
			P.Set(IrqDisabled); 
			nextPC = Read16(CpuMemory::kIrqVector);
		}
		break;

	case BVC: 
		if (!P.Test(Overflow))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case BVS: 
		if (P.Test(Overflow))
		{
			nextPC = GetBranchOrJmpLocation();
			branchTaken = true;
		}
		break;

	case CLC: 
		P.Clear(Carry);
		break;

	case CLD: 
		P.Clear(Decimal);
		break;

	case CLI: 
		P.Clear(IrqDisabled);
		break;

	case CLV: 
		P.Clear(Overflow);
		break;

	case CMP: 
		{
			const uint8 memValue = GetMemValue();
			const uint8 result = A - memValue;
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			P.Set(Carry, A >= memValue); 
		}
		break;

	case CPX: 
		{
			const uint8 memValue = GetMemValue();
			const uint8 result = X - memValue;
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			P.Set(Carry, X >= memValue); 
		}
		break;

	case CPY: 
		{
			const uint8 memValue = GetMemValue();
			const uint8 result = Y - memValue;
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			P.Set(Carry, Y >= memValue); 
		}
		break;

	case DEC: 
		{
			const uint8 result = GetMemValue() - 1;
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			SetMemValue(result);
		}
		break;

	case DEX: 
		--X;
		P.Set(Negative, CalcNegativeFlag(X));
		P.Set(Zero, CalcZeroFlag(X));
		break;

	case DEY: 
		--Y;
		P.Set(Negative, CalcNegativeFlag(Y));
		P.Set(Zero, CalcZeroFlag(Y));
		break;

	case EOR: 
		A = A ^ GetMemValue();
		P.Set(Negative, CalcNegativeFlag(A));
		P.Set(Zero, CalcZeroFlag(A));
		break;

	case INC: 
		{
			const uint8 result = GetMemValue() + 1;
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			SetMemValue(result);
		}
		break;

	case INX: 
		++X;
		P.Set(Negative, CalcNegativeFlag(X));
		P.Set(Zero, CalcZeroFlag(X));
		break;

	case INY: 
		++Y;
		P.Set(Negative, CalcNegativeFlag(Y));
		P.Set(Zero, CalcZeroFlag(Y));
		break;

	case JMP: 
		nextPC = GetBranchOrJmpLocation();
		break;

	case JSR: 
		{
			const uint16 returnAddr = PC + m_opCodeEntry->numBytes - 1;
			Push16(returnAddr);
			nextPC = GetBranchOrJmpLocation();
		}
		break;

	case LDA: 
		A = GetMemValue();
		P.Set(Negative, CalcNegativeFlag(A));
		P.Set(Zero, CalcZeroFlag(A));
		break;

	case LDX: 
		X = GetMemValue();
		P.Set(Negative, CalcNegativeFlag(X));
		P.Set(Zero, CalcZeroFlag(X));
		break;

	case LDY: 
		Y = GetMemValue();
		P.Set(Negative, CalcNegativeFlag(Y));
		P.Set(Zero, CalcZeroFlag(Y));
		break;

	case LSR: 
		{
			const uint8 value = GetAccumOrMemValue();
			const uint8 result = value >> 1;
			P.Set(Carry, value & 0x01); 
			P.Set(Zero, CalcZeroFlag(result));
			P.Clear(Negative); 
			SetAccumOrMemValue(result);
		}		
		break;

	case NOP: 
		break;

	case ORA: 
		A |= GetMemValue();
		P.Set(Negative, CalcNegativeFlag(A));
		P.Set(Zero, CalcZeroFlag(A));
		break;

	case PHA: 
		Push8(A);
		break;

	case PHP: 
		PushProcessorStatus(true);
		break;

	case PLA: 
		A = Pop8();
		P.Set(Negative, CalcNegativeFlag(A));
		P.Set(Zero, CalcZeroFlag(A));
		break;

	case PLP: 
		PopProcessorStatus();
		break;

	case ROL: 
		{
			const uint16 result = (TO16(GetAccumOrMemValue()) << 1) | TO16(P.Test01(Carry));
			P.Set(Carry, CalcCarryFlag(result));
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			SetAccumOrMemValue(TO8(result));
		}
		break;

	case ROR: 
		{
			const uint8 value = GetAccumOrMemValue();
			const uint8 result = (value >> 1) | (P.Test01(Carry) << 7);
			P.Set(Carry, value & 0x01);
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			SetAccumOrMemValue(result);
		}
		break;

	case RTI: 
		{
			PopProcessorStatus();
			nextPC = Pop16();
		}
		break;

	case RTS: 
		{
			nextPC = Pop16() + 1;
		}
		break;

	case SBC: 
		{
			const uint8 value = GetMemValue() ^ 0XFF;
			const uint16 result = TO16(A) + TO16(value) + TO16(P.Test01(Carry));
			P.Set(Negative, CalcNegativeFlag(result));
			P.Set(Zero, CalcZeroFlag(result));
			P.Set(Carry, CalcCarryFlag(result));
			P.Set(Overflow, CalcOverflowFlag(A, value, result));
			A = TO8(result);
		}
		break;

	case SEC: 
		P.Set(Carry);
		break;

	case SED: 
		P.Set(Decimal);
		break;

	case SEI: 
		P.Set(IrqDisabled);
		break;

	case STA: 
		SetMemValue(A);
		break;

	case STX: 
		SetMemValue(X);
		break;

	case STY: 
		SetMemValue(Y);
		break;

	case TAX: 
		X = A;
		P.Set(Negative, CalcNegativeFlag(X));
		P.Set(Zero, CalcZeroFlag(X));
		break;

	case TAY: 
		Y = A;
		P.Set(Negative, CalcNegativeFlag(Y));
		P.Set(Zero, CalcZeroFlag(Y));
		break;

	case TSX: 
		X = SP;
		P.Set(Negative, CalcNegativeFlag(X));
		P.Set(Zero, CalcZeroFlag(X));
		break;

	case TXA: 
		A = X;
		P.Set(Negative, CalcNegativeFlag(A));
		P.Set(Zero, CalcZeroFlag(A));
		break;

	case TXS: 
		SP = X;
		break;

	case TYA: 
		A = Y;
		P.Set(Negative, CalcNegativeFlag(A));
		P.Set(Zero, CalcZeroFlag(A));
		break;

	default:
		// Not implemented logic handled in OpCodeTable, but here is for safety
		break;
	}

	{
		uint16 cycles = m_opCodeEntry->numCycles;

		if (m_operandReadCrossedPage)
			cycles += m_opCodeEntry->pageCrossCycles;

		if (branchTaken)
		{
			++cycles;
			if (GetPageAddress(PC) != GetPageAddress(nextPC))
			{
				++cycles;
			}
		}

		m_cycles += cycles;
	}

	PC = nextPC;
}

void Cpu::ExecutePendingInterrupts()
{
	const uint16 kInterruptCycles = 7;

	if (m_pendingNmi)
	{
		Push16(PC);
		PushProcessorStatus(false);
		P.Clear(StatusFlag::BrkExecuted);
		P.Set(StatusFlag::IrqDisabled);
		PC = Read16(CpuMemory::kNmiVector);
		m_cycles += kInterruptCycles * 2;
		m_pendingNmi = false;
	}
	else if (m_pendingIrq)
	{
		Push16(PC);
		PushProcessorStatus(false);
		P.Clear(StatusFlag::BrkExecuted);
		P.Set(StatusFlag::IrqDisabled);
		PC = Read16(CpuMemory::kIrqVector);
		m_cycles += kInterruptCycles;
		m_pendingIrq = false;
	}
}

uint8 Cpu::GetAccumOrMemValue() const
{
	if (m_opCodeEntry->addrMode == AddressMode::Accumu)
		return A;
	
	uint8 result = Read8(m_operandAddress);
	return result;
}

void Cpu::SetAccumOrMemValue(uint8 value)
{
	if (m_opCodeEntry->addrMode == AddressMode::Accumu)
	{
		A = value;
	}
	else
	{
		Write8(m_operandAddress, value);
	}
}

uint8 Cpu::GetMemValue() const
{
	uint8 result = Read8(m_operandAddress);
	return result;
}

void Cpu::SetMemValue(uint8 value)
{
	Write8(m_operandAddress, value);
}

uint16 Cpu::GetBranchOrJmpLocation() const
{
	return m_operandAddress;
}

void Cpu::Push8(uint8 value)
{
	Write8(CpuMemory::kStackBase + SP, value);
	--SP;
}

void Cpu::Push16(uint16 value)
{
	Push8(value >> 8);
	Push8(value & 0x00FF);
}

uint8 Cpu::Pop8()
{
	++SP;
	return Read8(CpuMemory::kStackBase + SP);
}

uint16 Cpu::Pop16()
{
	const uint16 low = TO16(Pop8());
	const uint16 high = TO16(Pop8());
	return (high << 8) | low;
}

void Cpu::PushProcessorStatus(bool softwareInterrupt)
{
	uint8 brkFlag = softwareInterrupt? StatusFlag::BrkExecuted : 0;
	Push8(P.Value() | StatusFlag::Unused | brkFlag);
}

void Cpu::PopProcessorStatus()
{
	P.SetValue(Pop8() & ~StatusFlag::Unused & ~StatusFlag::BrkExecuted);
}
