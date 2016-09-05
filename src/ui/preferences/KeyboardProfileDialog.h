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

#ifndef MEERKAT_KEYBOARDPROFILEDIALOG_H
#define MEERKAT_KEYBOARDPROFILEDIALOG_H

#include "../Dialog.h"

#include <QtCore/QModelIndex>

namespace Meerkat
{

namespace Ui
{
	class KeyboardProfileDialog;
}

struct KeyboardProfile
{
	QString identifier;
	QString title;
	QString description;
	QString author;
	QString version;
	QHash<int, QVector<QKeySequence> > shortcuts;
	bool isModified;

	KeyboardProfile() : isModified(false) {}
};

class KeyboardProfileDialog : public Dialog
{
	Q_OBJECT

public:
	explicit KeyboardProfileDialog(const QString &profile, const QHash<QString, KeyboardProfile> &profiles, QWidget *parent = NULL);
	~KeyboardProfileDialog();

	KeyboardProfile getProfile() const;

protected:
	void changeEvent(QEvent *event);

protected slots:
	void addShortcut();
	void removeShortcut();
	void updateActionsActions();
	void updateShortcutsActions();
	void saveShortcuts();

private:
	QString m_profile;
	QModelIndex m_currentAction;
	bool m_isModified;
	Ui::KeyboardProfileDialog *m_ui;
};

}

#endif
