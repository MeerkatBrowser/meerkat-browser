/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef MEERKAT_NOTIFICATIONDIALOG_H
#define MEERKAT_NOTIFICATIONDIALOG_H

#include <QtCore/QPropertyAnimation>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

namespace Meerkat
{

class Notification;

class NotificationDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NotificationDialog(Notification *notification, QWidget *parent = NULL);

	bool eventFilter(QObject *object, QEvent *event);

protected:
	void closeEvent(QCloseEvent *event);
	void resizeEvent(QResizeEvent *event);

protected slots:
	void aboutToClose();
	void clean();

private:
	Notification *m_notification;
	QLabel *m_closeLabel;
	QPropertyAnimation *m_animation;
};

}

#endif
