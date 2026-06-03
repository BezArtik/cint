// core/token/token_types.hpp


#pragma once
#include <vector>
#include <memory>
#include <variant>

namespace core {

enum class token_type : uint8_t {
    LEFT_PAREN, RIGHT_PAREN, 
    LEFT_BRACE, RIGHT_BRACE,
	LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, SEMICOLON,

    PLUS, MINUS, STAR, SLASH, PERCENT,
    BANG, EQUAL,
    BANG_EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    INCREMENT, DECREMENT,
    PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, PERCENT_EQUAL,

    AND, OR,

    IDENTIFIER, STRING, NUMBER,

    KEYWORD,

    END_OF_FILE,
    UNKNOWN
};


class type {
public:

    type() = default;
    type(const type& other);
    type& operator=(const type& other);
    type(type&&) = default;
    type& operator=(type&&) = default;
    
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


    bool operator==(const type& other) const noexcept;
    bool operator!=(const type& other) const noexcept;

private:
    
    enum class kind : uint8_t {
        INT, DOUBLE, BOOL, STRING, VOID,
        FUNCTION,
        ARRAY,
        UNKNOWN
    };

    struct function_info {
        std::unique_ptr<type> return_type_{};
        std::vector<type> param_types_{};
    };

	struct array_info {
        std::unique_ptr<type> element_type_{};
        size_t size_{};
	};

    type(kind k);
	template <typename Info>
	type(kind k, Info info) : kind_(k), info_(std::move(info)) {}
    void swap(type& other) noexcept;

    kind kind_ = kind::UNKNOWN;
    std::variant<
        std::monostate,
        function_info,
		array_info
    > info_;
};

} // namespace core