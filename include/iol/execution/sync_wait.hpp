#ifndef IOL_EXECUTION_SYNC_WAIT_HPP
#define IOL_EXECUTION_SYNC_WAIT_HPP

#include <iol/tag_invoke.hpp>
#include <iol/concepts.hpp>
#include <iol/meta.hpp>

#include <iol/execution/scheduler.hpp>
#include <iol/execution/run_loop.hpp>
#include <iol/execution/sender.hpp>
#include <iol/execution/general_queries.hpp>
#include <iol/execution/completion_signatures.hpp>

#include <type_traits>
#include <exception>
#include <system_error>
#include <optional>
#include <variant>
#include <tuple>

namespace iol::execution
{

namespace _sync_wait
{

using default_scheduler_t = decltype(std::declval<run_loop>().get_scheduler());

struct env
{

  default_scheduler_t scheduler_;

  friend default_scheduler_t tag_invoke(get_scheduler_t, env const& e) noexcept
  {
    return e.scheduler_;
  }

  friend default_scheduler_t tag_invoke(get_delegate_scheduler_t, env const& e) noexcept
  {
    return e.scheduler_;
  }
};

template <typename T>
struct sync_wait_state
{
  using storage_t = std::variant<std::monostate, T, std::exception_ptr, std::error_code>;
  storage_t storage_;
};

template <typename T>
struct receiver
{

  sync_wait_state<T>* state_;
  run_loop*           loop_;

  template <typename... Ts>
  friend void tag_invoke(set_value_t, receiver&& self, Ts&&... ts) noexcept
  {
    auto& storage = self.state_->storage_;
    if constexpr (std::is_nothrow_constructible_v<T, Ts...>) {
      storage.template emplace<1>((Ts &&) ts...);
      self.loop_->finish();
    } else {
      try {
        storage.template emplace<1>((Ts &&) ts...);
        self.loop_->finish();
      } catch (...) {
        set_error((receiver &&) self, std::current_exception());
      }
    }
  }

  template <typename Error>
  friend void tag_invoke(set_error_t, receiver&& self, Error&& e) noexcept
  {
    if constexpr (decays_to<Error, std::exception_ptr>) {
      self.state_->storage_.template emplace<2>((Error &&) e);
    } else if constexpr (decays_to<Error, std::error_code>) {
      self.state_->storage_.template emplace<3>((Error &&) e);
    } else {
      self.state_->storage_.template emplace<2>(std::make_exception_ptr((Error &&) e));
    }
    self.loop_->finish();
  }

  friend void tag_invoke(set_stopped_t, receiver&& self) noexcept { self.loop_->finish(); }

  friend env tag_invoke(get_env_t, receiver const& self) noexcept
  {
    return {self.loop_->get_scheduler()};
  }
};

template <class S, class E>
  requires sender<S, E>
using into_variant_type = value_types_of_t<S, E>;

template <sender<env> S>
using sync_wait_type = std::optional<value_types_of_t<S, env, decayed_tuple, std::type_identity_t>>;

template <sender<env> S>
using sync_wait_with_variant_type = std::optional<into_variant_type<S, env>>;

template <typename S>
concept valid_sync_wait_sender = sender<S, env> &&
    (meta::m_size<value_types_of_t<S, env>>::value == 1);

template <typename S, typename Tag = set_value_t>
using completion_scheduler_of_t = decltype(get_completion_scheduler<Tag>(std::declval<S>()));

struct sync_wait_t;

template <typename S>
concept completion_sched_tag_invocable = requires
{
  typename completion_scheduler_of_t<S>;
}
&&tag_invocable<sync_wait_t, completion_scheduler_of_t<S>, S>;

struct sync_wait_t
{

  template <valid_sync_wait_sender S>
    requires completion_sched_tag_invocable<S>
  auto operator()(S&& s) const -> tag_invoke_result_t<sync_wait_t, completion_scheduler_of_t<S>, S>
  {
    return tag_invoke(*this, get_completion_scheduler<set_value_t>((S &&) s), (S &&) s);
  }

  template <valid_sync_wait_sender S>
    requires(
        !completion_sched_tag_invocable<S> && tag_invocable<sync_wait_t, S> &&
        std::same_as<tag_invoke_result_t<sync_wait_t, S>, sync_wait_type<S>>)
  auto operator()(S&& s) const -> tag_invoke_result_t<sync_wait_t, S>
  {
    return tag_invoke(*this, (S &&) s);
  }

  template <valid_sync_wait_sender S>
    requires(!completion_sched_tag_invocable<S> && !tag_invocable<sync_wait_t, S>)
  auto operator()(S&& s) const -> sync_wait_type<S>
  {

    using optional_type = sync_wait_type<S>;
    using value_type = typename optional_type::value_type;
    using storage_type = sync_wait_state<value_type>;

    storage_type storage;
    run_loop     loop;

    auto op_state = connect((S &&) s, receiver<value_type>{&storage, &loop});
    start(op_state);

    loop.run();

    switch (storage.storage_.index()) {

      // error-code
      case 3: throw std::system_error(std::get<3>(storage.storage_));
      // exception-ptr
      case 2: std::rethrow_exception(std::get<2>(storage.storage_));
      // value
      case 1: return std::move(std::get<1>(storage.storage_));
      default: return std::nullopt;
    }
  }
};

}  // namespace _sync_wait

using _sync_wait::sync_wait_t;

inline constexpr sync_wait_t sync_wait{};

}  // namespace iol::execution

#endif  // IOL_EXECUTION_SYNC_WAIT_HPP
