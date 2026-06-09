/**
 * @file storage.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "models/json_member.hpp"
#include "models/token.hpp"
#include <span>
#include <string_view>
#include <vector>

namespace zuu::models {

class Storage {
  public:
    using Type = JsonValue::Type;
    using JsonArray = std::span<const JsonValue>;
    using JsonObject = std::span<const JsonMember>;

    Storage() noexcept = default;
    Storage(const Storage&) noexcept = default;
    Storage(Storage&&) noexcept = default;
    Storage& operator=(const Storage&) noexcept = default;
    Storage& operator=(Storage&&) noexcept = default;
    ~Storage() noexcept = default;
    Storage(models::Hint<Token> hint) noexcept;

    [[nodiscard]] bool hasRoot() const noexcept;
    void setRoot(JsonValue value) noexcept;
    [[nodiscard]] const JsonValue& root() const noexcept;

    [[nodiscard]] size_t commitString(std::string_view value) noexcept;
    
    [[nodiscard]] size_t commitArray(std::span<const JsonValue> elements) noexcept;
    [[nodiscard]] size_t commitObject(std::span<const JsonMember> members) noexcept;

    [[nodiscard]] size_t getArrayOffset() const noexcept;
    void pushArrayElement(const JsonValue& val) noexcept;
    [[nodiscard]] size_t sealArray(size_t start_offset) noexcept;

    [[nodiscard]] size_t getObjectOffset() const noexcept;
    void pushObjectMember(const JsonMember& member) noexcept;
    [[nodiscard]] size_t sealObject(size_t start_offset) noexcept;

    [[nodiscard]] JsonArray array(size_t index) const noexcept;
    [[nodiscard]] JsonObject object(size_t index) const noexcept;
    [[nodiscard]] std::string_view string(size_t index) const noexcept;

  private:
    std::vector<std::string_view> strings_;
    std::vector<JsonValue> array_elements_;
    std::vector<std::pair<uint32_t, uint32_t>> arrays_;
    std::vector<JsonMember> object_elements_;
    std::vector<std::pair<uint32_t, uint32_t>> objects_;
    JsonValue root_{};
    bool root_set_{false};
};

} // namespace zuu::models