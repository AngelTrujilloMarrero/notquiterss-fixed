/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* © 2011-2020 QuiteRSS Project
* © 2026 Artem S. Tashkinov <aros@gmx.com> and ChatGPT
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "common.h"

#include <QtCore>
#include <QApplication>
#if defined Q_OS_WIN
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#ifdef Q_OS_MAC
#include <CoreServices/CoreServices.h>
#endif

bool Common::removePath(const QString &path)
{
  bool result = true;
  QFileInfo info(path);
  if (info.isDir()) {
    QDir dir(path);
    foreach (const QString &entry, dir.entryList(QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)) {
      result &= removePath(dir.absoluteFilePath(entry));
    }
    if (!info.dir().rmdir(info.fileName()))
      return false;
  } else {
    result = QFile::remove(path);
  }
  return result;
}

/** @brief Matches domain (assumes both pattern and domain not starting with dot)
 * @param pattern = domain to be matched
 * @param domain = site domain
 *----------------------------------------------------------------------------*/
bool Common::matchDomain(const QString &pattern, const QString &domain)
{
  if (pattern == domain) {
    return true;
  }

  if (!domain.endsWith(pattern)) {
    return false;
  }

  int index = domain.indexOf(pattern);

  return index > 0 && domain[index - 1] == QLatin1Char('.');
}

QString Common::filterCharsFromFilename(const QString &name)
{
  QString value = name;

  value.replace(QLatin1Char('/'), QLatin1Char('-'));
  value.remove(QLatin1Char('\\'));
  value.remove(QLatin1Char(':'));
  value.remove(QLatin1Char('*'));
  value.remove(QLatin1Char('?'));
  value.remove(QLatin1Char('"'));
  value.remove(QLatin1Char('<'));
  value.remove(QLatin1Char('>'));
  value.remove(QLatin1Char('|'));

  return value;
}

QString Common::sanitizeForDisplay(const QString &text)
{
  if (text.isEmpty())
    return text;
  // Workaround for Qt/fontconfig crash (FcCharSetHasChar segfault) when
  // shaping variation selectors in QTreeView/QTextLayout, e.g. feed name
  // "BandaAncha:\uFE0F ..." (U+FE0F). Strip variation selectors and
  // other default-ignorable format chars that have no visible glyph here.
  QString out;
  out.reserve(text.size());
  for (int i = 0; i < text.size(); ++i) {
    const uint u = text.at(i).unicode();
    // Variation Selectors U+FE00..U+FE0F
    if (u >= 0xFE00 && u <= 0xFE0F)
      continue;
    // Variation Selectors Supplement U+E0100..U+E01EF (surrogate pairs)
    if (QChar::isHighSurrogate(text.at(i).unicode()) && i + 1 < text.size() &&
        QChar::isLowSurrogate(text.at(i + 1).unicode())) {
      const uint cp = QChar::surrogateToUcs4(text.at(i).unicode(),
                                             text.at(i + 1).unicode());
      if (cp >= 0xE0100 && cp <= 0xE01EF) {
        ++i;
        continue;
      }
    }
    // Zero-width joiner/non-joiner, word joiner, zero-width space: keep ZWJ
    // for emoji sequences, drop others that only affect shaping.
    if (u == 0x200B || u == 0x2060 || u == 0xFEFF)
      continue;
    out.append(text.at(i));
  }
  return out;
}

QString Common::ensureUniqueFilename(const QString &name, const QString &appendFormat)
{
  if (!QFile::exists(name)) {
    return name;
  }

  QString tmpFileName = name;
  int i = 1;
  while (QFile::exists(tmpFileName)) {
    tmpFileName = name;
    int index = tmpFileName.lastIndexOf(QLatin1Char('.'));

    QString appendString = appendFormat.arg(i);
    if (index == -1) {
      tmpFileName.append(appendString);
    }
    else {
      tmpFileName = tmpFileName.left(index) + appendString + tmpFileName.mid(index);
    }
    i++;
  }
  return tmpFileName;
}

QString Common::readAllFileContents(const QString &filename)
{
  return QString::fromUtf8(readAllFileByteContents(filename));
}

QByteArray Common::readAllFileByteContents(const QString &filename)
{
  QFile file(filename);

  if (!filename.isEmpty() && file.open(QFile::ReadOnly)) {
    const QByteArray a = file.readAll();
    file.close();
    return a;
  }

  return QByteArray();
}

void Common::sleep(int ms)
{
#if defined(Q_OS_WIN)
  Sleep(DWORD(ms));
#else
  struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
  nanosleep(&ts, NULL);
#endif
}

QString Common::operatingSystem()
{
  return QSysInfo::prettyProductName();
}

QString Common::cpuArchitecture()
{
  return QSysInfo::currentCpuArchitecture();
}

QString Common::operatingSystemLong()
{
  QString os = Common::operatingSystem();
#ifdef Q_OS_UNIX
    if (QGuiApplication::platformName() == QL1S("xcb"))
        os.prepend(QL1S("X11; "));
    else if (QGuiApplication::platformName().startsWith(QL1S("wayland")))
        os.prepend(QL1S("Wayland; "));
#endif

  const QString arch = cpuArchitecture();
  if (arch.isEmpty())
    return os;
  return os + QSL(" ") + arch;
}
