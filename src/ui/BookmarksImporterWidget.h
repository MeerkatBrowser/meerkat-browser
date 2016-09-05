/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_BOOKMARKSIMPORTERWIDGET_H
#define MEERKAT_BOOKMARKSIMPORTERWIDGET_H

#include "../core/BookmarksManager.h"

#include <QtWidgets/QWidget>

namespace Meerkat
{

namespace Ui
{
	class BookmarksImporterWidget;
}

class BookmarksImporterWidget : public QWidget
{
	Q_OBJECT

public:
	explicit BookmarksImporterWidget(QWidget *parent = NULL);
	~BookmarksImporterWidget();

	BookmarksItem *getTargetFolder() const;
	QString getSubfolderName() const;
	bool allowDuplicates() const;
	bool isImportingIntoSubfolder() const;
	bool hasToRemoveExisting() const;

protected slots:
	void removeStateChanged(bool checked);
	void toSubfolderChanged(bool checked);

private:
	Ui::BookmarksImporterWidget *m_ui;
};

}

#endif
