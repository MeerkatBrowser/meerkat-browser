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

#include "Menu.h"
#include "ContentsWidget.h"
#include "ImportDialog.h"
#include "MainWindow.h"
#include "Window.h"
#include "../core/ActionsManager.h"
#include "../core/BookmarksManager.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/NotesManager.h"
#include "../core/SessionsManager.h"
#include "../core/ThemesManager.h"
#include "../core/ToolBarsManager.h"
#include "../core/Utils.h"
#include "../core/WindowsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTextCodec>
#include <QtGui/QMouseEvent>

namespace Meerkat
{

Menu::Menu(MenuRole role, QWidget *parent) : QMenu(parent),
	m_actionGroup(NULL),
	m_bookmark(NULL),
	m_role(role)
{
	switch (role)
	{
		case BookmarksMenuRole:
		case BookmarkSelectorMenuRole:
		case NotesMenuRole:
			{
				installEventFilter(this);

				Menu *parentMenu(qobject_cast<Menu*>(parent));

				if (!parentMenu || parentMenu->getRole() != m_role)
				{
					if (m_role == NotesMenuRole)
					{
						connect(NotesManager::getModel(), SIGNAL(modelModified()), this, SLOT(clearModelMenu()));
					}
					else
					{
						connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(clearModelMenu()));
					}
				}

				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateModelMenu()));
			}

			break;
		case CharacterEncodingMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateCharacterEncodingMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectCharacterEncoding(QAction*)));

			break;
		case ClosedWindowsMenu:
			{
				setIcon(ThemesManager::getIcon(QLatin1String("user-trash")));

				MainWindow *mainWindow(MainWindow::findMainWindow(parent));

				if (mainWindow)
				{
					setEnabled(!SessionsManager::getClosedWindows().isEmpty() || !mainWindow->getWindowsManager()->getClosedWindows().isEmpty());

					connect(mainWindow->getWindowsManager(), SIGNAL(closedWindowsAvailableChanged(bool)), this, SLOT(updateClosedWindowsMenu()));
				}

				connect(SessionsManager::getInstance(), SIGNAL(closedWindowsChanged()), this, SLOT(updateClosedWindowsMenu()));
				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateClosedWindowsMenu()));
			}

			break;
		case ImportExportMenuRole:
			{
				Action *importOperaBookmarksAction(addAction());
				importOperaBookmarksAction->setData(QLatin1String("OperaBookmarks"));
				importOperaBookmarksAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Bookmarks…"));

				Action *importHtmlBookmarksAction(addAction());
				importHtmlBookmarksAction->setData(QLatin1String("HtmlBookmarks"));
				importHtmlBookmarksAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import HTML Bookmarks…"));

				addSeparator();

				Action *importOperaNotesAction(addAction());
				importOperaNotesAction->setData(QLatin1String("OperaNotes"));
				importOperaNotesAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Notes…"));

				addSeparator();

				Action *importOperaSearchEnginesAction(addAction());
				importOperaSearchEnginesAction->setData(QLatin1String("OperaSearchEngines"));
				importOperaSearchEnginesAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Search Engines…"));

				addSeparator();

				Action *importOperaSessionAction(addAction());
				importOperaSessionAction->setData(QLatin1String("OperaSession"));
				importOperaSessionAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Session…"));

				connect(this, SIGNAL(triggered(QAction*)), this, SLOT(openImporter(QAction*)));
			}

			break;
		case SessionsMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateSessionsMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(openSession(QAction*)));

			break;
		case StyleSheetsMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateStyleSheetsMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectStyleSheet(QAction*)));

			break;
		case ToolBarsMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateToolBarsMenu()));

			break;
		case UserAgentMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateUserAgentMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectUserAgent(QAction*)));

			break;
		case WindowsMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateWindowsMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectWindow(QAction*)));

			break;
		default:
			break;
	}
}

void Menu::changeEvent(QEvent *event)
{
	QMenu::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && !m_title.isEmpty())
	{
		setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));
	}
}

