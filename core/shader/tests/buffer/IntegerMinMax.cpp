#include "AtomicKernel.hpp"
#include "FloatFixture.hpp"
#include <algorithm>
#include <bit>

namespace {
constexpr const char* Name = "dx.op.binary.i32";

ShaderRecompiler::IrProgram MakeKernel(unsigned mode) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    LLVMBC::Vector<LLVMBC::Value*> arguments{LLVMBC::ConstantInt::get(word, mode)};
    for (unsigned operand = 0; operand < 2; ++operand) {
        auto& shifted = builder.ShiftRightLogical(index, builder.Constant((1 - operand) * 4));
        auto& selected = builder.BitwiseAnd(shifted, builder.Constant(15));
        auto& offset = builder.ShiftLeftLogical(selected, builder.Constant(2));
        auto& value = fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
        auto* argument = context.construct<LLVMBC::Argument>(word, operand, false);
        fixture.scalar.Bind(*argument, value);
        arguments.push_back(argument);
    }
    fixture.source.Module().add_value_name(14, Name);
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>(3, word));
    auto* function = context.construct<LLVMBC::Function>(signature, 14, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    auto& result = fixture.scalar.Get(*call);
    fixture.Memory(IrOpcode::StoreAddressU32, builder.ShiftLeftLogical(index, builder.Constant(2)), 2, &result);
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}

void CheckInvalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 37), fixture.Integer(32, 1), fixture.Integer(32, 2)}; };
    fixture.Reject(*fixture.Call(Name, word, {}));
    fixture.Reject(*fixture.Call(Name, fixture.FloatType(), arguments()));
    auto shortCall = arguments();
    shortCall.pop_back();
    fixture.Reject(*fixture.Call(Name, word, std::move(shortCall)));
    for (unsigned mode : {36u, 41u}) {
        auto values = arguments();
        values[0] = fixture.Integer(32, mode);
        fixture.Reject(*fixture.Call(Name, word, std::move(values)));
    }
    auto dynamic = arguments();
    dynamic[0] = fixture.Make<LLVMBC::Argument>(word, 99, false);
    fixture.Reject(*fixture.Call(Name, word, std::move(dynamic)));
    for (unsigned index = 0; index < 3; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(16, 1);
        fixture.Reject(*fixture.Call(Name, word, std::move(values)));
    }
}
}

namespace BufferTests {
void CheckIntegerMinMax(AnyBox::Vulkan::Context& context) {
    CheckInvalid();
    std::array<std::uint32_t, 16> input{0, 1, 2, 3, 31, 32, 255, 256, 0x7FFFFFFF, 0x80000000, 0x80000001, UINT32_MAX, UINT32_MAX - 1, 0xAAAAAAAA, 0x55555555, 0x12345678};
    for (unsigned mode : {37u, 38u, 39u, 40u}) {
        auto program = MakeKernel(mode);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (auto left : input) {
            for (auto right : input) {
                if (mode < 39) {
                    auto first = std::bit_cast<std::int32_t>(left);
                    auto second = std::bit_cast<std::int32_t>(right);
                    expected.push_back(std::bit_cast<std::uint32_t>(mode == 37 ? std::max(first, second) : std::min(first, second)));
                } else {
                    expected.push_back(mode == 39 ? std::max(left, right) : std::min(left, right));
                }
            }
        }
        RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(input)));
    }
}
}
