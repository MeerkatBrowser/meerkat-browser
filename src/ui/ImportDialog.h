/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_IMPORTDIALOG_H
#define MEERKAT_IMPORTDIALOG_H

#include "Dialog.h"
#include "../core/Importer.h"

namespace Meerkat
{

namespace Ui
{
	class ImportDialog;
}

class ImportDialog : public Dialog
{
	Q_OBJECT

public:
	~ImportDialog();

	static void createDialog(const QString &importerName, QWidget *parent = NULL);

protected:
	explicit ImportDialog(Importer *importer, QWidget *parent);

	void changeEvent(QEvent *event);

protected slots:
	void import();
	void setPath(const QString &path);

private:
	Importer *m_importer;
	QString m_path;
	Ui::ImportDialog *m_ui;
};

}

#endif
