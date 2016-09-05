/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef MEERKAT_TRAYICON_H
#define MEERKAT_TRAYICON_H

#include <QtWidgets/QAction>
#include <QtWidgets/QSystemTrayIcon>

namespace Meerkat
{

class Application;
class Notification;

class TrayIcon : public QObject
{
	Q_OBJECT

public:
	explicit TrayIcon(Application *parent);

	void showMessage(Notification *notification);

public slots:
	void hide();

protected slots:
	void timerEvent(QTimerEvent *event);
	void activated(QSystemTrayIcon::ActivationReason reason);
	void triggerAction(QAction *action);
	void updateMenu();
	void messageClicked();
	void messageIgnored();

private:
	QSystemTrayIcon *m_trayIcon;
	Notification *m_notification;
	int m_autoHideTimer;
};

}

#endif