void Menu::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_role == BookmarksMenuRole && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton))
	{
		QAction *action(actionAt(event->pos()));

		if (action && action->isEnabled() && action->data().type() == QVariant::ModelIndex)
		{
			QWidget *menu(this);

			while (menu)
			{
				menu->close();
				menu = menu->parentWidget();

				if (!menu || !menu->inherits(QLatin1String("QMenu").data()))
				{
					break;
				}
			}

			MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

			if (mainWindow)
			{
				mainWindow->getWindowsManager()->open(BookmarksManager::getModel()->getBookmark(action->data().toModelIndex()), WindowsManager::calculateOpenHints(WindowsManager::DefaultOpen, event->button(), event->modifiers()));

				return;
			}
		}
	}

	QMenu::mouseReleaseEvent(event);
}

void Menu::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_role == BookmarksMenuRole)
	{
		QAction *action(actionAt(event->pos()));

		if (action && action->isEnabled() && action->data().type() == QVariant::ModelIndex)
		{
			m_bookmark = BookmarksManager::getModel()->getBookmark(action->data().toModelIndex());

			QMenu contextMenu(this);
			contextMenu.addAction(ThemesManager::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
			contextMenu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(WindowsManager::NewTabOpen);
			contextMenu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(static_cast<int>(WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
			contextMenu.addSeparator();
			contextMenu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(WindowsManager::NewWindowOpen);
			contextMenu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(static_cast<int>(WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen));
			contextMenu.exec(event->globalPos());

			return;
		}
	}

	QMenu::contextMenuEvent(event);
}

void Menu::load(const QString &path, const QStringList &options)
{
	QFile file(SessionsManager::getReadableDataPath(path));
	file.open(QIODevice::ReadOnly);

	load(QJsonDocument::fromJson(file.readAll()).object(), options);

	file.close();
}

void Menu::load(const QJsonObject &definition, const QStringList &options)
{
	const QString identifier(definition.value(QLatin1String("identifier")).toString());

	if (m_role == NoMenuRole)
	{
		clear();
	}

	m_title = definition.value(QLatin1String("title")).toString();

	setObjectName(identifier);
	setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));

	const QJsonArray actions(definition.value(QLatin1String("actions")).toArray());

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i).isObject())
		{
			const QJsonObject object(actions.at(i).toObject());

			if (object.contains(QLatin1String("includeIn")) && !options.contains(object.value(QLatin1String("includeIn")).toString()))
			{
				continue;
			}

			Menu *menu(new Menu(Menu::getRole(object.value(QLatin1String("identifier")).toString()), this));
			menu->load(object, options);

			if (object.value(QLatin1String("type")).toString() == QLatin1String("menu"))
			{
				addMenu(menu);
			}
			else if (object.value(QLatin1String("type")).toString() == QLatin1String("include"))
			{
				for (int j = 0; j < menu->actions().count(); ++j)
				{
					QMenu::addAction(menu->actions().at(j));
				}
			}
		}
		else
		{
			const QString rawAction(actions.at(i).toString());

			if (rawAction == QLatin1String("separator"))
			{
				addSeparator();
			}
			else
			{
				const int action(ActionsManager::getActionIdentifier(rawAction));

				if (action >= 0)
				{
					WindowsManager *manager(SessionsManager::getWindowsManager());

					if (manager && Action::isLocal(action) && manager->getAction(action))
					{
						QMenu::addAction(manager->getAction(action));
					}
					else
					{
						addAction(action, true);
					}
				}
			}
		}
	}
}

