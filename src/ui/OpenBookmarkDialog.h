/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef MEERKAT_OPENBOOKMARKDIALOG_H
#define MEERKAT_OPENBOOKMARKDIALOG_H

#include "Dialog.h"
#include "../core/BookmarksManager.h"

#include <QtWidgets/QCompleter>

namespace Meerkat
{

namespace Ui
{
	class OpenBookmarkDialog;
}

class OpenBookmarkDialog : public Dialog
{
	Q_OBJECT

public:
	explicit OpenBookmarkDialog(QWidget *parent = NULL);
	~OpenBookmarkDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void openBookmark();
	void setCompletion(const QString &text);

private:
	QCompleter *m_completer;
	Ui::OpenBookmarkDialog *m_ui;

signals:
	void requestedOpenBookmark(BookmarksItem *bookmark);
};

}

#endif
