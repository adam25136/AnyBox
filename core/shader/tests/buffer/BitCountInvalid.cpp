#include "FloatFixture.hpp"

void CheckBitCountInvalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto* pair = LLVMBC::StructType::get(fixture.IntegerType(32)->getContext(), {word, word}, "mask");
    auto* input = fixture.Make<LLVMBC::ConstantDataArray>(pair, LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 1), fixture.Integer(32, 1)});
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 130), input}; };
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call("xbox.S_BCNT1_U64", result, std::move(values))); };
    reject(word, {});
    reject(fixture.IntegerType(64), arguments());
    reject(fixture.FloatType(), arguments());
    auto extra = arguments();
    extra.push_back(input);
    reject(word, std::move(extra));
    for (auto* opcode : std::array<LLVMBC::Value*, 3>{fixture.Integer(16, 130), fixture.Integer(32, 131), fixture.Make<LLVMBC::Argument>(word, 0, false)}) {
        auto values = arguments();
        values[0] = opcode;
        reject(word, std::move(values));
    }
    for (auto* payload : std::array<LLVMBC::Value*, 3>{fixture.Integer(64, 1), fixture.Make<LLVMBC::ConstantAggregateZero>(LLVMBC::VectorType::get(3, word)), fixture.Make<LLVMBC::ConstantAggregateZero>(LLVMBC::VectorType::get(2, fixture.IntegerType(16)))}) {
        auto values = arguments();
        values[1] = payload;
        reject(word, std::move(values));
    }
    auto values = arguments();
    values[1] = fixture.Make<LLVMBC::ConstantDataArray>(pair, LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 1)});
    reject(word, std::move(values));
    values = arguments();
    values[1] = fixture.Make<LLVMBC::ConstantDataArray>(pair, LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(16, 1), fixture.Integer(32, 1)});
    reject(word, std::move(values));
    values = arguments();
    values[1] = fixture.Call("unsupported.word.pair", pair, {});
    reject(word, std::move(values));
}
