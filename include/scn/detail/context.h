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

#ifndef SCN_DETAIL_CONTEXT_H
#define SCN_DETAIL_CONTEXT_H

#include "args.h"
#include "locale.h"
#include "options.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    namespace detail {
        template <typename Stream,
                  typename FormatStringChar,
                  typename ParseContext,
                  typename Options,
                  typename LocaleRef>
        class context_base {
        public:
            using stream_type = Stream;
            using char_type = typename stream_type::char_type;
            using format_string_char_type = FormatStringChar;
            using parse_context_type = ParseContext;
            using options_type = Options;
            using locale_type = LocaleRef;

            template <typename T>
            using scanner_type = scanner<char_type, T>;

            SCN_CONSTEXPR14 stream_type& stream() noexcept
            {
                SCN_EXPECT(m_stream != nullptr);
                return *m_stream;
            }

            SCN_CONSTEXPR14 options_type& get_options() noexcept
            {
                return m_options;
            }

            SCN_CONSTEXPR14 parse_context_type& parse_context() noexcept
            {
                return m_parse_ctx;
            }

            locale_type& locale() noexcept
            {
                return m_locale;
            }
            method int_method() const noexcept
            {
                return m_options.int_method;
            }
            method float_method() const noexcept
            {
                return m_options.float_method;
            }

        protected:
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            context_base(stream_type& s,
                         parse_context_type pctx,
                         locale_type loc,
                         options_type opt)
                : m_stream(std::addressof(s)),
                  m_parse_ctx(std::move(pctx)),
                  m_options(std::move(opt)),
                  m_locale(std::move(loc))
            {
            }
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

        private:
            stream_type* m_stream;
            parse_context_type m_parse_ctx;
            options_type m_options;
            locale_type m_locale;
        };

        /// Scanning context.
        template <typename Stream, typename ParseContext>
        class arg_context_base
            : public detail::context_base<
                  Stream,
                  typename Stream::char_type,
                  ParseContext,
                  options,
                  basic_locale_ref<typename Stream::char_type>> {
            using base = detail::context_base<
                Stream,
                typename Stream::char_type,
                ParseContext,
                options,
                basic_locale_ref<typename Stream::char_type>>;

        public:
            using stream_type = Stream;
            using char_type = typename stream_type::char_type;
            using format_string_type = basic_string_view<char_type>;
            using args_type = basic_args<arg_context_base>;
            using parse_context_type = ParseContext;
            using locale_type = basic_locale_ref<char_type>;
            using options_type = typename base::options_type;
            using arg_type = basic_arg<arg_context_base>;
            using base::scanner_type;

            template <typename... Args>
            using arg_store_type = arg_store<arg_context_base, Args...>;

            arg_context_base(stream_type& s,
                             parse_context_type p,
                             args_type args)
                : base(s, std::move(p), locale_type{}, options_type{}),
                  m_args(std::move(args))
            {
            }
            arg_context_base(stream_type& s,
                             parse_context_type p,
                             args_type args,
                             options_type opt)
                : base(s,
                       std::move(p),
                       opt.template get_locale_ref<char_type>(),
                       std::move(opt)),
                  m_args(std::move(args))
            {
            }

            either<arg_type> next_arg()
            {
                return this->do_get_arg(this->parse_context().next_arg_id());
            }
            either<arg_type> arg(size_t id)
            {
                return this->parse_context().check_arg_id(id) ? do_get_arg(id)
                                                              : arg_type();
            }

            either<arg_type> arg(basic_string_view<char_type>)
            {
                return arg_type{};
            }

        private:
            either<arg_type> do_get_arg(size_t id)
            {
                auto a = this->m_args.get(id);
                if (!a && !this->m_args.check_id(id - 1)) {
                    return error(error::invalid_argument,
                                 "Argument id out of range");
                }
                return a;
            }

            args_type m_args;
            detail::arg_map<arg_context_base> m_map;
        };
    }  // namespace detail

    template <typename Stream>
    class basic_context : public detail::arg_context_base<
                              Stream,
                              basic_parse_context<typename Stream::char_type>> {
        using base = detail::arg_context_base<
            Stream,
            basic_parse_context<typename Stream::char_type>>;

    public:
        using stream_type = typename base::stream_type;
        using char_type = typename base::char_type;
        using args_type = typename base::args_type;
        using options_type = typename base::options_type;
        using parse_context_type = typename base::parse_context_type;

        basic_context(stream_type& s,
                      basic_string_view<char_type> f,
                      args_type args)
            : base(s, parse_context_type{f}, std::move(args))
        {
        }
        basic_context(stream_type& s,
                      basic_string_view<char_type> f,
                      args_type args,
                      options_type opt)
            : base(s, parse_context_type{f}, std::move(args), std::move(opt))
        {
        }
    };

    template <typename Stream>
    class basic_scanf_context
        : public detail::arg_context_base<
              Stream,
              basic_scanf_parse_context<typename Stream::char_type>> {
        using base = detail::arg_context_base<
            Stream,
            basic_scanf_parse_context<typename Stream::char_type>>;

    public:
        using stream_type = typename base::stream_type;
        using char_type = typename base::char_type;
        using args_type = typename base::args_type;
        using options_type = typename base::options_type;
        using parse_context_type = typename base::parse_context_type;

        basic_scanf_context(stream_type& s,
                            basic_string_view<char_type> f,
                            args_type args)
            : base(s, parse_context_type{f}, std::move(args))
        {
        }
        basic_scanf_context(stream_type& s,
                            basic_string_view<char_type> f,
                            args_type args,
                            options_type opt)
            : base(s, parse_context_type{f}, std::move(args), std::move(opt))
        {
        }
    };

    template <typename Stream>
    class basic_empty_context
        : public detail::arg_context_base<
              Stream,
              basic_empty_parse_context<typename Stream::char_type>> {
        using base = detail::arg_context_base<
            Stream,
            basic_empty_parse_context<typename Stream::char_type>>;

    public:
        using stream_type = typename base::stream_type;
        using char_type = typename base::char_type;
        using args_type = typename base::args_type;
        using options_type = typename base::options_type;
        using parse_context_type = typename base::parse_context_type;

        basic_empty_context(stream_type& s, int n_args, args_type args)
            : base(s, parse_context_type{n_args}, std::move(args))
        {
        }
        basic_empty_context(stream_type& s,
                            int n_args,
                            args_type args,
                            options_type opt)
            : base(s,
                   parse_context_type{n_args},
                   std::move(args),
                   std::move(opt))
        {
        }
    };

    SCN_CLANG_POP

    template <typename Stream, typename Context = basic_context<Stream>>
    Context make_context(Stream& s,
                         typename Context::format_string_type f,
                         typename Context::args_type a)
    {
        return Context(s, f, a);
    }
    template <typename Stream, typename Context = basic_context<Stream>>
    Context make_context(Stream& s,
                         typename Context::format_string_type f,
                         typename Context::args_type a,
                         options opt)
    {
        return Context(s, f, a, opt);
    }
    template <typename Stream, typename Context = basic_empty_context<Stream>>
    Context make_context(Stream& s, int n_args, typename Context::args_type a)
    {
        return Context(s, n_args, a);
    }
    template <typename Stream, typename Context = basic_empty_context<Stream>>
    Context make_context(Stream& s,
                         int n_args,
                         typename Context::args_type a,
                         options opt)
    {
        return Context(s, n_args, a, opt);
    }

    template <typename Context>
    Context context_with_args(Context& ctx, basic_args<Context> args)
    {
        return Context(ctx.stream(), ctx.parse_context().view(), args,
                       ctx.options());
    }

    namespace detail {
        template <typename CharT>
        struct dummy_stream {
            using char_type = CharT;
        };
        template <typename CharT>
        struct dummy_context {
            using type = basic_context<dummy_stream<CharT>>;
        };

        template <typename CharT>
        struct named_arg_base {
            using context_type = typename dummy_context<CharT>::type;
            using arg_type = basic_arg<context_type>;
            using storage_type =
                typename std::aligned_storage<sizeof(arg_type),
                                              alignof(arg_type)>::type;

            named_arg_base(basic_string_view<CharT> n) : name(n) {}

            template <typename Context>
            basic_arg<Context> deserialize()
            {
#if SCN_GCC >= SCN_COMPILER(8, 0, 0)
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wclass-memaccess")
#endif
                basic_arg<Context> arg;
                std::memcpy(&arg, &data, sizeof(arg_type));
                return arg;
#if SCN_GCC >= SCN_COMPILER(8, 0, 0)
                SCN_GCC_POP
#endif
            }

            basic_string_view<CharT> name;
            storage_type data;
        };

        template <typename T, typename CharT>
        struct named_arg : named_arg_base<CharT> {
            using base = named_arg_base<CharT>;

            named_arg(basic_string_view<CharT> name, T& val)
                : base(name), value(std::addressof(val))
            {
            }

            T* value;
        };
    }  // namespace detail

    template <typename T>
    detail::named_arg<T, char> arg(string_view name, T& arg)
    {
        return {name, std::addressof(arg)};
    }
    template <typename T>
    detail::named_arg<T, wchar_t> arg(wstring_view name, T& arg)
    {
        return {name, std::addressof(arg)};
    }

    template <typename S, typename T, typename Char>
    void arg(S, detail::named_arg<T, Char>) = delete;

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_CONTEXT_H
