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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef MEERKAT_ADDONSCONTENTSWIDGET_H
#define MEERKAT_ADDONSCONTENTSWIDGET_H

#include "../../../core/AddonsManager.h"
#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Meerkat
{

namespace Ui
{
	class AddonsContentsWidget;
}

class Window;

class AddonsContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		TypeRole = Qt::UserRole,
		NameRole = (Qt::UserRole + 1)
	};

	enum ReplaceMode
	{
		UnknownMode = 0,
		ReplaceAllMode,
		IgnoreAllMode
	};

	explicit AddonsContentsWidget(Window *window);
	~AddonsContentsWidget();

	void print(QPrinter *printer);
	Action* getAction(int identifier);
	QString getTitle() const;
	QLatin1String getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	WindowsManager::LoadingState getLoadingState() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());

protected:
	void changeEvent(QEvent *event);

protected slots:
	void populateAddons();
	void filterAddons(const QString &filter);
	void addAddon();
	void addAddon(Addon *addon);
	void reloadAddon();
	void removeAddons();
	void save();
	void showContextMenu(const QPoint &point);

private:
	QStandardItemModel *m_model;
	QHash<Addon::AddonType, int> m_types;
	QHash<int, Action*> m_actions;
	bool m_isLoading;
	Ui::AddonsContentsWidget *m_ui;
};

}

#endif
