//
// Created by tim on 28.09.25.
//

#ifndef GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_NAMEDENUM_H
#define GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_NAMEDENUM_H
#include <array>
#include <utility>

namespace named_enum {

using string_t=const char *;

/// Size interface
template<typename E> constexpr
size_t size( );

template<typename E> constexpr
size_t size( E const & ) {
  return size<E>();
}

/// Trait class to circumvent MSVC broken two-phase template lookup
template<typename E>
struct enum_name_traits{
  using string_array_t=std::array<string_t, size<E>()>;
};

/// Name interface
template<typename E> constexpr
const typename enum_name_traits<E>::string_array_t & names( );

template<typename E> constexpr
const typename enum_name_traits<E>::string_array_t & names( E const & ) {
  return names<E>( );
}

template<typename E> constexpr
string_t name( E const & e ){
  return names( e )[static_cast<size_t>(e)];
}

namespace detail {

template<size_t N>
constexpr size_t count_character( char const (&string)[N],
                                  char character ){
  auto count=size_t {};
  for (size_t i = 0; i < N; ++i) {
    if(string[i]==character) ++count;
  } // end of i-loop
  return count;
}

template<size_t N>
constexpr bool empty( char const (&)[N] ){
  return N==1; // Just the null-terminator
}

template<size_t N>
constexpr size_t length( char const (&)[N] ){
  return N;
}

template<size_t N>
constexpr bool has_trailing_comma( char const (&string)[N] ){
  return string[N-2]==',';
}

template<std::size_t... I>
constexpr std::array<string_t,sizeof...(I)> make_array( char * string,
                                                        char ** ids,
                                                        std::index_sequence<I...> ){
    return std::array<string_t,sizeof...(I)>{ids[I]...};
}

template<int N, size_t C>
class tokenizer {
  using string_array_t=std::array<string_t,C>;
  char string_[N] { };
  char * ids_[C] { };
  string_array_t strings_{};

public:
  constexpr tokenizer( char const (&string)[N] ){
    size_t count = 0;
    ids_[0]= &string_[0];
    for ( size_t i = 0; i < N; ++i ) {
      if ( string[i] == ',' ) {
        ids_[++count] = &string_[i + 2];
        string_[i] = '\0';
      }
      else {
        string_[i] = string[i];
      }
    } // end of i-loop

    constexpr auto sequence=std::make_index_sequence<C>();
    strings_=make_array(&string_[0],&ids_[0],sequence);
  }

  constexpr std::array<const char*,C> const & strings() const
  {
    return strings_;
  }
};

} // namespace detail
} // namespace named_enum

#define _MAKE_NAMED_ENUM_WITH_TYPE_IMPL(enum_name,enum_strictness,enum_type,...)       \
                                                                                       \
static_assert(!named_enum::detail::empty(#__VA_ARGS__),"No enumerators provided");     \
static_assert(!named_enum::detail::has_trailing_comma(#__VA_ARGS__),                   \
                                               "Trailing comma is not supported");     \
                                                                                       \
static_assert(named_enum::detail::count_character(#__VA_ARGS__,'=')==0,                \
  "Custom enumerators (=) are not supported");                                         \
enum enum_strictness enum_name : enum_type {                                           \
  __VA_ARGS__                                                                          \
};                                                                                     \
                                                                                       \
namespace named_enum {                                                                 \
                                                                                       \
template<>                                                                             \
constexpr size_t size<enum_name>( ){                                                   \
  return detail::count_character(#__VA_ARGS__,',')+1;                                  \
}                                                                                      \
                                                                                       \
template<>                                                                             \
struct enum_name_traits<enum_name>{                                                    \
  using string_array_t=std::array<string_t, size<enum_name>()>;                        \
};                                                                                     \
                                                                                       \
constexpr auto _##enum_name##_##tokenizer=                                             \
  detail::tokenizer<detail::length(#__VA_ARGS__),size<enum_name>()>(#__VA_ARGS__);     \
                                                                                       \
template<> constexpr                                                                   \
const typename enum_name_traits<enum_name>::string_array_t & names<enum_name>( ){      \
  return _##enum_name##_##tokenizer.strings();                                         \
}                                                                                      \
                                                                                       \
} // namespace named_enum

#define MAKE_NAMED_ENUM(enum_name,...)                                                 \
    _MAKE_NAMED_ENUM_WITH_TYPE_IMPL(enum_name,,int,__VA_ARGS__)

#define MAKE_NAMED_ENUM_CLASS(enum_name,...)                                           \
    _MAKE_NAMED_ENUM_WITH_TYPE_IMPL(enum_name,class,int,__VA_ARGS__)

#define MAKE_NAMED_ENUM_WITH_TYPE(enum_name,enum_type,...)                             \
    _MAKE_NAMED_ENUM_WITH_TYPE_IMPL(enum_name,,enum_type,__VA_ARGS__)

#define MAKE_NAMED_ENUM_CLASS_WITH_TYPE(enum_name,enum_type,...)                       \
    _MAKE_NAMED_ENUM_WITH_TYPE_IMPL(enum_name,class,enum_type,__VA_ARGS__)
#endif //GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_NAMEDENUM_H