#include "autorun.h"
#include "version.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <shlobj.h>
#endif

#ifdef Q_OS_WIN
static QString entry_path()
{
    const QString startup_dir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup";
    return QDir::toNativeSeparators(startup_dir + "/" + SOFT_NAME + ".lnk");
}

static void create_entry(const QString &lnk_path)
{
    CoInitialize(NULL);

    IShellLink *shell_link;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<void**>(&shell_link));
    if (result == S_OK) {
        IPersistFile *persist_file;
        shell_link->SetPath(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).utf16()));
        shell_link->SetWorkingDirectory(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(QCoreApplication::applicationDirPath()).utf16()));
        result = shell_link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persist_file));
        if (result == S_OK) {
            persist_file->Save(reinterpret_cast<const wchar_t *>(lnk_path.utf16()), TRUE);
            persist_file->Release();
        }
        shell_link->Release();
    }

    CoUninitialize();
}
#else
static QString entry_path()
{
    const QString autostart_dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    return autostart_dir + "/" + SOFT_NAME + ".desktop";
}

static void create_entry(const QString &desktop_file_path)
{
    QDir().mkpath(QFileInfo(desktop_file_path).path());

    QFile desktop_file(desktop_file_path);
    if (!desktop_file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream stream(&desktop_file);
    stream << "[Desktop Entry]\n"
           << "Type=Application\n"
           << "Exec=" << QCoreApplication::applicationFilePath() << "\n"
           << "Path=" << QCoreApplication::applicationDirPath() << "\n"
           << "Name=" << SOFT_NAME << "\n";
}
#endif

bool Autorun::is_enabled()
{
    return QFile::exists(entry_path());
}

void Autorun::set_enabled(bool enabled)
{
    const QString path = entry_path();

    QFile::remove(path);
    if (enabled) create_entry(path);
}
