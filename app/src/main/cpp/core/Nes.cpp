#include "Nes.h"
#include "Stream.h"
#include "Rom.h"
#include "System.h"
#include "Serializer.h"
#include "Renderer.h"
#include "IO.h"
#include "CircularBuffer.h"

Nes::~Nes() { SerializeSaveRam(true); }

void Nes::Initialize()
{
	m_apu.Initialize();
	m_cpu.Initialize(m_cpuMemoryBus, m_apu);
	m_ppu.Initialize(m_ppuMemoryBus, *this);
	m_cartridge.Initialize(*this);
	m_cpuInternalRam.Initialize();
	m_cpuMemoryBus.Initialize(m_cpu, m_ppu, m_cartridge, m_cpuInternalRam);
	m_ppuMemoryBus.Initialize(m_ppu, m_cartridge);
	m_turbo = false;
	const std::string& appDir = System::GetAppDirectory();
	m_saveDir = appDir + "saves/";
	System::CreateDirectory(m_saveDir.c_str());
}

RomHeader Nes::LoadRom(const char* file) {
    // Stub
    RomHeader h; return h;
}

// NOVA IMPLEMENTAÇÃO
RomHeader Nes::LoadRomFromMemory(const uint8* data, size_t size)
{
	SerializeSaveRam(true);
	m_romName = "game"; // Nome genérico pois veio da memória
	RomHeader romHeader = m_cartridge.LoadRomFromMemory(data, size);
	//SerializeSaveRam(false); // Sram desabilitado por enquanto
	m_rewindManager.Initialize(*this);
	return romHeader;
}

void Nes::Reset()
{
	m_frameTimer.Reset();
	m_cpu.Reset();
	m_ppu.Reset();
	m_apu.Reset();
	m_lastSaveRamTime = System::GetTimeSec();
}

void Nes::SerializeSaveRam(bool save) {} // Desativado no android por enquanto

bool Nes::SerializeSaveState(bool save)
{
    // ... Implementação simplificada ...
    return false; 
}

void Nes::Serialize(class Serializer& serializer)
{
	SERIALIZE(m_turbo);
	serializer.SerializeObject(m_cpu);
	serializer.SerializeObject(m_ppu);
	serializer.SerializeObject(m_apu);
	serializer.SerializeObject(m_cartridge);
	serializer.SerializeObject(m_cpuInternalRam);
}

void Nes::RewindSaveStates(bool enable) { m_rewindManager.SetRewinding(enable); }

void Nes::ExecuteFrame(bool paused)
{
	if (m_rewindManager.IsRewinding())
	{
		if (m_rewindManager.RewindFrame())
		{
			ExecuteCpuAndPpuFrame();
			m_ppu.RenderFrame();
		}
		return;
	}

	if (!paused)
	{
		ExecuteCpuAndPpuFrame();
		m_ppu.RenderFrame();
		m_rewindManager.SaveRewindState();
	}

	const float32 minFrameTime = 1.0f/60.0f;
	m_frameTimer.Update(m_turbo? 0.f: minFrameTime);
}

void Nes::ExecuteCpuAndPpuFrame()
{
	bool completedFrame = false;
	while (!completedFrame)
	{
		uint32 cpuCycles;
		m_cpu.Execute(cpuCycles);
		m_ppu.Execute(cpuCycles, completedFrame);
		m_apu.Execute(cpuCycles);
	}
}
