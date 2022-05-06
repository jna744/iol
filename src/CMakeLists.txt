find_package(Threads REQUIRED)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  operation_queue.cpp
  static_thread_pool.cpp
  simple_manual_reset_event.cpp
  fast_mutex.cpp
  execution/run_loop.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
