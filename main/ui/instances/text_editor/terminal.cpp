#include "terminal.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>

#ifdef TERMINAL_PLATFORM_WINDOWS
// windows.h
#else
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#ifdef TERMINAL_PLATFORM_LINUX
#include <pty.h>
#elif defined(TERMINAL_PLATFORM_MACOS)
#include <util.h>
#endif
#ifdef TERMINAL_PLATFORM_UNIX
#include <sys/ioctl.h>
#endif
#endif

constexpr uint32_t TerminalSystem::k_AnsiColors[16];

#ifdef TERMINAL_PLATFORM_UNIX
static bool FileExecutable(const std::string &path) {
  return access(path.c_str(), X_OK) == 0;
}
#endif

std::vector<ShellInfo> TerminalSystem::DetectShells() {
  std::vector<ShellInfo> result;
#ifdef TERMINAL_PLATFORM_WINDOWS
  auto tryAdd = [&](const std::string &name, const std::string &path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
      result.push_back({name, path, result.empty()});
  };
  char *pf = nullptr;
  char *pf86 = nullptr;
  size_t len = 0;
  _dupenv_s(&pf, &len, "ProgramFiles");
  _dupenv_s(&pf86, &len, "ProgramFiles(x86)");
  if (pf) {
    tryAdd("PowerShell 7", std::string(pf) + "\\PowerShell\\7\\pwsh.exe");
    free(pf);
  }
  if (pf86) {
    tryAdd("PowerShell 7 (x86)",
           std::string(pf86) + "\\PowerShell\\7\\pwsh.exe");
    free(pf86);
  }
  char sysdir[MAX_PATH] = {};
  GetSystemDirectoryA(sysdir, MAX_PATH);
  tryAdd("PowerShell 5",
         std::string(sysdir) + "\\WindowsPowerShell\\v1.0\\powershell.exe");
  tryAdd("cmd", std::string(sysdir) + "\\cmd.exe");
  tryAdd("Git Bash", "C:\\Program Files\\Git\\bin\\bash.exe");
  tryAdd("WSL Bash", "C:\\Windows\\System32\\wsl.exe");
#else
  const char *login = getenv("SHELL");
  const std::vector<std::pair<std::string, std::string>> candidates = {
      {"zsh", "/bin/zsh"},
      {"bash", "/bin/bash"},
      {"bash", "/usr/bin/bash"},
      {"fish", "/usr/bin/fish"},
      {"fish", "/usr/local/bin/fish"},
      {"ksh", "/bin/ksh"},
      {"dash", "/bin/dash"},
      {"sh", "/bin/sh"},
  };
  if (login && FileExecutable(login)) {
    std::string p(login);
    result.push_back({p.substr(p.rfind('/') + 1), p, true});
  }
  for (auto &[name, path] : candidates) {
    if (!FileExecutable(path))
      continue;
    bool dup = false;
    for (auto &s : result)
      if (s.path == path) {
        dup = true;
        break;
      }
    if (!dup)
      result.push_back({name, path, result.empty()});
  }
#endif
  return result;
}

TerminalSystem::TerminalSystem() { grid.emplace_back(); }

TerminalSystem::~TerminalSystem() { Stop(); }

void TerminalSystem::EnsureRows(int r) {
  while ((int)grid.size() <= r)
    grid.emplace_back();
}

void TerminalSystem::PutCell(char c) {
  EnsureRows(m_CurRow);
  auto &line = grid[m_CurRow];

  if ((int)line.size() <= m_CurCol)
    line.resize(m_CurCol + 1);
  line[m_CurCol] = {m_FgColor, m_BgColor, c, m_Bold, m_Underline};
  m_CurCol++;

  // wrap
  if (m_CurCol >= cols) {
    m_CurCol = 0;
    m_CurRow++;
    EnsureRows(m_CurRow);
  }
}

void TerminalSystem::NewLine() {
  m_CurRow++;
  EnsureRows(m_CurRow);
  // prompt detection
  if (m_CurRow > 0 && m_CurRow - 1 < (int)grid.size()) {
    std::string line;
    for (auto &cell : grid[m_CurRow - 1])
      line += cell.ch;

    // trim right
    size_t end = line.find_last_not_of(" \t");
    if (end != std::string::npos)
      line = line.substr(0, end + 1);
    if (!line.empty() &&
        (line.back() == '$' || line.back() == '#' || line.back() == '>'))
      last_prompt = line + " ";
  }
}

void TerminalSystem::CarriageReturn() { m_CurCol = 0; }

void TerminalSystem::MoveCursor(int row, int col) {
  m_CurRow = std::max(0, row);
  m_CurCol = std::max(0, col);
  EnsureRows(m_CurRow);
}

