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

#include "task_pool.h"

#include <memory>

TaskPool globalTaskPool;

TaskPool::TaskPool()
  : _state(INIT)
  , _threadLimit(std::max(1u, std::thread::hardware_concurrency()) - 1) {
}

void TaskPool::_init() {
  _state = RUN;
  std::function<void(void)> workerLoop = [this] {
    for (;;) {
      std::function<void()> task;

      {
        std::unique_lock<std::mutex> lock(this->_mutex);
        this->_condition.wait(lock,
                             [this] { return this->_state == FINISH || !this->_tasks.empty(); });
        if (this->_state == FINISH) {
          return;
        }
        task = std::move(this->_tasks.front());
        this->_tasks.pop();
      }
      task();
    }
  };
  for (unsigned i = 0, n = std::max((size_t)1, _threadLimit); i < n; ++i) {
    _workers.emplace_back(workerLoop);
  }
}

TaskPool::~TaskPool() {
  _state = FINISH;
  _condition.notify_all();
  for (auto& thr : _workers) {
    if (thr.joinable()) {
      thr.join();
    }
  }
}
