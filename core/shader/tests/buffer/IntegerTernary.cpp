#include "AtomicKernel.hpp"
#include "FloatFixture.hpp"
#include <algorithm>

namespace {
const char* Name(unsigned mode) { return mode == 23 ? "xbox.BFI" : mode == 14 ? "xbox.Min3_U32" : mode == 15 ? "xbox.Max3_U32" : "xbox.Med3_U32"; }

ShaderRecompiler::IrProgram MakeKernel(unsigned mode) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    LLVMBC::Vector<LLVMBC::Value*> arguments{LLVMBC::ConstantInt::get(word, mode)};
    for (unsigned operand = 0; operand < 3; ++operand) {
        auto& shifted = builder.ShiftRightLogical(index, builder.Constant((2 - operand) * 4));
        auto& selected = builder.BitwiseAnd(shifted, builder.Constant(15));
        auto& offset = builder.ShiftLeftLogical(selected, builder.Constant(2));
        auto& value = fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
        auto* argument = context.construct<LLVMBC::Argument>(word, operand, false);
        fixture.scalar.Bind(*argument, value);
        arguments.push_back(argument);
    }
    fixture.source.Module().add_value_name(14, Name(mode));
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>(4, word));
    auto* function = context.construct<LLVMBC::Function>(signature, 14, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    auto& result = fixture.scalar.Get(*call);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &result);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}

void CheckInvalid(unsigned mode) {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, mode), fixture.Integer(32, 1), fixture.Integer(32, 2), fixture.Integer(32, 3)}; };
    fixture.Reject(*fixture.Call(Name(mode), word, {}));
    fixture.Reject(*fixture.Call(Name(mode), fixture.FloatType(), arguments()));
    auto shortCall = arguments();
    shortCall.pop_back();
    fixture.Reject(*fixture.Call(Name(mode), word, std::move(shortCall)));
    auto wrongOpcode = arguments();
    wrongOpcode[0] = fixture.Integer(32, mode + 1);
    fixture.Reject(*fixture.Call(Name(mode), word, std::move(wrongOpcode)));
    auto dynamic = arguments();
    dynamic[0] = fixture.Make<LLVMBC::Argument>(word, 99, false);
    fixture.Reject(*fixture.Call(Name(mode), word, std::move(dynamic)));
    for (unsigned index = 0; index < 4; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(16, mode);
        fixture.Reject(*fixture.Call(Name(mode), word, std::move(values)));
    }
}
}

namespace BufferTests {
void CheckIntegerTernary(AnyBox::Vulkan::Context& context) {
    std::array<std::uint32_t, 16> input{0, 1, 2, 3, 31, 32, 255, 256, 0x7FFFFFFF, 0x80000000, 0x80000001, UINT32_MAX, UINT32_MAX - 1, 0xAAAAAAAA, 0x55555555, 0x12345678};
    for (unsigned mode : {14u, 15u, 16u, 23u}) {
        CheckInvalid(mode);
        auto program = MakeKernel(mode);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (auto first : input) {
            for (auto second : input) {
                for (auto third : input) {
                    std::array values{first, second, third};
                    std::sort(values.begin(), values.end());
                    expected.push_back(mode == 23 ? (first & second) | (~first & third) : values[mode == 14 ? 0 : mode == 15 ? 2 : 1]);
                }
            }
        }
        RunGpu(context, shader, expected, {64, 1, 1}, std::as_bytes(std::span(input)));
    }
}
}
