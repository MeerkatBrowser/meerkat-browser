/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
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

#ifndef MEERKAT_STARTPAGEMODEL_H
#define MEERKAT_STARTPAGEMODEL_H

#include "../../../core/BookmarksModel.h"

namespace Meerkat
{

class BookmarksItem;

class StartPageModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum
	{
		IsDraggedRole = BookmarksModel::UserRole,
		IsReloadingRole
	};

	explicit StartPageModel(QObject *parent = nullptr);

	QMimeData* mimeData(const QModelIndexList &indexes) const;
	QVariant data(const QModelIndex &index, int role) const;
	QStringList mimeTypes() const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
	bool event(QEvent *event);

public slots:
	void reloadModel();
	void reloadTile(const QModelIndex &index, bool full = false);

protected slots:
	void optionChanged(int identifier);
	void dragEnded();
	void thumbnailCreated(const QUrl &url, const QPixmap &thumbnail, const QString &title);

private:
	BookmarksItem *m_bookmark;
	QHash<QUrl, QPair<quint64, bool> > m_reloads;

signals:
	void modelModified();
	void isReloadingTileChanged(const QModelIndex &index);
};

}

#endif
