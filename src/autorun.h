#ifndef AUTORUN_H
#define AUTORUN_H

// Start with the user session: a shortcut in the Startup folder on Windows,
// a .desktop file in ~/.config/autostart elsewhere.
namespace Autorun {
bool is_enabled();
void set_enabled(bool enabled);
}

#endif // AUTORUN_H
