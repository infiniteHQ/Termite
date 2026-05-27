#include "text_editor.hpp"
#include "../../../src/module.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace ModuleUI {

void TextEditorAppWindow::PlusMinuxWidget(bool plus) {
  ImVec2 btn_pos = CherryGUI::GetCursorScreenPos();
  float btn_size = 22.0f;
  float rounding = 4.0f;
  float bar_thickness = 2.0f;
  float bar_half = 5.0f;

  ImVec2 btn_min = btn_pos;
  ImVec2 btn_max = ImVec2(btn_pos.x + btn_size, btn_pos.y + btn_size);
  ImVec2 center =
      ImVec2(btn_pos.x + btn_size * 0.5f, btn_pos.y + btn_size * 0.5f);

  CherryGUI::InvisibleButton("##zoom_in", ImVec2(btn_size, btn_size));
  bool hovered = CherryGUI::IsItemHovered();
  bool clicked = CherryGUI::IsItemClicked();

  if (plus) {
    if (clicked)
      ZoomIn();
  } else {
    if (clicked)
      ZoomOut();
  }

  ImDrawList *dl = CherryGUI::GetWindowDrawList();

  ImU32 bg_color =
      hovered ? IM_COL32(80, 80, 80, 255) : IM_COL32(55, 55, 55, 255);

  CherryGUI::AddRectFilled(dl, btn_min, btn_max, bg_color, rounding);

  CherryGUI::AddRect(dl, btn_min, btn_max, IM_COL32(120, 120, 120, 180),
                     rounding, 0, 1.0f);

  ImU32 fg_color = IM_COL32(220, 220, 220, 255);

  CherryGUI::AddRectFilled(
      dl, ImVec2(center.x - bar_half, center.y - bar_thickness * 0.5f),
      ImVec2(center.x + bar_half, center.y + bar_thickness * 0.5f), fg_color);

  if (plus) {
    CherryGUI::AddRectFilled(
        dl, ImVec2(center.x - bar_thickness * 0.5f, center.y - bar_half),
        ImVec2(center.x + bar_thickness * 0.5f, center.y + bar_half), fg_color);
  }
}

TextEditorAppWindow::TextEditorAppWindow(const std::string &path,
                                         const std::string &name) {
  namespace fs = std::filesystem;

  m_Type = detect_file(path);
  m_AppWindow = std::make_shared<Cherry::AppWindow>(name, name);
  term.Start();
  DefineWindowIcon();
  m_AppWindow->SetLeftMenubarCallback([this]() { RenderMenubar(); });
  m_AppWindow->SetRightMenubarCallback([this]() { RenderRightMenubar(); });
  m_AppWindow->SetLeftBottombarCallback([this]() { RenderBottombar(); });
  m_AppWindow->SetSaveMode(true);

  m_AppWindow->m_CloseCallback = [=]() {
    Cherry::DeleteAppWindow(m_AppWindow);
    m_AppWindow->SetVisibility(false);
  };

  std::shared_ptr<Cherry::AppWindow> win = m_AppWindow;
  m_FilePath = path;

  RefreshFile();

  this->ctx = vxe::get_current_context();
}

void TextEditorAppWindow::SetLanguage(const std::string &name) {}

// TODO: Let the user customize icons with custom vortex events
// TODO: Let the user desactivate this feature and always show icons/edit.png
void TextEditorAppWindow::DefineWindowIcon() {
  switch (m_Type) {
  case FileTypes::File_CPP: {
    m_AppWindow->SetIcon(
        TextEdit::GetPath("resources/icons/window_icons/cpp.png"));
    break;
  }
  case FileTypes::File_C: {
    m_AppWindow->SetIcon(
        TextEdit::GetPath("resources/icons/window_icons/c.png"));
    break;
  }
  case FileTypes::File_HPP: {
    m_AppWindow->SetIcon(
        TextEdit::GetPath("resources/icons/window_icons/hpp.png"));
    break;
  }
  case FileTypes::File_H: {
    m_AppWindow->SetIcon(
        TextEdit::GetPath("resources/icons/window_icons/h.png"));
    break;
  }
  case FileTypes::File_LUA: {
    m_AppWindow->SetIcon(
        TextEdit::GetPath("resources/icons/window_icons/lua.png"));
    break;
  }
  default: {
    m_AppWindow->SetIcon(TextEdit::GetPath("resources/icons/edit.png"));
    break;
  }
  }
}

