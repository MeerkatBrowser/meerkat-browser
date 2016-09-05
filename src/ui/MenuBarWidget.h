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

#ifndef MEERKAT_MENUBARWIDGET_H
#define MEERKAT_MENUBARWIDGET_H

#include <QtWidgets/QMenuBar>

namespace Meerkat
{

class MainWindow;
class ToolBarWidget;

class MenuBarWidget : public QMenuBar
{
	Q_OBJECT

public:
	explicit MenuBarWidget(MainWindow *parent);

protected:
	void changeEvent(QEvent *event);
	void resizeEvent(QResizeEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void setup();

protected slots:
	void toolBarModified(int identifier);
	void updateSize();

private:
	MainWindow *m_mainWindow;
	ToolBarWidget *m_leftToolBar;
	ToolBarWidget *m_rightToolBar;
};

}

#endif
