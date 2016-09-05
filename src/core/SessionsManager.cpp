/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "SessionsManager.h"
#include "ActionsManager.h"
#include "Application.h"
#include "Utils.h"
#include "WindowsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QDir>
#include <QtCore/QSaveFile>
#include <QtCore/QSettings>

namespace Meerkat
{

SessionsManager* SessionsManager::m_instance = NULL;
QPointer<MainWindow> SessionsManager::m_activeWindow = NULL;
QString SessionsManager::m_sessionPath;
QString SessionsManager::m_sessionTitle;
QString SessionsManager::m_cachePath;
QString SessionsManager::m_profilePath;
QList<MainWindow*> SessionsManager::m_windows;
QList<SessionMainWindow> SessionsManager::m_closedWindows;
bool SessionsManager::m_isDirty = false;
bool SessionsManager::m_isPrivate = false;
bool SessionsManager::m_isReadOnly = false;

SessionsManager::SessionsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void SessionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		m_isDirty = false;

		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (!m_isPrivate)
		{
			saveSession(QString(), QString(), NULL, false);
		}
	}
}

void SessionsManager::createInstance(const QString &profilePath, const QString &cachePath, bool isPrivate, bool isReadOnly, QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new SessionsManager(parent);
		m_cachePath = cachePath;
		m_profilePath = profilePath;
		m_isPrivate = isPrivate;
		m_isReadOnly = isReadOnly;
	}
}

void SessionsManager::scheduleSave()
{
	if (m_saveTimer == 0 && !m_isPrivate)
	{
		m_saveTimer = startTimer(1000);
	}
}

void SessionsManager::clearClosedWindows()
{
	m_closedWindows.clear();

	emit m_instance->closedWindowsChanged();
}

void SessionsManager::registerWindow(MainWindow *window)
{
	if (window)
	{
		m_windows.append(window);
	}
}

void SessionsManager::storeClosedWindow(MainWindow *window)
{
	if (!window)
	{
		return;
	}

	m_windows.removeAll(window);

	SessionMainWindow session(window->getWindowsManager()->getSession());
	session.geometry = window->saveGeometry();

	if (!session.windows.isEmpty())
	{
		m_closedWindows.prepend(session);

		emit m_instance->closedWindowsChanged();
	}
}

void SessionsManager::markSessionModified()
{
	if (!m_isPrivate && !m_isDirty && m_sessionPath == QLatin1String("default"))
	{
		m_isDirty = true;

		m_instance->scheduleSave();
	}
}

void SessionsManager::removeStoredUrl(const QString &url)
{
	emit m_instance->requestedRemoveStoredUrl(url);
}

void SessionsManager::setActiveWindow(MainWindow *window)
{
	m_activeWindow = window;
}

SessionsManager* SessionsManager::getInstance()
{
	return m_instance;
}

WindowsManager* SessionsManager::getWindowsManager()
{
	return (m_activeWindow ? m_activeWindow->getWindowsManager() : NULL);
}

MainWindow* SessionsManager::getActiveWindow()
{
	return m_activeWindow;
}

QString SessionsManager::getCurrentSession()
{
	return m_sessionPath;
}

QString SessionsManager::getCachePath()
{
	return (m_isReadOnly ? QString() : m_cachePath);
}

QString SessionsManager::getProfilePath()
{
	return m_profilePath;
}

QString SessionsManager::getReadableDataPath(const QString &path, bool forceBundled)
{
	const QString writablePath(getWritableDataPath(path));

	return ((!forceBundled && QFile::exists(writablePath)) ? writablePath : QLatin1String(":/") + (path.contains(QLatin1Char('/')) ? QString() : QLatin1String("other/")) + QDir::separator() + path);
}

QString SessionsManager::getWritableDataPath(const QString &path)
{
	return QDir::toNativeSeparators(m_profilePath + QDir::separator() + path);
}

QString SessionsManager::getSessionPath(const QString &path, bool isBound)
{
	QString cleanPath(path);

	if (cleanPath.isEmpty())
	{
		cleanPath = QLatin1String("default.ini");
	}
	else
	{
		if (!cleanPath.endsWith(QLatin1String(".ini")))
		{
			cleanPath += QLatin1String(".ini");
		}

		if (isBound)
		{
			cleanPath = cleanPath.replace(QLatin1Char('/'), QString()).replace(QLatin1Char('\\'), QString());
		}
		else if (QFileInfo(cleanPath).isAbsolute())
		{
			return cleanPath;
		}
	}

	return QDir::toNativeSeparators(m_profilePath + QLatin1String("/sessions/") + cleanPath);
}

