/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_ACCEPTLANGUAGEDIALOG_H
#define MEERKAT_ACCEPTLANGUAGEDIALOG_H

#include "../Dialog.h"

#include <QtGui/QStandardItemModel>

namespace Meerkat
{

namespace Ui
{
	class AcceptLanguageDialog;
}

class AcceptLanguageDialog : public Dialog
{
	Q_OBJECT

public:
	explicit AcceptLanguageDialog(const QString &languages, QWidget *parent = nullptr);
	~AcceptLanguageDialog();

	QString getLanguages();
	bool eventFilter(QObject *object, QEvent *event);

protected:
	void changeEvent(QEvent *event);
	void addLanguage(const QString &languageCode);

protected slots:
	void addLanguage();
	void updateActions();

private:
	QStandardItemModel *m_model;
	Ui::AcceptLanguageDialog *m_ui;
};


}
#endif
