#ifndef IOL_DETAIL_SIMPLE_MANUAL_RESET_EVENT_HPP
#define IOL_DETAIL_SIMPLE_MANUAL_RESET_EVENT_HPP

#include <iol/detail/config.hpp>

#if !defined(linux)
#include <condition_variable>
#include <mutex>
#endif

#include <atomic>

namespace iol::detail
{

class simple_manual_reset_event
{

public:

  simple_manual_reset_event() noexcept;

  simple_manual_reset_event(simple_manual_reset_event const&) = delete;

  simple_manual_reset_event& operator=(simple_manual_reset_event const&) = delete;

  void set();

  void reset();

  void wait() const;

  bool is_set() const;

private:

#if !defined(linux)
  mutable std::mutex              mut_;
  mutable std::condition_variable cv_;
  std::atomic_bool                state_;
#else
  std::atomic_int state_;
#endif
};

}  // namespace iol::detail

#endif  // SIMPLE_MANUAL_RESET_EVENT_H_
