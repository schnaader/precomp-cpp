/* Copyright 2018 Dirk Steinke

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#ifndef TASK_POOL_H
#define TASK_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class TaskPool {
public:
  TaskPool();
  ~TaskPool();

  template<class F, class... Args>
  auto addTask(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
    using R = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<R()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    if (_state == INIT) {
      _init();
    }
    std::future<R> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(_mutex);
      _tasks.emplace([task]() { (*task)(); });
    }
    _condition.notify_one();
    return res;
  }

  size_t extraThreadCount() const {
    return _threadLimit;
  }

private:
  enum State { INIT, RUN, FINISH };

  void _init();

  State _state;
  size_t _threadLimit;
  std::vector<std::thread> _workers;
  std::mutex _mutex;
  std::condition_variable _condition;
  std::queue<std::function<void()>> _tasks;
};

extern TaskPool globalTaskPool;

#endif /* TASK_POOL_H */