std::string TextEditorAppWindow::GetFileTypeStr(FileTypes type) {
  switch (type) {
  // Web and Markup
  case FileTypes::File_XML:
    return "file_xml";

  // Config
  case FileTypes::File_CFG:
    return "file_cfg";
  case FileTypes::File_JSON:
    return "file_json";
  case FileTypes::File_YAML:
    return "file_yaml";
  case FileTypes::File_INI:
    return "file_ini";

  // Documents
  case FileTypes::File_TXT:
    return "file_txt";
  case FileTypes::File_MD:
    return "file_md";

  // Miscellaneous
  case FileTypes::File_LOG:
    return "file_log";
  case FileTypes::File_BACKUP:
    return "file_backup";
  case FileTypes::File_TEMP:
    return "file_temp";
  case FileTypes::File_DATA:
    return "file_data";

  // Other
  case FileTypes::File_UNKNOWN:
    return "file_unknown";
  }

  return "file_unknown"; // fallback
}

std::shared_ptr<Cherry::AppWindow> &TextEditorAppWindow::GetAppWindow() {
  return m_AppWindow;
}

std::shared_ptr<TextEditorAppWindow>
TextEditorAppWindow::Create(const std::string &path, const std::string &name) {
  auto instance =
      std::shared_ptr<TextEditorAppWindow>(new TextEditorAppWindow(path, name));
  instance->SetupRenderCallback();
  return instance;
}

void TextEditorAppWindow::SetupRenderCallback() {
  auto self = shared_from_this();
  m_AppWindow->SetRenderCallback([self]() {
    if (self) {
      self->Render();
    }
  });
}

void TextEditorAppWindow::RenderMenubar() {
  CherryGUI::SetCursorPosX(CherryGUI::GetCursorPosX() + 3.0f);

  if (!m_FileEdited) {
    CherryGUI::BeginDisabled();
  }

  CherryNextComponent.SetProperty("padding_y", "5.5f");
  CherryNextComponent.SetProperty("padding_x", "6.0f");
  CherryNextComponent.SetProperty("size_x", "18");
  CherryNextComponent.SetProperty("size_y", "18");
  if (CherryKit::ButtonImage(
          TextEdit::GetPath("/resources/icons/icon_save.png"))
          .GetDataAs<bool>("isClicked")) {
    m_SavePending = true;
  }

  if (!m_FileEdited) {
    CherryGUI::EndDisabled();
  }

  CherryNextComponent.SetProperty("padding_y", "5.5f");
  CherryNextComponent.SetProperty("padding_x", "6.0f");
  CherryNextComponent.SetProperty("size_x", "18");
  CherryNextComponent.SetProperty("size_y", "18");
  if (CherryKit::ButtonImage(
          TextEdit::GetPath("/resources/icons/icon_refresh.png"))
          .GetDataAs<bool>("isClicked")) {
    m_RefreshReady = true;
  }

  CherryKit::Separator();

  CherryNextComponent.SetProperty("padding_y", "6.0f");
  CherryNextComponent.SetProperty("padding_x", "10.0f");
  if (CherryKit::ButtonImageText(
          "Find",
          TextEdit::GetPath("/resources/icons/icon_magnifying_glass.png"))
          .GetDataAs<bool>("isClicked")) {
    m_FindPending = true;
  }

  CherryNextComponent.SetProperty("padding_y", "6.0f");
  CherryNextComponent.SetProperty("padding_x", "10.0f");
  if (CherryKit::ButtonText("ami").GetDataAs<bool>("isClicked")) {
    term.SendCommand("whoami");
  }
  CherryNextComponent.SetProperty("padding_y", "6.0f");
  CherryNextComponent.SetProperty("padding_x", "10.0f");
  if (CherryKit::ButtonText("ls").GetDataAs<bool>("isClicked")) {
    term.SendCommand("ls -la");
  }
  CherryNextComponent.SetProperty("padding_y", "6.0f");
  CherryNextComponent.SetProperty("padding_x", "10.0f");
  if (CherryKit::ButtonText("pwd").GetDataAs<bool>("isClicked")) {
    term.SendCommand("pwd");
  }
}

