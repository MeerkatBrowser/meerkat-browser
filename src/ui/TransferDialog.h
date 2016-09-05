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

#ifndef MEERKAT_TRANSFERDIALOG_H
#define MEERKAT_TRANSFERDIALOG_H

#include "Dialog.h"

#include <QtCore/QPointer>
#include <QtWidgets/QPushButton>

namespace Meerkat
{

namespace Ui
{
	class TransferDialog;
}

class Transfer;

class TransferDialog : public Dialog
{
	Q_OBJECT

public:
	explicit TransferDialog(Transfer *transfer, QWidget *parent = NULL);
	~TransferDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void buttonClicked(QAbstractButton *button);
	void setProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
	QPointer<Transfer> m_transfer;
	Ui::TransferDialog *m_ui;
};

}

#endif
