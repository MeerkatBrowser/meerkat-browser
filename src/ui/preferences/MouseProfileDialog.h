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

#ifndef MEERKAT_MOUSEPROFILEDIALOG_H
#define MEERKAT_MOUSEPROFILEDIALOG_H

#include "../Dialog.h"
#include "../ItemDelegate.h"

namespace Meerkat
{

namespace Ui
{
	class MouseProfileDialog;
}

struct MouseProfile
{
	QString identifier;
	QString title;
	QString description;
	QString author;
	QString version;
	QHash<QString, QHash<QString, int> > gestures;
	bool isModified = false;
};

class GestureActionDelegate : public ItemDelegate
{
public:
	explicit GestureActionDelegate(QObject *parent);

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class MouseProfileDialog : public Dialog
{
	Q_OBJECT

public:
	explicit MouseProfileDialog(const QString &profile, const QHash<QString, MouseProfile> &profiles, QWidget *parent = nullptr);
	~MouseProfileDialog();

	MouseProfile getProfile() const;

protected:
	void changeEvent(QEvent *event);

protected slots:
	void addGesture();
	void removeGesture();
	void saveGesture();
	void addStep();
	void removeStep();
	void updateGesturesActions();
	void updateStepsActions();

private:
	QString m_profile;
	bool m_isModified;
	Ui::MouseProfileDialog *m_ui;
};

}

#endif
