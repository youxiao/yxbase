#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "base/timer/timer.h"
#include "base/bind.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include <iostream>
#include <string>

namespace thread {
  enum ID {
    UI = 0,
    FILE,
    NET,
    DB,
    ID_COUNT
  };
}

char* thread_names[thread::ID_COUNT] = {
  "UI Thread",
  "File Thread",
  "Net Thread",
  "DB Thread",
};

base::Thread* threads[thread::ID_COUNT] = {
  nullptr
};

base::RepeatingTimer timers[thread::ID_COUNT];

void ui_thread_func(const std::string& text);
void file_thread_func(const std::string& text);
void net_thread_func(const std::string& text);
void db_thread_func(const std::string& text);

void ui_thread_timer(const std::string& text) {
  std::cout << "UI Thread Timer:" << text << std::endl;
}
void ui_thread_func(const std::string& text) {
  DCHECK(true);
  std::cout << "UI Thread Recv Text:" << text << std::endl;

  threads[thread::FILE]->task_runner()->PostTask(FROM_HERE,
    base::Bind(&file_thread_func, "I'm File Thread"));

  timers[thread::UI].Start(FROM_HERE,
    base::TimeDelta::FromMilliseconds(1000),
    base::Bind(&ui_thread_timer, text));
}

void file_thread_timer(const std::string& text) {
  std::cout << "File Thread Timer:" << text << std::endl;
}
void file_thread_func(const std::string& text) {
  std::cout << "File Thread Recv Text:" << text << std::endl;

  threads[thread::NET]->task_runner()->PostTask(FROM_HERE,
    base::Bind(&net_thread_func, "I'm Net Thread"));

  timers[thread::FILE].Start(FROM_HERE,
    base::TimeDelta::FromMilliseconds(1000),
    base::Bind(&file_thread_timer, text));
}

void net_thread_timer(const std::string& text) {
  std::cout << "Net Thread Timer:" << text << std::endl;
}
void net_thread_func(const std::string& text) {
  std::cout << "Net Thread Recv Text:" << text << std::endl;

  threads[thread::DB]->task_runner()->PostTask(FROM_HERE,
    base::Bind(&db_thread_func, "I'm DB Thread"));

  timers[thread::NET].Start(FROM_HERE,
    base::TimeDelta::FromMilliseconds(1000),
    base::Bind(&net_thread_timer, text));
}

void db_thread_timer(const std::string& text) {
  std::cout << "DB Thread Timer:" << text << std::endl;
}
void db_thread_func(const std::string& text) {
  std::cout << "DB Thread Recv Text:" << text << std::endl;

  timers[thread::DB].Start(FROM_HERE,
    base::TimeDelta::FromMilliseconds(1000),
    base::Bind(&db_thread_timer, text));
}

int main(int argc, const char* const* argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  for (std::size_t i = 0; i < thread::ID_COUNT; i++) {
    threads[i] = new base::Thread(thread_names[i]);
    base::Thread::Options options;
    if (i == thread::ID::UI)
      options.message_loop_type = base::MessageLoop::TYPE_UI;
    else
      options.message_loop_type = base::MessageLoop::TYPE_IO;
    
    threads[i]->StartWithOptions(options);
  }

  bool start = false;
  while (1) {
    Sleep(1000);

    if (!start) {
      threads[thread::UI]->task_runner()->PostTask(FROM_HERE,
        base::Bind(&ui_thread_func, "I'm UI Thread"));

      start = true;
    }
  }
  
  system("pause");
}