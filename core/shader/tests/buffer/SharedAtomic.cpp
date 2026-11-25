#include "SharedAtomicFixture.hpp"
#include "Fixtures.hpp"
#include <bit>
#include <algorithm>

namespace {
std::uint32_t Evaluate(BufferTests::SharedOperation operation, std::uint32_t initial, std::uint32_t operand) {
    using Op = BufferTests::SharedOperation;
    auto left = std::bit_cast<std::int32_t>(initial);
    auto right = std::bit_cast<std::int32_t>(operand);
    switch (operation) {
        case Op::Xchg: return operand;
        case Op::Add: return initial + operand;
        case Op::Sub: return initial - operand;
        case Op::And: return initial & operand;
        case Op::Or: return initial | operand;
        case Op::Xor: return initial ^ operand;
        case Op::Max: return std::bit_cast<std::uint32_t>(std::max(left, right));
        case Op::Min: return std::bit_cast<std::uint32_t>(std::min(left, right));
        case Op::UMax: return std::max(initial, operand);
        case Op::UMin: return std::min(initial, operand);
        default: throw std::runtime_error("Invalid shared atomic reference operation");
    }
}
}

void CheckSharedAtomicInvalid();

void BufferTests::CheckSharedAtomic(AnyBox::Vulkan::Context& context) {
    CheckSharedAtomicInvalid();
    using Op = SharedOperation;
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 256;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.hostSubgroupSize = 32;
    for (unsigned wave : {32u, 64u}) {
        info.waveSize = wave;
        for (unsigned ordering : {2u, 6u}) {
            for (auto operation : {Op::Xchg, Op::Add, Op::Sub, Op::And, Op::Or, Op::Xor, Op::Max, Op::Min, Op::UMax, Op::UMin}) {
                auto program = SharedAtomicKernel(operation, ordering);
                program.SetWaveSize(wave);
                auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
                std::vector<std::uint32_t> expected;
                for (unsigned thread = 0; thread < 512; ++thread) {
                    auto initial = SharedAtomicValues[(thread >> 4) & 15];
                    auto operand = SharedAtomicValues[thread & 15];
                    expected.push_back(initial);
                    expected.push_back(Evaluate(operation, initial, operand));
                }
                RunGpu(context, shader, expected, {2, 1, 1}, std::as_bytes(std::span(SharedAtomicValues)));
            }
            auto program = SharedAtomicKernel(Op::Add, ordering, true);
            program.SetWaveSize(wave);
            auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
            std::vector<std::uint32_t> expected;
            for (unsigned thread = 0; thread < 512; ++thread) {
                expected.push_back(thread & 255);
                expected.push_back(256);
            }
            RunGpu(context, shader, expected, {2, 1, 1});
        }
    }
}
