#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace AnyBox::Graphics::Buffers {
struct AddressState;

class Mapping {
public:
    ~Mapping();
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    std::uint64_t GetGuestAddress() const noexcept { return guest; }
    std::uint64_t GetSize() const noexcept { return size; }
    std::uint64_t GetDeviceAddress() const noexcept { return device; }
    std::uint64_t Translate(std::uint64_t address, std::uint64_t bytes = 1) const;

private:
    friend class AddressSpace;
    Mapping(std::shared_ptr<AddressState> state, std::uint64_t guest, std::uint64_t size, std::uint64_t device, std::shared_ptr<void> lifetime);
    std::shared_ptr<AddressState> state;
    std::shared_ptr<void> lifetime;
    std::uint64_t guest;
    std::uint64_t size;
    std::uint64_t device;
    std::uint64_t identity = 0;
};

struct Snapshot {
    std::uint64_t generation;
    std::vector<std::shared_ptr<const Mapping>> mappings;
};

class AddressSpace {
public:
    AddressSpace();
    ~AddressSpace();
    AddressSpace(const AddressSpace&) = delete;
    AddressSpace& operator=(const AddressSpace&) = delete;
    std::shared_ptr<const Mapping> Register(std::uint64_t guest, std::uint64_t size, std::uint64_t device, std::shared_ptr<void> lifetime);
    std::shared_ptr<const Mapping> Resolve(std::uint64_t address, std::uint64_t bytes = 1) const;
    Snapshot Capture() const;
    std::uint64_t GetGeneration() const;

private:
    std::shared_ptr<AddressState> state;
};
}
