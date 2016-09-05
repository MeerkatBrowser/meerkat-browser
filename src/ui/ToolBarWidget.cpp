/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "ToolBarWidget.h"
#include "ContentsWidget.h"
#include "MainWindow.h"
#include "Menu.h"
#include "TabBarWidget.h"
#include "ToolBarAreaWidget.h"
#include "WidgetFactory.h"
#include "Window.h"
#include "toolbars/BookmarkWidget.h"
#include "../core/BookmarksManager.h"
#include "../core/GesturesManager.h"
#include "../core/ThemesManager.h"
#include "../core/WindowsManager.h"

#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

namespace Meerkat
{

ToolBarDragAreaWidget::ToolBarDragAreaWidget(ToolBarWidget *parent) : QWidget(parent),
	m_toolBar(parent)
{
	setToolTip(tr("Drag to move toolbar"));
	setCursor(QCursor(Qt::SizeAllCursor));

	if (m_toolBar->orientation() == Qt::Horizontal)
	{
		setFixedWidth(style()->pixelMetric(QStyle::PM_ToolBarHandleExtent));
	}
	else
	{
		setFixedHeight(style()->pixelMetric(QStyle::PM_ToolBarHandleExtent));
	}
}

void ToolBarDragAreaWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOption option;
	option.palette = palette();
	option.rect = QRect(QPoint(0, 0), size());

	if (m_toolBar->orientation() == Qt::Horizontal)
	{
		option.state = QStyle::State_Horizontal;
	}

	style()->drawPrimitive(QStyle::PE_IndicatorToolBarHandle, &option, &painter, this);
}

void ToolBarDragAreaWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);

	if (event->button() == Qt::LeftButton)
	{
		m_dragStartPosition = event->pos();
	}
}

void ToolBarDragAreaWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (!event->buttons().testFlag(Qt::LeftButton) || (event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance() || !m_toolBar->isMovable())
	{
		return;
	}

	m_toolBar->startToolBarDragging();

	QMimeData *mimeData(new QMimeData());
	mimeData->setData(QLatin1String("x-toolbar-identifier"), QString::number(m_toolBar->getIdentifier()).toUtf8());

	QDrag *drag(new QDrag(this));
	drag->setMimeData(mimeData);
	drag->exec(Qt::MoveAction);
}

void ToolBarDragAreaWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);

	m_toolBar->endToolBarDragging();
}

ToolBarWidget::ToolBarWidget(int identifier, Window *window, QWidget *parent) : QToolBar(parent),
	m_mainWindow(MainWindow::findMainWindow(parent)),
	m_window(window),
	m_bookmark(NULL),
	m_dragArea(NULL),
	m_identifier(identifier)
{
	setStyleSheet(QLatin1String("QToolBar {padding:0 3px;spacing:3px;}"));
	setAllowedAreas(Qt::NoToolBarArea);
	setFloatable(false);

	layout()->setContentsMargins(0, 0, 0, 0);

	if (identifier >= 0)
	{
		setToolBarLocked(ToolBarsManager::areToolBarsLocked());
		reload();

		connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(toolBarModified(int)));
		connect(ToolBarsManager::getInstance(), SIGNAL(toolBarRemoved(int)), this, SLOT(toolBarRemoved(int)));
		connect(ToolBarsManager::getInstance(), SIGNAL(toolBarsLockedChanged(bool)), this, SLOT(setToolBarLocked(bool)));
	}

	if (m_mainWindow && m_identifier != ToolBarsManager::NavigationBar && m_identifier != ToolBarsManager::ProgressBar)
	{
		connect(m_mainWindow->getWindowsManager(), SIGNAL(currentWindowChanged(quint64)), this, SLOT(notifyWindowChanged(quint64)));
	}
}

void ToolBarWidget::paintEvent(QPaintEvent *event)
{
	if (m_identifier >= 0 && m_identifier != ToolBarsManager::StatusBar)
	{
		QToolBar::paintEvent(event);
	}
}

void ToolBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_identifier < 0)
	{
		event->ignore();

		return;
	}

	if (event->reason() == QContextMenuEvent::Mouse)
	{
		event->accept();

		return;
	}

	if (m_identifier != ToolBarsManager::TabBar)
	{
		QMenu *menu(createCustomizationMenu(m_identifier));
		menu->exec(event->globalPos());
		menu->deleteLater();

		return;
	}

	QList<QAction*> actions;
	QAction *cycleAction(new QAction(tr("Switch tabs using the mouse wheel"), this));
	cycleAction->setCheckable(true);
	cycleAction->setChecked(!SettingsManager::getValue(SettingsManager::TabBar_RequireModifierToSwitchTabOnScrollOption).toBool());
	cycleAction->setEnabled(m_mainWindow->getTabBar());

	actions.append(cycleAction);

	if (m_mainWindow->getTabBar())
	{
		connect(cycleAction, SIGNAL(toggled(bool)), m_mainWindow->getTabBar(), SLOT(setCycle(bool)));
	}

	QMenu menu(this);
	menu.addAction(ActionsManager::getAction(ActionsManager::NewTabAction, this));
	menu.addAction(ActionsManager::getAction(ActionsManager::NewTabPrivateAction, this));
	menu.addSeparator();

	QMenu *arrangeMenu(menu.addMenu(tr("Arrange")));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::RestoreTabAction, this));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::RestoreAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MaximizeAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MinimizeAllAction, this));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::CascadeAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::TileAllAction, this));

	menu.addMenu(createCustomizationMenu(m_identifier, actions, &menu));
	menu.exec(event->globalPos());

	cycleAction->deleteLater();
}

void ToolBarWidget::startToolBarDragging()
{
	ToolBarAreaWidget *toolBarArea(qobject_cast<ToolBarAreaWidget*>(parentWidget()));

	if (toolBarArea)
	{
		toolBarArea->startToolBarDragging();
	}
}

void ToolBarWidget::endToolBarDragging()
{
	ToolBarAreaWidget *toolBarArea(qobject_cast<ToolBarAreaWidget*>(parentWidget()));

	if (toolBarArea)
	{
		toolBarArea->endToolBarDragging();
	}
}

void ToolBarWidget::reload()
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(m_identifier));

	setDefinition(definition);

	emit areaChanged(definition.location);
}

void ToolBarWidget::toolBarModified(int identifier)
{
	if (identifier == m_identifier)
	{
		reload();
	}
}

void ToolBarWidget::toolBarRemoved(int identifier)
{
	if (identifier == m_identifier)
	{
		deleteLater();
	}
}

void ToolBarWidget::bookmarkAdded(BookmarksItem *bookmark)
{
	if (bookmark->parent() == m_bookmark)
	{
		loadBookmarks();
	}
}

void ToolBarWidget::bookmarkMoved(BookmarksItem *bookmark, BookmarksItem *previousParent)
{
	if (bookmark->parent() == m_bookmark || previousParent == m_bookmark)
	{
		loadBookmarks();
	}
}

void ToolBarWidget::bookmarkRemoved(BookmarksItem *bookmark)
{
	if (bookmark == m_bookmark)
	{
		m_bookmark = NULL;

		loadBookmarks();
	}
	else if (bookmark->parent() == m_bookmark)
	{
		loadBookmarks();
	}
}

void ToolBarWidget::bookmarkTrashed(BookmarksItem *bookmark)
{
	if (bookmark->parent() == m_bookmark)
	{
		loadBookmarks();
	}
}

void ToolBarWidget::loadBookmarks()
{
	clear();

	m_dragArea = NULL;

	if (!ToolBarsManager::areToolBarsLocked() && qobject_cast<ToolBarAreaWidget*>(parentWidget()))
	{
		m_dragArea = new ToolBarDragAreaWidget(this);

		addWidget(m_dragArea);
	}

	if (!m_bookmark)
	{
		return;
	}

	for (int i = 0; i < m_bookmark->rowCount(); ++i)
	{
		BookmarksItem *bookmark(dynamic_cast<BookmarksItem*>(m_bookmark->child(i)));

		if (bookmark)
		{
			if (static_cast<BookmarksModel::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::SeparatorBookmark)
			{
				addSeparator();
			}
			else
			{
				addWidget(new BookmarkWidget(bookmark, ActionsManager::ActionEntryDefinition(), this));
			}
		}
	}
}

