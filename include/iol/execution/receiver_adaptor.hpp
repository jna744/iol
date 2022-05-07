#ifndef IOL_EXECUTION_RECEIVER_ADAPTOR_HPP
#define IOL_EXECUTION_RECEIVER_ADAPTOR_HPP

#include <iol/type_traits.hpp>
#include <iol/concepts.hpp>
#include <iol/meta.hpp>

#include <iol/execution/receiver.hpp>
#include <iol/execution/env.hpp>

#include <type_traits>
#include <concepts>

namespace iol::execution
{

namespace _receiver_adaptor
{

struct empty_receiver
{
  friend constexpr void tag_invoke(set_stopped_t, empty_receiver) noexcept {}
  template <typename Error>
  friend constexpr void tag_invoke(set_error_t, empty_receiver, Error) noexcept
  {}
  friend constexpr empty_env tag_invoke(get_env_t, empty_receiver) noexcept { return {}; }
};

template <typename Base>
struct receiver_adaptor_base_
{
  class type
  {

   public:

    template <typename B>
    explicit type(B&& base) : base_{(B &&) base}
    {}

   protected:

    Base const& base() const& noexcept { return base_; }

    Base& base() & noexcept { return base_; }

    Base const&& base() const&& noexcept { return std::move(base_); }

    Base&& base() && noexcept { return std::move(base_); }

   private:

    [[no_unique_address]] Base base_;
  };
};

template <>
class receiver_adaptor_base_<empty_receiver>
{
  class type
  {};
};

template <typename Base>
using receiver_adaptor_base = typename receiver_adaptor_base_<Base>::type;

#define IOL_EXEC_ADAPTOR_MEMBER(TAG)                                               \
  template <typename D, typename... Args>                                          \
  static constexpr auto _call_##TAG(                                               \
      D&& d, Args&&... args) noexcept(noexcept(((D &&) d).TAG((Args &&) args...))) \
      ->decltype(((D &&) d).TAG((Args &&) args...))                                \
  {                                                                                \
    return ((D &&) d).TAG((Args &&) args...);                                      \
  }
#define IOL_EXEC_ADAPTOR_CALL_MEMBER(TAG, ...) _call_##TAG(__VA_ARGS__)
#define IOL_EXEC_ADAPTOR_MISSING_MEMBER(D, TAG) (_tag_##TAG<D>())
#define IOL_EXEC_ADAPTOR_DEFINE_MEMBER(TAG)   \
  IOL_EXEC_ADAPTOR_MEMBER(TAG)                \
  template <class D>                          \
  static constexpr bool _tag_##TAG() noexcept \
  {                                           \
    return requires                           \
    {                                         \
      requires bool(D::TAG);                  \
    };                                        \
  }                                           \
  static constexpr bool TAG = true

template <class_type Derived, receiver Base>
struct receiver_adaptor_
{
  class type : public receiver_adaptor_base<Base>
  {

    friend Derived;
    IOL_EXEC_ADAPTOR_DEFINE_MEMBER(set_value);
    IOL_EXEC_ADAPTOR_DEFINE_MEMBER(set_error);
    IOL_EXEC_ADAPTOR_DEFINE_MEMBER(set_stopped);
    IOL_EXEC_ADAPTOR_DEFINE_MEMBER(get_env);

    template <typename R, typename... As>
    static constexpr bool with_set_value_member_v = requires(R&& r, As&&... as)
    {
      ((R &&) r).set_value((As &&) as...);
    };

    template <typename R, typename E>
    static constexpr bool with_set_error_member_v = requires(R&& r, E&& e)
    {
      ((R &&) r).set_error((E &&) e);
    };

    template <typename R>
    static constexpr bool with_set_stopped_member_v = requires(R&& r)
    {
      ((R &&) r).set_stopped();
    };

    template <typename R>
    static constexpr bool with_get_env_member_v = requires(R&& r)
    {
      ((R &&) r).get_env();
    };

    static constexpr bool with_base_v = !std::same_as<Base, empty_receiver>;

    template <typename D>
    static constexpr decltype(auto) get_base(D&& d) noexcept
    {
      if constexpr (with_base_v) {
        return ((copy_cvref_t<D&&, type>)((D &&) d)).base();
      } else {
        return ((D &&) d).base();
      }
    }

    template <typename D>
    using no_base_t = decltype(std::declval<D>().base());

    template <typename D>
    using with_base_t = decltype(((copy_cvref_t<D&&, type>)std::declval<D>()).base());

