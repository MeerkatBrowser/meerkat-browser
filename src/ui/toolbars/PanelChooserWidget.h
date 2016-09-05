/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_PANELCHOOSERWIDGET_H
#define MEERKAT_PANELCHOOSERWIDGET_H

#include "ToolButtonWidget.h"

namespace Meerkat
{

class PanelChooserWidget : public ToolButtonWidget
{
	Q_OBJECT

public:
	explicit PanelChooserWidget(const ActionsManager::ActionEntryDefinition &definition, QWidget *parent);

	void changeEvent(QEvent *event);
	QSize minimumSizeHint() const;

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void menuAboutToShow();
	void selectPanel(QAction *action);
};

}

#endif
