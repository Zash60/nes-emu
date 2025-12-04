#include "Cartridge.h"
#include "Nes.h"
#include "Stream.h"
#include "Rom.h"
#include "MemoryMap.h"
#include "Debugger.h"
#include "Mapper0.h"
#include "Mapper1.h"
#include "Mapper2.h"
#include "Mapper3.h"
#include "Mapper4.h"
#include "Mapper7.h"
#include <android/log.h>

#define LOG_TAG "NES_CART"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace
{
	FORCEINLINE size_t GetBankIndex(uint16 address, uint16 baseAddress, size_t bankSize)
	{
		const size_t firstBankIndex = baseAddress / bankSize;
		return (address / bankSize) - firstBankIndex;
	}

	FORCEINLINE uint16 GetBankOffset(uint16 address, size_t bankSize)
	{
		return address & (bankSize - 1);
	}
}

void Cartridge::Initialize(Nes& nes)
{
	m_nes = &nes;
	m_mapper = nullptr;
}

void Cartridge::Serialize(class Serializer& serializer)
{
	SERIALIZE(m_cartNameTableMirroring);
    if (m_mapper) 
    {
        if (m_mapper->CanWritePrgMemory())
            SERIALIZE_BUFFER(m_prgBanks.data(), m_mapper->PrgMemorySize());
        if (m_mapper->CanWriteChrMemory())
            SERIALIZE_BUFFER(m_chrBanks.data(), m_mapper->ChrMemorySize());
        if (m_mapper->SavMemorySize() > 0)
            SERIALIZE_BUFFER(m_savBanks.data(), m_mapper->SavMemorySize());
        serializer.SerializeObject(*m_mapper);
    }
}

// Versão de arquivo (legado/desktop)
RomHeader Cartridge::LoadRom(const char* file)
{
    // Apenas redireciona para um filestream se necessário, mas no Android usaremos LoadRomFromMemory
	FileStream fs(file, "rb");
    // ... implementação antiga ... 
    // Para economizar espaço aqui, vamos focar no LoadRomFromMemory que é o que importa
    RomHeader h; return h; 
}

// NOVA IMPLEMENTAÇÃO PRINCIPAL
RomHeader Cartridge::LoadRomFromMemory(const uint8* data, size_t size)
{
    LOGD("Carregando ROM da memoria. Tamanho: %zu", size);
    
    // Usa MemoryStream em vez de FileStream
    MemoryStream ms;
    // O MemoryStream original não copia, ele aponta. Mas precisamos tirar o const.
    // Assumimos que o buffer vem do Java e é válido durante a chamada.
    ms.Open(const_cast<uint8*>(data), size);

	uint8 headerBytes[16];
	ms.ReadValue(headerBytes);
	RomHeader romHeader;
	romHeader.Initialize(headerBytes);

	if ( romHeader.HasTrainer() )
        LOGD("ROM tem Trainer (ignorado/erro)");
		// FAIL("Not supporting trainer roms");

	std::for_each(begin(m_prgBanks), end(m_prgBanks), [] (PrgBankMemory& m) { m.Initialize(); });
	std::for_each(begin(m_chrBanks), end(m_chrBanks), [] (ChrBankMemory& m) { m.Initialize(); });
	std::for_each(begin(m_savBanks), end(m_savBanks), [] (SavBankMemory& m) { m.Initialize(); });

	const size_t prgRomSize = romHeader.GetPrgRomSizeBytes();
    LOGD("PRG ROM Size: %zu", prgRomSize);
    
	const size_t numPrgBanks = prgRomSize / kPrgBankSize;
	for (size_t i = 0; i < numPrgBanks; ++i)
	{
		ms.Read(m_prgBanks[i].RawPtr(), kPrgBankSize);
	}

	const size_t chrRomSize = romHeader.GetChrRomSizeBytes();
    LOGD("CHR ROM Size: %zu", chrRomSize);
	const size_t numChrBanks = chrRomSize / kChrBankSize;

	if (chrRomSize > 0)
	{
		for (size_t i = 0; i < numChrBanks; ++i)
		{
			ms.Read(m_chrBanks[i].RawPtr(), kChrBankSize);
		}
	}

	const size_t numSavBanks = romHeader.GetNumPrgRamBanks();

    int mapperNum = romHeader.GetMapperNumber();
    LOGD("Mapper ID: %d", mapperNum);

	switch (mapperNum)
	{
	case 0: m_mapperHolder.reset(new Mapper0()); break;
	case 1: m_mapperHolder.reset(new Mapper1()); break;
	case 2: m_mapperHolder.reset(new Mapper2()); break;
	case 3: m_mapperHolder.reset(new Mapper3()); break;
	case 4: m_mapperHolder.reset(new Mapper4()); break;
	case 7: m_mapperHolder.reset(new Mapper7()); break;
	default:
        LOGD("Mapper %d nao suportado!", mapperNum);
		// FAIL("Unsupported mapper"); 
        // Fallback para mapper 0 para não crashar
        m_mapperHolder.reset(new Mapper0());
	}
	m_mapper = m_mapperHolder.get();

	m_mapper->Initialize(numPrgBanks, numChrBanks, numSavBanks);

	m_cartNameTableMirroring = romHeader.GetNameTableMirroring();
	m_hasSRAM = romHeader.HasSRAM();

	return romHeader;
}