void ToolBarWidget::notifyWindowChanged(quint64 identifier)
{
	m_window = m_mainWindow->getWindowsManager()->getWindowByIdentifier(identifier);

	emit windowChanged(m_window);
}

void ToolBarWidget::updateVisibility()
{
	if (m_identifier == ToolBarsManager::TabBar && ToolBarsManager::getToolBarDefinition(ToolBarsManager::TabBar).normalVisibility == ToolBarsManager::AutoVisibilityToolBar && m_mainWindow->getTabBar())
	{
		setVisible(m_mainWindow->getTabBar()->count() > 1);
	}
}

void ToolBarWidget::setDefinition(const ToolBarsManager::ToolBarDefinition &definition)
{
	TabBarWidget *tabBar((m_identifier == ToolBarsManager::TabBar && m_mainWindow) ? m_mainWindow->getTabBar() : NULL);

	setVisible(definition.normalVisibility != ToolBarsManager::AlwaysHiddenToolBar);
	setOrientation((definition.location == Qt::LeftToolBarArea || definition.location == Qt::RightToolBarArea) ? Qt::Vertical : Qt::Horizontal);

	m_dragArea = NULL;

	if (m_identifier == ToolBarsManager::TabBar)
	{
		for (int i = (actions().count() - 1); i >= 0; --i)
		{
			if (widgetForAction(actions().at(i)) != tabBar)
			{
				removeAction(actions().at(i));
			}
		}
	}
	else
	{
		clear();
	}

	setToolButtonStyle(definition.buttonStyle);

	if (definition.iconSize > 0)
	{
		setIconSize(QSize(definition.iconSize, definition.iconSize));
	}

	emit buttonStyleChanged(definition.buttonStyle);
	emit iconSizeChanged(definition.iconSize);

	if (!definition.bookmarksPath.isEmpty())
	{
		m_bookmark = (definition.bookmarksPath.startsWith(QLatin1Char('#')) ? BookmarksManager::getBookmark(definition.bookmarksPath.mid(1).toULongLong()) : BookmarksManager::getModel()->getItem(definition.bookmarksPath));

		loadBookmarks();

		connect(BookmarksManager::getModel(), SIGNAL(bookmarkAdded(BookmarksItem*)), this, SLOT(bookmarkAdded(BookmarksItem*)));
		connect(BookmarksManager::getModel(), SIGNAL(bookmarkMoved(BookmarksItem*,BookmarksItem*,int)), this, SLOT(bookmarkMoved(BookmarksItem*,BookmarksItem*)));
		connect(BookmarksManager::getModel(), SIGNAL(bookmarkTrashed(BookmarksItem*)), this, SLOT(bookmarkTrashed(BookmarksItem*)));
		connect(BookmarksManager::getModel(), SIGNAL(bookmarkRestored(BookmarksItem*)), this, SLOT(bookmarkTrashed(BookmarksItem*)));
		connect(BookmarksManager::getModel(), SIGNAL(bookmarkRemoved(BookmarksItem*)), this, SLOT(bookmarkRemoved(BookmarksItem*)));

		return;
	}

	if (!ToolBarsManager::areToolBarsLocked() && qobject_cast<ToolBarAreaWidget*>(parentWidget()))
	{
		m_dragArea = new ToolBarDragAreaWidget(this);

		addWidget(m_dragArea);
	}

	for (int i = 0; i < definition.entries.count(); ++i)
	{
		if (definition.entries.at(i).action == QLatin1String("separator"))
		{
			addSeparator();
		}
		else
		{
			if (m_identifier == ToolBarsManager::TabBar && tabBar && definition.entries.at(i).action == QLatin1String("TabBarWidget"))
			{
				addWidget(tabBar);
			}
			else
			{
				const bool isTabBar(definition.entries.at(i).action == QLatin1String("TabBarWidget"));

				if (isTabBar && (m_identifier != ToolBarsManager::TabBar || !m_mainWindow || m_mainWindow->getTabBar()))
				{
					continue;
				}

				QWidget *widget(WidgetFactory::createToolBarItem(definition.entries.at(i), this, m_window));

				if (widget)
				{
					addWidget(widget);

					if (isTabBar)
					{
						connect(widget, SIGNAL(tabsAmountChanged(int)), this, SLOT(updateVisibility()));

						updateVisibility();
					}
				}
			}
		}
	}
}

