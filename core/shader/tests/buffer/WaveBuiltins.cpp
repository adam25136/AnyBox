#include "AtomicKernel.hpp"
#include "FloatFixture.hpp"

namespace {
void CheckInvalid(const char* name, unsigned opcode) {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        fixture.Reject(*fixture.Call(name, result, std::move(arguments)));
    };
    reject(word, {});
    reject(word, {fixture.Integer(32, opcode), fixture.Integer(32, 0)});
    reject(fixture.FloatType(), {fixture.Integer(32, opcode)});
    reject(word, {fixture.Integer(16, opcode)});
    reject(word, {fixture.Integer(32, opcode + 1)});
    reject(word, {fixture.Argument(0)});
}

void CheckGpu(AnyBox::Vulkan::Context& context, unsigned waveSize, bool count) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    fixture.program.SetWaveSize(waveSize);
    auto& symbols = fixture.source.Module();
    auto& llvmContext = fixture.source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(llvmContext);
    auto opcode = count ? 112u : 111u;
    symbols.add_value_name(111, count ? "dx.op.waveGetLaneCount" : "dx.op.waveGetLaneIndex");
    auto* signature = llvmContext.construct<LLVMBC::FunctionType>(llvmContext, word, LLVMBC::Vector<LLVMBC::Type*>{word});
    auto* function = llvmContext.construct<LLVMBC::Function>(signature, 111, symbols);
    auto* call = llvmContext.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, opcode)});
    auto& value = fixture.scalar.Get(*call);
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &value);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});

    ShaderComputeInputInfo info;
    info.threadsNum[0] = 128;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = waveSize;
    info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(fixture.program, info, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected(256);
    for (unsigned thread = 0; thread < expected.size(); ++thread) expected[thread] = count ? waveSize : thread % waveSize;
    BufferTests::RunGpu(context, shader, expected, {2, 1, 1});
}
}

void BufferTests::CheckWaveBuiltinsGpu(AnyBox::Vulkan::Context& context) {
    CheckInvalid("dx.op.waveGetLaneIndex", 111);
    CheckInvalid("dx.op.waveGetLaneCount", 112);
    for (auto waveSize : {32u, 64u}) {
        CheckGpu(context, waveSize, false);
        CheckGpu(context, waveSize, true);
    }
}
