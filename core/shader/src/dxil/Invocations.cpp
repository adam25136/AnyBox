#include "Invocations.hpp"
#include <cast.hpp>
#include <function.hpp>
#include <instruction.hpp>
#include <algorithm>
#include <array>

bool AnyBox::Shader::Dxil::IsInvocationLocalIntrinsic(std::string_view name) {
    static constexpr std::array names{
        "dx.op.createHandle", "dx.op.barrier",
        "dx.op.unary.f32", "dx.op.binary.f32", "dx.op.tertiary.f32",
        "dx.op.binary.i32", "dx.op.binary.i64", "dx.op.tertiary.i32", "dx.op.quaternary.i32",
        "dx.op.unary.i32", "dx.op.unary.i64", "dx.op.unaryBits.i32", "dx.op.unaryBits.i64",
        "dx.op.dot2.f32", "dx.op.dot3.f32", "dx.op.dot4.f32",
        "dx.op.legacyF16ToF32", "dx.op.legacyF32ToF16", "llvm.copysign.f32",
        "dx.op.threadId.i32", "dx.op.groupId.i32", "dx.op.threadIdInGroup.i32", "dx.op.flattenedThreadIdInGroup.i32",
        "dx.op.cbufferLoadLegacy.i32", "dx.op.cbufferLoadLegacy.i64", "dx.op.cbufferLoadLegacy.f32",
        "dx.op.rawBufferLoad.i16", "dx.op.rawBufferLoad.i32", "dx.op.rawBufferLoad.i64", "dx.op.rawBufferLoad.f16", "dx.op.rawBufferLoad.f32",
        "dx.op.rawBufferStore.i16", "dx.op.rawBufferStore.i32", "dx.op.rawBufferStore.i64", "dx.op.rawBufferStore.f16", "dx.op.rawBufferStore.f32", "dx.op.atomicBinOp.i32",
        "xbox.global_load_dword_with_cache", "xbox.global_load_dwordx2_with_cache",
        "xbox.global_load_dwordx3_with_cache", "xbox.global_load_dwordx4_with_cache",
        "xbox.global_store_dword_with_cache", "xbox.global_store_dwordx2_with_cache",
        "xbox.global_store_dwordx3_with_cache", "xbox.global_store_dwordx4_with_cache",
        "xbox.global_atomic_fmin", "xbox.global_atomic_fmax", "xbox.global_atomic_add", "xbox.global_atomic_sub",
        "xbox.TypedLoad.i32", "xbox.V_ALIGNBYTE_B32", "xbox.BFI", "xbox.S_BCNT1_U64",
        "xbox.Min3_F32", "xbox.Max3_F32", "xbox.Med3_F32", "xbox.Min3_U32", "xbox.Max3_U32", "xbox.Med3_U32"
    };
    return std::ranges::find(names, name) != names.end();
}

bool AnyBox::Shader::Dxil::HasIndependentInvocations(const LLVMBC::Function& function) {
    if (function.empty()) return false;
    for (const auto& block : function) {
        for (const auto& instruction : block) {
            auto* call = LLVMBC::dyn_cast<LLVMBC::CallInst>(&instruction);
            if (!call) continue;
            auto* callee = call->getCalledFunction();
            if (!callee) return false;
            const auto& name = callee->getName();
            if (!IsInvocationLocalIntrinsic(std::string_view(name.data(), name.size()))) return false;
        }
    }
    return true;
}