void TextEditorAppWindow::RefreshFile() {
  m_FileEdited = false;
  m_FileUpdated = true;
  try {
    if (m_FilePath.empty()) {
      std::cerr << "RefreshFile: no file path set\n";
      return;
    }

    namespace fs = std::filesystem;
    std::error_code ec;

    if (!fs::exists(m_FilePath, ec)) {
      std::cerr << "RefreshFile: file does not exist: " << m_FilePath << "\n";
      return;
    }

    std::ifstream ifs(m_FilePath, std::ios::binary);
    if (!ifs.is_open()) {
      std::cerr << "RefreshFile: failed to open file: " << m_FilePath << "\n";
      return;
    }

    std::string content;
    ifs.seekg(0, std::ios::end);
    auto size = ifs.tellg();
    if (size > 0) {
      content.resize(static_cast<size_t>(size));
      ifs.seekg(0, std::ios::beg);
      ifs.read(&content[0], size);
    } else {
      content.clear();
    }
    ifs.close();

    if (content != m_FileEditBuffer) {
      m_FileEditBuffer = std::move(content);
    }

    m_LastWriteTime = fs::last_write_time(m_FilePath, ec);

  } catch (const std::exception &e) {
    std::cerr << "RefreshFile: exception: " << e.what() << "\n";
  }
}

