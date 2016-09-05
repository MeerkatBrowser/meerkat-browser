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

#ifndef MEERKAT_BOOKMARKSCONTENTSWIDGET_H
#define MEERKAT_BOOKMARKSCONTENTSWIDGET_H

#include "../../../core/BookmarksManager.h"
#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Meerkat
{

namespace Ui
{
	class BookmarksContentsWidget;
}

class Window;

class BookmarksContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit BookmarksContentsWidget(Window *window);
	~BookmarksContentsWidget();

	void print(QPrinter *printer);
	Action* getAction(int identifier);
	QString getTitle() const;
	QLatin1String getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());

protected:
	void changeEvent(QEvent *event);
	BookmarksItem* findFolder(const QModelIndex &index);

protected slots:
	void addBookmark();
	void addFolder();
	void addSeparator();
	void removeBookmark();
	void restoreBookmark();
	void openBookmark(const QModelIndex &index = QModelIndex());
	void bookmarkProperties();
	void showContextMenu(const QPoint &point);
	void updateActions();

private:
	QHash<int, Action*> m_actions;
	Ui::BookmarksContentsWidget *m_ui;
};

}

#endif
