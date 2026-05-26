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

  auto &editor = ModuleUI::TextArea(
      &test.x, &test.y, &m_FileEditBuffer, &m_TextSize, &m_CurrentLine,
      &m_CurrentColumn, &m_TotalLines, &m_CurrentLanguageDef, &m_CanOverrite);

  if (!m_FileUpdated) {
    if (editor.GetDataAs<bool>("text_changed")) {
      m_FileEdited = true;
    }
  } else {
    m_FileUpdated = false;
  }

  if (m_FileEdited) {
    this->m_AppWindow->SetSaved(false);
  } else {
    this->m_AppWindow->SetSaved(true);
  }

  // TODO: If not langauge set by the user
  if (m_Type == FileTypes::File_CPP || m_Type == FileTypes::File_HPP) {
    editor.SetProperty("language_name", "C++");
  } else if (m_Type == FileTypes::File_C || m_Type == FileTypes::File_H) {
    editor.SetProperty("language_name", "C");
  } else if (m_Type == FileTypes::File_LUA) {
    editor.SetProperty("language_name", "Lua");
  } else if (m_Type == FileTypes::File_PYTHON) {
    editor.SetProperty("language_name", "Python");
  } else if (m_Type == FileTypes::File_CS) {
    editor.SetProperty("language_name", "C#");
  } else if (m_Type == FileTypes::File_JSON) {
    editor.SetProperty("language_name", "Json");
  } else if (m_Type == FileTypes::File_SQL) {
    editor.SetProperty("language_name", "Sql");
  } else {
    // TODO : Extend this module languages support to plugins (or modules)
  }

  if (editor.GetData("save_ready") == "true") {
    m_SaveReady = true;
    editor.SetData("save_ready", "false");
  }

  if (m_SavePending) {
    editor.SetProperty("save_pending", "true");
    m_SavePending = false;
  }

  if (m_UndoPending) {
    editor.SetProperty("undo_pending", "true");
    m_UndoPending = false;
  }

  if (m_FindPending) {
    editor.SetProperty("find_pending", "true");
    m_FindPending = false;
  }

  if (m_RedoPending) {
    editor.SetProperty("redo_pending", "true");
    m_RedoPending = false;
  }

  if (m_CopyPending) {
    editor.SetProperty("copy_pending", "true");
    m_CopyPending = false;
  }

  if (m_PastePending) {
    editor.SetProperty("paste_pending", "true");
    m_PastePending = false;
  }

  if (m_SaveReady) {
    SaveFile();
    m_SaveReady = false;
  }

  if (m_AutoRefresh && !m_FilePath.empty()) {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto currentWriteTime = fs::last_write_time(m_FilePath, ec);
    if (!ec && currentWriteTime != m_LastWriteTime) {
      RefreshFile();
      editor.SetProperty("refresh_pending", "true");
    }
  }

  if (show_spaces_) {
    editor.SetProperty("show_spaces", "true");
  } else {
    editor.SetProperty("show_spaces", "false");
  }

  if (show_scrollbar_minimap_) {
    editor.SetProperty("show_scrollbar_minimap", "true");
  } else {
    editor.SetProperty("show_scrollbar_minimap", "false");
  }

  if (show_minimap_) {
    editor.SetProperty("show_minimap", "true");
  } else {
    editor.SetProperty("show_minimap", "false");
  }

  if (word_wrap_) {
    editor.SetProperty("word_wrap", "true");
  } else {
    editor.SetProperty("word_wrap", "false");
  }

  if (line_folding_) {
    editor.SetProperty("line_folding", "true");
  } else {
    editor.SetProperty("line_folding", "false");
  }

  if (m_RefreshReady) {
    RefreshFile();
    editor.SetProperty("refresh_pending", "true");
    m_RefreshReady = false;
  }

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
