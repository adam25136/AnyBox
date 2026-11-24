#include "Graph.hpp"
#include "Optimize.hpp"
#include "Reduction.hpp"
#include "Dead.hpp"
#include "../Validation.hpp"
#include "../Environment.hpp"
#include <spirv-tools/optimizer.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace {
void Eliminate(std::vector<std::uint32_t>& code, std::uint32_t vulkanVersion) {
    spvtools::Optimizer optimizer(AnyBox::Shader::GetSpirvEnvironment(vulkanVersion));
    std::string diagnostics;
    optimizer.SetMessageConsumer([&](spv_message_level_t, const char*, const spv_position_t&, const char* message) {
        if (message) diagnostics += std::string(message) + '\n';
    });
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
    optimizer.RegisterPass(spvtools::CreateTrimCapabilitiesPass());
    std::vector<std::uint32_t> optimized;
    if (!optimizer.Run(code.data(), code.size(), &optimized)) throw std::runtime_error("Wave reduction optimization failed: " + diagnostics);
    code = std::move(optimized);
}
}

bool AnyBox::Shader::OptimizeWaveReductions(std::vector<std::uint32_t>& code, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target) {
    if (input.waveSize != 32 || target.subgroupSize != 32) return false;
    if (std::ranges::find(target.supportedCapabilities, static_cast<std::uint32_t>(spv::CapabilityGroupNonUniformArithmetic)) == target.supportedCapabilities.end()) return false;
    if (ReduceWaveOperations(code) == 0) return false;
    ValidateSpirv(code, target.vulkanVersion);
    do { Eliminate(code, target.vulkanVersion); } while (RemoveDeadSubgroupOperations(code, target.vulkanVersion));
    return true;
}
