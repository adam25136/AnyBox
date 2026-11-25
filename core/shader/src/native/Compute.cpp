#include <anybox/shader/NativeCompute.hpp>
#include "Optimize.hpp"
#include "Options.hpp"
#include "ScratchWrites.hpp"
#include "Slices.hpp"
#include <Translation/InstructionTranslator.hpp>
#include <Optimization/SsaBuilder.hpp>
#include <Recompiler.hpp>
#include <algorithm>
#include <stdexcept>

AnyBox::Shader::NativeComputeTranslation AnyBox::Shader::LowerNativeCompute(const NativeFrontend& frontend, const Graphics::Container::NativeComputeState& state, const ShaderRecompiler::ShaderFloatMode& floatMode, const ShaderRecompiler::SpirvTargetOptions& target) {
    using namespace ShaderRecompiler;
    constexpr std::uint32_t supported = 0x3E | 0x380 | 0x400 | 0x1800 | 0xFF8000;
    if ((state.resourceControl & ~supported) != 0) throw std::runtime_error("Xbox native compute scratch, traps and exception modes are not implemented");
    auto userRegisters = (state.resourceControl >> 1) & 31;
    auto threadIds = ((state.resourceControl >> 11) & 3) + 1;
    if (userRegisters > 16 || threadIds > 3) throw std::invalid_argument("Xbox native compute initial register allocation is invalid");
    if (target.subgroupSize != 32 && target.subgroupSize != 64) throw std::invalid_argument("Xbox native compute target subgroup size is invalid");
    if (floatMode.floatMode > 255) throw std::invalid_argument("Xbox native compute floating mode exceeds its register field");
    std::uint64_t totalThreads = 1;
    for (auto size : state.threads) {
        if (size == 0 || size > 1024) throw std::invalid_argument("Xbox native compute workgroup dimension is invalid");
        totalThreads *= size;
    }
    if (totalThreads > 1024) throw std::invalid_argument("Xbox native compute workgroup has too many threads");
    if ((static_cast<std::uint64_t>(state.declaredSharedBytes) + 511) / 512 != ((state.resourceControl >> 15) & 511)) throw std::invalid_argument("Xbox native compute shared memory declarations disagree");
    if (target.float32FlushToZero && (target.float32DenormPreserve || (floatMode.floatMode & 0x30) != 0)) throw std::invalid_argument("Xbox native compute target float controls disagree with the initial mode");
    NativeComputeTranslation result;
    auto& input = result.input;
    std::copy(state.threads.begin(), state.threads.end(), input.threadsNum);
    input.waveSize = frontend.GetWaveSize();
    input.hostSubgroupSize = target.subgroupSize;
    input.workgroupRegister = static_cast<int>(userRegisters);
    input.threadIdsNum = static_cast<int>(threadIds);
    input.tgSizeEn = (state.resourceControl & 0x400) != 0;
    input.ldsSizeDwords = ((state.resourceControl >> 15) & 511) * 128;
    for (unsigned axis = 0; axis < 3; ++axis) input.groupId[axis] = (state.resourceControl & (128u << axis)) != 0;
    auto options = MakeNativeOptions(ShaderStageKind::Compute, input.waveSize, userRegisters, floatMode, target);
    options.sharedMemoryBytes = input.ldsSizeDwords * 4;
    options.inputInfo.compute = &input;
    auto adjusted = RemoveUnusedScratchWrites(frontend.GetDecoded());
    result.program = TranslateNativeProgram(adjusted ? *adjusted : frontend.GetDecoded(), frontend.GetGraph(), options);
    SsaBuilder{}.Rewrite(result.program);
    OptimizeNativeProgram(result.program, input.waveSize);
    return result;
}