void TextEditorAppWindow::SaveFile() {
  m_FileEdited = false;
  m_FileUpdated = true;

  try {
    if (m_FilePath.empty()) {
      std::cerr << "SaveFile: no file path set\n";
      return;
    }

    namespace fs = std::filesystem;
    std::error_code ec;

    fs::path target = m_FilePath;
    fs::path parent = target.parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
      if (!fs::create_directories(parent, ec)) {
        std::cerr << "SaveFile: unable to create parent directories: " << parent
                  << " (" << ec.message() << ")\n";
        return;
      }
    }

    auto timestamp =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    fs::path tempPath = parent / (target.filename().string() + ".tmp." +
                                  std::to_string(timestamp));

    {
      std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
      if (!ofs.is_open()) {
        std::cerr << "SaveFile: failed to open temp file for writing: "
                  << tempPath << "\n";
        std::error_code rmec;
        fs::remove(tempPath, rmec);
        return;
      }

      ofs.write(m_FileEditBuffer.data(),
                static_cast<std::streamsize>(m_FileEditBuffer.size()));
      if (!ofs) {
        std::cerr << "SaveFile: write failed to temp file: " << tempPath
                  << "\n";
        ofs.close();
        fs::remove(tempPath, ec);
        return;
      }
      ofs.flush();
      ofs.close();
    }

    if (fs::exists(target, ec)) {
      std::error_code removeEc;
      fs::remove(target, removeEc);
      std::error_code renameEc;
      fs::rename(tempPath, target, renameEc);
      if (renameEc) {
        std::error_code copyEc;
        fs::copy_file(tempPath, target, fs::copy_options::overwrite_existing,
                      copyEc);
        if (copyEc) {
          std::cerr << "SaveFile: failed to replace target file: " << target
                    << " (rename: " << renameEc.message()
                    << ", copy: " << copyEc.message() << ")\n";
          fs::remove(tempPath, ec);
          return;
        }
        fs::remove(tempPath, ec);
      }
    } else {
      std::error_code renameEc;
      fs::rename(tempPath, target, renameEc);
      if (renameEc) {
        std::error_code copyEc;
        fs::copy_file(tempPath, target, fs::copy_options::none, copyEc);
        if (copyEc) {
          std::cerr << "SaveFile: failed to move temp file to target: "
                    << target << " (rename: " << renameEc.message()
                    << ", copy: " << copyEc.message() << ")\n";
          fs::remove(tempPath, ec);
          return;
        }
        fs::remove(tempPath, ec);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "SaveFile: exception: " << e.what() << "\n";
  }
}

void TextEditorAppWindow::Undo() { m_UndoPending = true; }
void TextEditorAppWindow::Redo() { m_RedoPending = true; }

FileTypes TextEditorAppWindow::detect_file(const std::string &path) {
  static const std::unordered_map<std::string, FileTypes> extension_map = {
      // Web and Markup
      {"xml", FileTypes::File_XML},
      {"json", FileTypes::File_JSON},
      {"yaml", FileTypes::File_YAML},
      {"yml", FileTypes::File_YAML},
      {"cpp", FileTypes::File_CPP},
      {"hpp", FileTypes::File_HPP},
      {"lua", FileTypes::File_LUA},
      {"python", FileTypes::File_PYTHON},
      {"c", FileTypes::File_C},
      {"h", FileTypes::File_H},
      {"cs", FileTypes::File_CS},

      // Config
      {"cfg", FileTypes::File_CFG},
      {"ini", FileTypes::File_INI},
      {"env", FileTypes::File_INI},

      // Documents
      {"txt", FileTypes::File_TXT},
      {"md", FileTypes::File_MD},
      {"rst", FileTypes::File_MD},

      // Miscellaneous
      {"log", FileTypes::File_LOG},
      {"bak", FileTypes::File_BACKUP},
      {"tmp", FileTypes::File_TEMP},
      {"dat", FileTypes::File_DATA},
  };

  std::string extension = get_extension(path);
  auto it = extension_map.find(extension);
  if (it != extension_map.end()) {
    return it->second;
  } else {
    return FileTypes::File_UNKNOWN;
  }
}

void TextEditorAppWindow::RenderCustomMenu() { CherryGUI::Text("Helo"); }

void TextEditorAppWindow::Render() {
  vxe::push_custom_menu("TextEdit", [this]() { RenderCustomMenu(); });

  CherryApp.PushComponentPool(&m_ComponentPool);
  bool isWindowFocused =
      CherryGUI::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
  bool isWindowHovered =
      CherryGUI::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
  bool ctrl = CherryGUI::IsKeyCtrlPressed();

  bool fPressed = CherryGUI::IsKeyPressed(ImGuiKey_F);
  bool vPressed = CherryGUI::IsKeyPressed(ImGuiKey_V);
  bool cPressed = CherryGUI::IsKeyPressed(ImGuiKey_C);
  bool sPressed = CherryGUI::IsKeyPressed(ImGuiKey_S);
  bool yPressed = CherryGUI::IsKeyPressed(ImGuiKey_Y);
  bool zeroPressed = CherryGUI::IsKeyPressed(ImGuiKey_0);
  bool plusPressed = CherryGUI::IsKeyPressed(ImGuiKey_KeypadAdd) ||
                     CherryGUI::IsKeyPressed(ImGuiKey_Equal);
  bool minusPressed = CherryGUI::IsKeyPressed(ImGuiKey_KeypadSubtract) ||
                      CherryGUI::IsKeyPressed(ImGuiKey_Minus);

  float wheel = CherryGUI::GetMouseWheel();

  if (isWindowFocused && ctrl) {
    if (vPressed) {
      m_PastePending = true;
    }
    if (cPressed) {
      m_CopyPending = true;
    }
    if (sPressed) {
      m_SavePending = true;
    }
    if (yPressed) {
      m_RedoPending = true;
    }
    if (fPressed) {
      m_FindPending = true;
    }

    if (plusPressed) {

      ZoomIn();
    }
    if (minusPressed) {
      ZoomOut();
    }
    if (zeroPressed) {
      ResetZoom();
    }
  }

  if (isWindowHovered && ctrl && wheel != 0.0f) {
    if (wheel > 0.0f) {
      ZoomIn();
    } else {
      ZoomOut();
    }
  }

  auto test = CherryGUI::GetContentRegionAvail();

  auto avail = CherryGUI::GetContentRegionAvail();

  Cherry::PushFont("JetBrainsMono");
  CherryStyle::PushFontSize(m_TextSize);

  ImFont *font = ImGui::GetFont();
  float fsize = ImGui::GetFontSize();
  ImVec2 charSz = font->CalcTextSizeA(fsize, FLT_MAX, 0, "M");
  float charW = charSz.x;
  float charH = charSz.y + 2.0f;

  int newCols = std::max(1, (int)(avail.x / charW));
  int newRows = std::max(1, (int)(avail.y / charH));
  if (newCols != term.cols || newRows != term.rows)
    term.Resize(newCols, newRows);

  ImVec2 canvasPos = ImGui::GetCursorScreenPos();
  ImVec2 canvasSize = avail;

  ImDrawList *dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(
      canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
      IM_COL32(18, 18, 18, 255));

  ImGui::InvisibleButton("##terminal_canvas", canvasSize,
                         ImGuiButtonFlags_MouseButtonLeft |
                             ImGuiButtonFlags_MouseButtonRight);
  bool termFocused = ImGui::IsItemFocused() || ImGui::IsItemActive();

  static bool firstFrame = true;
  if (firstFrame) {
    ImGui::SetKeyboardFocusHere(-1);
    firstFrame = false;
  }

  if (termFocused && term.IsRunning()) {
    auto &io = ImGui::GetIO();

    // c
    if (ImGui::IsKeyDown(ImGuiKey_ModCtrl) &&
        ImGui::IsKeyPressed(ImGuiKey_C, false))
      term.SendInterrupt();

    // l
    else if (ImGui::IsKeyDown(ImGuiKey_ModCtrl) &&
             ImGui::IsKeyPressed(ImGuiKey_L, false)) {
      term.SendRaw("\f"); // form feed = clear screen
    }

    // entry
    else if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
             ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
      if (!m_TermInput.empty())
        term.PushHistory(m_TermInput);
      term.SendRaw(m_TermInput + "\n");
      m_TermInput.clear();
      m_TermScrollToBottom = true;
    }

    // backspace
    else if (ImGui::IsKeyPressed(ImGuiKey_Backspace, true)) {
      if (!m_TermInput.empty()) {
        m_TermInput.pop_back();
        term.SendBackspace();
      }
    }

    // tab
    else if (ImGui::IsKeyPressed(ImGuiKey_Tab, false))
      term.SendRaw("\t");

    // up
    else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
      std::string entry = term.HistoryNavigate(+1);
      if (!entry.empty()) {
        for (size_t k = 0; k < m_TermInput.size(); ++k)
          term.SendBackspace();
        m_TermInput = entry;
        term.SendRaw(entry);
      }
    }

    // down
    else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
      std::string entry = term.HistoryNavigate(-1);
      for (size_t k = 0; k < m_TermInput.size(); ++k)
        term.SendBackspace();
      m_TermInput = entry;
      if (!entry.empty())
        term.SendRaw(entry);
    }

    // arrows
    else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
      term.SendRaw("\x1B[D");
    else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
      term.SendRaw("\x1B[C");

    for (ImWchar wc : io.InputQueueCharacters) {
      if (wc >= 32 && wc < 127) {
        char ch = (char)wc;
        m_TermInput += ch;
        term.SendChar(ch);
      }
    }
    io.InputQueueCharacters.resize(0);
  }

  // scroll
  if (ImGui::IsItemHovered()) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f) {
      m_TermScrollY -= wheel * charH * 3.0f;
      m_TermScrollToBottom = false;
    }
  }

  {
    std::lock_guard<std::mutex> lk(term.grid_mutex);

    float totalH = (float)term.grid.size() * charH;
    float visibleH = canvasSize.y;

    if (m_TermScrollToBottom || totalH <= visibleH)
      m_TermScrollY = std::max(0.0f, totalH - visibleH);
    m_TermScrollY = std::max(
        0.0f, std::min(m_TermScrollY, std::max(0.0f, totalH - visibleH)));

    int firstLine = (int)(m_TermScrollY / charH);
    int lastLine = std::min((int)term.grid.size(),
                            firstLine + (int)(visibleH / charH) + 2);

    dl->PushClipRect(
        canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

    for (int lineIdx = firstLine; lineIdx < lastLine; ++lineIdx) {
      const auto &line = term.grid[lineIdx];
      float y = canvasPos.y + (float)(lineIdx - firstLine) * charH;

      for (int colIdx = 0; colIdx < (int)line.size(); ++colIdx) {
        const auto &cell = line[colIdx];
        float x = canvasPos.x + colIdx * charW;

        if ((cell.bg & 0xFF000000) != 0)
          dl->AddRectFilled(ImVec2(x, y), ImVec2(x + charW, y + charH),
                            cell.bg);

        if (cell.ch != ' ') {
          uint32_t rgba = cell.fg;
          ImU32 imcol = IM_COL32((rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF,
                                 (rgba) & 0xFF, (rgba >> 24) & 0xFF);
          char buf[2] = {cell.ch, 0};
          dl->AddText(font, fsize, ImVec2(x, y), imcol, buf);
        }

        if (cell.underline)
          dl->AddLine(ImVec2(x, y + charH - 1),
                      ImVec2(x + charW, y + charH - 1), cell.fg);
      }
    }

    if (termFocused && term.IsRunning()) {
      static float cursorTimer = 0.0f;
      cursorTimer += ImGui::GetIO().DeltaTime;
      if (fmodf(cursorTimer, 1.0f) < 0.5f) {
        int cr = term.cursor_row;
        int cc = term.cursor_col;
        if (cr >= firstLine && cr < lastLine) {
          float cx = canvasPos.x + cc * charW;
          float cy = canvasPos.y + (float)(cr - firstLine) * charH;
          dl->AddRectFilled(ImVec2(cx, cy), ImVec2(cx + charW, cy + charH),
                            IM_COL32(255, 255, 255, 180));
        }
      }
    }

    dl->PopClipRect();
    term.grid_dirty = false;
  }

  CherryStyle::PopFontSize();
  Cherry::PopFont();

  CherryApp.PopComponentPool();
}

void TextEditorAppWindow::RenderRightMenubar() {
  CherryGUI::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));

  if (m_TextSize > m_TextSizeMax)
    m_TextSize = m_TextSizeMax;

  if (m_TextSize < m_TextSizeMin)
    m_TextSize = m_TextSizeMin;

  CherryNextComponent.SetProperty("padding_y", "6.0f");
  CherryNextComponent.SetProperty("padding_x", "10.0f");
  CherryNextComponent.SetProperty("disable_callback", "true");

  if (CherryKit::ButtonImageTextDropdown(
          "Settings", GetPath("resources/imgs/icons/misc/icon_settings.png"))
          .GetDataAs<bool>("isClicked")) {
    CherryGUI::OpenPopup("SettingsMenuPopup");
  }

  auto zoomRender = [this]() {
    int currentPercent = static_cast<int>(std::round(m_TextSize * 200.0f));

    CherryGUI::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    PlusMinuxWidget(false);

    CherryGUI::SameLine();

    CherryGUI::AlignTextToFramePadding();
    CherryGUI::SetNextItemWidth(40.0f);
    CherryGUI::Text("%d%%", currentPercent);

    CherryGUI::SameLine();

    PlusMinuxWidget(true);
    if (CherryGUI::IsItemHovered() && CherryGUI::IsMouseDoubleClicked(0)) {
      ResetZoom();
    }

    CherryGUI::PopStyleVar();
  };

  ImVec2 popupSize(320, 0);
  ImVec2 mousePos = CherryGUI::GetMousePos();
  ImVec2 popupPos = ImVec2(mousePos.x - popupSize.x, mousePos.y + 5);

  CherryGUI::SetNextWindowPos(popupPos, ImGuiCond_Appearing);
  CherryGUI::SetNextWindowSize(popupSize, ImGuiCond_Always);

  CherryGUI::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  CherryGUI::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
  CherryGUI::PushStyleColor(ImGuiCol_Border, Cherry::HexToRGBA("#343434"));
  CherryGUI::PushStyleColor(ImGuiCol_PopupBg, Cherry::HexToRGBA("#121212E6"));

  if (CherryGUI::BeginPopup("SettingsMenuPopup",
                            ImGuiWindowFlags_NoMove |
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoSavedSettings)) {

    CherryNextComponent.SetProperty("header_visible", false);
    CherryNextComponent.SetProperty("padding_x", "2");
    CherryNextComponent.SetProperty("padding_y", "4");

    auto cmp = CherryKit::TableSimple(
        CherryID("Parameters"), "ParamTable",
        {{CherryKit::KeyValCustom("Zoom", zoomRender)},
         {CherryKit::KeyValBool("Auto refresh", &m_AutoRefresh)},
         {CherryKit::KeyValBool("Show spaces", &show_spaces_)},
         {CherryKit::KeyValBool("Show scrollbar minimap",
                                &show_scrollbar_minimap_)},
         {CherryKit::KeyValBool("Show Minimap", &show_minimap_)},
         {CherryKit::KeyValBool("Word wrapping", &word_wrap_)},
         {CherryKit::KeyValBool("Line Folding", &line_folding_)}});

    CherryGUI::EndPopup();
  }

  CherryGUI::PopStyleColor(2);
  CherryGUI::PopStyleVar(2);
  CherryGUI::SetCursorPosY(CherryGUI::GetCursorPosY() - 3.0f);

  CherryGUI::PopStyleColor();
  CherryGUI::SetCursorPosY(CherryGUI::GetCursorPosY() - 1.5f);
}

