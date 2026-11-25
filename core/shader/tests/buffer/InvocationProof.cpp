#include "../resources/Fixture.hpp"
#include "Invocations.hpp"
#include "Fixtures.hpp"

namespace {
bool CheckCalls(std::initializer_list<std::string_view> names) {
    ResourceTests::Fixture fixture;
    auto& context = fixture.Context();
    auto& module = fixture.Module();
    auto* type = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
    auto* function = context.construct<LLVMBC::Function>(type, 0, module);
    auto* block = context.construct<LLVMBC::BasicBlock>(context);
    function->set_basic_blocks({block});
    unsigned id = 1;
    for (auto name : names) {
        module.add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        auto* callee = context.construct<LLVMBC::Function>(type, id++, module);
        block->add_instruction(context.construct<LLVMBC::CallInst>(type, callee, LLVMBC::Vector<LLVMBC::Value*>{}));
    }
    block->add_instruction(context.construct<LLVMBC::ReturnInst>(nullptr));
    return AnyBox::Shader::Dxil::HasIndependentInvocations(*function);
}
}

void CheckInvocationProof() {
    Fixtures::Require(CheckCalls({}));
    Fixtures::Require(CheckCalls({"dx.op.threadId.i32", "dx.op.barrier", "dx.op.rawBufferStore.i32"}));
    Fixtures::Require(CheckCalls({"xbox.TypedLoad.i32", "xbox.global_load_dwordx4_with_cache", "xbox.V_ALIGNBYTE_B32"}));
    for (auto name : {
        "dx.op.waveGetLaneCount", "dx.op.waveGetLaneIndex", "dx.op.waveIsFirstLane",
        "dx.op.waveReadLaneAt.i32", "dx.op.waveReadLaneFirst.f32", "dx.op.waveActiveBallot",
        "dx.op.waveAllOp", "dx.op.wavePrefixOp", "dx.op.wavePrefixOp.i32", "dx.op.waveActiveOp.i64",
        "xbox.MakeUniform.i32", "xbox.MakeUniform.i64", "xbox.Ballot64", "xbox.LaneSwizzle.i32",
        "xbox.ReadLane.i32", "xbox.GetLaneID", "xbox.WaveMin_U32", "xbox.MBCNT64",
        "xbox.V_CNDMASK_B32", "xbox.s_load_dword_with_cache", "dx.op.unknown", "userFunction"
    }) {
        Fixtures::Require(!CheckCalls({"dx.op.threadId.i32", name, "dx.op.rawBufferStore.i32"}));
    }
}
