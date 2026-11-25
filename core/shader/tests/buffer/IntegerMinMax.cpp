#include "AtomicKernel.hpp"
#include "FloatFixture.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <type_traits>

namespace {
const char* Name(unsigned bits) { return bits == 32 ? "dx.op.binary.i32" : "dx.op.binary.i64"; }

ShaderRecompiler::IrProgram MakeKernel(unsigned mode, unsigned bits) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& context = fixture.source.Context();
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto* word = LLVMBC::Type::getIntTy(context, bits);
    auto* opcode = LLVMBC::Type::getInt32Ty(context);
    LLVMBC::Vector<LLVMBC::Value*> arguments{LLVMBC::ConstantInt::get(opcode, mode)};
    for (unsigned operand = 0; operand < 2; ++operand) {
        auto& shifted = builder.ShiftRightLogical(index, builder.Constant((1 - operand) * 4));
        auto& selected = builder.BitwiseAnd(shifted, builder.Constant(15));
        auto& offset = builder.ShiftLeftLogical(selected, builder.Constant(bits == 32 ? 2 : 3));
        auto& low = fixture.Memory(IrOpcode::LoadAddressU32, offset, 1);
        auto& value = bits == 32 ? low : builder.ConstructU64(low, fixture.Memory(IrOpcode::LoadAddressU32, builder.IAdd(offset, builder.Constant(4)), 1));
        auto* argument = context.construct<LLVMBC::Argument>(word, operand, false);
        fixture.scalar.Bind(*argument, value);
        arguments.push_back(argument);
    }
    fixture.source.Module().add_value_name(14, Name(bits));
    auto* signature = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{opcode, word, word});
    auto* function = context.construct<LLVMBC::Function>(signature, 14, fixture.source.Module());
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    auto& result = fixture.scalar.Get(*call);
    auto& offset = builder.ShiftLeftLogical(index, builder.Constant(bits == 32 ? 2 : 3));
    auto& low = bits == 32 ? result : builder.CompositeExtract(result, 0);
    fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &low);
    if (bits == 64) fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(4)), 2, &builder.CompositeExtract(result, 1));
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return std::move(fixture.program);
}

void CheckInvalid(unsigned bits) {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(bits);
    auto* name = Name(bits);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 37), fixture.Integer(bits, 1), fixture.Integer(bits, 2)}; };
    fixture.Reject(*fixture.Call(name, word, {}));
    fixture.Reject(*fixture.Call(name, fixture.FloatType(), arguments()));
    auto shortCall = arguments();
    shortCall.pop_back();
    fixture.Reject(*fixture.Call(name, word, std::move(shortCall)));
    for (unsigned mode : {36u, 41u}) {
        auto values = arguments();
        values[0] = fixture.Integer(32, mode);
        fixture.Reject(*fixture.Call(name, word, std::move(values)));
    }
    auto dynamic = arguments();
    dynamic[0] = fixture.Make<LLVMBC::Argument>(word, 99, false);
    fixture.Reject(*fixture.Call(name, word, std::move(dynamic)));
    auto wideOpcode = arguments();
    wideOpcode[0] = fixture.Integer(64, 37);
    fixture.Reject(*fixture.Call(name, word, std::move(wideOpcode)));
    for (unsigned index = 0; index < 3; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(16, 1);
        fixture.Reject(*fixture.Call(name, word, std::move(values)));
    }
}
}

namespace BufferTests {
template<class TWord>
void CheckIntegerWidth(AnyBox::Vulkan::Context& context) {
    constexpr unsigned bits = sizeof(TWord) * 8;
    constexpr TWord maximum = std::numeric_limits<TWord>::max();
    constexpr TWord sign = TWord{1} << (bits - 1);
    CheckInvalid(bits);
    std::array<TWord, 16> input{0, 1, 2, 3, 31, 32, 255, 256, sign - 1, sign, sign + 1, maximum, maximum - 1, sign + 0x1234567, sign + 0x2345678, 0x12345678};
    for (unsigned mode : {37u, 38u, 39u, 40u}) {
        auto program = MakeKernel(mode, bits);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (auto left : input) {
            for (auto right : input) {
                TWord result;
                if (mode < 39) {
                    auto first = std::bit_cast<std::make_signed_t<TWord>>(left);
                    auto second = std::bit_cast<std::make_signed_t<TWord>>(right);
                    result = std::bit_cast<TWord>(mode == 37 ? std::max(first, second) : std::min(first, second));
                } else {
                    result = mode == 39 ? std::max(left, right) : std::min(left, right);
                }
                expected.push_back(static_cast<std::uint32_t>(result));
                if constexpr (sizeof(TWord) == 8) expected.push_back(static_cast<std::uint32_t>(result >> 32));
            }
        }
        RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(input)));
    }
}

void CheckIntegerMinMax(AnyBox::Vulkan::Context& context) {
    CheckIntegerWidth<std::uint32_t>(context);
    CheckIntegerWidth<std::uint64_t>(context);
}
}
