#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define TERMINAL_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
#define TERMINAL_PLATFORM_MACOS 1
#define TERMINAL_PLATFORM_UNIX 1
#else
#define TERMINAL_PLATFORM_LINUX 1
#define TERMINAL_PLATFORM_UNIX 1
#endif

#ifdef TERMINAL_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

struct TermCell {
  uint32_t fg = 0xFFD4D4D4;
  uint32_t bg = 0x00000000;
  char ch = ' ';
  bool bold = false;
  bool underline = false;
};

using TermLine = std::vector<TermCell>;

struct ShellInfo {
  std::string name;
  std::string path;
  bool is_default = false;
};

class TerminalSystem {
public:
  TerminalSystem();
  ~TerminalSystem();

  TerminalSystem(const TerminalSystem &) = delete;
  TerminalSystem &operator=(const TerminalSystem &) = delete;

  static std::vector<ShellInfo> DetectShells();

  bool Start(const std::string &shellPath = "",
             const std::string &workingDir = "");
  void Stop();
  bool IsRunning() const { return m_Running.load(); }

  void SendRaw(const std::string &text);
  void SendCommand(const std::string &cmd);
  void SendInterrupt();
  void SendBackspace();
  void SendChar(char c);

  std::vector<TermLine> grid;
  std::mutex grid_mutex;
  bool grid_dirty = false;

  int cols = 220;
  int rows = 50;

  int cursor_row = 0;
  int cursor_col = 0;

  std::string last_prompt;

  std::string HistoryNavigate(int direction);
  const std::vector<std::string> &History() const { return m_History; }

  void Resize(int newCols, int newRows);

  void PushHistory(const std::string &cmd);

private:
  void ReadLoop();
  void ProcessAnsi(const char *buf, size_t n);

  static constexpr uint32_t k_AnsiColors[16] = {
      0xFF1E1E1E, // 0  black
      0xFFCD3131, // 1  red
      0xFF0DBC79, // 2  green
      0xFFE5E510, // 3  yellow
      0xFF2472C8, // 4  blue
      0xFFBC3FBC, // 5  magenta
      0xFF11A8CD, // 6  cyan
      0xFFE5E5E5, // 7  white
      0xFF666666, // 8  bright black
      0xFFF14C4C, // 9  bright red
      0xFF23D18B, // 10 bright green
      0xFFF5F543, // 11 bright yellow
      0xFF3B8EEA, // 12 bright blue
      0xFFD670D6, // 13 bright magenta
      0xFF29B8DB, // 14 bright cyan
      0xFFFFFFFF, // 15 bright white
  };

  uint32_t m_FgColor = 0xFFD4D4D4;
  uint32_t m_BgColor = 0x00000000;
  bool m_Bold = false;
  bool m_Underline = false;

  std::string m_EscBuf;
  bool m_InEsc = false;
  bool m_InOsc = false;
  bool m_InCsi = false;

  int m_CurRow = 0;
  int m_CurCol = 0;
  int m_SavedRow = 0;
  int m_SavedCol = 0;

  void PutCell(char c);
  void NewLine();
  void CarriageReturn();
  void EnsureRows(int r);
  void ProcessCSI(const std::string &seq);
  void ProcessSGR(const std::string &params);
  void ResetSGR();
  void MoveCursor(int row, int col);

  std::atomic<bool> m_Running{false};
  std::thread m_ReadThread;

  std::vector<std::string> m_History;
  int m_HistoryPos = -1;

#ifdef TERMINAL_PLATFORM_WINDOWS
  HANDLE m_hChildStdinRead = INVALID_HANDLE_VALUE;
  HANDLE m_hChildStdinWrite = INVALID_HANDLE_VALUE;
  HANDLE m_hChildStdoutRead = INVALID_HANDLE_VALUE;
  HANDLE m_hChildStdoutWrite = INVALID_HANDLE_VALUE;
  HANDLE m_hProcess = INVALID_HANDLE_VALUE;
  HANDLE m_hThread = INVALID_HANDLE_VALUE;
#else
  int m_MasterFd = -1;
  pid_t m_ChildPid = -1;
#endif
};