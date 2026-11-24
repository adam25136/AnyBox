#pragma once

#include <IntermediateRepresentation/IrBuilder.hpp>
#include <function.hpp>
#include <instruction.hpp>
#include <optional>
#include <unordered_map>

namespace AnyBox::Shader::Dxil {
class ScalarLowering;

class LocalLowering {
public:
    LocalLowering(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrBuilder& builder);
    void Prepare(const LLVMBC::Function& function);
    bool Emit(const LLVMBC::Instruction& instruction, ScalarLowering& scalar);
    ShaderRecompiler::IrValue& Load(const LLVMBC::LoadInst& instruction, ScalarLowering& scalar);

private:
    using IrValue = ShaderRecompiler::IrValue;
    struct Allocation {
        std::uint32_t base;
        std::uint32_t bytes;
        bool shared = false;
    };
    struct Pointer {
        Allocation allocation;
        LLVMBC::Type* element;
        IrValue* offset;
        std::optional<std::int64_t> constant;
    };
    struct Access {
        IrValue& offset;
        IrValue& active;
    };
    static std::uint32_t size(const LLVMBC::Type& type);
    Pointer resolve(const LLVMBC::Value& pointer, ScalarLowering& scalar);
    Pointer element(const LLVMBC::GetElementPtrInst& instruction, ScalarLowering& scalar);
    Access access(const Pointer& pointer, const LLVMBC::Type& type);
    ShaderRecompiler::MemoryFlags flags(bool shared);
    Allocation sharedAllocation(const LLVMBC::GlobalVariable& variable);
    void store(const LLVMBC::StoreInst& instruction, ScalarLowering& scalar);
    ShaderRecompiler::IrProgram& program;
    ShaderRecompiler::IrBuilder& builder;
    std::unordered_map<const LLVMBC::Value*, Allocation> allocations;
    std::unordered_map<const LLVMBC::Value*, Pointer> pointers;
};
}
