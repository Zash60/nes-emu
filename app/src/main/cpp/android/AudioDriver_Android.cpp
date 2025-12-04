#include "../core/AudioDriver.h"
class AudioDriver::AudioDriverImpl { public: void Initialize(){} void Shutdown(){} size_t GetSampleRate()const{return 44100;} float32 GetBufferUsageRatio()const{return 0;} void AddSampleF32(float32 s){} };
AudioDriver::AudioDriver() : m_impl(new AudioDriverImpl) {}
AudioDriver::~AudioDriver() { delete m_impl; }
void AudioDriver::Initialize() { m_impl->Initialize(); }
void AudioDriver::Shutdown() { m_impl->Shutdown(); }
size_t AudioDriver::GetSampleRate() const { return m_impl->GetSampleRate(); }
float32 AudioDriver::GetBufferUsageRatio() const { return m_impl->GetBufferUsageRatio(); }
void AudioDriver::AddSampleF32(float32 sample) { m_impl->AddSampleF32(sample); }