void Menu::populateModelMenu()
{
	Menu *menu(qobject_cast<Menu*>(sender()));

	if (!menu || !menu->menuAction())
	{
		return;
	}

	QModelIndex index(menu->menuAction()->data().toModelIndex());

	if (!menu->actions().isEmpty() && !(m_role == BookmarksMenuRole && !index.isValid() && menu->actions().count() == 3))
	{
		return;
	}

	if (!index.isValid())
	{
		index = ((m_role == NotesMenuRole) ? NotesManager::getModel() : BookmarksManager::getModel())->getRootItem()->index();
	}

	const QAbstractItemModel *model(index.model());

	if (!model)
	{
		return;
	}

	if (model->rowCount(index) > 1 && m_role == BookmarksMenuRole)
	{
		Action *openAllAction(menu->addAction());
		openAllAction->setData(index);
		openAllAction->setIcon(ThemesManager::getIcon(QLatin1String("document-open-folder")));
		openAllAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Open All"));

		menu->addSeparator();

		connect(openAllAction, SIGNAL(triggered()), this, SLOT(openBookmark()));
	}

	if (m_role == BookmarkSelectorMenuRole)
	{
		Action *addFolderAction(menu->addAction());
		addFolderAction->setData(index);
		addFolderAction->setIcon(ThemesManager::getIcon(QLatin1String("document-open-folder")));
		addFolderAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "This Folder"));

		menu->addSeparator();
	}

	for (int i = 0; i < model->rowCount(index); ++i)
	{
		const QModelIndex childIndex(index.child(i, 0));

		if (!childIndex.isValid())
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(childIndex.data(BookmarksModel::TypeRole).toInt()));

		if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark)
		{
			Action *action(menu->addAction());
			action->setData(childIndex);
			action->setIcon(childIndex.data(Qt::DecorationRole).value<QIcon>());
			action->setToolTip(childIndex.data(BookmarksModel::DescriptionRole).toString());
			action->setStatusTip(childIndex.data(BookmarksModel::UrlRole).toString());

			if (childIndex.data(BookmarksModel::TitleRole).toString().isEmpty())
			{
				action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
			}
			else
			{
				action->setText(Utils::elideText(QString(childIndex.data(BookmarksModel::TitleRole).toString()).replace(QLatin1Char('&'), QLatin1String("&&")), menu));
			}

			if (type == BookmarksModel::UrlBookmark && m_role == BookmarksMenuRole)
			{
				connect(action, SIGNAL(triggered()), this, SLOT(openBookmark()));
			}
			else if (type == BookmarksModel::FolderBookmark)
			{
				if (model->rowCount(childIndex) > 0)
				{
					action->setMenu(new Menu(m_role, this));
				}
				else
				{
					action->setEnabled(false);
				}
			}
		}
		else
		{
			menu->addSeparator();
		}
	}
}

void Menu::populateCharacterEncodingMenu()
{
	if (!m_actionGroup)
	{
		const QList<int> textCodecs({106, 1015, 1017, 4, 5, 6, 7, 8, 82, 10, 85, 12, 13, 109, 110, 112, 2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, 2258, 18, 39, 17, 38, 2026});

		m_actionGroup = new QActionGroup(this);
		m_actionGroup->setExclusive(true);

		Action *defaultAction(addAction());
		defaultAction->setData(-1);
		defaultAction->setCheckable(true);
		defaultAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Auto Detect"));

		m_actionGroup->addAction(defaultAction);

		addSeparator();

		for (int i = 0; i < textCodecs.count(); ++i)
		{
			QTextCodec *codec(QTextCodec::codecForMib(textCodecs.at(i)));

			if (!codec)
			{
				continue;
			}

			QAction *textCodecAction(QMenu::addAction(Utils::elideText(codec->name(), this)));
			textCodecAction->setData(textCodecs.at(i));
			textCodecAction->setCheckable(true);

			m_actionGroup->addAction(textCodecAction);
		}
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString encoding(mainWindow ? mainWindow->getWindowsManager()->getOption(SettingsManager::Content_DefaultCharacterEncodingOption).toString().toLower() : QString());

	for (int i = 2; i < actions().count(); ++i)
	{
		QAction *action(actions().at(i));

		if (!action)
		{
			continue;
		}

		action->setChecked(encoding == action->text().toLower());

		if (action->isChecked())
		{
			break;
		}
	}

	if (!m_actionGroup->checkedAction() && !actions().isEmpty())
	{
		actions().first()->setChecked(true);
	}
}

void Menu::populateClosedWindowsMenu()
{
	clear();

	Action *clearAction(addAction());
	clearAction->setData(0);
	clearAction->setIcon(ThemesManager::getIcon(QLatin1String("edit-clear")));
	clearAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Clear"));

	addSeparator();

	const QStringList windows(SessionsManager::getClosedWindows());

	if (!windows.isEmpty())
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			QMenu::addAction(Utils::elideText(tr("Window - %1").arg(windows.at(i)), this), this, SLOT(restoreClosedWindow()))->setData(-(i + 1));
		}

		addSeparator();
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		const QList<ClosedWindow> tabs(mainWindow->getWindowsManager()->getClosedWindows());

		for (int i = 0; i < tabs.count(); ++i)
		{
			QAction *action(QMenu::addAction(HistoryManager::getIcon(QUrl(tabs.at(i).window.getUrl())), Utils::elideText(tabs.at(i).window.getTitle().replace(QLatin1Char('&'), QLatin1String("&&")), this), this, SLOT(restoreClosedWindow())));
			action->setData(i + 1);
			action->setStatusTip(tabs.at(i).window.getUrl());
		}
	}

	connect(clearAction, SIGNAL(triggered()), this, SLOT(clearClosedWindows()));
}