void ToolBarWidget::setToolBarLocked(bool locked)
{
	setMovable(!locked);

	if (locked && m_dragArea)
	{
		m_dragArea->deleteLater();
		m_dragArea = NULL;
	}
	else if (!locked && !m_dragArea && qobject_cast<ToolBarAreaWidget*>(parentWidget()))
	{
		m_dragArea = new ToolBarDragAreaWidget(this);

		insertWidget(((actions().count() > 0) ? actions().at(0) : NULL), m_dragArea);
	}
}

QMenu* ToolBarWidget::createCustomizationMenu(int identifier, QList<QAction*> actions, QWidget *parent)
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifier));
	QMenu *menu(new QMenu(parent));
	menu->setTitle(tr("Customize"));

	QMenu *toolBarMenu(menu->addMenu(definition.title.isEmpty() ? tr("(Untitled)") : definition.title));
	toolBarMenu->addAction(tr("Configure…"), ToolBarsManager::getInstance(), SLOT(configureToolBar()))->setData(identifier);

	QAction *resetAction(toolBarMenu->addAction(tr("Reset to Defaults…"), ToolBarsManager::getInstance(), SLOT(resetToolBar())));
	resetAction->setData(identifier);
	resetAction->setEnabled(definition.canReset);

	if (!actions.isEmpty())
	{
		toolBarMenu->addSeparator();
		toolBarMenu->addActions(actions);

		for (int i = 0; i < actions.count(); ++i)
		{
			actions.at(i)->setParent(toolBarMenu);
		}
	}

	toolBarMenu->addSeparator();

	QAction *removeAction(toolBarMenu->addAction(ThemesManager::getIcon(QLatin1String("list-remove")), tr("Remove…"), ToolBarsManager::getInstance(), SLOT(removeToolBar())));
	removeAction->setData(identifier);
	removeAction->setEnabled(definition.identifier >= ToolBarsManager::OtherToolBar);

	menu->addMenu(new Menu(Menu::ToolBarsMenuRole, menu))->setText(tr("Toolbars"));

	return menu;
}

Qt::ToolBarArea ToolBarWidget::getArea() const
{
	ToolBarAreaWidget *toolBarArea(qobject_cast<ToolBarAreaWidget*>(parentWidget()));

	if (toolBarArea)
	{
		return toolBarArea->getArea();
	}

	return Qt::NoToolBarArea;
}

Qt::ToolButtonStyle ToolBarWidget::getButtonStyle() const
{
	return ToolBarsManager::getToolBarDefinition(m_identifier).buttonStyle;
}

int ToolBarWidget::getIdentifier() const
{
	return m_identifier;
}

int ToolBarWidget::getIconSize() const
{
	return ToolBarsManager::getToolBarDefinition(m_identifier).iconSize;
}

int ToolBarWidget::getMaximumButtonSize() const
{
	return ToolBarsManager::getToolBarDefinition(m_identifier).maximumButtonSize;
}

bool ToolBarWidget::event(QEvent *event)
{
	if (event->type() == QEvent::LayoutRequest)
	{
		const bool result(QToolBar::event(event));

		layout()->setContentsMargins(0, 0, 0, 0);

		return result;
	}

	if (!GesturesManager::isTracking() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::Wheel))
	{
		QList<GesturesManager::GesturesContext> contexts;

		if (m_identifier == ToolBarsManager::TabBar)
		{
			contexts.append(GesturesManager::NoTabHandleGesturesContext);
		}

		contexts.append(GesturesManager::ToolBarGesturesContext);
		contexts.append(GesturesManager::GenericGesturesContext);

		GesturesManager::startGesture(this, event, contexts);
	}

	return QToolBar::event(event);
}

}