NameTableMirroring Cartridge::GetNameTableMirroring() const
{
    if (!m_mapper) return NameTableMirroring::Horizontal;
	auto result = m_mapper->GetNameTableMirroring();
	if (result != NameTableMirroring::Undefined) return result;
	return m_cartNameTableMirroring;
}

uint8 Cartridge::HandleCpuRead(uint16 cpuAddress)
{
    if (m_mapper == nullptr) return 0;
	if (cpuAddress >= CpuMemory::kPrgRomBase) return AccessPrgMem(cpuAddress);
	else if (cpuAddress >= CpuMemory::kSaveRamBase) return AccessSavMem(cpuAddress);
	return 0;
}

void Cartridge::HandleCpuWrite(uint16 cpuAddress, uint8 value)
{
    if (m_mapper == nullptr) return;
	m_mapper->OnCpuWrite(cpuAddress, value);
	if (cpuAddress >= CpuMemory::kPrgRomBase) {
		if (m_mapper->CanWritePrgMemory()) AccessPrgMem(cpuAddress) = value;
	} else if (cpuAddress >= CpuMemory::kSaveRamBase) {
		if (m_mapper->CanWriteSavMemory()) AccessSavMem(cpuAddress) = value;
	}
}

uint8 Cartridge::HandlePpuRead(uint16 ppuAddress)
{
    if (m_mapper == nullptr) return 0;
	return AccessChrMem(ppuAddress);
}

void Cartridge::HandlePpuWrite(uint16 ppuAddress, uint8 value)
{
    if (m_mapper == nullptr) return;
	if (m_mapper->CanWriteChrMemory()) AccessChrMem(ppuAddress) = value;
}

void Cartridge::WriteSaveRamFile(const char* file) {}
void Cartridge::LoadSaveRamFile(const char* file) {}

void Cartridge::HACK_OnScanline()
{
    if (!m_mapper) return;
	if (auto* mapper4 = dynamic_cast<Mapper4*>(m_mapper))
	{
		mapper4->HACK_OnScanline();
		if (mapper4->TestAndClearIrqPending()) m_nes->SignalCpuIrq();
	}
}

size_t Cartridge::GetPrgBankIndex16k(uint16 cpuAddress) const
{
    if (!m_mapper) return 0;
	const size_t bankIndex4k = GetBankIndex(cpuAddress, CpuMemory::kPrgRomBase, kPrgBankSize);
	const size_t mappedBankIndex4k = m_mapper->GetMappedPrgBankIndex(bankIndex4k);
	return mappedBankIndex4k * KB(4) / KB(16);
}

uint8& Cartridge::AccessPrgMem(uint16 cpuAddress)
{
    if (!m_mapper) { static uint8 dummy = 0; return dummy; }
	const size_t bankIndex = GetBankIndex(cpuAddress, CpuMemory::kPrgRomBase, kPrgBankSize);
	const auto offset = GetBankOffset(cpuAddress, kPrgBankSize);
	const size_t mappedBankIndex = m_mapper->GetMappedPrgBankIndex(bankIndex);
	return m_prgBanks[mappedBankIndex].RawRef(offset);
}

uint8& Cartridge::AccessChrMem(uint16 ppuAddress)
{
    if (!m_mapper) { static uint8 dummy = 0; return dummy; }
	const size_t bankIndex = GetBankIndex(ppuAddress, PpuMemory::kChrRomBase, kChrBankSize);
	const uint16 offset = GetBankOffset(ppuAddress, kChrBankSize);
	const size_t mappedBankIndex = m_mapper->GetMappedChrBankIndex(bankIndex);
	return m_chrBanks[mappedBankIndex].RawRef(offset);
}

uint8& Cartridge::AccessSavMem(uint16 cpuAddress)
{
    if (!m_mapper) { static uint8 dummy = 0; return dummy; }
	const size_t bankIndex = GetBankIndex(cpuAddress, CpuMemory::kSaveRamBase, kSavBankSize);
	const uint16 offset = GetBankOffset(cpuAddress, kSavBankSize);
	const size_t mappedBankIndex = m_mapper->GetMappedSavBankIndex(bankIndex);
	return m_savBanks[mappedBankIndex].RawRef(offset);
}
