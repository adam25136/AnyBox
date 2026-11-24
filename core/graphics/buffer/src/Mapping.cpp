#include "State.hpp"
#include <exception>
#include <limits>
#include <stdexcept>

using namespace AnyBox::Graphics::Buffers;

Mapping::Mapping(std::shared_ptr<AddressState> state, std::uint64_t guest, std::uint64_t size, std::uint64_t device, std::shared_ptr<void> lifetime) : state(std::move(state)), lifetime(std::move(lifetime)), guest(guest), size(size), device(device) {}

Mapping::~Mapping() {
    if (identity == 0) return;
    std::scoped_lock lock(state->mutex);
    if (Erase(state->root, guest, identity)) {
        if (state->generation == std::numeric_limits<std::uint64_t>::max()) std::terminate();
        --state->count;
        ++state->generation;
    }
}

std::uint64_t Mapping::Translate(std::uint64_t address, std::uint64_t bytes) const {
    if (bytes == 0 || address < guest || address - guest >= size || bytes > size - (address - guest)) throw std::out_of_range("Buffer translation exceeds the registered mapping");
    return device + (address - guest);
}
