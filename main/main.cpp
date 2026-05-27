#include "./src/module.hpp"

#ifndef CTermite
Termite::Context *CTermite = NULL;
#endif

class Module : public ModuleInterface {
public:
  void execute() override {
    // Create the context pointer of this module
    Termite::CreateContext();

    // Get the interface pointer
    CTermite->m_interface =
        ModuleInterface::get_editor_module_by_name(this->name());

    // TODO : add_editor_tool()
    // this->add_content_browser_item_handler(ItemHandlerInterface(
    //    "file_cpp", Termite::StartTerminal, "Edit", "Edit this C++ file",
    //    Termite::GetPath("resources/icons/edit.png")));

    this->set_credits_file(Termite::GetPath("CREDITS"));
    this->add_documentation("Take the editor", "Edit a txt file",
                            Termite::GetPath("docs/main.md"));
    this->add_documentation("Take the editor", "Find specific text",
                            Termite::GetPath("docs/main.md"));
  }

  void init_ui() override {
    // CherryApp.AddFont(
    //     "JetBrainsMono",
    //     Termite::GetPath("resources/fonts/JetBrainsMono-Regular.ttf"), 40.0f);
  }

  void destroy() override {
    // Reset module
    this->reset_module();

    // Clear windows
    for (auto i : CTermite->m_text_editor_instances) {
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
