/**
 * @file storage.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#include "models/storage.hpp"

namespace zuu::models {

Storage::Storage(models::Hint<Token> hint) noexcept {
    strings_.reserve(hint.string_count);
    arrays_.reserve(hint.array_count);
    objects_.reserve(hint.object_count);
    const size_t elem_hint = hint.comma_count + hint.array_count + hint.object_count;
    array_elements_.reserve(elem_hint);
    object_elements_.reserve(elem_hint);
}

bool Storage::hasRoot() const noexcept {
    return root_set_;
}

void Storage::setRoot(JsonValue value) noexcept {
    root_ = value;
    root_set_ = true;
}

const JsonValue& Storage::root() const noexcept {
    return root_;
}

size_t Storage::commitString(std::string_view value) noexcept {
    strings_.push_back(value);
    return strings_.size() - 1;
}

size_t Storage::commitArray(std::span<const JsonValue> elements) noexcept {
    const auto offset = static_cast<uint32_t>(array_elements_.size());
    const auto size = static_cast<uint32_t>(elements.size());
    array_elements_.insert(array_elements_.end(), elements.begin(), elements.end());
    arrays_.emplace_back(offset, size);
    return arrays_.size() - 1;
}

size_t Storage::commitObject(std::span<const JsonMember> members) noexcept {
    const auto offset = static_cast<uint32_t>(object_elements_.size());
    const auto size = static_cast<uint32_t>(members.size());
    object_elements_.insert(object_elements_.end(), members.begin(), members.end());
    objects_.emplace_back(offset, size);
    return objects_.size() - 1;
}

Storage::JsonArray Storage::array(size_t index) const noexcept {
    const auto& [offset, size] = arrays_[index];
    return {array_elements_.data() + offset, size};
}

Storage::JsonObject Storage::object(size_t index) const noexcept {
    const auto& [offset, size] = objects_[index];
    return {object_elements_.data() + offset, size};
}

std::string_view Storage::string(size_t index) const noexcept {
    return strings_[index];
}

size_t Storage::getArrayOffset() const noexcept {
    return array_elements_.size();
}

void Storage::pushArrayElement(const JsonValue& val) noexcept {
    array_elements_.push_back(val);
}

size_t Storage::sealArray(size_t start_offset) noexcept {
    const auto size = static_cast<uint32_t>(array_elements_.size() - start_offset);
    arrays_.emplace_back(static_cast<uint32_t>(start_offset), size);
    return arrays_.size() - 1;
}

size_t Storage::getObjectOffset() const noexcept {
    return object_elements_.size();
}

void Storage::pushObjectMember(const JsonMember& member) noexcept {
    object_elements_.push_back(member);
}

size_t Storage::sealObject(size_t start_offset) noexcept {
    const auto size = static_cast<uint32_t>(object_elements_.size() - start_offset);
    objects_.emplace_back(static_cast<uint32_t>(start_offset), size);
    return objects_.size() - 1;
}

} // namespace zuu::models