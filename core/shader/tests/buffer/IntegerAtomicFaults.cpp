#include "IntegerAtomicKernel.hpp"
#include <BdaAbi.hpp>
#include <stdexcept>
#include <string>

namespace {
void CheckFault(AnyBox::Vulkan::Context& context, bool subtract, unsigned wave, std::uint32_t offset, unsigned high, ShaderRecompiler::BdaAbi::FaultReason reason, bool initialData = true) {
    BufferTests::IntegerAtomicKernel fixture;
    (void)fixture.Modify(subtract, fixture.builder.Constant(offset), fixture.builder.Constant(1), high);
    auto shader = fixture.Compile(wave);
    std::array initial{0u};
    auto data = initialData ? std::as_bytes(std::span(initial)) : std::span<const std::byte>{};
    bool rejected = false;
    try { BufferTests::RunGpu(context, shader, initial, {1, 1, 1}, {}, {}, data); }
    catch (const std::runtime_error& error) {
        auto expected = "DXIL BDA GPU test recorded fault " + std::to_string(static_cast<unsigned>(reason));
        if (error.what() != expected) throw std::runtime_error("Integer atomic expected " + expected + ", received " + error.what());
        rejected = true;
    }
    if (!rejected) throw std::runtime_error("Integer atomic did not report the expected fault " + std::to_string(static_cast<unsigned>(reason)));
}
}

void CheckIntegerAtomicFaults(AnyBox::Vulkan::Context& context) {
    using Reason = ShaderRecompiler::BdaAbi::FaultReason;
    for (bool subtract : {false, true}) {
        for (unsigned wave : {32u, 64u}) {
            CheckFault(context, subtract, wave, 1, 2, Reason::Unaligned);
            CheckFault(context, subtract, wave, 0, 3, Reason::Unmapped);
            CheckFault(context, subtract, wave, 0, 1, Reason::Permission);
            CheckFault(context, subtract, wave, 0, 2, Reason::Permission, false);
        }
    }
}
