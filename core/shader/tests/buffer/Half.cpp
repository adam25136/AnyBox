#include "../resources/Fixture.hpp"
#include "lowering/Scalar.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"
#include <Recompiler.hpp>

void CheckHalfPackGpu(AnyBox::Vulkan::Context& context);

namespace {
using namespace ShaderRecompiler;

LLVMBC::CallInst* Call(ResourceTests::Fixture& source, const char* name, unsigned id, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
    auto& context = source.Context();
    source.Module().add_value_name(id, name);
    LLVMBC::Vector<LLVMBC::Type*> parameters;
    for (auto* argument : arguments) parameters.push_back(argument->getType());
    auto* signature = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
    auto* function = context.construct<LLVMBC::Function>(signature, id, source.Module());
    return context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
}

IrProgram Make(std::uint32_t highBits) {
    ResourceTests::Fixture source;
    auto& context = source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto integer = [&](unsigned value) { return LLVMBC::ConstantInt::get(word, value); };
    IrProgram program;
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder() = {&block};
    program.Metadata().blockInfo.resize(1);
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    AnyBox::Shader::Dxil::ScalarLowering scalar(program, builder);
    auto& index = scalar.Get(*Call(source, "dx.op.threadId.i32", 93, word, {integer(93), integer(0)}));
    auto& packed = builder.BitwiseOr(index, builder.Constant(highBits));
    auto* argument = context.construct<LLVMBC::Argument>(word, 0, false);
    scalar.Bind(*argument, packed);
    auto& converted = scalar.Get(*Call(source, "dx.op.legacyF16ToF32", 131, LLVMBC::Type::getFloatTy(context), {integer(131), argument}));
    auto& nan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&converted});
    auto& bits = builder.Select(nan, builder.Constant(0x7FC00000U), builder.BitCastU32(converted));
    auto& low = builder.ShiftLeftLogical(index, builder.Constant(2));
    auto& high = builder.Constant(2);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    program.Resources().memoryInfo.push_back(memory);
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &low, &high, &bits, &builder.ConstantBool(true)}, MemoryFlags{0, 0});
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    return program;
}

std::uint32_t Expected(std::uint32_t half) {
    auto sign = (half & 0x8000U) << 16;
    auto exponent = (half >> 10) & 31;
    auto fraction = half & 1023;
    if (exponent == 31) return fraction == 0 ? sign | 0x7F800000U : 0x7FC00000U;
    if (exponent != 0) return sign | ((exponent + 112) << 23) | (fraction << 13);
    if (fraction == 0) return sign;
    int power = -14;
    while ((fraction & 1024) == 0) { fraction <<= 1; --power; }
    return sign | (static_cast<std::uint32_t>(power + 127) << 23) | ((fraction & 1023) << 13);
}

void CheckInvalid() {
    ResourceTests::Fixture source;
    auto& context = source.Context();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto* floating = LLVMBC::Type::getFloatTy(context);
    auto integer = [&](unsigned value) { return LLVMBC::ConstantInt::get(word, value); };
    IrProgram program;
    IrBuilder builder(program);
    AnyBox::Shader::Dxil::ScalarLowering scalar(program, builder);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto* instruction = Call(source, "dx.op.legacyF16ToF32", 131, result, std::move(arguments));
        Fixtures::Reject<std::runtime_error>([&] { scalar.Get(*instruction); });
    };
    reject(floating, {});
    reject(floating, {integer(131)});
    reject(floating, {integer(131), integer(0), integer(0)});
    reject(floating, {integer(130), integer(0)});
    reject(word, {integer(131), integer(0)});
    reject(floating, {integer(131), LLVMBC::ConstantInt::get(LLVMBC::Type::getInt16Ty(context), 0)});
}
}

void BufferTests::CheckLegacyHalfGpu(AnyBox::Vulkan::Context& context) {
    CheckInvalid();
    CheckHalfPackGpu(context);
    std::vector<std::uint32_t> expected(65536);
    for (unsigned half = 0; half < expected.size(); ++half) expected[half] = Expected(half);
    for (std::uint32_t highBits : {0U, 0xABCD0000U, 0xFFFF0000U}) {
        auto program = Make(highBits);
        ShaderComputeInputInfo input;
        input.threadsNum[0] = 64;
        input.threadsNum[1] = input.threadsNum[2] = 1;
        input.waveSize = input.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, input, Target(), {0, 0, 0, 128});
        for (std::size_t position = 5; position < shader.code.size();) {
            auto count = shader.code[position] >> 16;
            auto opcode = shader.code[position] & 65535;
            Fixtures::Require(count != 0 && count <= shader.code.size() - position);
            if (opcode == spv::OpTypeFloat || opcode == spv::OpTypeInt) Fixtures::Require(shader.code[position + 2] != 16);
            position += count;
        }
        RunGpu(context, shader, expected, {1024, 1, 1});
    }
}
