#include <anybox/buffer/AddressSpace.hpp>
#include <anybox/ExceptionTest.hpp>
#include <limits>
#include <stdexcept>

using namespace AnyBox::Graphics::Buffers;
void CheckRandomMappings();
void CheckConcurrentMappings();

namespace {
void Require(bool value) {
    if (!value) throw std::runtime_error("GPU buffer address test failed");
}
template<class TFunction>
void Reject(TFunction&& function) {
    try { function(); }
    catch (const std::exception&) { return; }
    throw std::runtime_error("Invalid GPU buffer address operation was accepted");
}

void Check() {
    AddressSpace space;
    auto owner = std::make_shared<int>(7);
    auto first = space.Register(0x1000, 256, 0x8000, owner);
    Require(first->Translate(0x1010, 16) == 0x8010);
    Require(space.Resolve(0x10FF)->Translate(0x10FF) == 0x80FF);
    Reject([&] { (void)space.Resolve(0x1100); });
    Reject([&] { (void)space.Resolve(0x10FF, 2); });
    Reject([&] { (void)first->Translate(0x1000, 0); });
    Reject([&] { (void)first->Translate(0xFFF); });
    auto alias = space.Register(0x1080, 512, 0x9000, owner);
    Require(space.Resolve(0x1080).get() == alias.get());
    Require(space.Resolve(0x10F0, 300)->Translate(0x10F0, 300) == 0x9070);
    auto snapshot = space.Capture();
    Require(snapshot.generation == 2 && snapshot.mappings.size() == 2);
    alias.reset();
    Require(space.Resolve(0x1200)->Translate(0x1200) == 0x9180);
    snapshot.mappings.clear();
    Reject([&] { (void)space.Resolve(0x1200); });
    Require(space.Capture().generation == 3);
    first.reset();
    Require(space.Capture().mappings.empty());
    Reject([&] { (void)space.Register(0, 1, 1, owner); });
    Reject([&] { (void)space.Register(1, 0, 1, owner); });
    Reject([&] { (void)space.Register(1, 1, 0, owner); });
    Reject([&] { (void)space.Register(1, 1, 1, {}); });
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    Reject([&] { (void)space.Register(maximum - 1, 2, 1, owner); });
    Reject([&] { (void)space.Register(1, 2, maximum - 1, owner); });
    Reject([&] { (void)space.Resolve(maximum, 1); });
    Reject([&] { (void)space.Resolve(1, 0); });
}

void CheckLifetime() {
    unsigned destructions = 0;
    std::shared_ptr<const Mapping> retained;
    struct Owner {
        unsigned& destructions;
        ~Owner() { ++destructions; }
    };
    {
        AddressSpace space;
        auto owner = std::shared_ptr<Owner>(new Owner{destructions});
        retained = space.Register(0x1000, 64, 0x8000, owner);
        owner.reset();
        Require(destructions == 0);
    }
    Require(retained->Translate(0x1000) == 0x8000 && destructions == 0);
    retained.reset();
    Require(destructions == 1);
}
}

int main() {
    return AnyBox::Tests::Run([] { Check(); CheckLifetime(); CheckRandomMappings(); CheckConcurrentMappings(); });
}
