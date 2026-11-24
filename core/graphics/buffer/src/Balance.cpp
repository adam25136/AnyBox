#include "State.hpp"
#include <algorithm>
#include <utility>

namespace {
using AnyBox::Graphics::Buffers::AddressNode;
unsigned Height(const std::unique_ptr<AddressNode>& node) noexcept { return node ? node->height : 0; }

void Update(AddressNode& node) noexcept {
    node.height = 1 + std::max(Height(node.left), Height(node.right));
    node.maximum = node.end;
    if (node.left) node.maximum = std::max(node.maximum, node.left->maximum);
    if (node.right) node.maximum = std::max(node.maximum, node.right->maximum);
}

void Left(std::unique_ptr<AddressNode>& node) noexcept {
    auto pivot = std::move(node->right);
    node->right = std::move(pivot->left);
    Update(*node);
    pivot->left = std::move(node);
    Update(*pivot);
    node = std::move(pivot);
}

void Right(std::unique_ptr<AddressNode>& node) noexcept {
    auto pivot = std::move(node->left);
    node->left = std::move(pivot->right);
    Update(*node);
    pivot->right = std::move(node);
    Update(*pivot);
    node = std::move(pivot);
}
}

void AnyBox::Graphics::Buffers::Balance(std::unique_ptr<AddressNode>& node) noexcept {
    if (!node) return;
    Update(*node);
    auto left = Height(node->left);
    auto right = Height(node->right);
    if (left > right + 1) {
        if (Height(node->left->right) > Height(node->left->left)) Left(node->left);
        Right(node);
    } else if (right > left + 1) {
        if (Height(node->right->left) > Height(node->right->right)) Right(node->right);
        Left(node);
    }
}