void TextEditorAppWindow::RenderBottombar() {
  std::string posText = std::to_string(m_CurrentLine + 1) + "/" +
                        std::to_string(m_CurrentColumn + 1);
  std::string linesText = std::to_string(m_TotalLines) + " lines";
  std::string languageText = m_CurrentLanguageDef;

  Cherry::PushFont("JetBrainsMono");
  CherryStyle::PushFontSize(0.5f);
  CherryNextProp("color_text", "#898989");
  CherryStyle::AddMarginX(8.0f);
  CherryStyle::RemoveMarginY(8.0f);
  CherryKit::TextSimple(posText);

  CherryStyle::AddMarginX(8.0f);
  CherryGUI::PushStyleColor(ImGuiCol_Separator, Cherry::HexToRGBA("#454545"));
  CherryGUI::Separator();
  CherryGUI::PopStyleColor();

  CherryStyle::AddMarginX(8.0f);
  CherryNextProp("color_text", "#898989");
  CherryKit::TextSimple(linesText);

  CherryStyle::AddMarginX(8.0f);
  CherryGUI::PushStyleColor(ImGuiCol_Separator, Cherry::HexToRGBA("#454545"));
  CherryGUI::Separator();
  CherryGUI::PopStyleColor();

  CherryStyle::AddMarginX(8.0f);
  CherryNextProp("color_text", "#898989");
  CherryKit::TextSimple(languageText);

  if (m_TextSize != 0.5) {
    int currentPercent = static_cast<int>(std::round(m_TextSize * 200.0f));
    CherryStyle::AddMarginX(8.0f);
    CherryGUI::PushStyleColor(ImGuiCol_Separator, Cherry::HexToRGBA("#454545"));
    CherryGUI::Separator();
    CherryGUI::PopStyleColor();

    CherryStyle::AddMarginX(8.0f);
    CherryNextProp("color_text", "#898989");
    CherryKit::TextSimple(std::to_string(currentPercent) + "%%");
  }

  if (m_AutoRefresh) {
    CherryStyle::AddMarginX(8.0f);
    CherryGUI::PushStyleColor(ImGuiCol_Separator, Cherry::HexToRGBA("#454545"));
    CherryGUI::Separator();
    CherryGUI::PopStyleColor();

    CherryStyle::AddMarginX(8.0f);
    CherryKit::TextSimple("AutoRefresh");
  }
  CherryStyle::PopFontSize();
  Cherry::PopFont();
}

}; // namespace ModuleUI
