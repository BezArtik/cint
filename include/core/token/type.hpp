// core/token/type.hpp

#pragma once
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace core {

class type {
public:
    enum class kind : uint8_t { INT, DOUBLE, BOOL, STRING, VOID, FUNCTION, ARRAY, STRUCT, UNKNOWN };

    type() = default;
    type(const type& other);
    type& operator=(const type& other);
    type(type&&) noexcept = default;
    type& operator=(type&&) noexcept = default;

    static constexpr type int_type() { return type(kind::INT); }
    static constexpr type double_type() { return type(kind::DOUBLE); }
    static constexpr type bool_type() { return type(kind::BOOL); }
    static constexpr type string_type() { return type(kind::STRING); }
    static constexpr type void_type() { return type(kind::VOID); }
    static constexpr type unknown_type() { return type(kind::UNKNOWN); }

    static type function_type(type return_type, std::vector<type> param_types);
    static type array_type(type element_type, size_t size = 0);
    using field_t = std::pair<std::string_view, type>;
    static type struct_type(std::string_view name, std::vector<field_t> fields);

    bool is_primitive() const noexcept;
    bool is_numeric() const noexcept;
    bool is_int() const noexcept;
    bool is_double() const noexcept;
    bool is_bool() const noexcept;
    bool is_string() const noexcept;
    bool is_void() const noexcept;
    bool is_unknown() const noexcept;
    bool is_function() const noexcept;
    bool is_array() const noexcept;
    bool is_struct() const noexcept;

    const type& return_type() const;
    std::span<const type> param_types() const;

    const type& element_type() const;
    size_t array_size() const;

    std::string_view struct_name() const;
    std::span<const field_t> struct_fields() const;
    std::optional<size_t> field_index(std::string_view name) const noexcept;

    bool is_assignable_from(const type& source) const noexcept;

    kind get_kind() const noexcept;

    bool operator==(const type& other) const noexcept;
    bool operator!=(const type& other) const noexcept;

private:
    struct function_info {
        std::unique_ptr<type> return_type_;
        std::vector<type> param_types_;
    };

    struct array_info {
        std::unique_ptr<type> element_type_;
        size_t size_;
    };

    struct struct_info {
        std::string_view name_;
        std::vector<field_t> fields_;
    };

    constexpr type(kind k) : kind_(k), info_(std::monostate{}) {}

    template <typename Info>
    type(kind k, Info info) : kind_(k), info_(std::move(info)) {}
    void swap(type& other) noexcept;

    kind kind_ = kind::UNKNOWN;
    std::variant<function_info, array_info, struct_info, std::monostate> info_;
};

}  // namespace core