void Menu::populateSessionsMenu()
{
	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();
	}

	clear();

	addAction(ActionsManager::SaveSessionAction, true);
	addAction(ActionsManager::SessionsAction, true);

	addSeparator();

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	const QStringList sessions(SessionsManager::getSessions());
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session(SessionsManager::getSession(sessions.at(i)));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted(information.values());
	const QString currentSession(SessionsManager::getCurrentSession());

	for (int i = 0; i < sorted.count(); ++i)
	{
		int windows(0);

		for (int j = 0; j < sorted.at(i).windows.count(); ++j)
		{
			windows += sorted.at(i).windows.at(j).windows.count();
		}

		QAction *action(QMenu::addAction(tr("%1 (%n tab(s))", "", windows).arg(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : QString(sorted.at(i).title).replace(QLatin1Char('&'), QLatin1String("&&")))));
		action->setData(sorted.at(i).path);
		action->setCheckable(true);
		action->setChecked(sorted.at(i).path == currentSession);

		m_actionGroup->addAction(action);
	}
}

void Menu::populateStyleSheetsMenu()
{
	clear();

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	Action *defaultAction(addAction());
	defaultAction->setData(-1);
	defaultAction->setCheckable(true);
	defaultAction->setChecked(true);
	defaultAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Default Style"));

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getWindowsManager()->getWindowByIndex(mainWindow->getWindowsManager()->getWindowIndex(-1)));

	if (!window)
	{
		return;
	}

	addSeparator();

	const QString activeStyleSheet(window->getContentsWidget()->getActiveStyleSheet());
	const QStringList styleSheets(window->getContentsWidget()->getStyleSheets());
	QActionGroup *actionGroup(new QActionGroup(this));
	actionGroup->setExclusive(true);
	actionGroup->addAction(defaultAction);

	for (int i = 0; i < styleSheets.count(); ++i)
	{
		QAction *action(QMenu::addAction(styleSheets.at(i)));

		action->setCheckable(true);
		action->setChecked(styleSheets.at(i) == activeStyleSheet);

		actionGroup->addAction(action);
	}

	defaultAction->setChecked(activeStyleSheet.isEmpty() || !styleSheets.contains(activeStyleSheet));
}

void Menu::populateToolBarsMenu()
{
	clear();

	const QVector<ToolBarsManager::ToolBarDefinition> definitions(ToolBarsManager::getToolBarDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		Action *action(addAction());
		action->setData(definitions.at(i).identifier);
		action->setCheckable(true);
		action->setChecked(definitions.at(i).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar);

		if (definitions.at(i).getTitle().isEmpty())
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
		}
		else
		{
			action->setText(definitions.at(i).getTitle());
		}

		connect(action, SIGNAL(toggled(bool)), this, SLOT(setToolBarVisibility(bool)));
	}

	addAction(ActionsManager::ShowSidebarAction, true);
	addAction(ActionsManager::ShowErrorConsoleAction, true);
	addSeparator();

	Menu *addNewMenu(new Menu(NoMenuRole, this));
	Action *addNewAction(addAction());
	addNewAction->setMenu(addNewMenu);
	addNewAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add New"));

	Action *addToolBarAction(addNewMenu->addAction());
	addToolBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Toolbar…"));

	Action *addBookmarksBarAction(addNewMenu->addAction());
	addBookmarksBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Bookmarks Bar…"));

	addSeparator();
	addAction(ActionsManager::LockToolBarsAction, true);
	addSeparator();
	addAction(ActionsManager::ResetToolBarsAction, true);

	connect(addToolBarAction, SIGNAL(triggered()), ToolBarsManager::getInstance(), SLOT(addToolBar()));
	connect(addBookmarksBarAction, SIGNAL(triggered()), ToolBarsManager::getInstance(), SLOT(addBookmarksBar()));
}

