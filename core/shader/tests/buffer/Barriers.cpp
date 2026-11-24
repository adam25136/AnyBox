#include "AtomicKernel.hpp"

namespace {
void Barrier(BufferTests::AtomicKernel& fixture, unsigned mode) {
    auto& context = fixture.source.Context();
    auto& symbols = fixture.source.Module();
    auto* word = LLVMBC::Type::getInt32Ty(context);
    symbols.add_value_name(80, "dx.op.barrier");
    auto* signature = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{word, word});
    auto* function = context.construct<LLVMBC::Function>(signature, 80, symbols);
    auto* call = context.construct<LLVMBC::CallInst>(signature, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 80), LLVMBC::ConstantInt::get(word, mode)});
    fixture.scalar.Emit(*call);
}
}

namespace BufferTests {
void CheckBufferBarriers(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    for (unsigned mode = 1; mode <= 15; ++mode) {
        AtomicKernel fixture;
        auto& builder = fixture.builder;
        auto& index = fixture.GlobalIndex();
        auto& offset = builder.ShiftLeftLogical(index, builder.Constant(2));
        auto& value = builder.IAdd(builder.IMul(index, builder.Constant(17)), builder.Constant(5));
        fixture.Memory(IrOpcode::StoreAddressU32, offset, 2, &value);
        Barrier(fixture, mode);
        bool exchange = (mode & 1) != 0 && (mode & 6) != 0;
        auto& selected = exchange ? builder.BitwiseXor(index, builder.Constant(32)) : index;
        auto& loaded = fixture.Memory(IrOpcode::LoadAddressU32, builder.ShiftLeftLogical(selected, builder.Constant(2)), 2);
        fixture.Memory(IrOpcode::StoreAddressU32, builder.IAdd(offset, builder.Constant(512)), 2, &loaded);
        auto shader = fixture.Compile();
        std::vector<std::uint32_t> expected(256);
        for (unsigned thread = 0; thread < 128; ++thread) {
            expected[thread] = thread * 17 + 5;
            expected[thread + 128] = (exchange ? thread ^ 32 : thread) * 17 + 5;
        }
        std::vector<std::uint32_t> initial(expected.size(), 0);
        RunGpu(context, shader, expected, {2, 1, 1}, {}, {}, std::as_bytes(std::span(initial)));
    }
}
}
