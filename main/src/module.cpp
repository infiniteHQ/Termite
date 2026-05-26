#include "module.hpp"

void TextEdit::CreateContext() {
  TextEdit::Context *ctx = new (TextEdit::Context);
  CTextEdit = ctx;
}

void TextEdit::DestroyContext() { free(CTextEdit); }

bool TextEdit::IsValidFile(const std::string &path) {
  namespace fs = std::filesystem;

  if (!fs::is_directory(path)) {
    return false;
  }

  for (const auto &entry : fs::directory_iterator(path)) {
    if (entry.is_regular_file() &&
        entry.path().filename() == "SampleConfig.txt") {
      return true;
    }
  }

  return false;
}

void TextEdit::StartTextEditorInstance(const std::string &path) {
  std::string filename = fs::path(path).filename().string();

  const size_t maxLen = 24;
  if (filename.size() > maxLen) {
    filename = filename.substr(0, maxLen - 3) + "...";
  }

  std::string window_name =
      filename + "####" +
      std::to_string(CTextEdit->m_text_editor_instances.size());

  auto inst = ModuleUI::TextEditorAppWindow::Create(path, window_name);
  Cherry::AddAppWindow(inst->GetAppWindow());
  CTextEdit->m_text_editor_instances.push_back(inst);
}

std::string TextEdit::GetPath(const std::string &path) {
  return CTextEdit->m_interface->cook_path(path);
}

void TextEdit::Hello() { vxe::log_info("Tt", "cc"); }
