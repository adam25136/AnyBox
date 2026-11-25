#include "LocalExpression.hpp"
#include <bit>

using namespace ShaderRecompiler;
using AnyBox::Shader::Control::LocalExpression;

std::optional<std::uint32_t> LocalExpression::Evaluate(const IrValue& expression) {
    const auto& value = *expression.Resolve();
    if (value.Type() != IrType::U32 && value.Type() != IrType::Bool) return {};
    if (value.HasImmediate()) return value.Type() == IrType::Bool ? static_cast<std::uint32_t>(value.ImmediateBool()) : value.ImmediateU32();
    if (auto found = known.find(&value); found != known.end()) return found->second;
    known.emplace(&value, std::nullopt);
    auto result = evaluate(value);
    known[&value] = result;
    return result;
}

std::optional<std::uint32_t> LocalExpression::evaluate(const IrValue& value) {
    if (value.Opcode() == IrOpcode::GetBuiltin && value.ArgumentCount() == 2) {
        auto kind = Evaluate(*value.Argument(0));
        auto component = Evaluate(*value.Argument(1));
        if (!kind || !component || *component > 2) return {};
        if (*kind == static_cast<unsigned>(StageInputKind::LocalInvocationIndex)) return *component == 0 ? std::optional(invocation) : std::nullopt;
        if (*kind != static_cast<unsigned>(StageInputKind::LocalInvocationId)) return {};
        std::uint32_t divisor = 1;
        for (unsigned index = 0; index < *component; ++index) divisor *= input.threadsNum[index];
        return invocation / divisor % input.threadsNum[*component];
    }
    if (value.ArgumentCount() < 1 || value.ArgumentCount() > 3) return {};
    auto left = Evaluate(*value.Argument(0));
    if (!left) return {};
    if (value.ArgumentCount() == 1) {
        if (value.Opcode() == IrOpcode::BitwiseNot32) return ~*left;
        if (value.Opcode() == IrOpcode::LogicalNot) return !*left;
        return {};
    }
    if (value.ArgumentCount() == 3) {
        if (value.Opcode() != IrOpcode::SelectU32 && value.Opcode() != IrOpcode::SelectU1) return {};
        return Evaluate(*value.Argument(*left ? 1 : 2));
    }
    auto right = Evaluate(*value.Argument(1));
    if (!right) return {};
    auto signedLeft = std::bit_cast<std::int32_t>(*left);
    auto signedRight = std::bit_cast<std::int32_t>(*right);
    switch (value.Opcode()) {
        case IrOpcode::IAdd32: return *left + *right;
        case IrOpcode::ISub32: return *left - *right;
        case IrOpcode::IMul32: return *left * *right;
        case IrOpcode::BitwiseAnd32: return *left & *right;
        case IrOpcode::BitwiseOr32: return *left | *right;
        case IrOpcode::BitwiseXor32: return *left ^ *right;
        case IrOpcode::ShiftLeftLogical32: if (*right < 32) return *left << *right; return {};
        case IrOpcode::ShiftRightLogical32: if (*right < 32) return *left >> *right; return {};
        case IrOpcode::ShiftRightArithmetic32: if (*right < 32) return std::bit_cast<std::uint32_t>(signedLeft >> *right); return {};
        case IrOpcode::IEqual32: return *left == *right;
        case IrOpcode::INotEqual32: return *left != *right;
        case IrOpcode::ULessThan32: return *left < *right;
        case IrOpcode::ULessThanEqual32: return *left <= *right;
        case IrOpcode::UGreaterThan32: return *left > *right;
        case IrOpcode::UGreaterThanEqual32: return *left >= *right;
        case IrOpcode::SLessThan32: return signedLeft < signedRight;
        case IrOpcode::SLessThanEqual32: return signedLeft <= signedRight;
        case IrOpcode::SGreaterThan32: return signedLeft > signedRight;
        case IrOpcode::SGreaterThanEqual32: return signedLeft >= signedRight;
        case IrOpcode::LogicalAnd: return *left && *right;
        case IrOpcode::LogicalOr: return *left || *right;
        case IrOpcode::LogicalXor: return static_cast<bool>(*left) != static_cast<bool>(*right);
        default: return {};
    }
}

bool AnyBox::Shader::Control::IsWaveLocalUniform(const IrValue& expression, const ShaderComputeInputInfo& input) {
    std::uint64_t count = 1;
    for (auto size : input.threadsNum) {
        if (size == 0 || size > 1024) return false;
        count *= size;
    }
    if (count > 1024 || count % 64 != 0) return false;
    std::optional<std::uint32_t> first;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto value = LocalExpression(input, index).Evaluate(expression);
        if (!value) return false;
        if (index % 64 == 0) first = value;
        else if (value != first) return false;
    }
    return true;
}