SessionInformation SessionsManager::getSession(const QString &path)
{
	const QString sessionPath(getSessionPath(path));
	QSettings sessionData(sessionPath, QSettings::IniFormat);
	sessionData.setIniCodec("UTF-8");

	SessionInformation session;
	session.path = path;
	session.title = sessionData.value(QLatin1String("Session/title"), ((path == QLatin1String("default")) ? tr("Default") : tr("(Untitled)"))).toString();
	session.index = (sessionData.value(QLatin1String("Session/index"), 1).toInt() - 1);
	session.isClean = sessionData.value(QLatin1String("Session/clean"), true).toBool();

	const int windows(sessionData.value(QLatin1String("Session/windows"), 0).toInt());
	const int defaultZoom(SettingsManager::getValue(SettingsManager::Content_DefaultZoomOption).toInt());

	for (int i = 1; i <= windows; ++i)
	{
		const int tabs(sessionData.value(QStringLiteral("%1/Properties/windows").arg(i), 0).toInt());
		SessionMainWindow sessionEntry;
		sessionEntry.geometry = QByteArray::fromBase64(sessionData.value(QStringLiteral("%1/Properties/geometry").arg(i), 1).toString().toLatin1());
		sessionEntry.index = (sessionData.value(QStringLiteral("%1/Properties/index").arg(i), 1).toInt() - 1);

		for (int j = 1; j <= tabs; ++j)
		{
			const QString state(sessionData.value(QStringLiteral("%1/%2/Properties/state").arg(i).arg(j), QString()).toString());
			const QString searchEngine(sessionData.value(QStringLiteral("%1/%2/Properties/searchEngine").arg(i).arg(j), QString()).toString());
			const QString userAgent(sessionData.value(QStringLiteral("%1/%2/Properties/userAgent").arg(i).arg(j), QString()).toString());
			const QStringList geometry(sessionData.value(QStringLiteral("%1/%2/Properties/geometry").arg(i).arg(j), QString()).toString().split(QLatin1Char(',')));
			const int history(sessionData.value(QStringLiteral("%1/%2/Properties/history").arg(i).arg(j), 0).toInt());
			const int reloadTime(sessionData.value(QStringLiteral("%1/%2/Properties/reloadTime").arg(i).arg(j), -1).toInt());
			SessionWindow sessionWindow;
			sessionWindow.geometry = ((geometry.count() == 4) ? QRect(geometry.at(0).toInt(), geometry.at(1).toInt(), geometry.at(2).toInt(), geometry.at(3).toInt()) : QRect());
			sessionWindow.state = ((state == QLatin1String("maximized")) ? MaximizedWindowState : ((state == QLatin1String("minimized")) ? MinimizedWindowState : NormalWindowState));
			sessionWindow.parentGroup = sessionData.value(QStringLiteral("%1/%2/Properties/group").arg(i).arg(j), 0).toInt();
			sessionWindow.historyIndex = (sessionData.value(QStringLiteral("%1/%2/Properties/index").arg(i).arg(j), 1).toInt() - 1);
			sessionWindow.isAlwaysOnTop = sessionData.value(QStringLiteral("%1/%2/Properties/alwaysOnTop").arg(i).arg(j), false).toBool();
			sessionWindow.isPinned = sessionData.value(QStringLiteral("%1/%2/Properties/pinned").arg(i).arg(j), false).toBool();

			if (!searchEngine.isEmpty())
			{
				sessionWindow.overrides[SettingsManager::Search_DefaultSearchEngineOption] = searchEngine;
			}

			if (!userAgent.isEmpty())
			{
				sessionWindow.overrides[SettingsManager::Network_UserAgentOption] = userAgent;
			}

			if (reloadTime >= 0)
			{
				sessionWindow.overrides[SettingsManager::Content_PageReloadTimeOption] = reloadTime;
			}

			for (int k = 1; k <= history; ++k)
			{
				const QStringList position(sessionData.value(QStringLiteral("%1/%2/History/%3/position").arg(i).arg(j).arg(k), 1).toStringList());
				WindowHistoryEntry historyEntry;
				historyEntry.url = sessionData.value(QStringLiteral("%1/%2/History/%3/url").arg(i).arg(j).arg(k), 0).toString();
				historyEntry.title = sessionData.value(QStringLiteral("%1/%2/History/%3/title").arg(i).arg(j).arg(k), 1).toString();
				historyEntry.position = QPoint(position.value(0, QString::number(0)).toInt(), position.value(1, QString::number(0)).toInt());
				historyEntry.zoom = sessionData.value(QStringLiteral("%1/%2/History/%3/zoom").arg(i).arg(j).arg(k), defaultZoom).toInt();

				sessionWindow.history.append(historyEntry);
			}

			sessionEntry.windows.append(sessionWindow);
		}

		session.windows.append(sessionEntry);
	}

	return session;
}

