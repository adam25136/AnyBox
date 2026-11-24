#pragma once

#include "AtomicKernel.hpp"
#include "lowering/Buffer.hpp"
#include <optional>

namespace BufferTests {
class BufferAtomicBindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding&, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override {
        if (!index.HasImmediate() || index.ImmediateU32() != 4 || nonUniform) throw std::runtime_error("Unexpected atomic test resource selection");
        return {builder.ConstantU64(0x200000000), builder.ConstantU64(1024)};
    }
};

class BufferAtomicFixture : public AtomicKernel {
public:
    explicit BufferAtomicFixture(bool structured, unsigned category = 1) : structured(structured) {
        source.Set(category, source.Buffer(category, structured ? 8 : 0));
        layout.emplace(source.Module());
        buffers.emplace(program, builder, *layout, bindings);
        scalar.SetBuffers(*buffers);
        handle = source.Handle(category, 4);
        scalar.Emit(*handle);
    }
    LLVMBC::Value* Integer(unsigned bits, std::uint64_t value) { return LLVMBC::ConstantInt::get(LLVMBC::Type::getIntTy(source.Context(), bits), value); }
    LLVMBC::Value* Undefined(unsigned bits = 32) { return LLVMBC::UndefValue::get(LLVMBC::Type::getIntTy(source.Context(), bits)); }
    LLVMBC::Vector<LLVMBC::Value*> Arguments(unsigned mode) { return {Integer(32, 78), handle, Integer(32, mode), Integer(32, 0), structured ? Integer(32, 0) : Undefined(), Undefined(), Integer(32, 1234)}; }
    LLVMBC::CallInst* Call(LLVMBC::Vector<LLVMBC::Value*> arguments, LLVMBC::Type* result = nullptr) {
        auto& context = source.Context();
        if (!result) result = LLVMBC::Type::getInt32Ty(context);
        auto id = nextId++;
        source.Module().add_value_name(id, "dx.op.atomicBinOp.i32");
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* signature = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* function = context.construct<LLVMBC::Function>(signature, id, source.Module());
        return context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    }
    LLVMBC::Value* Bind(ShaderRecompiler::IrValue& value) {
        auto* argument = source.Context().construct<LLVMBC::Argument>(LLVMBC::Type::getInt32Ty(source.Context()), nextId++, false);
        scalar.Bind(*argument, value);
        return argument;
    }

private:
    bool structured;
    BufferAtomicBindings bindings;
    std::optional<AnyBox::Shader::Dxil::ResourceLayout> layout;
    std::optional<AnyBox::Shader::Dxil::BufferLowering> buffers;
    LLVMBC::CallInst* handle;
};
}
