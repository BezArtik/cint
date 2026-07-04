// core/token/token_types.hpp

#pragma once
#include <array>
#include <memory>
#include <string_view>
#include <variant>
#include <vector>

namespace core {

#define TOKEN_TYPES(X)                                                                                                \
    X(LEFT_PAREN)                                                                                                     \
    X(RIGHT_PAREN)                                                                                                    \
    X(LEFT_BRACE) X(RIGHT_BRACE) X(LEFT_BRACKET) X(RIGHT_BRACKET) X(COMMA) X(DOT) X(SEMICOLON) X(PLUS) X(MINUS)       \
        X(STAR) X(SLASH) X(PERCENT) X(BANG) X(EQUAL) X(BANG_EQUAL) X(EQUAL_EQUAL) X(GREATER) X(GREATER_EQUAL) X(LESS) \
            X(LESS_EQUAL) X(INCREMENT) X(DECREMENT) X(PLUS_EQUAL) X(MINUS_EQUAL) X(STAR_EQUAL) X(SLASH_EQUAL)         \
                X(PERCENT_EQUAL) X(AND) X(OR) X(IDENTIFIER) X(STRING) X(NUMBER) X(KEYWORD) X(END_OF_FILE) X(UNKNOWN)

enum class token_type : uint8_t {
#define X(name) name,
    TOKEN_TYPES(X)
#undef X
};

inline constexpr std::array token_type_names = {
#define X(name) std::string_view(#name),
    TOKEN_TYPES(X)
#undef X
};

constexpr std::string_view to_string(token_type t) {
    return token_type_names[static_cast<size_t>(t)];
}

class type {
public:
    enum class kind : uint8_t { INT, DOUBLE, BOOL, STRING, VOID, FUNCTION, ARRAY, UNKNOWN };

    type() = default;
    type(const type& other);
    type& operator=(const type& other);
    type(type&&) noexcept = default;
    type& operator=(type&&) noexcept = default;

    static type int_type();
    static type double_type();
    static type bool_type();
    static type string_type();
    static type void_type();
    static type unknown_type();

    static type function_type(type return_type, std::vector<type> param_types);
    static type array_type(type element_type, size_t size = 0);

    bool is_primitive() const noexcept;
    bool is_numeric() const noexcept;
    bool is_int() const noexcept;
    bool is_double() const noexcept;
    bool is_bool() const noexcept;
    bool is_string() const noexcept;
    bool is_void() const noexcept;
    bool is_unknown() const noexcept;
    bool is_function() const noexcept;

    const type& return_type() const;
    const std::vector<type>& param_types() const;

    bool is_assignable_from(const type& source) const noexcept;
    type common_arithmetic_type(const type& other) const noexcept;

    bool is_array() const noexcept;
    const type& element_type() const;
    size_t array_size() const;

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

    type(kind k);
    template <typename Info>
    type(kind k, Info info) : kind_(k), info_(std::move(info)) {}
    void swap(type& other) noexcept;

    kind kind_ = kind::UNKNOWN;
    std::variant<std::monostate, function_info, array_info> info_;
};

}  // namespace core