QList<MainWindow*> SessionsManager::getWindows()
{
	return m_windows;
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		const SessionMainWindow window(m_closedWindows.at(i));
		const QString title(window.windows.value(window.index, SessionWindow()).getTitle());

		closedWindows.append(title.isEmpty() ? tr("(Untitled)") : title);
	}

	return closedWindows;
}

QStringList SessionsManager::getSessions()
{
	QStringList entries(QDir(m_profilePath + QLatin1String("/sessions/")).entryList(QStringList(QLatin1String("*.ini")), QDir::Files));

	for (int i = 0; i < entries.count(); ++i)
	{
		entries[i] = QFileInfo(entries.at(i)).completeBaseName();
	}

	if (!m_sessionPath.isEmpty() && !entries.contains(m_sessionPath))
	{
		entries.append(m_sessionPath);
	}

	if (!entries.contains(QLatin1String("default")))
	{
		entries.append(QLatin1String("default"));
	}

	entries.sort();

	return entries;
}

bool SessionsManager::restoreClosedWindow(int index)
{
	if (index < 0)
	{
		index = 0;
	}

	Application::getInstance()->createWindow(Application::NoFlags, false, m_closedWindows.value(index, SessionMainWindow()));

	m_closedWindows.removeAt(index);

	emit m_instance->closedWindowsChanged();

	return true;
}

bool SessionsManager::restoreSession(const SessionInformation &session, MainWindow *window, bool isPrivate)
{
	if (session.windows.isEmpty())
	{
		if (m_sessionPath.isEmpty() && session.path == QLatin1String("default"))
		{
			m_sessionPath = QLatin1String("default");
		}

		return false;
	}

	if (m_sessionPath.isEmpty())
	{
		m_sessionPath = session.path;
		m_sessionTitle = session.title;
	}

	for (int i = 0; i < session.windows.count(); ++i)
	{
		if (window && i == 0)
		{
			window->getWindowsManager()->restore(session.windows.first());
		}
		else
		{
			Application::getInstance()->createWindow((isPrivate ? Application::PrivateFlag : Application::NoFlags), false, session.windows.at(i));
		}
	}

	return true;
}

bool SessionsManager::saveSession(const QString &path, const QString &title, MainWindow *window, bool isClean)
{
	if (m_isPrivate && path.isEmpty())
	{
		return false;
	}

	SessionInformation session;
	session.path = getSessionPath(path);
	session.title = (title.isEmpty() ? m_sessionTitle : title);
	session.isClean = isClean;

	QList<MainWindow*> windows;

	if (window)
	{
		windows.append(window);
	}
	else
	{
		windows = Application::getInstance()->getWindows();
	}

	for (int i = 0; i < windows.count(); ++i)
	{
		session.windows.append(windows.at(i)->getWindowsManager()->getSession());
		session.windows.last().geometry = windows.at(i)->saveGeometry();
	}

	return saveSession(session);
}

