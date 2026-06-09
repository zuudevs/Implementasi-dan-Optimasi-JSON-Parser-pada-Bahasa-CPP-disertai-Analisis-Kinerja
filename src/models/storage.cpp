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
    const size_t max_strings = hint.string_count;
    const size_t max_arrays = hint.array_count;
    const size_t max_objects = hint.object_count;
    // Estimasi batas atas (upper bound) yang aman untuk menjamin tidak ada overflow
    const size_t max_elements = hint.comma_count + hint.array_count + hint.object_count;

    // Hitung ukuran raw bytes untuk setiap blok
    const size_t strings_bytes = max_strings * sizeof(std::string_view);
    const size_t array_elem_bytes = max_elements * sizeof(JsonValue);
    const size_t arrays_bytes = max_arrays * sizeof(std::pair<uint32_t, uint32_t>);
    const size_t obj_elem_bytes = max_elements * sizeof(JsonMember);
    const size_t objects_bytes = max_objects * sizeof(std::pair<uint32_t, uint32_t>);

    const size_t total_bytes = strings_bytes + array_elem_bytes + arrays_bytes + obj_elem_bytes + objects_bytes;

    if (total_bytes > 0) {
        // Melakukan HANYA SATU KALI alokasi untuk seumur hidup dokumen JSON
		// NOLINTNEXTLINE(modernize-avoid-c-arrays)
        arena_ = std::make_unique<std::byte[]>(total_bytes);
        
        std::byte* ptr = arena_.get();

        // Partisi memori dan mapping pointer
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        strings_ = reinterpret_cast<std::string_view*>(ptr);
        ptr += strings_bytes;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        array_elements_ = reinterpret_cast<JsonValue*>(ptr);
        ptr += array_elem_bytes;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        arrays_ = reinterpret_cast<std::pair<uint32_t, uint32_t>*>(ptr);
        ptr += arrays_bytes;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        object_elements_ = reinterpret_cast<JsonMember*>(ptr);
        ptr += obj_elem_bytes;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        objects_ = reinterpret_cast<std::pair<uint32_t, uint32_t>*>(ptr);
    }
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

// ── BUMP ALLOCATOR OPERATIONS ───────────────────────────────────────────────
// Operasi di bawah ini telah dikonversi menjadi instuksi pointer aritmetika
// yang akan langsung diterjemahkan menjadi register assignment pada Assembly CPU.

size_t Storage::commitString(std::string_view value) noexcept {
    strings_[strings_size_] = value;
    return strings_size_++;
}

size_t Storage::getArrayOffset() const noexcept {
    return array_elements_size_;
}

void Storage::pushArrayElement(const JsonValue& val) noexcept {
    array_elements_[array_elements_size_++] = val;
}

size_t Storage::sealArray(size_t start_offset) noexcept {
    const auto size = static_cast<uint32_t>(array_elements_size_ - start_offset);
    arrays_[arrays_size_] = {static_cast<uint32_t>(start_offset), size};
    return arrays_size_++;
}

size_t Storage::getObjectOffset() const noexcept {
    return object_elements_size_;
}

void Storage::pushObjectMember(const JsonMember& member) noexcept {
    object_elements_[object_elements_size_++] = member;
}

size_t Storage::sealObject(size_t start_offset) noexcept {
    const auto size = static_cast<uint32_t>(object_elements_size_ - start_offset);
    objects_[objects_size_] = {static_cast<uint32_t>(start_offset), size};
    return objects_size_++;
}

// ─────────────────────────────────────────────────────────────────────────────

Storage::JsonArray Storage::array(size_t index) const noexcept {
    const auto& [offset, size] = arrays_[index];
    return {array_elements_ + offset, size};
}

Storage::JsonObject Storage::object(size_t index) const noexcept {
    const auto& [offset, size] = objects_[index];
    return {object_elements_ + offset, size};
}

std::string_view Storage::string(size_t index) const noexcept {
    return strings_[index];
}

} // namespace zuu::models