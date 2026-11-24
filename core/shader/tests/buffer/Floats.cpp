#include "FloatFixture.hpp"
#include <bit>
#include <cmath>

namespace {
std::uint32_t PredicateMask(float left, float right) {
    bool unordered = std::isnan(left) || std::isnan(right);
    std::array<bool, 16> values{false, !unordered && left == right, !unordered && left > right, !unordered && left >= right, !unordered && left < right, !unordered && left <= right, !unordered && left != right, !unordered, unordered, unordered || left == right, unordered || left > right, unordered || left >= right, unordered || left < right, unordered || left <= right, unordered || left != right, true};
    std::uint32_t mask = 0;
    for (unsigned index = 0; index < values.size(); ++index) if (values[index]) mask |= 1u << index;
    return mask;
}
}

void BufferTests::CheckFloats(AnyBox::Vulkan::Context& context) {
    using Op = LLVMBC::Instruction;
    FloatFixture fixture;
    std::array<std::uint32_t, 16> input{0x3F800000, 0xC0000000, 0x3F000000, 0x4B800000, 0, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC12345, 0x7F812345, 1, 0x80000001, 0x4F000000, 0x4F7FFFFF, 0x3FC00000, 0xBFC00000};
    for (unsigned index = 0; index < input.size(); ++index) {
        auto* value = fixture.Argument(index);
        fixture.Expect(*fixture.FloatConstant(input[index]), input[index]);
        auto* negated = fixture.Make<LLVMBC::UnaryOperator>(LLVMBC::UnaryOperator::UnaryOps::FNeg, value);
        fixture.Expect(*negated, input[index] ^ 0x80000000);
        auto* bitcast = fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), value, Op::BitCast);
        fixture.Expect(*bitcast, input[index]);
        auto* selected = fixture.Make<LLVMBC::SelectInst>(value, fixture.Argument(0), fixture.Integer(1, 1));
        fixture.Expect(*selected, input[index]);
    }
    for (auto pair : std::array<std::array<unsigned, 2>, 8>{{{0, 1}, {2, 0}, {4, 5}, {6, 6}, {6, 7}, {8, 0}, {0, 8}, {9, 8}}}) {
        fixture.Predicates(fixture.Argument(pair[0]), fixture.Argument(pair[1]), PredicateMask(std::bit_cast<float>(input[pair[0]]), std::bit_cast<float>(input[pair[1]])));
    }
    for (auto pair : std::array<std::array<unsigned, 2>, 4>{{{0, 1}, {0, 2}, {14, 15}, {3, 0}}}) {
        auto* left = fixture.Argument(pair[0]);
        auto* right = fixture.Argument(pair[1]);
        float a = std::bit_cast<float>(input[pair[0]]);
        float b = std::bit_cast<float>(input[pair[1]]);
        fixture.Expect(*fixture.Make<LLVMBC::BinaryOperator>(left, right, Op::FAdd), std::bit_cast<std::uint32_t>(a + b));
        fixture.Expect(*fixture.Make<LLVMBC::BinaryOperator>(left, right, Op::FSub), std::bit_cast<std::uint32_t>(a - b));
        fixture.Expect(*fixture.Make<LLVMBC::BinaryOperator>(left, right, Op::FMul), std::bit_cast<std::uint32_t>(a * b));
    }
    for (unsigned bits : {1u, 8u, 16u, 32u}) {
        auto mask = (std::uint64_t{1} << bits) - 1;
        auto* value = fixture.Integer(bits, mask);
        auto* unsignedConversion = fixture.Make<LLVMBC::CastInst>(fixture.FloatType(), value, Op::UIToFP);
        auto* signedConversion = fixture.Make<LLVMBC::CastInst>(fixture.FloatType(), value, Op::SIToFP);
        fixture.Expect(*unsignedConversion, std::bit_cast<std::uint32_t>(static_cast<float>(mask)));
        fixture.Expect(*signedConversion, 0xBF800000);
    }
    fixture.Expect(*fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), fixture.Argument(14), Op::FPToSI), 1);
    fixture.Expect(*fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), fixture.Argument(15), Op::FPToSI), UINT32_MAX);
    fixture.Expect(*fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), fixture.Argument(12), Op::FPToUI), 0x80000000);
    fixture.Expect(*fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), fixture.Argument(13), Op::FPToUI), 0xFFFFFF00);
    auto* epsilon = fixture.FloatConstant(0x34000000);
    auto* aboveOne = fixture.Make<LLVMBC::BinaryOperator>(fixture.Argument(0), epsilon, Op::FAdd);
    auto* belowOne = fixture.Make<LLVMBC::BinaryOperator>(fixture.Argument(0), epsilon, Op::FSub);
    auto* product = fixture.Make<LLVMBC::BinaryOperator>(aboveOne, belowOne, Op::FMul);
    fixture.Expect(*fixture.Make<LLVMBC::BinaryOperator>(product, fixture.Argument(0), Op::FSub), 0);
    fixture.Expect(*fixture.Make<LLVMBC::BinaryOperator>(fixture.Argument(0), fixture.Argument(1), Op::FDiv), 0xBF000000);
    fixture.Reject(*fixture.Make<LLVMBC::BinaryOperator>(fixture.Argument(0), fixture.Argument(1), Op::FRem));
    fixture.Run(context, input);
}
