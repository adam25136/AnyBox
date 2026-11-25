#include "FloatFixture.hpp"
#include "HalfExtremaReference.hpp"

void CheckHalfExtremaInputs(AnyBox::Vulkan::Context& context) {
    BufferTests::FloatFixture fixture;
    auto* half = LLVMBC::Type::getHalfTy(fixture.FloatType()->getContext());
    auto* pair = LLVMBC::StructType::get(half->getContext(), {half, half}, "pair");
    auto* zero = LLVMBC::ConstantFP::get(half, 0);
    auto* zeros = fixture.Make<LLVMBC::ConstantAggregateZero>(pair);
    auto* integer = fixture.IntegerType(16);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) { fixture.Reject(*fixture.Call("xbox.V_PK_MIN_F16", result, std::move(arguments))); };
    auto* opcode = fixture.Integer(32, 223);
    reject(pair, {});
    reject(pair, {opcode, zeros});
    reject(pair, {opcode, zeros, zeros, zeros});
    reject(half, {opcode, zeros, zeros});
    reject(pair, {fixture.Integer(16, 223), zeros, zeros});
    reject(pair, {fixture.Integer(32, 224), zeros, zeros});
    reject(pair, {fixture.Make<LLVMBC::Argument>(fixture.IntegerType(32), 0, false), zeros, zeros});
    reject(pair, {opcode, zero, zeros});
    auto* shortPair = fixture.Make<LLVMBC::CompositeConstructInst>(pair, LLVMBC::Vector<LLVMBC::Value*>{zero});
    reject(pair, {opcode, shortPair, zeros});
    auto* wrongPair = fixture.Make<LLVMBC::CompositeConstructInst>(pair, LLVMBC::Vector<LLVMBC::Value*>{zero, fixture.Integer(16, 0)});
    reject(pair, {opcode, wrongPair, zeros});
    auto rejectScalar = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) { fixture.Reject(*fixture.Call("dx.op.binary.f16", result, std::move(arguments))); };
    rejectScalar(half, {});
    rejectScalar(half, {fixture.Integer(32, 36), zero});
    rejectScalar(half, {fixture.Integer(32, 36), zero, zero, zero});
    rejectScalar(fixture.FloatType(), {fixture.Integer(32, 36), zero, zero});
    rejectScalar(half, {fixture.Integer(16, 36), zero, zero});
    rejectScalar(half, {fixture.Integer(32, 37), zero, zero});
    rejectScalar(half, {fixture.Integer(32, 36), fixture.FloatConstant(0), zero});
    for (bool maximum : {false, true}) {
        auto name = maximum ? "xbox.V_PK_MAX_F16" : "xbox.V_PK_MIN_F16";
        for (unsigned a : {0u, 0x8000u, 1u, 0x8001u, 0x7C00u, 0xFC00u, 0x7E13u, 0x7C01u, 0xFD55u}) {
            auto* left = fixture.Make<LLVMBC::ConstantAggregate>(pair, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantFP::get(half, a), zero});
            auto* right = fixture.Make<LLVMBC::ConstantAggregate>(pair, LLVMBC::Vector<LLVMBC::Value*>{zero, LLVMBC::ConstantFP::get(half, a)});
            auto* result = fixture.Call(name, pair, {fixture.Integer(32, maximum ? 224 : 223), left, right});
            for (unsigned index = 0; index < 2; ++index) {
                auto* part = fixture.Make<LLVMBC::ExtractValueInst>(half, result, LLVMBC::Vector<unsigned>{index});
                auto reference = BufferTests::HalfMinMaxReference(index == 0 ? a : 0, index == 0 ? 0 : a, maximum);
                fixture.Expect(*fixture.Make<LLVMBC::CastInst>(integer, part, LLVMBC::Instruction::BitCast), reference);
            }
        }
        for (auto* value : {static_cast<LLVMBC::Value*>(zeros), static_cast<LLVMBC::Value*>(LLVMBC::UndefValue::get(pair))}) {
            auto* result = fixture.Call(name, pair, {fixture.Integer(32, maximum ? 224 : 223), value, zeros});
            auto* part = fixture.Make<LLVMBC::ExtractValueInst>(half, result, LLVMBC::Vector<unsigned>{0});
            fixture.Expect(*fixture.Make<LLVMBC::CastInst>(integer, part, LLVMBC::Instruction::BitCast), 0);
            fixture.Reject(*fixture.Make<LLVMBC::ExtractValueInst>(half, result, LLVMBC::Vector<unsigned>{2}));
            fixture.Reject(*fixture.Make<LLVMBC::ExtractValueInst>(fixture.IntegerType(32), result, LLVMBC::Vector<unsigned>{0}));
        }
    }
    std::array<std::uint32_t, 16> unused{};
    fixture.Run(context, unused);
}
