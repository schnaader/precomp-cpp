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

#include <functional>
#include <memory>

class Task {
public:
  virtual bool execute() = 0;
};
class FunctionTask : public Task {
public:
  FunctionTask(std::function<bool()> && fun) 
  : _fun(fun) {
  }

  bool execute() {
    return _fun();
  }

private:
  std::function<bool()> _fun;
};

class TaskPool {
public:
  TaskPool();
  ~TaskPool();

  void addTask(std::unique_ptr<Task>&& task);
  void addTask(std::function<bool()> && fun);
};

#endif /* TASK_POOL_H */
