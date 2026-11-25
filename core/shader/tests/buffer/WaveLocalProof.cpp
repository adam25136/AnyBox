#include "AtomicKernel.hpp"
#include "../../src/backend/control/LocalExpression.hpp"
#include "Fixtures.hpp"
#include <algorithm>

namespace BufferTests {
void CheckWaveLocalProof() {
    using namespace ShaderRecompiler;
    using AnyBox::Shader::Control::IsWaveLocalUniform;
    AtomicKernel fixture;
    auto& builder = fixture.builder;
    auto builtin = [&](StageInputKind kind, unsigned component) -> IrValue& {
        return builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(kind)), &builder.Constant(component)});
    };
    auto& local = builtin(StageInputKind::LocalInvocationIndex, 0);
    auto& x = builtin(StageInputKind::LocalInvocationId, 0);
    auto& y = builtin(StageInputKind::LocalInvocationId, 1);
    auto& z = builtin(StageInputKind::LocalInvocationId, 2);
    auto& global = builtin(StageInputKind::GlobalInvocationId, 0);
    auto& unknown = fixture.Memory(IrOpcode::LoadAddressU32, builder.Constant(0), 1);
    ShaderComputeInputInfo input;
    input.waveSize = 64;
    input.hostSubgroupSize = 32;
    input.threadsNum[0] = 128;
    input.threadsNum[1] = input.threadsNum[2] = 1;
    for (unsigned threshold : {0u, 64u, 128u, 256u, UINT32_MAX}) {
        Fixtures::Require(IsWaveLocalUniform(builder.ULessThan(local, builder.Constant(threshold)), input));
        Fixtures::Require(IsWaveLocalUniform(builder.ULessThan(x, builder.Constant(threshold)), input));
    }
    for (unsigned threshold : {1u, 31u, 32u, 63u, 65u, 127u}) Fixtures::Require(!IsWaveLocalUniform(builder.ULessThan(local, builder.Constant(threshold)), input));
    auto& shifted = builder.ShiftRightLogical(local, builder.Constant(6));
    auto& parity = builder.INotEqual(builder.BitwiseAnd(shifted, builder.Constant(1)), builder.Constant(0));
    Fixtures::Require(IsWaveLocalUniform(parity, input));
    Fixtures::Require(!IsWaveLocalUniform(builder.IEqual(local, builder.Constant(0)), input));
    Fixtures::Require(!IsWaveLocalUniform(global, input) && !IsWaveLocalUniform(unknown, input));
    Fixtures::Require(!IsWaveLocalUniform(builder.ShiftLeftLogical(shifted, builder.Constant(32)), input));
    auto& selected = builder.Select(parity, builder.Constant(7), builder.Constant(11));
    Fixtures::Require(IsWaveLocalUniform(selected, input));
    Fixtures::Require(IsWaveLocalUniform(builder.IAdd(selected, builder.Constant(UINT32_MAX)), input));
    for (auto dimensions : {std::array<unsigned, 3>{64, 2, 1}, {32, 4, 1}, {16, 2, 4}, {128, 2, 2}}) {
        std::copy(dimensions.begin(), dimensions.end(), input.threadsNum);
        Fixtures::Require(IsWaveLocalUniform(builder.ULessThan(y, builder.Constant(1)), input) == (dimensions[0] >= 64));
        Fixtures::Require(IsWaveLocalUniform(builder.INotEqual(z, builder.Constant(0)), input) == (dimensions[0] * dimensions[1] >= 64));
        Fixtures::Require(IsWaveLocalUniform(builder.ULessThan(x, builder.Constant(32)), input) == (dimensions[0] <= 32));
    }
    for (unsigned count : {0u, 1u, 65u, 1025u}) {
        input.threadsNum[0] = count;
        input.threadsNum[1] = input.threadsNum[2] = 1;
        Fixtures::Require(!IsWaveLocalUniform(parity, input));
    }
}
}
