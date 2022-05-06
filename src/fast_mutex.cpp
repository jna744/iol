#include <iol/detail/fast_mutex.hpp>

#if defined(linux)

#include <linux/futex.h> /* Definition of FUTEX_* constants */
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>

#include <numeric>

namespace
{

namespace local
{

inline long futex(
    uint32_t* uaddr, int futex_op, uint32_t val, timespec const* timeout, uint32_t* uaddr2,
    uint32_t val3)
{
  return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

inline std::uint32_t cmpxchg(
    std::atomic<std::uint32_t>* atom, std::uint32_t expected, std::uint32_t desired)
{
  auto* ep = &expected;
  std::atomic_compare_exchange_strong(atom, ep, desired);
  return *ep;
}

}  // namespace local

}  // namespace

namespace iol::detail
{

#if defined(linux)

fast_mutex::fast_mutex() noexcept : state_{0} {}

void fast_mutex::lock()
{
  auto old_state = local::cmpxchg(&state_, 0, 1);
  if (old_state != 0) {
    if (old_state != 2)
      old_state = state_.exchange(2, std::memory_order_acq_rel);
    while (old_state != 0) {
      local::futex((std::uint32_t*)&state_, FUTEX_WAIT_PRIVATE, 2, 0, 0, 0);
      old_state = state_.exchange(2, std::memory_order_acq_rel);
    }
  }
}

bool fast_mutex::try_lock()
{
  return !state_.load(std::memory_order_relaxed) && local::cmpxchg(&state_, 0, 1) == 0;
}

void fast_mutex::unlock()
{

  // We could either wake one up or wake up several?
  // What to do...
  constexpr auto waiters_to_wake_up = 1;
  if (state_.fetch_sub(1, std::memory_order_acq_rel) != 1) {
    [[maybe_unused]] auto n_awoken = local::futex(
        (uint32_t*)&state_, FUTEX_WAKE_PRIVATE, waiters_to_wake_up, nullptr, nullptr, 0);
    IOL_ASSERT(n_awoken != -1);
  }
}

#else

#endif

}  // namespace iol::detail

#endif
