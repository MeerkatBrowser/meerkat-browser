/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef MEERKAT_BOOKMARKPROPERTIESDIALOG_H
#define MEERKAT_BOOKMARKPROPERTIESDIALOG_H

#include "Dialog.h"
#include "../core/BookmarksManager.h"

namespace Meerkat
{

namespace Ui
{
	class BookmarkPropertiesDialog;
}

class BookmarksItem;

class BookmarkPropertiesDialog : public Dialog
{
	Q_OBJECT

public:
	explicit BookmarkPropertiesDialog(BookmarksItem *bookmark, QWidget *parent = NULL);
	BookmarkPropertiesDialog(const QUrl &url, const QString &title, const QString &description = QString(), BookmarksItem *folder = NULL, int index = -1, bool isUrl = true, QWidget *parent = NULL);
	~BookmarkPropertiesDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void saveBookmark();

private:
	BookmarksItem *m_bookmark;
	int m_index;
	Ui::BookmarkPropertiesDialog *m_ui;
};

}

#endif
