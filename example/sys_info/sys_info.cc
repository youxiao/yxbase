#include "base/sys_info.h"
#include "base/command_line.h"
#include "base/at_exit.h"
#include "ui/display/screen.h"
#include "ui/display/display.h"
#include "ui/display/win/screen_win.h"
#include "ui/display/win/dpi.h"
#include <iostream>
#include <ShellScalingApi.h>

// Win8.1 supports monitor-specific DPI scaling.
bool SetProcessDpiAwarenessWrapper(PROCESS_DPI_AWARENESS value) {
  typedef HRESULT(WINAPI *SetProcessDpiAwarenessPtr)(PROCESS_DPI_AWARENESS);
  SetProcessDpiAwarenessPtr set_process_dpi_awareness_func =
      reinterpret_cast<SetProcessDpiAwarenessPtr>(
          GetProcAddress(GetModuleHandleA("user32.dll"),
                         "SetProcessDpiAwarenessInternal"));
  if (set_process_dpi_awareness_func) {
    HRESULT hr = set_process_dpi_awareness_func(value);
    if (SUCCEEDED(hr)) {
      VLOG(1) << "SetProcessDpiAwareness succeeded.";
      return true;
    } else if (hr == E_ACCESSDENIED) {
      LOG(ERROR) << "Access denied error from SetProcessDpiAwareness. "
          "Function called twice, or manifest was used.";
    }
  }
  return false;
}

// This function works for Windows Vista through Win8. Win8.1 must use
// SetProcessDpiAwareness[Wrapper].
BOOL SetProcessDPIAwareWrapper() {
  typedef BOOL(WINAPI *SetProcessDPIAwarePtr)(VOID);
  SetProcessDPIAwarePtr set_process_dpi_aware_func =
    reinterpret_cast<SetProcessDPIAwarePtr>(
      GetProcAddress(GetModuleHandleA("user32.dll"),
        "SetProcessDPIAware"));
  return set_process_dpi_aware_func &&
    set_process_dpi_aware_func();
}

void EnableHighDPISupport() {
  // Enable per-monitor DPI for Win10 or above instead of Win8.1 since Win8.1
  // does not have EnableChildWindowDpiMessage, necessary for correct non-client
  // area scaling across monitors.

  PROCESS_DPI_AWARENESS process_dpi_awareness = PROCESS_PER_MONITOR_DPI_AWARE;
  if (!SetProcessDpiAwarenessWrapper(process_dpi_awareness)) {
    SetProcessDPIAwareWrapper();
  }
}

int main(int argc, const char* const* argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  EnableHighDPISupport();

  display::win::ScreenWin screen;
  float screen_scale = display::win::GetDPIScale();
  gfx::Size screen_size = screen.GetPrimaryDisplaySize();

  int nScreenWidth, nScreenHeight;
  nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
  nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

  std::cout << "NumberOfProcessors:" << base::SysInfo::NumberOfProcessors() << "\n"
            << "AmountOfPhysicalMemoryMB:" << base::SysInfo::AmountOfPhysicalMemoryMB() << "MB\n"
            << "OperatingSystemName:" << base::SysInfo::OperatingSystemName() << "\n"
            << "OperatingSystemVersion:" << base::SysInfo::OperatingSystemVersion() << "\n"
            << "OperatingSystemArchitecture:" << base::SysInfo::OperatingSystemArchitecture() << "\n"
            << "Screen Scale:" << screen_scale << "\n"
            << "Screen Size:" << screen_size.width() * screen_scale << " x " << screen_size.height() * screen_scale << "\n"
            << "Screen Size:" << nScreenWidth << " x " << nScreenHeight << "\n"
            << "CPUModelName:" << base::SysInfo::CPUModelName() << std::endl;
  
  system("pause");
}