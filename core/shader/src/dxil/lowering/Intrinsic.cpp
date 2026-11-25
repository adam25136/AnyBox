#include "Scalar.hpp"
#include "Buffer.hpp"
#include "FloatBits.hpp"
#include "AddressLoad.hpp"
#include "IntegerExtrema.hpp"
#include "BitSelect.hpp"
#include "MakeUniform.hpp"
#include "Ballot.hpp"
#include "LaneSwizzle.hpp"
#include "ConditionalMask.hpp"
#include <cast.hpp>
#include <function.hpp>
#include <string_view>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::call(const LLVMBC::CallInst& instruction) {
    auto* function = instruction.getCalledFunction();
    if (!function) throw std::runtime_error("DXIL indirect function call is not implemented");
    const auto& name = function->getName();
    auto identifier = std::string_view(name.data(), name.size());
    if (identifier == "dx.op.atomicBinOp.i32") {
        if (!buffers) throw std::runtime_error("DXIL buffer atomic has no resource bindings");
        return buffers->Atomic(instruction, *this);
    }
    if (identifier == "dx.op.barrier") return barrier(instruction);
    if (identifier == "dx.op.unary.f32") return LowerUnaryFloat(instruction, *this, builder);
    if (identifier == "llvm.copysign.f32") return LowerCopySign(instruction, *this, builder);
    if (identifier == "dx.op.legacyF16ToF32") return legacyHalf(instruction);
    if (identifier == "dx.op.binary.f32") return floatBinaryIntrinsic(instruction);
    if (identifier == "dx.op.binary.i32") return LowerIntegerExtrema(instruction, *this, builder, 32);
    if (identifier == "dx.op.binary.i64") return LowerIntegerExtrema(instruction, *this, builder, 64);
    if (identifier == "dx.op.tertiary.f32") return floatTernaryIntrinsic(instruction);
    if (identifier == "xbox.Min3_F32") return floatExtrema3(instruction, 8);
    if (identifier == "xbox.Max3_F32") return floatExtrema3(instruction, 9);
    if (identifier == "xbox.Med3_F32") return floatExtrema3(instruction, 10);
    if (identifier == "xbox.Min3_U32") return LowerUnsignedExtrema3(instruction, *this, builder, 14);
    if (identifier == "xbox.Max3_U32") return LowerUnsignedExtrema3(instruction, *this, builder, 15);
    if (identifier == "xbox.Med3_U32") return LowerUnsignedExtrema3(instruction, *this, builder, 16);
    if (identifier == "xbox.BFI") return LowerBitSelect(instruction, *this, builder);
    if (identifier == "xbox.MakeUniform.i32") return LowerMakeUniform(instruction, *this, builder, program, 32);
    if (identifier == "xbox.MakeUniform.i64") return LowerMakeUniform(instruction, *this, builder, program, 64);
    if (identifier == "xbox.Ballot64") return LowerBallot64(instruction, *this, builder, program);
    if (identifier == "xbox.LaneSwizzle.i32") return LowerLaneSwizzle(instruction, *this, builder, program);
    if (identifier == "xbox.V_CNDMASK_B32") return LowerConditionalMask(instruction, *this, builder, program);
    if (identifier == "dx.op.dot2.f32") return floatDot(instruction, 2);
    if (identifier == "dx.op.dot3.f32") return floatDot(instruction, 3);
    if (identifier == "dx.op.dot4.f32") return floatDot(instruction, 4);
    if (identifier == "dx.op.waveIsFirstLane") return waveFirst(instruction);
    if (identifier == "dx.op.waveGetLaneIndex") return waveBuiltin(instruction, 111);
    if (identifier == "dx.op.waveGetLaneCount") return waveBuiltin(instruction, 112);
    if (identifier == "dx.op.waveAllOp") return waveCount(instruction);
    if (identifier == "dx.op.waveActiveOp.f32") return waveReduction(instruction);
    if (identifier == "dx.op.waveActiveOp.i32") return waveInteger(instruction, false);
    if (identifier == "dx.op.wavePrefixOp.i32") return waveInteger(instruction, true);
    if (identifier == "xbox.global_atomic_fmin") return globalFloatAtomic(instruction, false);
    if (identifier == "xbox.global_atomic_fmax") return globalFloatAtomic(instruction, true);
    if (identifier == "dx.op.threadId.i32") return thread(instruction, 93);
    if (identifier == "dx.op.groupId.i32") return thread(instruction, 94);
    if (identifier == "dx.op.threadIdInGroup.i32") return thread(instruction, 95);
    if (identifier == "dx.op.flattenedThreadIdInGroup.i32") return thread(instruction, 96);
    if (identifier == "dx.op.tertiary.i32") return bitField(instruction, false);
    if (identifier == "dx.op.unaryBits.i32") return unaryBits(instruction);
    if (identifier == "dx.op.quaternary.i32") return bitField(instruction, true);
    if (identifier == "xbox.global_load_dword_with_cache") return globalLoad(instruction, 1);
    if (identifier == "xbox.s_load_dword_with_cache") return LowerAddressLoad(instruction, *this, builder, program, 1, 364);
    if (identifier == "xbox.global_load_dwordx2_with_cache") return globalLoad(instruction, 2);
    if (identifier == "xbox.global_load_dwordx3_with_cache") return globalLoad(instruction, 3);
    if (identifier == "xbox.global_load_dwordx4_with_cache") return globalLoad(instruction, 4);
    if (identifier == "xbox.global_store_dword_with_cache") return globalStore(instruction, 1);
    if (identifier == "xbox.global_store_dwordx2_with_cache") return globalStore(instruction, 2);
    if (identifier == "xbox.global_store_dwordx3_with_cache") return globalStore(instruction, 3);
    if (identifier == "xbox.global_store_dwordx4_with_cache") return globalStore(instruction, 4);
    if (identifier != "xbox.V_ALIGNBYTE_B32") throw std::runtime_error("DXIL intrinsic is not implemented: " + std::string(name.data(), name.size()));
    if (instruction.getNumArgOperands() != 4 || width(instruction) != 32) throw std::runtime_error("Xbox byte alignment intrinsic has an invalid signature");
    for (unsigned index = 0; index < 4; ++index) {
        if (width(*instruction.getArgOperand(index)) != 32) throw std::runtime_error("Xbox byte alignment intrinsic requires 32-bit operands");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 332) throw std::runtime_error("Xbox byte alignment intrinsic has an invalid opcode");

    auto& high = Get(*instruction.getArgOperand(1));
    auto& low = Get(*instruction.getArgOperand(2));
    auto& offset = Get(*instruction.getArgOperand(3));
    auto& bytes = builder.BitwiseAnd(offset, builder.Constant(3));
    auto& bits = builder.ShiftLeftLogical(bytes, builder.Constant(3));
    auto& source = builder.ConstructU64(low, high);
    auto& shifted = builder.Emit(IrOpcode::ShiftRightLogical64, IrType::U64, {&source, &bits});
    return builder.CompositeExtract(shifted, 0);
}
