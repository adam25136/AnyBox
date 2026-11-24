#include "WaveKernel.hpp"
#include "Fixtures.hpp"
#include <Recompiler.hpp>
#include <bit>

namespace {

void CheckInvalid(AnyBox::Vulkan::Context& context) {
    BufferTests::FloatFixture fixture;
    auto* boolean = fixture.IntegerType(1);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        fixture.Reject(*fixture.Call("dx.op.waveIsFirstLane", result, std::move(arguments)));
    };
    reject(boolean, {});
    reject(boolean, {fixture.Integer(32, 110), fixture.Integer(32, 0)});
    reject(fixture.IntegerType(32), {fixture.Integer(32, 110)});
    reject(boolean, {fixture.Integer(16, 110)});
    reject(boolean, {fixture.Integer(32, 111)});
    reject(boolean, {fixture.Argument(0)});
    auto* dynamic = fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), fixture.Argument(0), LLVMBC::Instruction::BitCast);
    reject(boolean, {dynamic});
    auto* first = fixture.Call("dx.op.waveIsFirstLane", boolean, {fixture.Integer(32, 110)});
    fixture.Expect(*fixture.Make<LLVMBC::SelectInst>(fixture.Integer(32, 1), fixture.Integer(32, 0), first), 1);
    std::array<std::uint32_t, 16> input{};
    fixture.Run(context, input);
}
}

void BufferTests::CheckWaveFirstGpu(AnyBox::Vulkan::Context& context) {
    std::vector<std::uint64_t> masks{0, 1, UINT64_MAX, 0xA55A8420A55A8420ULL, 0xFFFF000000000000ULL, 0x8000000080000000ULL};
    for (unsigned lane = 0; lane < 64; ++lane) masks.push_back(std::uint64_t{1} << lane);
    for (unsigned waveSize : {32u, 64u}) {
        for (auto mask : masks) {
            auto program = MakeWaveKernel(mask, waveSize);
            ShaderRecompiler::ShaderComputeInputInfo info;
            info.threadsNum[0] = waveSize;
            info.threadsNum[1] = info.threadsNum[2] = 1;
            info.waveSize = waveSize;
            info.hostSubgroupSize = 32;
            if (waveSize == 64) {
                if (mask != UINT64_MAX) {
                    Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128}); });
                    continue;
                }
                auto& entry = program.EntryBlock();
                auto* exit = program.BlockOrder().back();
                std::erase(entry.Successors(), exit);
                std::erase(exit->Predecessors(), &entry);
                auto& terminator = program.Metadata().blockInfo.front().terminator;
                terminator.kind = ShaderRecompiler::TerminatorKind::Branch;
                terminator.falseBlock = terminator.mergeBlock = ShaderRecompiler::InvalidControlFlowId;
            }
            auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
            auto used = waveSize == 32 ? mask & UINT32_MAX : mask;
            auto first = static_cast<unsigned>(std::countr_zero(used));
            std::vector<std::uint32_t> expected(waveSize * 2, UINT32_MAX);
            for (unsigned index = 0; index < expected.size(); ++index) {
                auto lane = index % waveSize;
                if (((used >> lane) & 1) != 0) expected[index] = lane == first ? 1 : 0;
            }
            RunGpu(context, shader, expected, {2, 1, 1});
        }
    }
    CheckInvalid(context);
}
