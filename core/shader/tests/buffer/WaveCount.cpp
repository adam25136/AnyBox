#include "AtomicKernel.hpp"
#include "FloatFixture.hpp"
#include "WaveKernel.hpp"
#include <bit>

namespace {
void CheckActiveCount(AnyBox::Vulkan::Context& context, std::uint32_t mask) {
    auto program = BufferTests::MakeWaveKernel(mask, 32, 135);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 32;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected(64, UINT32_MAX);
    for (unsigned thread = 0; thread < expected.size(); ++thread) {
        if ((mask & (1u << (thread & 31))) != 0) expected[thread] = std::popcount(mask);
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}

void CheckCount(AnyBox::Vulkan::Context& context, unsigned waveSize, std::uint64_t mask) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    fixture.program.SetWaveSize(waveSize);
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto& lane = builder.BitwiseAnd(index, builder.Constant(63));
    auto& selected = builder.Select(builder.ULessThan(lane, builder.Constant(32)), builder.Constant(static_cast<std::uint32_t>(mask)), builder.Constant(static_cast<std::uint32_t>(mask >> 32)));
    auto& shift = builder.BitwiseAnd(lane, builder.Constant(31));
    auto& predicate = builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(selected, shift), builder.Constant(1)), builder.Constant(0));
    auto& source = fixture.source;
    auto& symbols = source.Module();
    auto& llvmContext = source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(llvmContext);
    auto* boolean = LLVMBC::Type::getInt1Ty(llvmContext);
    auto* argument = llvmContext.construct<LLVMBC::Argument>(boolean, 0, false);
    fixture.scalar.Bind(*argument, predicate);
    symbols.add_value_name(135, "dx.op.waveAllOp");
    auto* signature = llvmContext.construct<LLVMBC::FunctionType>(llvmContext, word, LLVMBC::Vector<LLVMBC::Type*>{word, boolean});
    auto* function = llvmContext.construct<LLVMBC::Function>(signature, 135, symbols);
    auto* call = llvmContext.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 135), argument});
    auto& value = fixture.scalar.Get(*call);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &value);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = waveSize;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(fixture.program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected(128);
    for (unsigned thread = 0; thread < expected.size(); ++thread) {
        expected[thread] = waveSize == 64 ? std::popcount(mask) : std::popcount(static_cast<std::uint32_t>(mask >> (((thread / 32) & 1) * 32)));
    }
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}

void CheckInvalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        fixture.Reject(*fixture.Call("dx.op.waveAllOp", result, std::move(arguments)));
    };
    reject(word, {});
    reject(word, {fixture.Integer(32, 135)});
    reject(word, {fixture.Integer(16, 135), fixture.Integer(1, 1)});
    reject(word, {fixture.Integer(32, 134), fixture.Integer(1, 1)});
    reject(word, {fixture.Integer(32, 135), fixture.Integer(32, 1)});
    reject(fixture.FloatType(), {fixture.Integer(32, 135), fixture.Integer(1, 1)});
}
}

void BufferTests::CheckWaveCountGpu(AnyBox::Vulkan::Context& context) {
    CheckInvalid();
    std::vector<std::uint64_t> masks{0, UINT64_MAX, 0xFFFF000000000000ULL, 0xA55A8420F000000FULL};
    for (unsigned lane = 0; lane < 64; ++lane) masks.push_back(std::uint64_t{1} << lane);
    for (unsigned waveSize : {32u, 64u}) {
        for (auto mask : masks) CheckCount(context, waveSize, mask);
    }
    for (auto mask : masks) CheckActiveCount(context, static_cast<std::uint32_t>(mask));
}