void Menu::populateUserAgentMenu()
{
	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();

		clear();
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QStringList userAgents(NetworkManagerFactory::getUserAgents());
	const QString userAgent(mainWindow ? mainWindow->getWindowsManager()->getOption(SettingsManager::Network_UserAgentOption).toString() : QString());

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	Action *defaultAction(addAction());
	defaultAction->setData(QLatin1String("default"));
	defaultAction->setCheckable(true);
	defaultAction->setChecked(userAgent == QLatin1String("default"));
	defaultAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Default User Agent"));

	m_actionGroup->addAction(defaultAction);

	addSeparator();

	for (int i = 0; i < userAgents.count(); ++i)
	{
		if (userAgents.at(i).isEmpty())
		{
			addSeparator();
		}
		else
		{
			const QString title(NetworkManagerFactory::getUserAgent(userAgents.at(i)).title);
			Action *action(addAction());
			action->setData(userAgents.at(i));
			action->setCheckable(true);
			action->setChecked(userAgent == userAgents.at(i));

			if (title.isEmpty())
			{
				action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
			}
			else
			{
				action->setText(Utils::elideText(QCoreApplication::translate("userAgents", title.toUtf8()), this));
			}

			m_actionGroup->addAction(action);
		}
	}

	addSeparator();

	Action *customAction(addAction());
	customAction->setData(QLatin1String("custom"));
	customAction->setCheckable(true);
	customAction->setChecked(userAgent.startsWith(QLatin1String("custom;")));
	customAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Custom User Agent…"));

	m_actionGroup->addAction(customAction);

	if (!m_actionGroup->checkedAction() && actions().count() > 2)
	{
		actions().first()->setChecked(true);
	}
}

void Menu::populateWindowsMenu()
{
	if (actions().isEmpty())
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow)
		{
			connect(mainWindow->getWindowsManager(), SIGNAL(windowAdded(qint64)), this, SLOT(populateWindowsMenu()));
			connect(mainWindow->getWindowsManager(), SIGNAL(windowRemoved(qint64)), this, SLOT(populateWindowsMenu()));
			connect(mainWindow->getWindowsManager(), SIGNAL(windowTitleChanged(QString)), this, SLOT(populateWindowsMenu()));
		}

		disconnect(this, SIGNAL(aboutToShow()), this, SLOT(populateWindowsMenu()));
	}

	clear();

	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (!mainWindow)
	{
		return;
	}

	for (int i = 0; i < mainWindow->getWindowsManager()->getWindowCount(); ++i)
	{
		Window *window(mainWindow->getWindowsManager()->getWindowByIndex(i));

		if (window)
		{
			Action *action(addAction());
			action->setData(window->getIdentifier());
			action->setIcon(window->getIcon());

			if (window->getTitle().isEmpty())
			{
				action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
			}
			else
			{
				action->setText(Utils::elideText(window->getTitle()));
			}
		}
	}
}

void Menu::clearModelMenu()
{
	const int offset((m_role == BookmarksMenuRole && menuAction() && !menuAction()->data().toModelIndex().isValid()) ? 3 : 0);

	for (int i = (actions().count() - 1); i >= offset; --i)
	{
		actions().at(i)->deleteLater();

		removeAction(actions().at(i));
	}

	if (m_role != BookmarksMenuRole && menuAction())
	{
		const QAbstractItemModel *model(menuAction()->data().toModelIndex().model());

		if (model)
		{
			menuAction()->setEnabled(model->rowCount(menuAction()->data().toModelIndex()) > 0);
		}
	}

	connect(this, SIGNAL(aboutToShow()), this, SLOT(populateModelMenu()));
}

void Menu::clearClosedWindows()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		mainWindow->getWindowsManager()->clearClosedWindows();
	}

	SessionsManager::clearClosedWindows();
}

void Menu::restoreClosedWindow()
{
	QAction *action(qobject_cast<QAction*>(sender()));
	const int index((action && action->data().type() == QVariant::Int) ? action->data().toInt() : 0);

	if (index > 0)
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

		if (mainWindow)
		{
			mainWindow->getWindowsManager()->restore(index - 1);
		}
	}
	else if (index < 0)
	{
		SessionsManager::restoreClosedWindow(-index - 1);
	}
}

