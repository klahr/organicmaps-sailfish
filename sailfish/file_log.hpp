#pragma once

#include <QString>

namespace sailfish::file_log
{
// Debug logs of the core and Qt in a file, for bug reports, like the Android "Enable logging" setting.
void Enable(bool enabled);
bool IsEnabled();
QString Path();
}  // namespace sailfish::file_log