    using m_get_base =
        std::conditional_t<with_base_v, meta::m_quote<with_base_t>, meta::m_quote<no_base_t>>;

    template <typename D>
    using base_type = meta::m_invoke_q<m_get_base, D&&>;

   public:

    type() = default;
    using receiver_adaptor_base<Base>::receiver_adaptor_base;

   private:

    template <typename D = Derived, typename... Args>
      requires with_set_value_member_v<D, Args...>
    friend constexpr void tag_invoke(set_value_t, Derived&& self, Args&&... args) noexcept
    {
      static_assert(
          noexcept(IOL_EXEC_ADAPTOR_CALL_MEMBER(set_value, (Derived &&) self, (Args &&) args...)));
      IOL_EXEC_ADAPTOR_CALL_MEMBER(set_value, (Derived &&) self, (Args &&) args...);
    }

    template <typename D = Derived, typename... Args>
      requires(
          !with_set_value_member_v<D, Args...> && IOL_EXEC_ADAPTOR_MISSING_MEMBER(D, set_value))
    friend constexpr void tag_invoke(set_value_t, Derived&& self, Args&&... args) noexcept
    {
      execution::set_value(get_base((Derived &&) self), (Args &&) args...);
    }

    template <typename E, typename D = Derived>
      requires with_set_error_member_v<D, E>
    friend constexpr void tag_invoke(set_error_t, Derived&& self, E&& e) noexcept
    {
      static_assert(noexcept(IOL_EXEC_ADAPTOR_CALL_MEMBER(set_error, (Derived &&) self, (E &&) e)));
      IOL_EXEC_ADAPTOR_CALL_MEMBER(set_error, (Derived &&) self, (E &&) e);
    }

    template <typename E, typename D = Derived>
      requires(!with_set_error_member_v<D, E> && IOL_EXEC_ADAPTOR_MISSING_MEMBER(D, set_error))
    friend constexpr void tag_invoke(set_error_t, Derived&& self, E&& e) noexcept
    {
      execution::set_error(get_base((Derived &&) self), (E &&) e);
    }

    template <typename D = Derived>
      requires with_set_stopped_member_v<D>
    friend constexpr void tag_invoke(set_stopped_t, Derived&& self) noexcept
    {
      static_assert(noexcept(IOL_EXEC_ADAPTOR_CALL_MEMBER(set_stopped, (Derived &&) self)));
      IOL_EXEC_ADAPTOR_CALL_MEMBER(set_stopped, (Derived &&) self);
    }

    template <typename D = Derived>
      requires(!with_set_stopped_member_v<D> && IOL_EXEC_ADAPTOR_MISSING_MEMBER(D, set_stopped))
    friend constexpr void tag_invoke(set_stopped_t, Derived&& self) noexcept
    {
      execution::set_stopped(get_base((Derived &&) self));
    }

    template <typename D = Derived>
      requires with_get_env_member_v<D>
    friend constexpr decltype(auto) tag_invoke(get_env_t, Derived const& self) noexcept(
        noexcept(IOL_EXEC_ADAPTOR_CALL_MEMBER(get_env, (Derived const&)self)))
    {
      return IOL_EXEC_ADAPTOR_CALL_MEMBER(get_env, self);
    }

    template <typename D = Derived>
      requires(!with_get_env_member_v<D> && IOL_EXEC_ADAPTOR_MISSING_MEMBER(D, get_env))
    friend constexpr decltype(auto) tag_invoke(get_env_t, Derived const& self) noexcept(
        noexcept(execution::get_env(get_base(self))))
    {
      return execution::get_env(get_base(self));
    }

    template <is_forwarding_receiver_query Tag, typename D = Derived, typename... Args>
      requires is_callable_v<Tag, base_type<D const&>, Args...>
    friend constexpr auto tag_invoke(Tag tag, Derived const& self, Args&&... args) noexcept(
        is_nothrow_callable_v<Tag, base_type<D const&>, Args...>)
    {
      return std::move(tag)(get_base(self), std::forward<Args>(args)...);
    }
  };
};

}  // namespace _receiver_adaptor

template <class_type Derived, receiver Base = _receiver_adaptor::empty_receiver>
using receiver_adaptor = typename _receiver_adaptor::receiver_adaptor_<Derived, Base>::type;

};  // namespace iol::execution

#endif  // IOL_EXECUTION_RECEIVER_ADAPTER_HPP
