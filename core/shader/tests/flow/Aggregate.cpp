#include "Fixture.hpp"
#include "Fixtures.hpp"
#include "bitcode/AnyBoxInsertValue.hpp"
#include "lowering/Scalar.hpp"
#include <cast.hpp>

void FlowTests::CheckAggregates() {
    using namespace LLVMBC;
    using namespace AnyBox::Shader::Dxil;
    Fixture fixture;
    auto& context = fixture.Context();
    auto* word = Type::getInt32Ty(context);
    auto* wide = Type::getInt64Ty(context);
    auto* pair = StructType::get(context, {word, wide}, "pair");
    auto* array = ArrayType::get(pair, 2);
    auto* aggregate = UndefValue::get(array);
    auto* first = ConstantInt::get(word, 17);
    auto* second = ConstantInt::get(wide, 0x123456789ABCDEF0);
    const std::array<std::uint64_t, 2> firstPath{1, 0};
    const std::array<std::uint64_t, 2> secondPath{1, 1};
    auto* initial = Bitcode::InsertValue(context, *aggregate, *first, firstPath);
    Fixtures::Require(initial != nullptr);
    auto* updated = Bitcode::InsertValue(context, *initial, *second, secondPath);
    Fixtures::Require(updated != nullptr);
    auto* overwritten = Bitcode::InsertValue(context, *updated, *ConstantInt::get(word, 29), firstPath);
    Fixtures::Require(overwritten != nullptr);

    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrBuilder builder(program);
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder().push_back(&block);
    program.Metadata().blockInfo.push_back({});
    builder.SetInsertionPoint(block);
    ScalarLowering scalar(program, builder);
    auto extract = [&](Value* value, Type* type, Vector<unsigned> path) -> ShaderRecompiler::IrValue& {
        auto* instruction = context.construct<ExtractValueInst>(type, value, std::move(path));
        return scalar.Get(*instruction);
    };
    Fixtures::Require(extract(initial, word, {1, 0}).ImmediateU32() == 17);
    Fixtures::Require(extract(updated, wide, {1, 1}).ImmediateU64() == 0x123456789ABCDEF0);
    Fixtures::Require(extract(overwritten, word, {1, 0}).ImmediateU32() == 29);
    Fixtures::Require(extract(updated, word, {1, 0}).ImmediateU32() == 17);
    const std::array<std::uint64_t, 1> outside{2};
    Fixtures::Require(!Bitcode::InsertValue(context, *aggregate, *first, outside));
    Fixtures::Require(!Bitcode::InsertValue(context, *aggregate, *second, firstPath));
    Fixtures::Require(!Bitcode::InsertValue(context, *aggregate, *first, {}));
    Fixtures::Reject<std::runtime_error>([&] { (void)extract(updated, word, {2, 0}); });
    Fixtures::Reject<std::runtime_error>([&] { (void)extract(updated, wide, {1, 0}); });

    auto* entry = fixture.Block();
    entry->add_instruction(cast<CompositeConstructInst>(initial));
    fixture.Emit<ExtractValueInst>(entry, word, initial, Vector<unsigned>{1, 0});
    fixture.Emit<ReturnInst>(entry, nullptr);
    (void)fixture.Lower();
}
