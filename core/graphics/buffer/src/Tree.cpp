#include "State.hpp"
#include <utility>

namespace AnyBox::Graphics::Buffers {
void Insert(std::unique_ptr<AddressNode>& root, std::unique_ptr<AddressNode> node) noexcept {
    if (!root) { root = std::move(node); return; }
    if (std::pair(node->begin, node->identity) < std::pair(root->begin, root->identity)) Insert(root->left, std::move(node));
    else Insert(root->right, std::move(node));
    Balance(root);
}

bool Erase(std::unique_ptr<AddressNode>& root, std::uint64_t begin, std::uint64_t identity) noexcept {
    if (!root) return false;
    auto key = std::pair(begin, identity);
    auto current = std::pair(root->begin, root->identity);
    bool erased;
    if (key < current) erased = Erase(root->left, begin, identity);
    else if (key > current) erased = Erase(root->right, begin, identity);
    else {
        erased = true;
        if (!root->left) {
            auto replacement = std::move(root->right);
            root = std::move(replacement);
        } else if (!root->right) {
            auto replacement = std::move(root->left);
            root = std::move(replacement);
        } else {
            auto* successor = root->right.get();
            while (successor->left) successor = successor->left.get();
            root->begin = successor->begin;
            root->end = successor->end;
            root->identity = successor->identity;
            root->mapping = successor->mapping;
            Erase(root->right, successor->begin, successor->identity);
        }
    }
    if (erased) Balance(root);
    return erased;
}

const AddressNode* Find(const AddressNode* root, std::uint64_t begin, std::uint64_t end) noexcept {
    if (!root || root->maximum < end) return nullptr;
    auto* selected = Find(root->left.get(), begin, end);
    if (root->begin > begin) return selected;
    if (root->end >= end && (!selected || root->end > selected->end)) selected = root;
    auto* right = root->right && (!selected || root->right->maximum > selected->end) ? Find(root->right.get(), begin, selected ? selected->end + 1 : end) : nullptr;
    if (right && (!selected || right->end > selected->end)) selected = right;
    return selected;
}

void Collect(const AddressNode* root, std::vector<std::shared_ptr<const Mapping>>& mappings) {
    if (!root) return;
    Collect(root->left.get(), mappings);
    if (auto mapping = root->mapping.lock()) mappings.push_back(std::move(mapping));
    Collect(root->right.get(), mappings);
}
}
