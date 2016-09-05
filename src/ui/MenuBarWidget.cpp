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

#include "MenuBarWidget.h"
#include "MainWindow.h"
#include "WorkspaceWidget.h"
#include "Menu.h"
#include "ToolBarWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SessionsManager.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionMenuItem>

namespace Meerkat
{

MenuBarWidget::MenuBarWidget(MainWindow *parent) : QMenuBar(parent),
	m_mainWindow(parent),
	m_leftToolBar(NULL),
	m_rightToolBar(NULL)
{
	QFile file(SessionsManager::getReadableDataPath(QLatin1String("menu/menuBar.json")));
	file.open(QIODevice::ReadOnly);

	const QJsonArray definition(QJsonDocument::fromJson(file.readAll()).array());

	file.close();

	for (int i = 0; i < definition.count(); ++i)
	{
		const QJsonObject object(definition.at(i).toObject());
		Menu *menu(new Menu(Menu::getRole(object.value(QLatin1String("identifier")).toString()), this));
		menu->load(object);

		addMenu(menu);
	}

	if (!isNativeMenuBar())
	{
		setup();

		connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(toolBarModified(int)));
	}
}

void MenuBarWidget::changeEvent(QEvent *event)
{
	QMenuBar::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		QTimer::singleShot(100, this, SLOT(updateSize()));
	}
}

void MenuBarWidget::resizeEvent(QResizeEvent *event)
{
	QMenuBar::resizeEvent(event);

	updateSize();
}

void MenuBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu(ToolBarWidget::createCustomizationMenu(ToolBarsManager::MenuBar));
	menu->exec(event->globalPos());
	menu->deleteLater();
}

void MenuBarWidget::setup()
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar));
	QStringList actions;

	for (int i = 0; i < definition.entries.count(); ++i)
	{
		actions.append(definition.entries.at(i).action);
	}

	if (actions.count() == 1 && actions.at(0) == QLatin1String("MenuBarWidget"))
	{
		if (m_leftToolBar)
		{
			m_leftToolBar->deleteLater();
			m_leftToolBar = NULL;

			setCornerWidget(NULL, Qt::TopLeftCorner);
		}

		if (m_rightToolBar)
		{
			m_rightToolBar->deleteLater();
			m_rightToolBar = NULL;

			setCornerWidget(NULL, Qt::TopRightCorner);
		}

		return;
	}

	const int position(actions.indexOf(QLatin1String("MenuBarWidget")));
	const bool needsLeftToolbar(position != 0);
	const bool needsRightToolbar(position != (definition.entries.count() - 1));

	if (needsLeftToolbar && !m_leftToolBar)
	{
		m_leftToolBar = new ToolBarWidget(-1, m_mainWindow->getWorkspace()->getActiveWindow(), this);

		setCornerWidget(m_leftToolBar, Qt::TopLeftCorner);
	}
	else if (!needsLeftToolbar && m_leftToolBar)
	{
		m_leftToolBar->deleteLater();
		m_leftToolBar = NULL;

		setCornerWidget(NULL, Qt::TopLeftCorner);
	}

	if (needsRightToolbar && !m_rightToolBar)
	{
		m_rightToolBar = new ToolBarWidget(-1, m_mainWindow->getWorkspace()->getActiveWindow(), this);

		setCornerWidget(m_rightToolBar, Qt::TopRightCorner);
	}
	else if (!needsRightToolbar && m_rightToolBar)
	{
		m_rightToolBar->deleteLater();
		m_rightToolBar = NULL;

		setCornerWidget(NULL, Qt::TopRightCorner);
	}

	ToolBarsManager::ToolBarDefinition leftDefinition(definition);
	leftDefinition.entries.clear();

	ToolBarsManager::ToolBarDefinition rightDefinition(definition);
	rightDefinition.entries.clear();

	for (int i = 0; i < definition.entries.count(); ++i)
	{
		if (i != position)
		{
			if (i < position)
			{
				leftDefinition.entries.append(definition.entries.at(i));
			}
			else
			{
				rightDefinition.entries.append(definition.entries.at(i));
			}
		}
	}

	if (m_leftToolBar)
	{
		m_leftToolBar->setDefinition(leftDefinition);
	}

	if (m_rightToolBar)
	{
		m_rightToolBar->setDefinition(rightDefinition);
	}

	const int menuBarHeight(actionGeometry(this->actions().at(0)).height());
	const int toolBarHeight((m_leftToolBar || m_rightToolBar) ? (definition.iconSize + 12) : 0);

	setFixedHeight((toolBarHeight > 0 && toolBarHeight > menuBarHeight) ? toolBarHeight : menuBarHeight);

	QTimer::singleShot(100, this, SLOT(updateSize()));
}

void MenuBarWidget::toolBarModified(int identifier)
{
	if (identifier == ToolBarsManager::MenuBar)
	{
		setup();
	}
}

void MenuBarWidget::updateSize()
{
	if (!m_leftToolBar && !m_rightToolBar)
	{
		return;
	}

	int size(0);

	if (actions().count() > 0)
	{
		size = ((style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, this) * 2) + (style()->pixelMetric(QStyle::PM_MenuBarItemSpacing, 0, this) * actions().count()));

		for (int i = 0; i < actions().count(); ++i)
		{
			QStyleOptionMenuItem option;

			initStyleOption(&option, actions().at(i));

			size += style()->sizeFromContents(QStyle::CT_MenuBarItem, &option, fontMetrics().size(Qt::TextShowMnemonic, option.text), this).width();
		}
	}

	if (m_rightToolBar && width() > (size + (m_leftToolBar ? m_leftToolBar->sizeHint().width() : 0) + (m_rightToolBar ? m_rightToolBar->sizeHint().width() : 0)))
	{
		const int offset(size - (m_leftToolBar ? m_leftToolBar->sizeHint().width() : 0));
		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(m_rightToolBar));
		toolBar->move(QPoint(offset, 0));
		toolBar->resize((width() - offset), toolBar->height());
	}
}

}
