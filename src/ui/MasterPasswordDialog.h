/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef MEERKAT_MASTERPASSWORDDIALOG_H
#define MEERKAT_MASTERPASSWORDDIALOG_H

#include "Dialog.h"

namespace Meerkat
{

namespace Ui
{
	class MasterPasswordDialog;
}

class MasterPasswordDialog : public Dialog
{
	Q_OBJECT

public:
	explicit MasterPasswordDialog(QWidget *parent = nullptr);
	~MasterPasswordDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::MasterPasswordDialog *m_ui;
};

}

#endif