void Menu::openBookmark()
{
	QWidget *menu(this);

	while (menu)
	{
		menu->close();
		menu = menu->parentWidget();

		if (!menu || !menu->inherits(QLatin1String("QMenu").data()))
		{
			break;
		}
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action && action->data().type() == QVariant::ModelIndex)
	{
		m_bookmark = BookmarksManager::getModel()->getBookmark(action->data().toModelIndex());
	}

	if (mainWindow)
	{
		const WindowsManager::OpenHints hints(action ? static_cast<WindowsManager::OpenHints>(action->data().toInt()) : WindowsManager::DefaultOpen);

		mainWindow->getWindowsManager()->open(m_bookmark, ((hints == WindowsManager::DefaultOpen) ? WindowsManager::calculateOpenHints() : hints));
	}

	m_bookmark = NULL;
}

void Menu::openImporter(QAction *action)
{
	if (action)
	{
		ImportDialog::createDialog(action->data().toString(), MainWindow::findMainWindow(this));
	}
}

void Menu::openSession(QAction *action)
{
	if (!action->data().isNull())
	{
		SessionsManager::restoreSession(SessionsManager::getSession(action->data().toString()), (SettingsManager::getValue(SettingsManager::Sessions_OpenInExistingWindowOption).toBool() ? SessionsManager::getActiveWindow() : NULL));
	}
}

void Menu::selectCharacterEncoding(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (!mainWindow)
	{
		return;
	}

	QString encoding(QLatin1String("auto"));

	if (action && action->data().toInt() > 0)
	{
		QTextCodec *codec(QTextCodec::codecForMib(action->data().toInt()));

		if (codec)
		{
			encoding = codec->name();
		}
	}

	mainWindow->getWindowsManager()->setOption(SettingsManager::Content_DefaultCharacterEncodingOption, encoding.toLower());
}

void Menu::selectStyleSheet(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getWindowsManager()->getWindowByIndex(mainWindow->getWindowsManager()->getWindowIndex(-1)));

	if (window && action)
	{
		window->getContentsWidget()->setActiveStyleSheet(action->data().isNull() ? action->text() : QString());
	}
}

void Menu::selectUserAgent(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (action && mainWindow)
	{
		mainWindow->getWindowsManager()->setOption(SettingsManager::Network_UserAgentOption, action->data().toString());
	}
}

void Menu::selectWindow(QAction *action)
{
	if (action)
	{
		QVariantMap parameters;
		parameters[QLatin1String("window")] = action->data().toULongLong();

		ActionsManager::triggerAction(ActionsManager::ActivateTabAction, this, parameters);
	}
}

void Menu::updateClosedWindowsMenu()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	setEnabled((mainWindow && mainWindow->getWindowsManager()->getClosedWindows().count() > 0) || SessionsManager::getClosedWindows().count() > 0);
}

void Menu::setToolBarVisibility(bool visible)
{
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action)
	{
		ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(action->data().toInt()));
		definition.normalVisibility = (visible ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

		ToolBarsManager::setToolBar(definition);
	}
}

Action* Menu::addAction(int identifier, bool useGlobal)
{
	Action *action(useGlobal ? ActionsManager::getAction(identifier, this) : new Action(identifier, this));

	QMenu::addAction(action);

	return action;
}

Menu::MenuRole Menu::getRole() const
{
	return m_role;
}

Menu::MenuRole Menu::getRole(const QString &identifier)
{
	if (identifier == QLatin1String("BookmarksMenu"))
	{
		return BookmarksMenuRole;
	}

	if (identifier == QLatin1String("CharacterEncodingMenu"))
	{
		return CharacterEncodingMenuRole;
	}

	if (identifier == QLatin1String("ClosedWindowsMenu"))
	{
		return ClosedWindowsMenu;
	}

	if (identifier == QLatin1String("ImportExportMenu"))
	{
		return ImportExportMenuRole;
	}

	if (identifier == QLatin1String("SessionsMenu"))
	{
		return SessionsMenuRole;
	}

	if (identifier == QLatin1String("StyleSheetsMenu"))
	{
		return StyleSheetsMenuRole;
	}

	if (identifier == QLatin1String("ToolBarsMenu"))
	{
		return ToolBarsMenuRole;
	}

	if (identifier == QLatin1String("UserAgentMenu"))
	{
		return UserAgentMenuRole;
	}

	if (identifier == QLatin1String("WindowsMenu"))
	{
		return WindowsMenuRole;
	}

	return NoMenuRole;
}

}
