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
#include "ReadLane.hpp"
#include "BitCount.hpp"
#include "FirstLane.hpp"
#include "HalfPack.hpp"
#include "UnaryInteger.hpp"
#include "WaveReadLane.hpp"
#include "WaveMinimum.hpp"
#include "WaveCount.hpp"
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
    if (identifier == "dx.op.legacyF32ToF16") return LowerFloatToHalf(instruction, *this, builder);
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
    if (identifier == "xbox.Ballot64") return LowerBallot(instruction, *this, builder, program, false);
    if (identifier == "dx.op.waveActiveBallot") return LowerBallot(instruction, *this, builder, program, true);
    if (identifier == "xbox.LaneSwizzle.i32") return LowerLaneSwizzle(instruction, *this, builder, program);
    if (identifier == "xbox.V_CNDMASK_B32") return LowerConditionalMask(instruction, *this, builder, program);
    if (identifier == "xbox.ReadLane.i32") return LowerReadLane(instruction, *this, builder, program);
    if (identifier == "dx.op.waveReadLaneAt.i1") return LowerWaveReadLane(instruction, *this, builder, program, 1);
    if (identifier == "dx.op.waveReadLaneAt.i32") return LowerWaveReadLane(instruction, *this, builder, program, 32);
    if (identifier == "dx.op.waveReadLaneAt.i64") return LowerWaveReadLane(instruction, *this, builder, program, 64);
    if (identifier == "dx.op.waveReadLaneAt.f32") return LowerWaveReadLane(instruction, *this, builder, program, 32, true);
    if (identifier == "xbox.GetLaneID") return waveBuiltin(instruction, 64);
    if (identifier == "xbox.WaveMin_U32") return LowerWaveMinimum(instruction, *this, builder, program);
    if (identifier == "xbox.S_BCNT1_U64") return LowerBitCount64(instruction, *this, builder);
    if (identifier == "xbox.MBCNT64") return LowerMaskBitCount64(instruction, *this, builder, program);
    if (identifier == "dx.op.waveReadLaneFirst.i1") return LowerFirstLane(instruction, *this, builder, program, 1);
    if (identifier == "dx.op.waveReadLaneFirst.i32") return LowerFirstLane(instruction, *this, builder, program, 32);
    if (identifier == "dx.op.waveReadLaneFirst.i64") return LowerFirstLane(instruction, *this, builder, program, 64);
    if (identifier == "dx.op.waveReadLaneFirst.f32") return LowerFirstLane(instruction, *this, builder, program, 32, true);
    if (identifier == "dx.op.dot2.f32") return floatDot(instruction, 2);
    if (identifier == "dx.op.dot3.f32") return floatDot(instruction, 3);
    if (identifier == "dx.op.dot4.f32") return floatDot(instruction, 4);
    if (identifier == "dx.op.waveIsFirstLane") return waveFirst(instruction);
    if (identifier == "dx.op.waveGetLaneIndex") return waveBuiltin(instruction, 111);
    if (identifier == "dx.op.waveGetLaneCount") return waveBuiltin(instruction, 112);
    if (identifier == "dx.op.waveAllOp") return waveCount(instruction);
    if (identifier == "dx.op.wavePrefixOp") return LowerWaveBitCount(instruction, *this, builder, program, true);
    if (identifier == "dx.op.waveActiveOp.f32") return waveReduction(instruction);
    if (identifier == "dx.op.waveActiveOp.i32" || identifier == "dx.op.wavePrefixOp.i32" || identifier == "dx.op.waveActiveOp.i64" || identifier == "dx.op.wavePrefixOp.i64") {
        if (width(instruction) != (identifier.ends_with("i64") ? 64u : 32u)) throw std::runtime_error("DXIL integer wave overload has an invalid result type");
        return waveInteger(instruction, identifier.starts_with("dx.op.wavePrefixOp"));
    }
    if (identifier == "xbox.global_atomic_fmin") return globalFloatAtomic(instruction, false);
    if (identifier == "xbox.global_atomic_fmax") return globalFloatAtomic(instruction, true);
    if (identifier == "xbox.global_atomic_add") return globalIntegerAtomic(instruction, false);
    if (identifier == "xbox.global_atomic_sub") return globalIntegerAtomic(instruction, true);
    if (identifier == "dx.op.threadId.i32") return thread(instruction, 93);
    if (identifier == "dx.op.groupId.i32") return thread(instruction, 94);
    if (identifier == "dx.op.threadIdInGroup.i32") return thread(instruction, 95);
    if (identifier == "dx.op.flattenedThreadIdInGroup.i32") return thread(instruction, 96);
    if (identifier == "dx.op.tertiary.i32") return bitField(instruction, false);
    if (identifier == "dx.op.unaryBits.i32") return unaryBits(instruction);
    if (identifier == "dx.op.unaryBits.i64") return LowerUnaryInteger(instruction, *this, builder, 64, false);
    if (identifier == "dx.op.unary.i32") return LowerUnaryInteger(instruction, *this, builder, 32, true);
    if (identifier == "dx.op.unary.i64") return LowerUnaryInteger(instruction, *this, builder, 64, true);
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