bool SessionsManager::saveSession(const SessionInformation &session)
{
	QDir().mkpath(m_profilePath + QLatin1String("/sessions/"));

	if (session.windows.isEmpty())
	{
		return false;
	}

	QString path(session.path);

	if (path.isEmpty())
	{
		path = m_profilePath + QLatin1String("/sessions/") + session.title + QLatin1String(".ini");

		if (QFileInfo(path).exists())
		{
			int i = 1;

			while (QFileInfo(m_profilePath + QLatin1String("/sessions/") + session.title + QString::number(i) + QLatin1String(".ini")).exists())
			{
				++i;
			}

			path = m_profilePath + QLatin1String("/sessions/") + session.title + QString::number(i) + QLatin1String(".ini");
		}
	}

	QSaveFile file(path);

	if (!file.open(QIODevice::WriteOnly))
	{
		return false;
	}

	const QString defaultSearchEngine(SettingsManager::getValue(SettingsManager::Search_DefaultSearchEngineOption).toString());
	const QString defaultUserAgent(SettingsManager::getValue(SettingsManager::Network_UserAgentOption).toString());
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream << QLatin1String("[Session]\n");
	stream << Utils::formatConfigurationEntry(QLatin1String("title"), session.title, true);

	if (!session.isClean)
	{
		stream << QLatin1String("clean=false\n");
	}

	stream << QLatin1String("windows=") << session.windows.count() << QLatin1Char('\n');
	stream << QLatin1String("index=1\n\n");

	for (int i = 0; i < session.windows.count(); ++i)
	{
		const SessionMainWindow sessionEntry(session.windows.at(i));

		stream << QStringLiteral("[%1/Properties]\n").arg(i + 1);
		stream << Utils::formatConfigurationEntry(QLatin1String("geometry"), session.windows.at(i).geometry.toBase64(), true);
		stream << QLatin1String("groups=0\n");
		stream << QLatin1String("windows=") << sessionEntry.windows.count() << QLatin1Char('\n');
		stream << QLatin1String("index=") << (sessionEntry.index + 1) << QLatin1String("\n\n");

		for (int j = 0; j < sessionEntry.windows.count(); ++j)
		{
			stream << QStringLiteral("[%1/%2/Properties]\n").arg(i + 1).arg(j + 1);

			if (sessionEntry.windows.at(j).state == NormalWindowState)
			{
				stream << Utils::formatConfigurationEntry(QLatin1String("geometry"), QStringLiteral("%1,%2,%3,%4").arg(sessionEntry.windows.at(j).geometry.x()).arg(sessionEntry.windows.at(j).geometry.y()).arg(sessionEntry.windows.at(j).geometry.width()).arg(sessionEntry.windows.at(j).geometry.height()), true);
			}
			else
			{
				stream << Utils::formatConfigurationEntry(QLatin1String("state"), ((sessionEntry.windows.at(j).state == MaximizedWindowState) ? QLatin1String("maximized") : QLatin1String("minimized")));
			}

			if (sessionEntry.windows.at(j).overrides.value(SettingsManager::Search_DefaultSearchEngineOption, QString()).toString() != defaultSearchEngine)
			{
				stream << Utils::formatConfigurationEntry(QLatin1String("searchEngine"), sessionEntry.windows.at(j).overrides[SettingsManager::Search_DefaultSearchEngineOption].toString());
			}

			if (sessionEntry.windows.at(j).overrides.value(SettingsManager::Network_UserAgentOption, QString()).toString() != defaultUserAgent)
			{
				stream << Utils::formatConfigurationEntry(QLatin1String("userAgent"), sessionEntry.windows.at(j).overrides[SettingsManager::Network_UserAgentOption].toString(), true);
			}

			if (sessionEntry.windows.at(j).overrides.value(SettingsManager::Content_PageReloadTimeOption, -1).toInt() != -1)
			{
				stream << Utils::formatConfigurationEntry(QLatin1String("reloadTime"), QString::number(sessionEntry.windows.at(j).overrides[SettingsManager::Content_PageReloadTimeOption].toInt()));
			}

			if (sessionEntry.windows.at(j).isAlwaysOnTop)
			{
				stream << QLatin1String("alwaysOnTop=true\n");
			}

			if (sessionEntry.windows.at(j).isPinned)
			{
				stream << QLatin1String("pinned=true\n");
			}

			stream << QLatin1String("history=") << sessionEntry.windows.at(j).history.count() << QLatin1Char('\n');
			stream << QLatin1String("index=") << (sessionEntry.windows.at(j).historyIndex + 1) << QLatin1String("\n\n");

			for (int k = 0; k < sessionEntry.windows.at(j).history.count(); ++k)
			{
				stream << QStringLiteral("[%1/%2/History/%3]\n").arg(i + 1).arg(j + 1).arg(k + 1);
				stream << Utils::formatConfigurationEntry(QLatin1String("url"), sessionEntry.windows.at(j).history.at(k).url, true);
				stream << Utils::formatConfigurationEntry(QLatin1String("title"), sessionEntry.windows.at(j).history.at(k).title, true);
				stream << QLatin1String("position=") << sessionEntry.windows.at(j).history.at(k).position.x() << ',' << sessionEntry.windows.at(j).history.at(k).position.y() << QLatin1Char('\n');
				stream << QLatin1String("zoom=") << sessionEntry.windows.at(j).history.at(k).zoom << QLatin1String("\n\n");
			}
		}
	}

	return file.commit();
}

bool SessionsManager::deleteSession(const QString &path)
{
	const QString cleanPath(getSessionPath(path, true));

	if (QFile::exists(cleanPath))
	{
		return QFile::remove(cleanPath);
	}

	return false;
}

bool SessionsManager::isLastWindow()
{
	return (m_windows.count() == 1);
}

bool SessionsManager::isPrivate()
{
	return m_isPrivate;
}

bool SessionsManager::isReadOnly()
{
	return m_isReadOnly;
}

bool SessionsManager::hasUrl(const QUrl &url, bool activate)
{
	for (int i = 0; i < m_windows.count(); ++i)
	{
		if (m_windows.at(i)->getWindowsManager()->hasUrl(url, activate))
		{
			QWidget *window(qobject_cast<QWidget*>(m_windows.at(i)->parent()));

			if (window)
			{
				window->raise();
				window->activateWindow();
			}

			return true;
		}
	}

	return false;
}

}
