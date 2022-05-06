#ifndef IOL_DETAIL_FAST_MUTEX_HPP
#define IOL_DETAIL_FAST_MUTEX_HPP

#include <iol/detail/config.hpp>

#if !defined(linux)
#include <mutex>
#include <condition_variable>
#else
#include <atomic>
#endif

namespace iol::detail
{

class fast_mutex
{

 public:

  fast_mutex() noexcept;

  fast_mutex(fast_mutex&&) = delete;

  void lock();

  bool try_lock();

  void unlock();

  class cond_var;

  class scoped_lock
  {

   public:

    scoped_lock(fast_mutex& mut) : mutex_{mut}, locked_(false)
    {
      mut.lock();
      locked_ = true;
    };

    scoped_lock(scoped_lock&&) = delete;

    ~scoped_lock()
    {
      if (locked_)
        mutex_.unlock();
    }

    void lock()
    {
      IOL_ASSERT(!locked_);
      mutex_.lock();
      locked_ = true;
    }

    void unlock()
    {
      IOL_ASSERT(locked_);
      mutex_.unlock();
      locked_ = false;
    }

   private:

    friend cond_var;
    fast_mutex& mutex_;
    bool        locked_;
  };

  class cond_var
  {
   public:

    void wait(scoped_lock& lock);
  };

 private:

#if defined(linux)
  std::atomic<std::uint32_t> state_;
#else
  std::mutex              mut_{};
  std::condition_variable cv_{};
#endif
};

}  // namespace iol::detail

#endif  // IOL_DETAIL_FAST_MUTEX_HPP
