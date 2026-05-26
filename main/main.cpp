#include "./src/module.hpp"

#ifndef CTextEdit
TextEdit::Context *CTextEdit = NULL;
#endif

class Module : public ModuleInterface {
public:
  void execute() override {
    // Create the context pointer of this module
    TextEdit::CreateContext();

    // Get the interface pointer
    CTextEdit->m_interface =
        ModuleInterface::get_editor_module_by_name(this->name());

    this->add_content_browser_item_handler(ItemHandlerInterface(
        "file_cpp", TextEdit::StartTextEditorInstance, "Edit",
        "Edit this C++ file", TextEdit::GetPath("resources/icons/edit.png")));
    this->add_content_browser_item_handler(ItemHandlerInterface(
        "file_lua", TextEdit::StartTextEditorInstance, "Edit",
        "Edit this Lua file", TextEdit::GetPath("resources/icons/edit.png")));
    this->add_content_browser_item_handler(ItemHandlerInterface(
        "file_json", TextEdit::StartTextEditorInstance, "Edit",
        "Edit this JSON file", TextEdit::GetPath("resources/icons/edit.png")));
    this->add_content_browser_item_handler(
        ItemHandlerInterface("file_hpp", TextEdit::StartTextEditorInstance,
                             "Edit", "Edit this C++ header file",
                             TextEdit::GetPath("resources/icons/edit.png")));
    this->add_content_browser_item_handler(ItemHandlerInterface(
        "file_c", TextEdit::StartTextEditorInstance, "Edit", "Edit this C file",
        TextEdit::GetPath("resources/icons/edit.png")));
    this->add_content_browser_item_handler(
        ItemHandlerInterface("file_h", TextEdit::StartTextEditorInstance,
                             "Edit", "Edit this C header file",
                             TextEdit::GetPath("resources/icons/edit.png")));
    this->add_content_browser_item_handler(
        ItemHandlerInterface("file_python", TextEdit::StartTextEditorInstance,
                             "Edit", "Edit this Python file",
                             TextEdit::GetPath("resources/icons/edit.png")));

    this->add_content_browser_item_identifier(ItemIdentifierInterface(
        TextEdit::IsValidFile, "text_edit:superfile", "Super file", "#553333"));

    this->set_credits_file(TextEdit::GetPath("CREDITS"));
    this->add_documentation("Take the editor", "Edit a txt file",
                            TextEdit::GetPath("docs/main.md"));
    this->add_documentation("Take the editor", "Find specific text",
                            TextEdit::GetPath("docs/main.md"));
  }

  void init_ui() override {
    // CherryApp.AddFont(
    //     "JetBrainsMono",
    //     TextEdit::GetPath("resources/fonts/JetBrainsMono-Regular.ttf"), 40.0f);
  }

  void destroy() override {
    // Reset module
    this->reset_module();

    // Clear windows
    for (auto i : CTextEdit->m_text_editor_instances) {
      CherryApp.DeleteAppWindow(i->GetAppWindow());
    }

    // Clear context
    // DestroyContext();
  }
};

#ifdef _WIN32
extern "C" __declspec(dllexport) ModuleInterface *create_em() {
  return new Module();
}
#else
extern "C" ModuleInterface *create_em() { return new Module(); }
#endif