void TerminalSystem::ResetSGR() {
  m_FgColor = 0xFFD4D4D4;
  m_BgColor = 0x00000000;
  m_Bold = false;
  m_Underline = false;
}

void TerminalSystem::ProcessSGR(const std::string &params) {
  std::vector<int> codes;
  std::istringstream ss(params);
  std::string token;
  while (std::getline(ss, token, ';')) {
    if (token.empty())
      codes.push_back(0);
    else {
      try {
        codes.push_back(std::stoi(token));
      } catch (...) {
        codes.push_back(0);
      }
    }
  }
  if (codes.empty())
    codes.push_back(0);

  for (size_t i = 0; i < codes.size(); ++i) {
    int c = codes[i];
    if (c == 0) {
      ResetSGR();
    } else if (c == 1) {
      m_Bold = true;
    } else if (c == 4) {
      m_Underline = true;
    } else if (c == 22) {
      m_Bold = false;
    } else if (c == 24) {
      m_Underline = false;
    }
    // foreground
    else if (c >= 30 && c <= 37) {
      m_FgColor = k_AnsiColors[c - 30 + (m_Bold ? 8 : 0)];
    }
    // bright foreground
    else if (c >= 90 && c <= 97) {
      m_FgColor = k_AnsiColors[c - 90 + 8];
    }
    // def fg
    else if (c == 39) {
      m_FgColor = 0xFFD4D4D4;
    }
    // background
    else if (c >= 40 && c <= 47) {
      m_BgColor = k_AnsiColors[c - 40];
    }
    // def bg
    else if (c == 49) {
      m_BgColor = 0x00000000;
    }
    // 256 colors
    else if (c == 38 && i + 2 < codes.size() && codes[i + 1] == 5) {
      int idx = codes[i + 2];
      i += 2;
      // 256 colours
      if (idx < 16)
        m_FgColor = k_AnsiColors[idx];
      else if (idx >= 232) {
        uint8_t v = (uint8_t)(8 + (idx - 232) * 10);
        m_FgColor = 0xFF000000 | (v << 16) | (v << 8) | v;
      } else {
        idx -= 16;
        uint8_t r = (uint8_t)((idx / 36) * 51),
                g = (uint8_t)(((idx / 6) % 6) * 51),
                b = (uint8_t)((idx % 6) * 51);
        m_FgColor = 0xFF000000 | (r << 16) | (g << 8) | b;
      }
    } else if (c == 48 && i + 2 < codes.size() && codes[i + 1] == 5) {
      int idx = codes[i + 2];
      i += 2;
      if (idx < 16)
        m_BgColor = k_AnsiColors[idx];
      else if (idx >= 232) {
        uint8_t v = (uint8_t)(8 + (idx - 232) * 10);
        m_BgColor = 0xFF000000 | (v << 16) | (v << 8) | v;
      } else {
        idx -= 16;
        uint8_t r = (uint8_t)((idx / 36) * 51),
                g = (uint8_t)(((idx / 6) % 6) * 51),
                b = (uint8_t)((idx % 6) * 51);
        m_BgColor = 0xFF000000 | (r << 16) | (g << 8) | b;
      }
    }

    else if (c == 38 && i + 4 < codes.size() && codes[i + 1] == 2) {
      uint8_t r = (uint8_t)codes[i + 2], g = (uint8_t)codes[i + 3],
              b = (uint8_t)codes[i + 4];
      i += 4;
      m_FgColor = 0xFF000000 | (r << 16) | (g << 8) | b;
    } else if (c == 48 && i + 4 < codes.size() && codes[i + 1] == 2) {
      uint8_t r = (uint8_t)codes[i + 2], g = (uint8_t)codes[i + 3],
              b = (uint8_t)codes[i + 4];
      i += 4;
      m_BgColor = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
  }
}

void TerminalSystem::ProcessCSI(const std::string &seq) {
  if (seq.empty())
    return;
  char cmd = seq.back();
  std::string params = seq.substr(0, seq.size() - 1);

  // remove ? for private sequences
  if (!params.empty() && params.front() == '?')
    params = params.substr(1);

  auto getNum = [&](int def = 1) -> int {
    if (params.empty())
      return def;
    try {
      return std::stoi(params);
    } catch (...) {
      return def;
    }
  };
  auto getNums = [&](int def = 1) -> std::vector<int> {
    std::vector<int> v;
    std::istringstream ss(params);
    std::string t;
    while (std::getline(ss, t, ';')) {
      if (t.empty())
        v.push_back(def);
      else {
        try {
          v.push_back(std::stoi(t));
        } catch (...) {
          v.push_back(def);
        }
      }
    }
    if (v.empty())
      v.push_back(def);
    return v;
  };

  switch (cmd) {
  case 'A':
    m_CurRow = std::max(0, m_CurRow - getNum(1));
    EnsureRows(m_CurRow);
    break; // cursor up
  case 'B':
    m_CurRow += getNum(1);
    EnsureRows(m_CurRow);
    break; // cursor down
  case 'C':
    m_CurCol += getNum(1);
    break; // cursor forward
  case 'D':
    m_CurCol = std::max(0, m_CurCol - getNum(1));
    break; // cursor back
  case 'E':
    m_CurRow += getNum(1);
    m_CurCol = 0;
    EnsureRows(m_CurRow);
    break;
  case 'F':
    m_CurRow = std::max(0, m_CurRow - getNum(1));
    m_CurCol = 0;
    break;
  case 'G':
    m_CurCol = std::max(0, getNum(1) - 1);
    break; // cursor column
  case 'H':
  case 'f': { // cursor position
    auto v = getNums(1);
    int r = (v.size() > 0 ? v[0] : 1) - 1;
    int c = (v.size() > 1 ? v[1] : 1) - 1;
    MoveCursor(std::max(0, r), std::max(0, c));
    break;
  }
  case 'J': { // erase display
    int n = getNum(0);
    if (n == 2 || n == 3) { // clear all
      std::lock_guard<std::mutex> lk(grid_mutex);
      grid.clear();
      grid.emplace_back();
      m_CurRow = 0;
      m_CurCol = 0;
      grid_dirty = true;
    } else if (n == 0) { // clear from cursor to end
      if (m_CurRow < (int)grid.size()) {
        grid[m_CurRow].resize(m_CurCol);
        grid.erase(grid.begin() + m_CurRow + 1, grid.end());
      }
    }
    break;
  }
  case 'K': { // erase line
    int n = getNum(0);
    if (m_CurRow < (int)grid.size()) {
      auto &line = grid[m_CurRow];
      if (n == 0 && m_CurCol < (int)line.size())
        line.resize(m_CurCol); // to end
      else if (n == 1) {
        for (int i = 0; i < m_CurCol && i < (int)line.size(); ++i)
          line[i].ch = ' ';
      } // to start
      else if (n == 2) {
        for (auto &cell : line)
          cell.ch = ' ';
      } // whole line
    }
    break;
  }
  case 'm':
    ProcessSGR(params);
    break;
  case 's':
    m_SavedRow = m_CurRow;
    m_SavedCol = m_CurCol;
    break; // save cursor
  case 'u':
    m_CurRow = m_SavedRow;
    m_CurCol = m_SavedCol;
    EnsureRows(m_CurRow);
    break; // restore cursor
  case 'r':
    break; // scroll region
  case 'l':
  case 'h':
    break;    // set/reset
  case 'P': { // DCH delete chars
    int n = getNum(1);
    if (m_CurRow < (int)grid.size()) {
      auto &line = grid[m_CurRow];
      if (m_CurCol < (int)line.size())
        line.erase(line.begin() + m_CurCol,
                   line.begin() + std::min((int)line.size(), m_CurCol + n));
    }
    break;
  }
  default:
    break;
  }
}

void TerminalSystem::ProcessAnsi(const char *buf, size_t n) {
  std::lock_guard<std::mutex> lk(grid_mutex);

  for (size_t i = 0; i < n; ++i) {
    unsigned char c = (unsigned char)buf[i];

    if (m_InOsc) {
      if (c == 0x07 ||
          (c == '\\' && !m_EscBuf.empty() && m_EscBuf.back() == '\x1B'))
        m_InOsc = false;
      else
        m_EscBuf += (char)c;
      continue;
    }

    if (m_InCsi) {
      m_EscBuf += (char)c;
      if (c >= 0x40 && c <= 0x7E) {
        ProcessCSI(m_EscBuf);
        m_EscBuf.clear();
        m_InCsi = false;
        m_InEsc = false;
      }
      continue;
    }

    if (m_InEsc) {
      m_InEsc = false;
      if (c == '[') {
        m_InCsi = true;
        m_EscBuf.clear();
      } else if (c == ']') {
        m_InOsc = true;
        m_EscBuf.clear();
      } else if (c == '(' || c == ')') {
      } else if (c == '7') {
        m_SavedRow = m_CurRow;
        m_SavedCol = m_CurCol;
      } else if (c == '8') {
        m_CurRow = m_SavedRow;
        m_CurCol = m_SavedCol;
        EnsureRows(m_CurRow);
      } else if (c == 'M') {
        m_CurRow = std::max(0, m_CurRow - 1);
      }
      continue;
    }

    switch (c) {
    case 0x1B:
      m_InEsc = true;
      break; // esc
    case '\n':
      NewLine();
      break;
    case '\r':
      CarriageReturn();
      break;
    case '\b':
      if (m_CurCol > 0)
        m_CurCol--;
      break; // backspace
    case '\t':
      m_CurCol = (m_CurCol / 8 + 1) * 8;
      break; // tab
    case 0x07:
      break;
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x0E:
    case 0x0F:
      break;
    default:
      if (c >= 0x20)
        PutCell((char)c);
      break;
    }
  }

  cursor_row = m_CurRow;
  cursor_col = m_CurCol;
  grid_dirty = true;
}

void TerminalSystem::PushHistory(const std::string &cmd) {
  if (cmd.empty())
    return;
  if (m_History.empty() || m_History.back() != cmd)
    m_History.push_back(cmd);
  m_HistoryPos = -1;
}

std::string TerminalSystem::HistoryNavigate(int direction) {
  if (m_History.empty())
    return "";
  if (direction > 0) {
    if (m_HistoryPos == -1)
      m_HistoryPos = (int)m_History.size() - 1;
    else if (m_HistoryPos > 0)
      --m_HistoryPos;
  } else {
    if (m_HistoryPos != -1) {
      ++m_HistoryPos;
      if (m_HistoryPos >= (int)m_History.size())
        m_HistoryPos = -1;
    }
  }
  if (m_HistoryPos == -1)
    return "";
  return m_History[(size_t)m_HistoryPos];
}

void TerminalSystem::SendCommand(const std::string &cmd) {
  PushHistory(cmd);
  SendRaw(cmd + "\n");
}

void TerminalSystem::Resize(int newCols, int newRows) {
  cols = newCols;
  rows = newRows;
#ifdef TERMINAL_PLATFORM_UNIX
  if (m_MasterFd >= 0) {
    struct winsize ws{};
    ws.ws_col = (unsigned short)newCols;
    ws.ws_row = (unsigned short)newRows;
    ioctl(m_MasterFd, TIOCSWINSZ, &ws);
  }
#endif
}

// unix
#ifdef TERMINAL_PLATFORM_UNIX

bool TerminalSystem::Start(const std::string &shellPath,
                           const std::string &workingDir) {
  if (m_Running)
    return false;
  std::string shell = shellPath;
  if (shell.empty()) {
    auto shells = DetectShells();
    if (shells.empty())
      return false;
    shell = shells.front().path;
  }
  struct winsize ws{};
  ws.ws_col = (unsigned short)cols;
  ws.ws_row = (unsigned short)rows;
  m_ChildPid = forkpty(&m_MasterFd, nullptr, nullptr, &ws);
  if (m_ChildPid < 0)
    return false;
  if (m_ChildPid == 0) {
    if (!workingDir.empty())
      chdir(workingDir.c_str());
    // do not desactivate echo
    execl(shell.c_str(), shell.c_str(), "-i", nullptr);
    _exit(127);
  }
  int flags = fcntl(m_MasterFd, F_GETFL, 0);
  fcntl(m_MasterFd, F_SETFL, flags | O_NONBLOCK);
  m_Running = true;
  m_ReadThread = std::thread(&TerminalSystem::ReadLoop, this);
  return true;
}

void TerminalSystem::Stop() {
  m_Running = false;
  if (m_ChildPid > 0) {
    kill(m_ChildPid, SIGKILL);
    waitpid(m_ChildPid, nullptr, 0);
    m_ChildPid = -1;
  }
  if (m_MasterFd >= 0) {
    close(m_MasterFd);
    m_MasterFd = -1;
  }
  if (m_ReadThread.joinable())
    m_ReadThread.join();
}

void TerminalSystem::SendRaw(const std::string &text) {
  if (m_MasterFd < 0)
    return;
  ::write(m_MasterFd, text.c_str(), text.size());
}

void TerminalSystem::SendInterrupt() {
  if (m_ChildPid > 0)
    kill(m_ChildPid, SIGINT);
}
void TerminalSystem::SendBackspace() { SendRaw("\x7F"); }
void TerminalSystem::SendChar(char c) { SendRaw(std::string(1, c)); }

void TerminalSystem::ReadLoop() {
  char buf[4096];
  struct pollfd pfd{m_MasterFd, POLLIN, 0};
  while (m_Running) {
    int ret = poll(&pfd, 1, 50);
    if (ret <= 0)
      continue;
    if (!(pfd.revents & POLLIN))
      continue;
    ssize_t n = read(m_MasterFd, buf, sizeof(buf) - 1);
    if (n <= 0) {
      m_Running = false;
      break;
    }
    ProcessAnsi(buf, (size_t)n);
  }
}

// microsoft windows
#elif defined(TERMINAL_PLATFORM_WINDOWS)

bool TerminalSystem::Start(const std::string &shellPath,
                           const std::string &workingDir) {
  if (m_Running)
    return false;
  std::string shell = shellPath;
  if (shell.empty()) {
    auto shells = DetectShells();
    if (shells.empty())
      return false;
    shell = shells.front().path;
  }
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  if (!CreatePipe(&m_hChildStdoutRead, &m_hChildStdoutWrite, &sa, 0) ||
      !CreatePipe(&m_hChildStdinRead, &m_hChildStdinWrite, &sa, 0))
    return false;
  SetHandleInformation(m_hChildStdoutRead, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(m_hChildStdinWrite, HANDLE_FLAG_INHERIT, 0);
  std::string cmdLine = shell;
  std::string ls = shell;
  std::transform(ls.begin(), ls.end(), ls.begin(), ::tolower);
  if (ls.find("powershell") != std::string::npos ||
      ls.find("pwsh") != std::string::npos)
    cmdLine += " -NoLogo -NoExit";
  std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
  cmdBuf.push_back('\0');
  STARTUPINFOA si{};
  si.cb = sizeof(si);
  si.hStdError = m_hChildStdoutWrite;
  si.hStdOutput = m_hChildStdoutWrite;
  si.hStdInput = m_hChildStdinRead;
  si.dwFlags |= STARTF_USESTDHANDLES;
  PROCESS_INFORMATION pi{};
  if (!CreateProcessA(
          nullptr, cmdBuf.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
          nullptr, workingDir.empty() ? nullptr : workingDir.c_str(), &si, &pi))
    return false;
  m_hProcess = pi.hProcess;
  m_hThread = pi.hThread;
  CloseHandle(m_hChildStdoutWrite);
  m_hChildStdoutWrite = INVALID_HANDLE_VALUE;
  CloseHandle(m_hChildStdinRead);
  m_hChildStdinRead = INVALID_HANDLE_VALUE;
  m_Running = true;
  m_ReadThread = std::thread(&TerminalSystem::ReadLoop, this);
  return true;
}

void TerminalSystem::Stop() {
  m_Running = false;
  if (m_hProcess != INVALID_HANDLE_VALUE) {
    TerminateProcess(m_hProcess, 0);
    WaitForSingleObject(m_hProcess, 2000);
    CloseHandle(m_hProcess);
    m_hProcess = INVALID_HANDLE_VALUE;
    CloseHandle(m_hThread);
    m_hThread = INVALID_HANDLE_VALUE;
  }
  auto sc = [](HANDLE &h) {
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
      h = INVALID_HANDLE_VALUE;
    }
  };
  sc(m_hChildStdinWrite);
  sc(m_hChildStdinRead);
  sc(m_hChildStdoutRead);
  sc(m_hChildStdoutWrite);
  if (m_ReadThread.joinable())
    m_ReadThread.join();
}

void TerminalSystem::SendRaw(const std::string &text) {
  if (m_hChildStdinWrite == INVALID_HANDLE_VALUE)
    return;
  DWORD w = 0;
  WriteFile(m_hChildStdinWrite, text.c_str(), (DWORD)text.size(), &w, nullptr);
}
void TerminalSystem::SendInterrupt() {
  if (m_hProcess != INVALID_HANDLE_VALUE)
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, GetProcessId(m_hProcess));
}
void TerminalSystem::SendBackspace() { SendRaw("\b"); }
void TerminalSystem::SendChar(char c) { SendRaw(std::string(1, c)); }

void TerminalSystem::ReadLoop() {
  char buf[4096];
  while (m_Running) {
    DWORD available = 0;
    if (!PeekNamedPipe(m_hChildStdoutRead, nullptr, 0, nullptr, &available,
                       nullptr))
      break;
    if (available == 0) {
      DWORD exitCode = 0;
      if (GetExitCodeProcess(m_hProcess, &exitCode) && exitCode != STILL_ACTIVE)
        break;
      Sleep(20);
      continue;
    }
    DWORD bytesRead = 0;
    if (!ReadFile(m_hChildStdoutRead, buf, (DWORD)(sizeof(buf) - 1), &bytesRead,
                  nullptr) ||
        bytesRead == 0)
      break;
    ProcessAnsi(buf, bytesRead);
  }
  m_Running = false;
}

#endif