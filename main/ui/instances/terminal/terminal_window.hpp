#pragma once
#include "terminal.hpp"
#include <vxcore/include/vortex.h>
#include <vxcore/include/vortex_internals.h>

#ifndef TEXT_EDITOR_HPP
#define TEXT_EDITOR_HPP

namespace ModuleUI {

class TermiteAppWindow : public std::enable_shared_from_this<TermiteAppWindow> {
public:
  TermiteAppWindow(const std::string &name);

  std::shared_ptr<Cherry::AppWindow> &GetAppWindow();
  static std::shared_ptr<TermiteAppWindow> Create(const std::string &name);
  void SetupRenderCallback();
  void Render();
  void RenderMenubar();
  void RenderRightMenubar();
  void RenderBottombar();

  void SetLanguage(const std::string &name);
  void AutoSetLanguage();

  void PlusMinuxWidget(bool plus);
  void ZoomIn();
  void ZoomOut();
  void ResetZoom();

private:
  std::shared_ptr<VxContext> ctx;
  bool opened;

  std::string m_TermInput;
  float m_TermScrollY = 0.0f;
  bool m_TermScrollToBottom = true;
  ImFont *m_TermFont = nullptr;

  bool m_SaveReady = false;
  bool m_RefreshReady = false;
  float m_TextSize = 0.5f;
  float m_TextSizeMin = 0.3f;
  float m_TextSizeMax = 2.0f;

  int m_CurrentLine = 0;
  int m_CurrentColumn = 0;
  int m_TotalLines = 0;
  std::string m_CurrentLanguageDef = "";
  bool m_CanOverrite = false;

  bool m_AutoRefresh = false;
  bool show_spaces_ = false;
  bool show_scrollbar_minimap_ = false;
  bool show_minimap_ = false;
  bool word_wrap_ = false;
  bool line_folding_ = false;
  std::filesystem::file_time_type m_LastWriteTime{};
  TerminalSystem term;
  float buffer_size;
  int total_lines;
  // Cherry
  std::shared_ptr<Cherry::AppWindow> m_AppWindow;
  ComponentsPool m_ComponentPool;
};
}; // namespace ModuleUI

#endif // LOGUTILITY_H
