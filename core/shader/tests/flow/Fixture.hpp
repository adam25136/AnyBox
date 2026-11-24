#pragma once

#include "Session.hpp"
#include "lowering/Function.hpp"
#include <context.hpp>
#include <module.hpp>

#undef llvm

namespace FlowTests {
class Fixture {
public:
    Fixture() : module(context), function(context.construct<LLVMBC::Function>(context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{}), 0, module)) {}
    LLVMBC::BasicBlock* Block() {
        auto* block = context.construct<LLVMBC::BasicBlock>(context);
        blocks.push_back(block);
        return block;
    }
    LLVMBC::ConstantInt* Constant(std::uint32_t value) { return LLVMBC::ConstantInt::get(LLVMBC::Type::getInt16Ty(context), value); }
    template<class TInstruction, class... TArguments>
    TInstruction* Emit(LLVMBC::BasicBlock* block, TArguments&&... arguments) {
        auto* value = context.construct<TInstruction>(std::forward<TArguments>(arguments)...);
        block->add_instruction(value);
        return value;
    }
    ShaderRecompiler::IrProgram Lower() {
        function->set_basic_blocks(blocks);
        ShaderRecompiler::IrProgram output;
        AnyBox::Shader::Dxil::FunctionLowering(output).Run(*function);
        return output;
    }
    LLVMBC::LLVMContext& Context() { return context; }

private:
    AnyBox::Shader::DxilSession session;
    LLVMBC::LLVMContext context;
    LLVMBC::Module module;
    LLVMBC::Function* function;
    LLVMBC::Vector<LLVMBC::BasicBlock*> blocks;
};

std::vector<std::uint32_t> Evaluate(const ShaderRecompiler::IrProgram& program);
void CheckSwitches();
void CheckGlobalLoads();
void CheckBarriers();
void CheckAggregates();
}
