#include "terminal_window.hpp"
#include "../../../src/module.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace ModuleUI {

void TermiteAppWindow::PlusMinuxWidget(bool plus) {
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

TermiteAppWindow::TermiteAppWindow(const std::string &name) {
  m_AppWindow = std::make_shared<Cherry::AppWindow>(name, name);
  term.Start();
  m_AppWindow->SetLeftMenubarCallback([this]() { RenderMenubar(); });
  m_AppWindow->SetRightMenubarCallback([this]() { RenderRightMenubar(); });
  m_AppWindow->SetSaveMode(true);

  m_AppWindow->m_CloseCallback = [=]() {
    Cherry::DeleteAppWindow(m_AppWindow);
    m_AppWindow->SetVisibility(false);
  };

  std::shared_ptr<Cherry::AppWindow> win = m_AppWindow;

  this->ctx = vxe::get_current_context();
}

std::shared_ptr<Cherry::AppWindow> &TermiteAppWindow::GetAppWindow() {
  return m_AppWindow;
}

std::shared_ptr<TermiteAppWindow>
TermiteAppWindow::Create(const std::string &name) {
  auto instance = std::shared_ptr<TermiteAppWindow>(new TermiteAppWindow(name));
  instance->SetupRenderCallback();
  return instance;
}

void TermiteAppWindow::SetupRenderCallback() {
  auto self = shared_from_this();
  m_AppWindow->SetRenderCallback([self]() {
    if (self) {
      self->Render();
    }
  });
}

void TermiteAppWindow::RenderMenubar() {
  CherryGUI::SetCursorPosX(CherryGUI::GetCursorPosX() + 3.0f);

  CherryKit::Separator();

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

void TermiteAppWindow::Render() {
  CherryApp.PushComponentPool(&m_ComponentPool);

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

void TermiteAppWindow::ZoomIn() {
  m_TextSize += 0.05f;
  if (m_TextSize > 1.15f)
    m_TextSize = 1.15f;
}

void TermiteAppWindow::ZoomOut() {
  m_TextSize -= 0.05f;
  if (m_TextSize < 0.35f)
    m_TextSize = 0.35f;
}

void TermiteAppWindow::ResetZoom() {
  m_TextSize = 0.50f; // 100%
}

void TermiteAppWindow::RenderRightMenubar() {
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

    auto cmp =
        CherryKit::TableSimple(CherryID("Parameters"), "ParamTable",
                               {{CherryKit::KeyValCustom("Zoom", zoomRender)}});

    CherryGUI::EndPopup();
  }

  CherryGUI::PopStyleColor(2);
  CherryGUI::PopStyleVar(2);
  CherryGUI::SetCursorPosY(CherryGUI::GetCursorPosY() - 3.0f);

  CherryGUI::PopStyleColor();
  CherryGUI::SetCursorPosY(CherryGUI::GetCursorPosY() - 1.5f);
}
}; // namespace ModuleUI
