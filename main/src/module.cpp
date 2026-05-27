#include "module.hpp"

void Termite::CreateContext() {
  Termite::Context *ctx = new (Termite::Context);
  CTermite = ctx;
}

void Termite::DestroyContext() { free(CTermite); }

void Termite::StartTerminal() {
  // TODO: Custom names with dynamic state of terminals (to implement with
  // Cherry Locales systeme)
  auto inst = ModuleUI::TextEditorAppWindow::Create("Terminal");
  Cherry::AddAppWindow(inst->GetAppWindow());
  CTermite->m_text_editor_instances.push_back(inst);
}

std::string Termite::GetPath(const std::string &path) {
  return CTermite->m_interface->cook_path(path);
}
