#include "Queue.hpp"
#include <stdexcept>

D3D12_COMMAND_QUEUE_DESC AnyBox::Graphics::Queue::GetDescription(Queue* self) { return self->description; }

HRESULT AnyBox::Graphics::Queue::GetTimestampFrequency(Queue* self, std::uint64_t* frequency) {
    if (!frequency) throw std::invalid_argument("Command queue timestamp frequency output is null");
    auto result = self->host->GetTimestampFrequency(frequency);
    CheckGraphicsResult(result, "ID3D12CommandQueue::GetTimestampFrequency");
    return result;
}

HRESULT AnyBox::Graphics::Queue::GetClockCalibration(Queue* self, std::uint64_t* gpuTimestamp, std::uint64_t* cpuTimestamp) {
    if (!gpuTimestamp || !cpuTimestamp) throw std::invalid_argument("Command queue clock calibration output is null");
    auto result = self->host->GetClockCalibration(gpuTimestamp, cpuTimestamp);
    CheckGraphicsResult(result, "ID3D12CommandQueue::GetClockCalibration");
    return result;
}

void AnyBox::Graphics::Queue::SetMarker(Queue* self, std::uint32_t metadata, const void* data, std::uint32_t size) {
    if (!data && size != 0) throw std::invalid_argument("Command queue marker payload is null");
    auto lock = self->lockCommands();
    self->host->SetMarker(metadata, data, size);
}

void AnyBox::Graphics::Queue::BeginEvent(Queue* self, std::uint32_t metadata, const void* data, std::uint32_t size) {
    if (!data && size != 0) throw std::invalid_argument("Command queue event payload is null");
    auto lock = self->lockCommands();
    self->host->BeginEvent(metadata, data, size);
}

void AnyBox::Graphics::Queue::EndEvent(Queue* self) { auto lock = self->lockCommands(); self->host->EndEvent(); }
