#pragma once

#include <anybox/buffer/AddressSpace.hpp>
#include <cstddef>
#include <mutex>

namespace AnyBox::Graphics::Buffers {
struct AddressNode {
    std::uint64_t begin;
    std::uint64_t end;
    std::uint64_t identity;
    std::weak_ptr<const Mapping> mapping;
    std::unique_ptr<AddressNode> left;
    std::unique_ptr<AddressNode> right;
    std::uint64_t maximum;
    unsigned height = 1;
};

struct AddressState {
    mutable std::mutex mutex;
    std::unique_ptr<AddressNode> root;
    std::uint64_t nextIdentity = 1;
    std::uint64_t generation = 0;
    std::size_t count = 0;
};

void Balance(std::unique_ptr<AddressNode>& node) noexcept;
void Insert(std::unique_ptr<AddressNode>& root, std::unique_ptr<AddressNode> node) noexcept;
bool Erase(std::unique_ptr<AddressNode>& root, std::uint64_t begin, std::uint64_t identity) noexcept;
const AddressNode* Find(const AddressNode* root, std::uint64_t begin, std::uint64_t end) noexcept;
void Collect(const AddressNode* root, std::vector<std::shared_ptr<const Mapping>>& mappings);
}
