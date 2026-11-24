#include "State.hpp"
#include <limits>
#include <stdexcept>

using namespace AnyBox::Graphics::Buffers;

AddressSpace::AddressSpace() : state(std::make_shared<AddressState>()) {}
AddressSpace::~AddressSpace() = default;

std::shared_ptr<const Mapping> AddressSpace::Register(std::uint64_t guest, std::uint64_t size, std::uint64_t device, std::shared_ptr<void> lifetime) {
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (!lifetime || guest == 0 || device == 0 || size == 0 || size > maximum - guest || size > maximum - device) throw std::invalid_argument("Buffer mapping addresses, size or lifetime are invalid");
    auto mapping = std::shared_ptr<Mapping>(new Mapping(state, guest, size, device, std::move(lifetime)));
    auto node = std::make_unique<AddressNode>();
    node->begin = guest;
    node->end = node->maximum = guest + size;
    node->mapping = mapping;
    std::scoped_lock lock(state->mutex);
    if (state->nextIdentity == maximum || state->generation == maximum) throw std::overflow_error("Buffer mapping identity or generation is exhausted");
    node->identity = state->nextIdentity++;
    mapping->identity = node->identity;
    Insert(state->root, std::move(node));
    ++state->count;
    ++state->generation;
    return mapping;
}

std::shared_ptr<const Mapping> AddressSpace::Resolve(std::uint64_t address, std::uint64_t bytes) const {
    if (address == 0 || bytes == 0 || bytes > std::numeric_limits<std::uint64_t>::max() - address) throw std::invalid_argument("Buffer lookup range is invalid");
    std::scoped_lock lock(state->mutex);
    for (;;) {
        auto* found = Find(state->root.get(), address, address + bytes);
        if (!found) throw std::out_of_range("GPU buffer address range is not registered");
        if (auto mapping = found->mapping.lock()) return mapping;
        auto begin = found->begin;
        auto identity = found->identity;
        if (state->generation == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Buffer mapping generation is exhausted");
        Erase(state->root, begin, identity);
        --state->count;
        ++state->generation;
    }
}

Snapshot AddressSpace::Capture() const {
    Snapshot snapshot;
    std::scoped_lock lock(state->mutex);
    snapshot.generation = state->generation;
    snapshot.mappings.reserve(state->count);
    Collect(state->root.get(), snapshot.mappings);
    return snapshot;
}

std::uint64_t AddressSpace::GetGeneration() const { std::scoped_lock lock(state->mutex); return state->generation; }
