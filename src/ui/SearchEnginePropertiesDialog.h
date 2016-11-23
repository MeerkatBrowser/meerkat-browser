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

#ifndef MEERKAT_SEARCHENGINEPROPERTIESDIALOG_H
#define MEERKAT_SEARCHENGINEPROPERTIESDIALOG_H

#include "Dialog.h"
#include "../core/SearchEnginesManager.h"

#include <QtWidgets/QLineEdit>

namespace Meerkat
{

namespace Ui
{
	class SearchEnginePropertiesDialog;
}

class SearchEnginePropertiesDialog : public Dialog
{
	Q_OBJECT

public:
	explicit SearchEnginePropertiesDialog(const SearchEnginesManager::SearchEngineDefinition &searchEngine, const QStringList &keywords, bool isDefault, QWidget *parent = nullptr);
	~SearchEnginePropertiesDialog();

	SearchEnginesManager::SearchEngineDefinition getSearchEngine() const;
	bool isDefault() const;
	bool eventFilter(QObject *object, QEvent *event);

protected:
	void changeEvent(QEvent *event);

protected slots:
	void insertPlaceholder(QAction *action);

private:
	QLineEdit *m_currentLineEdit;
	QString m_identifier;
	QStringList m_keywords;
	Ui::SearchEnginePropertiesDialog *m_ui;
};

}

#endif
