#include "../ui/instances/terminal/terminal_window.hpp"
#include <vxcore/include/vortex.h>
#include <vxcore/include/vortex_internals.h>
#include <vxgui/editor/main/editor.hpp>

#ifndef SAMPLE_MODULE_HPP
#define SAMPLE_MODULE_HPP

namespace Termite {
struct Context {
  std::shared_ptr<ModuleInterface> m_interface;
  std::vector<std::shared_ptr<ModuleUI::TermiteAppWindow>>
      m_text_editor_instances;
};
} // namespace Termite

#ifndef TERMITE_API
#define TERMITE_API
#endif

#ifndef CTermite
extern TERMITE_API Termite::Context *CTermite;
#endif

namespace Termite {
TERMITE_API void CreateContext();
TERMITE_API void DestroyContext();

// utils
TERMITE_API std::string GetPath(const std::string &path);

// main features
TERMITE_API void StartTerminal();
} // namespace Termite

#endif // SAMPLE_MODULE_HPP