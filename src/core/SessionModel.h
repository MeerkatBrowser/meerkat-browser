/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef MEERKAT_SESSIONMODEL_H
#define MEERKAT_SESSIONMODEL_H

#include <QtGui/QStandardItemModel>

namespace Meerkat
{

class SessionModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum EntityType
	{
		UnknownEntity = 0,
		SessionEntity = 1,
		TrashEntity = 2,
		MainWindowEntity = 3,
		WindowEntity = 4,
		HistoryEntity = 5
	};

	enum EntityRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		IdentifierRole = Qt::UserRole,
		TypeRole = (Qt::UserRole + 1),
		IndexRole = (Qt::UserRole + 2),
		LastActivityRole = (Qt::UserRole + 3),
		ZoomRole = (Qt::UserRole + 4),
		PinnedRole = (Qt::UserRole + 5),
		SettingsRole = (Qt::UserRole + 6)
	};

	explicit SessionModel(const QString &path, QObject *parent);

private:
	QString m_path;
};

}

#endif
