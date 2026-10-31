#ifndef LOGGER_H
#define LOGGER_H

#include <QString>

// Redirects qInfo()/qWarning()/... to a file. The previous log is kept as
// "<name>.old" once the file grows over the size limit.
void install_file_logger(const QString &path);

#endif // LOGGER_H
