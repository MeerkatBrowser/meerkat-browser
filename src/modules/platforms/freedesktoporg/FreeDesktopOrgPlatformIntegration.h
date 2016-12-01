/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2010 David Sansome <me@davidsansome.com>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_FREEDESKTOPORGPLATFORMINTEGRATION_H
#define MEERKAT_FREEDESKTOPORGPLATFORMINTEGRATION_H

#include "../../../core/Utils.h"
#include "../../../core/PlatformIntegration.h"

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusPendingCallWatcher>

QDBusArgument& operator<<(QDBusArgument &argument, const QImage &image);
const QDBusArgument& operator>>(const QDBusArgument &argument, QImage &image);

namespace Meerkat
{

class FreeDesktopOrgPlatformIntegration : public PlatformIntegration
{
	Q_OBJECT

public:
	explicit FreeDesktopOrgPlatformIntegration(Application *parent);

	void runApplication(const QString &command, const QUrl &url = QUrl()) const;
	QList<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType);
	bool canShowNotifications() const;

public slots:
	void showNotification(Notification *notification);

protected slots:
	void createApplicationsCache();
	void createApplicationsCacheThread();
	void notificationCallFinished(QDBusPendingCallWatcher *watcher);
	void notificationIgnored(quint32 identifier, quint32 reason);
	void notificationClicked(quint32 identifier, const QString &action);

private:
	QDBusInterface *m_notificationsInterface;
	QHash<QDBusPendingCallWatcher*, Notification*> m_notificationWatchers;
	QHash<quint32, Notification*> m_notifications;
	QHash<QString, QList<ApplicationInformation> > m_applicationsCache;
};

}

#endif
