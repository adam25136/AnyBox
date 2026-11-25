#pragma once

#include "AtomicKernel.hpp"
#include "ComputeCompiler.hpp"

namespace BufferTests {
struct IntegerAtomicKernel : AtomicKernel {
    IrValue& Modify(bool subtract, IrValue& offset, IrValue& value, unsigned high = 2) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* wide = LLVMBC::Type::getInt64Ty(context);
        auto* address = context.construct<LLVMBC::Argument>(wide, 0, false);
        auto* input = context.construct<LLVMBC::Argument>(word, 1, false);
        scalar.Bind(*address, builder.ConstructU64(offset, builder.Constant(high)));
        scalar.Bind(*input, value);
        auto id = nextId++;
        source.Module().add_value_name(id, subtract ? "xbox.global_atomic_sub" : "xbox.global_atomic_add");
        auto* type = context.construct<LLVMBC::FunctionType>(context, word, LLVMBC::Vector<LLVMBC::Type*>{word, wide, word});
        auto* function = context.construct<LLVMBC::Function>(type, id, source.Module());
        auto* call = context.construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, subtract ? 367 : 355), address, input});
        return scalar.Get(*call);
    }
    AnyBox::Shader::ComputeCompilation Compile(unsigned wave = 32, unsigned threads = 64) {
        (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
        program.Metadata().blockInfo[0].terminator.kind = ShaderRecompiler::TerminatorKind::Return;
        program.SetWaveSize(wave);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = threads;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = wave;
        info.hostSubgroupSize = 32;
        return AnyBox::Shader::Dxil::CompileCompute(program, info, Target(), {0, 0, 0, 128});
    }
};
}
