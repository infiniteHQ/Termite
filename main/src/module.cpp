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
  // TODO: Custom names with dynamic state of terminals (to implement with
  // Cherry Locales systeme)
  auto inst = ModuleUI::TextEditorAppWindow::Create(path, "Terminal");
  Cherry::AddAppWindow(inst->GetAppWindow());
  CTextEdit->m_text_editor_instances.push_back(inst);
}

std::string TextEdit::GetPath(const std::string &path) {
  return CTextEdit->m_interface->cook_path(path);
}

void TextEdit::Hello() { vxe::log_info("Tt", "cc"); }
