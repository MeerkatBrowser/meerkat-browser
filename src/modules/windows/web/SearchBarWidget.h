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

#ifndef MEERKAT_SEARCHBARWIDGET_H
#define MEERKAT_SEARCHBARWIDGET_H

#include "../../../ui/WebWidget.h"

namespace Meerkat
{

namespace Ui
{
	class SearchBarWidget;
}

class SearchBarWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SearchBarWidget(QWidget *parent = NULL);
	~SearchBarWidget();

	void selectAll();
	void setQuery(const QString &query);
	void setVisible(bool visible);
	QString getQuery() const;
	WebWidget::FindFlags getFlags() const;

public slots:
	void setResultsFound(bool found);

protected:
	void changeEvent(QEvent *event);
	void keyPressEvent(QKeyEvent *event);

protected slots:
	void notifyRequestedSearch();
	void notifyFlagsChanged();

private:
	Ui::SearchBarWidget *m_ui;

signals:
	void requestedSearch(WebWidget::FindFlags flags);
	void flagsChanged(WebWidget::FindFlags flags);
	void queryChanged();
};

}

#endif
