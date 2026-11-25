#include "Signature.hpp"
#include "../object/Table.hpp"

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetCommandSignatureTable() {
    return GetObjectTable<CommandSignature>();
}
