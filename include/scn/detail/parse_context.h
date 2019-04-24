// Copyright 2017-2019 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of scnlib:
//     https://github.com/eliaskosunen/scnlib

#ifndef SCN_DETAIL_PARSE_CONTEXT_H
#define SCN_DETAIL_PARSE_CONTEXT_H

#include "result.h"
#include "string_view.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        class parse_context_base {
        public:
            SCN_CONSTEXPR14 size_t next_arg_id()
            {
                if (m_next_arg_id >= 0) {
                    return static_cast<size_t>(m_next_arg_id++);
                }
                return 0;
            }
            SCN_CONSTEXPR14 bool check_arg_id(size_t)
            {
                if (m_next_arg_id > 0) {
                    return false;
                }
                m_next_arg_id = -1;
                return true;
            }

        protected:
            parse_context_base() = default;

            int m_next_arg_id{0};
        };
        template <typename Char>
        class basic_parse_context_base : public parse_context_base {
        public:
            using parse_context_base::check_arg_id;
            SCN_CONSTEXPR14 void check_arg_id(basic_string_view<Char>) {}
        };
    }  // namespace detail

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    template <typename Char>
    class basic_parse_context : public detail::basic_parse_context_base<Char> {
    public:
        using char_type = Char;
        using string_view_type = basic_string_view<char_type>;
        using iterator = typename string_view_type::iterator;

        explicit SCN_CONSTEXPR basic_parse_context(
            basic_string_view<char_type> f)
            : m_str(f)
        {
        }

        template <typename Locale>
        bool should_skip_ws(const Locale& loc)
        {
            bool skip = false;
            while (*this && loc.is_space(next())) {
                skip = true;
                advance();
            }
            return skip;
        }
        template <typename Locale>
        bool should_read_literal(const Locale&)
        {
            const auto brace = detail::ascii_widen<char_type>('{');
            if (next() != brace) {
                if (next() == detail::ascii_widen<char_type>('}')) {
                    advance();
                }
                return true;
            }
            if (SCN_UNLIKELY(m_str.size() > 1 &&
                             *(m_str.begin() + 1) == brace)) {
                advance();
                return true;
            }
            return false;
        }
        SCN_CONSTEXPR bool check_literal(char_type ch) const
        {
            return ch == next();
        }

        SCN_CONSTEXPR bool good() const
        {
            return !m_str.empty();
        }
        SCN_CONSTEXPR explicit operator bool() const
        {
            return good();
        }

        SCN_CONSTEXPR14 void advance(size_t n = 1) noexcept
        {
            SCN_EXPECT(good());
            m_str.remove_prefix(n);
        }
        SCN_CONSTEXPR char_type next() const
        {
            return m_str.front();
        }

        template <typename Locale>
        bool check_arg_begin(const Locale&) const
        {
            return next() == detail::ascii_widen<char_type>('{');
        }
        template <typename Locale>
        bool check_arg_end(const Locale&) const
        {
            return next() == detail::ascii_widen<char_type>('}');
        }

        void arg_begin() {}
        void arg_end() {}

        SCN_CONSTEXPR14 void arg_handled() const {}

        template <typename Scanner, typename Context>
        error parse(Scanner& s, Context& ctx)
        {
            return s.parse(ctx);
        }

        template <typename Locale>
        either<string_view_type> parse_arg_id(const Locale& loc)
        {
            SCN_EXPECT(good());
            advance();
            if (SCN_UNLIKELY(!good())) {
                return error(error::invalid_format_string,
                             "Unexpected end of format argument");
            }
            auto it = m_str.begin();
            for (size_t i = 0; good(); ++i, (void)advance()) {
                if (check_arg_end(loc)) {
                    return string_view_type{it, i};
                }
                if (next() == detail::ascii_widen<char_type>(':')) {
                    advance();
                    return string_view_type{it, i};
                }
            }
            return error(error::invalid_format_string,
                         "Unexpected end of format argument");
        }

    private:
        string_view_type m_str;
    };

    template <typename Char>
    class basic_scanf_parse_context
        : public detail::basic_parse_context_base<Char> {
    public:
        using char_type = Char;
        using string_view_type = basic_string_view<char_type>;
        using iterator = typename string_view_type::iterator;

        explicit SCN_CONSTEXPR basic_scanf_parse_context(
            basic_string_view<char_type> f)
            : m_str(f), m_it(m_str.begin())
        {
        }

        template <typename Locale>
        bool should_skip_ws(const Locale& loc)
        {
            bool skip = false;
            while (good() && loc.is_space(next())) {
                skip = true;
                advance();
            }
            return skip;
        }
        template <typename Locale>
        bool should_read_literal(const Locale&)
        {
            const auto percent = detail::ascii_widen<char_type>('%');
            if (next() != percent) {
                return true;
            }
            if (SCN_UNLIKELY(std::distance(m_it, m_str.end()) > 1 &&
                             *(m_it + 1) == percent)) {
                advance();
                return true;
            }
            return false;
        }
        SCN_CONSTEXPR bool check_literal(char_type ch) const
        {
            return ch == next();
        }

        SCN_CONSTEXPR bool good() const
        {
            return m_it != m_str.end();
        }
        SCN_CONSTEXPR explicit operator bool() const
        {
            return good();
        }

        SCN_CONSTEXPR14 void advance(size_t n = 1) noexcept
        {
            SCN_EXPECT(good());
            m_it += n;
        }
        SCN_CONSTEXPR char_type next() const
        {
            return *m_it;
        }

        template <typename Locale>
        bool check_arg_begin(const Locale&) const
        {
            return next() == detail::ascii_widen<char_type>('%');
        }
        template <typename Locale>
        bool check_arg_end(const Locale& loc) const
        {
            return !good() || check_arg_begin(loc) || loc.is_space(next());
        }

        void arg_begin()
        {
            advance();
        }
        void arg_end()
        {
            if (good()) {
                backward();
            }
        }

        SCN_CONSTEXPR14 void arg_handled() const {}

        template <typename Scanner, typename Context>
        error parse(Scanner& s, Context& ctx)
        {
            return s.parse(ctx);
        }

        template <typename Locale>
        either<string_view_type> parse_arg_id(const Locale&)
        {
            SCN_EXPECT(good());
            return string_view_type{};
        }

    private:
        void backward(size_t n = 1) noexcept
        {
            SCN_EXPECT(m_it != m_str.begin());
            m_it -= n;
        }

        basic_string_view<char_type> m_str;
        iterator m_it;
    };

    template <typename Char>
    class basic_empty_parse_context
        : public detail::basic_parse_context_base<Char> {
    public:
        using char_type = Char;
        using string_view_type = basic_string_view<char_type>;

        explicit SCN_CONSTEXPR basic_empty_parse_context(int args)
            : m_args_left(args)
        {
        }

        template <typename Locale>
        SCN_CONSTEXPR14 bool should_skip_ws(const Locale&)
        {
            if (m_should_skip_ws) {
                m_should_skip_ws = false;
                return true;
            }
            return false;
        }
        template <typename Locale>
        SCN_CONSTEXPR bool should_read_literal(const Locale&) const
        {
            return false;
        }
        SCN_CONSTEXPR bool check_literal(char_type) const
        {
            return false;
        }

        SCN_CONSTEXPR bool good() const
        {
            return m_args_left > 0;
        }
        SCN_CONSTEXPR explicit operator bool() const
        {
            return good();
        }

        SCN_CONSTEXPR14 void advance(size_t = 1) const noexcept {}
        char_type next() const
        {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }

        template <typename Locale>
        SCN_CONSTEXPR bool check_arg_begin(const Locale&) const
        {
            return true;
        }
        template <typename Locale>
        SCN_CONSTEXPR bool check_arg_end(const Locale&) const
        {
            return true;
        }

        SCN_CONSTEXPR14 void arg_begin() const {}
        SCN_CONSTEXPR14 void arg_end() {}

        SCN_CONSTEXPR14 void arg_handled()
        {
            m_should_skip_ws = true;
            --m_args_left;
        }

        template <typename Scanner, typename Context>
        error parse(Scanner&, Context&)
        {
            return {};
        }

        template <typename Locale>
        either<string_view_type> parse_arg_id(const Locale&)
        {
            SCN_EXPECT(good());
            return string_view_type{};
        }

    private:
        int m_args_left;
        bool m_should_skip_ws{false};
    };

    SCN_CLANG_POP

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_PARSE_CONTEXT_H
