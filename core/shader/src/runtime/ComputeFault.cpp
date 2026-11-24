#include <anybox/shader/runtime/ComputeMemory.hpp>
#include <cstring>
#include <string>

void AnyBox::Shader::Runtime::ComputeMemory::CheckFault() {
    namespace Abi = ShaderRecompiler::BdaAbi;
    Abi::Fault fault;
    std::memcpy(&fault, faults->Map().data(), sizeof(fault));
    if (fault.state == Abi::FaultState::Empty) return;
    if (fault.state == Abi::FaultState::Writing) throw std::runtime_error("GPU fault record is incomplete");
    if (fault.state != Abi::FaultState::Ready) throw std::runtime_error("GPU fault record state is invalid");
    throw std::runtime_error("GPU shader fault: reason=" + std::to_string(static_cast<std::uint32_t>(fault.reason)) + " address=" + std::to_string(fault.address) + " bytes=" + std::to_string(fault.bytes) + " stage=" + std::to_string(fault.stage) + " instruction=" + std::to_string(fault.instruction));
}
