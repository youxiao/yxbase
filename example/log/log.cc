#include "base/logging.h"
#include "base/command_line.h"
#include "base/at_exit.h"

int main(int argc, const char* const* argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

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
  }

  LOG(INFO) << "info log";

  LOG(WARNING) << "warning log";

  LOG(ERROR) << "error log";

  system("pause");
}