#include "FloatFixture.hpp"

namespace BufferTests { void CheckGlobalStoreInvalid(); }

void BufferTests::CheckGlobalStoreInvalid() {
    FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto* empty = LLVMBC::Type::getVoidTy(word->getContext());
    constexpr const char* name = "xbox.global_store_dword_with_cache";
    auto* opcode = fixture.Integer(32, 360);
    auto* address = fixture.Integer(64, 0x200000000);
    auto* data = fixture.Integer(32, 123);
    auto* cache = fixture.Make<LLVMBC::UndefValue>(word);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) { fixture.Reject(*fixture.Call(name, result, std::move(arguments))); };
    reject(empty, {});
    reject(empty, {opcode, address, data, cache});
    reject(word, {opcode, address, data, cache, cache});
    reject(empty, {fixture.Integer(32, 359), address, data, cache, cache});
    reject(empty, {fixture.Integer(16, 360), address, data, cache, cache});
    reject(empty, {opcode, fixture.Integer(32, 0), data, cache, cache});
    reject(empty, {opcode, address, fixture.Integer(16, 123), cache, cache});
    reject(empty, {opcode, address, data, fixture.Integer(32, 0), cache});
    reject(empty, {opcode, address, data, cache, fixture.Integer(32, 0)});
    reject(empty, {opcode, address, data, fixture.Make<LLVMBC::UndefValue>(fixture.IntegerType(16)), cache});
    fixture.Reject(*fixture.Call("xbox.global_store_dwordx4_with_cache", empty, {fixture.Integer(32, 363), address, data, cache, cache}));
    auto* pair = LLVMBC::StructType::get(word->getContext(), {word, word}, "");
    auto rejectPair = [&](LLVMBC::Value* payload) { fixture.Reject(*fixture.Call("xbox.global_store_dwordx2_with_cache", empty, {fixture.Integer(32, 361), address, payload, cache, cache})); };
    rejectPair(fixture.Make<LLVMBC::CompositeConstructInst>(pair, LLVMBC::Vector<LLVMBC::Value*>{data}));
    rejectPair(fixture.Make<LLVMBC::ConstantDataArray>(pair, LLVMBC::Vector<LLVMBC::Value*>{data}));
    rejectPair(fixture.Make<LLVMBC::ConstantAggregate>(pair, LLVMBC::Vector<LLVMBC::Value*>{data}));
    auto* narrow = fixture.IntegerType(16);
    auto* invalid = LLVMBC::StructType::get(word->getContext(), {word, narrow}, "");
    rejectPair(fixture.Make<LLVMBC::CompositeConstructInst>(invalid, LLVMBC::Vector<LLVMBC::Value*>{data, fixture.Integer(16, 123)}));
}
