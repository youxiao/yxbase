#ifndef NOMINMAX
# define NOMINMAX
#endif

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/process/process_iterator.h"
#include "base/files/file_util.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/logging.h"
#include "base/command_line.h"
#include <iostream>

int main(int argc, const char* const* argv) {
  base::CommandLine::Init(argc, argv);

  logging::LoggingSettings settings;
  settings.log_file = L"example.log";
  bool success = logging::InitLogging(settings);
  DCHECK(success);

  if (success) {
    logging::SetLogItems(true,  // enable_process_id
                         true,  // enable_thread_id
                         true,  // enable_timestamp
                         false);  // enable_tickcount

    //logging::SetMinLogLevel(logging::LOG_ERROR);

    LOG(INFO) << "init log file example.log succeed";
  }

  base::FilePath dir;
  PathService::Get(base::DIR_EXE, &dir);

  std::cout << "compute " << base::UTF16ToUTF8(dir.value()) << " size..." << std::endl;

  LOG(INFO) << "compute " << base::UTF16ToUTF8(dir.value()) << " size...";

  int64_t dir_size = base::ComputeDirectorySize(dir);

  std::cout << base::Int64ToString(dir_size) << std::endl;

  LOG(INFO) << base::Int64ToString(dir_size);
  
  int count = base::GetProcessCount(L"notepad.exe", nullptr);
  std::cout << count << std::endl;

  LOG(INFO) << count;

  LOG(ERROR) << "test log error level";
  
  system("pause");
}