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

#ifndef MEERKAT_NAVIGATIONACTIONWIDGET_H
#define MEERKAT_NAVIGATIONACTIONWIDGET_H

#include "ActionWidget.h"
#include "../../../core/SessionsManager.h"

namespace Meerkat
{

class NavigationActionWidget : public ActionWidget
{
	Q_OBJECT

public:
	explicit NavigationActionWidget(Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent = nullptr);

	bool eventFilter(QObject *object, QEvent *event);

protected:
	void addMenuEntry(int index, const WindowHistoryEntry &entry);
	bool event(QEvent *event);

protected slots:
	void goToHistoryIndex(QAction *action);
	void updateMenu();
};

}

#endif
