/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "SearchWidget.h"
#include "../../../core/SearchEnginesManager.h"
#include "../../../core/SearchSuggester.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/LineEditWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/PreferencesDialog.h"
#include "../../../ui/SearchDelegate.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QAbstractItemView>

namespace Meerkat
{

SearchWidget::SearchWidget(Window *window, QWidget *parent) : ComboBoxWidget(parent),
	m_window(NULL),
	m_lineEdit(new LineEditWidget(this)),
	m_completer(new QCompleter(this)),
	m_suggester(NULL),
	m_lastValidIndex(0),
	m_isIgnoringActivation(false),
	m_isPopupUpdated(false),
	m_isSearchEngineLocked(false),
	m_wasPopupVisible(false)
{
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setCompletionMode(QCompleter::PopupCompletion);
	m_completer->setCompletionRole(Qt::DisplayRole);

	setEditable(true);
	setLineEdit(m_lineEdit);
	setMinimumWidth(100);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	setItemDelegate(new SearchDelegate(this));
	setModel(SearchEnginesManager::getSearchEnginesModel());
	setInsertPolicy(QComboBox::NoInsert);
	optionChanged(SettingsManager::AddressField_DropActionOption, SettingsManager::getValue(SettingsManager::AddressField_DropActionOption));
	optionChanged(SettingsManager::AddressField_SelectAllOnFocusOption, SettingsManager::getValue(SettingsManager::AddressField_SelectAllOnFocusOption));
	optionChanged(SettingsManager::Search_SearchEnginesSuggestionsOption, SettingsManager::getValue(SettingsManager::Search_SearchEnginesSuggestionsOption));

	m_lineEdit->setCompleter(m_completer);
	m_lineEdit->setStyleSheet(QLatin1String("QLineEdit {background:transparent;}"));

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}

	connect(SearchEnginesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(storeCurrentSearchEngine()));
	connect(SearchEnginesManager::getInstance(), SIGNAL(searchEnginesModelModified()), this, SLOT(restoreCurrentSearchEngine()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
	connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	connect(m_lineEdit, SIGNAL(textDropped(QString)), this, SLOT(sendRequest(QString)));
	connect(m_completer, SIGNAL(activated(QString)), this, SLOT(sendRequest(QString)));

	setWindow(window);
}

void SearchWidget::changeEvent(QEvent *event)
{
	ComboBoxWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && itemData(currentIndex(), Qt::AccessibleDescriptionRole).toString().isEmpty())
	{
		m_lineEdit->setPlaceholderText(tr("Search using %1").arg(currentData(Qt::UserRole).toString()));
	}
}

void SearchWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOptionFrame panel;
	panel.initFrom(m_lineEdit);
	panel.rect = rect();
	panel.palette = palette();
	panel.lineWidth = 1;

	style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

	if (isEnabled())
	{
		painter.drawPixmap(m_iconRectangle, currentData(Qt::DecorationRole).value<QIcon>().pixmap(m_iconRectangle.size()));

		QStyleOption arrow;
		arrow.initFrom(this);
		arrow.rect = m_dropdownArrowRectangle;

		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, this);
	}

	painter.drawPixmap(m_searchButtonRectangle, ThemesManager::getIcon(QLatin1String("edit-find")).pixmap(m_searchButtonRectangle.size(), (isEnabled() ? QIcon::Active : QIcon::Disabled)));
}

void SearchWidget::resizeEvent(QResizeEvent *event)
{
	ComboBoxWidget::resizeEvent(event);

	QStyleOptionFrame panel;
	panel.initFrom(m_lineEdit);
	panel.rect = rect();
	panel.lineWidth = 1;

	const QRect rectangle(style()->subElementRect(QStyle::SE_LineEditContents, &panel, this));

	m_iconRectangle = rectangle;
	m_iconRectangle.setWidth(rectangle.height());
	m_iconRectangle = m_iconRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	if (m_isSearchEngineLocked)
	{
		m_dropdownArrowRectangle = QRect();
	}
	else
	{
		m_dropdownArrowRectangle = rectangle;
		m_dropdownArrowRectangle.setLeft(rectangle.left() + rectangle.height());
		m_dropdownArrowRectangle.setWidth(12);
	}

	if (m_options.value(QLatin1String("showSearchButton"), true).toBool())
	{
		m_searchButtonRectangle = rectangle;
		m_searchButtonRectangle.setLeft(rectangle.right() - rectangle.height());
		m_searchButtonRectangle = m_searchButtonRectangle.marginsRemoved(QMargins(2, 2, 2, 2));
	}
	else
	{
		m_searchButtonRectangle = QRect();
	}

	m_lineEdit->resize((rectangle.width() - m_iconRectangle.width() - m_dropdownArrowRectangle.width() - m_searchButtonRectangle.width() - 10), rectangle.height());
	m_lineEdit->move((m_isSearchEngineLocked ? m_iconRectangle : m_dropdownArrowRectangle).topRight() + QPoint(3, 0));

	m_lineEditRectangle = m_lineEdit->geometry();
}

void SearchWidget::focusInEvent(QFocusEvent *event)
{
	ComboBoxWidget::focusInEvent(event);

	m_lineEdit->activate(event->reason());
}

void SearchWidget::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && !(m_completer->popup() && m_completer->popup()->isVisible()))
	{
		sendRequest(m_lineEdit->text().trimmed());
	}
	else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
	{
		if (m_isSearchEngineLocked)
		{
			QWidget::keyPressEvent(event);

			return;
		}

		disconnect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));

		m_isIgnoringActivation = true;
	}

	ComboBoxWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
	{
		m_isIgnoringActivation = false;

		m_lineEdit->setText(m_query);

		connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	}
}

void SearchWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(tr("Undo"), m_lineEdit, SLOT(undo()), QKeySequence(QKeySequence::Undo))->setEnabled(m_lineEdit->isUndoAvailable());
	menu.addAction(tr("Redo"), m_lineEdit, SLOT(redo()), QKeySequence(QKeySequence::Redo))->setEnabled(m_lineEdit->isRedoAvailable());
	menu.addSeparator();
	menu.addAction(tr("Cut"), m_lineEdit, SLOT(cut()), QKeySequence(QKeySequence::Cut))->setEnabled(m_lineEdit->hasSelectedText());
	menu.addAction(tr("Copy"), m_lineEdit, SLOT(copy()), QKeySequence(QKeySequence::Copy))->setEnabled(m_lineEdit->hasSelectedText());
	menu.addAction(tr("Paste"), m_lineEdit, SLOT(paste()), QKeySequence(QKeySequence::Paste))->setEnabled(!QApplication::clipboard()->text().isEmpty());
	menu.addAction(tr("Paste and Go"), this, SLOT(pasteAndGo()))->setEnabled(!QApplication::clipboard()->text().isEmpty());
	menu.addAction(tr("Delete"), m_lineEdit, SLOT(deleteText()), QKeySequence(QKeySequence::Delete))->setEnabled(m_lineEdit->hasSelectedText());
	menu.addSeparator();
	menu.addAction(tr("Copy to Note"), m_lineEdit, SLOT(copyToNote()))->setEnabled(!m_lineEdit->text().isEmpty());
	menu.addSeparator();
	menu.addAction(tr("Clear All"), m_lineEdit, SLOT(clear()))->setEnabled(!m_lineEdit->text().isEmpty());
	menu.addAction(tr("Select All"), m_lineEdit, SLOT(selectAll()))->setEnabled(!m_lineEdit->text().isEmpty());

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QList<QAction*>(), &menu));
	}

	menu.exec(event->globalPos());
}

void SearchWidget::mousePressEvent(QMouseEvent *event)
{
	m_wasPopupVisible = (m_popupHideTime.isValid() && m_popupHideTime.msecsTo(QTime::currentTime()) < 100);

	QWidget::mousePressEvent(event);
}

void SearchWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (!m_isSearchEngineLocked && (m_iconRectangle.contains(event->pos()) || m_dropdownArrowRectangle.contains(event->pos())))
		{
			m_popupHideTime = QTime();

			if (m_wasPopupVisible)
			{
				hidePopup();
			}
			else
			{
				showPopup();
			}
		}
		else if (m_searchButtonRectangle.contains(event->pos()))
		{
			sendRequest();
		}
	}

	QWidget::mouseReleaseEvent(event);
}

void SearchWidget::wheelEvent(QWheelEvent *event)
{
	if (m_isSearchEngineLocked)
	{
		QWidget::wheelEvent(event);

		return;
	}

	disconnect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));

	m_isIgnoringActivation = true;

	ComboBoxWidget::wheelEvent(event);

	m_isIgnoringActivation = false;

	m_lineEdit->setText(m_query);

	connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
}

void SearchWidget::hidePopup()
{
	if (!m_query.isEmpty())
	{
		m_isPopupUpdated = true;
	}

	m_popupHideTime = QTime::currentTime();

	ComboBoxWidget::hidePopup();
}

void SearchWidget::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::AddressField_DropActionOption)
	{
		const QString dropAction(value.toString());

		if (dropAction == QLatin1String("pasteAndGo"))
		{
			m_lineEdit->setDropMode(LineEditWidget::ReplaceAndNotifyDropMode);
		}
		else if (dropAction == QLatin1String("replace"))
		{
			m_lineEdit->setDropMode(LineEditWidget::ReplaceDropMode);
		}
		else
		{
			m_lineEdit->setDropMode(LineEditWidget::PasteDropMode);
		}
	}
	else if (identifier == SettingsManager::AddressField_SelectAllOnFocusOption)
	{
		m_lineEdit->setSelectAllOnFocus(value.toBool());
	}
	else if (identifier == SettingsManager::Search_SearchEnginesSuggestionsOption)
	{
		if (value.toBool() && !m_suggester)
		{
			m_suggester = new SearchSuggester(getCurrentSearchEngine(), this);

			m_completer->setModel(m_suggester->getModel());

			connect(m_lineEdit, SIGNAL(textEdited(QString)), m_suggester, SLOT(setQuery(QString)));
		}
		else if (!value.toBool() && m_suggester)
		{
			m_suggester->deleteLater();
			m_suggester = NULL;

			m_completer->setModel(NULL);
		}
	}
}

void SearchWidget::currentIndexChanged(int index)
{
	if (!m_storedSearchEngine.isEmpty())
	{
		return;
	}

	if (itemData(index, Qt::AccessibleDescriptionRole).toString().isEmpty())
	{
		m_lastValidIndex = index;

		if (!m_isSearchEngineLocked)
		{
			SessionsManager::markSessionModified();

			emit searchEngineChanged(currentData(Qt::UserRole + 1).toString());
		}

		m_lineEdit->setPlaceholderText(tr("Search using %1").arg(itemData(index, Qt::UserRole).toString()));
		m_lineEdit->setText(m_query);

		if (m_suggester)
		{
			m_suggester->setSearchEngine(getCurrentSearchEngine());
			m_suggester->setQuery(QString());
		}

		if (!m_query.isEmpty() && sender() == this)
		{
			sendRequest();
		}
	}
	else
	{
		const QString query(m_query);

		setCurrentIndex(m_lastValidIndex);

		if (query != itemText(index))
		{
			m_lineEdit->setText(query);
		}

		if (itemData(index, Qt::AccessibleDescriptionRole).toString() == QLatin1String("configure"))
		{
			PreferencesDialog dialog(QLatin1String("search"), this);
			dialog.exec();
		}
	}

	m_lineEdit->setGeometry(m_lineEditRectangle);
}

void SearchWidget::queryChanged(const QString &query)
{
	if (m_isPopupUpdated)
	{
		m_isPopupUpdated = false;
	}
	else
	{
		m_query = query;
	}
}

void SearchWidget::sendRequest(const QString &query)
{
	if (!query.isEmpty())
	{
		m_query = query;
	}

	if (!m_isIgnoringActivation)
	{
		if (m_query.isEmpty())
		{
			const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(currentData(Qt::UserRole + 1).toString()));

			if (searchEngine.formUrl.isValid())
			{
				emit requestedOpenUrl(searchEngine.formUrl, WindowsManager::calculateOpenHints());
			}
		}
		else
		{
			emit requestedSearch(m_query, currentData(Qt::UserRole + 1).toString(), WindowsManager::calculateOpenHints());
		}
	}
}

void SearchWidget::pasteAndGo()
{
	m_lineEdit->paste();

	sendRequest();
}

void SearchWidget::storeCurrentSearchEngine()
{
	m_storedSearchEngine = getCurrentSearchEngine();

	hidePopup();

	disconnect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
	disconnect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
}

void SearchWidget::restoreCurrentSearchEngine()
{
	if (!m_storedSearchEngine.isEmpty())
	{
		setSearchEngine(m_storedSearchEngine);

		m_storedSearchEngine = QString();
	}

	m_lineEdit->setText(m_query);
	m_lineEdit->setGeometry(m_lineEditRectangle);

	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
	connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
}

void SearchWidget::activate(Qt::FocusReason reason)
{
	m_lineEdit->activate(reason);
}

void SearchWidget::setSearchEngine(const QString &searchEngine)
{
	if (m_isSearchEngineLocked && searchEngine != m_options.value(QLatin1String("searchEngine")).toString())
	{
		return;
	}

	const QStringList searchEngines(SearchEnginesManager::getSearchEngines());

	if (searchEngines.isEmpty())
	{
		hidePopup();
		setEnabled(false);

		m_lineEdit->setPlaceholderText(QString());

		return;
	}

	const int index(qMax(0, searchEngines.indexOf(searchEngine.isEmpty() ? SettingsManager::getValue(SettingsManager::Search_DefaultSearchEngineOption).toString() : searchEngine)));

	if (index == currentIndex())
	{
		currentIndexChanged(currentIndex());
	}
	else
	{
		setCurrentIndex(index);
	}

	setEnabled(true);

	if (m_suggester)
	{
		m_suggester->setSearchEngine(getCurrentSearchEngine());
	}
}

void SearchWidget::setOptions(const QVariantMap &options)
{
	m_options = options;

	if (m_options.contains(QLatin1String("searchEngine")))
	{
		m_isSearchEngineLocked = true;

		setSearchEngine(m_options[QLatin1String("searchEngine")].toString());
	}
	else
	{
		m_isSearchEngineLocked = false;
	}

	resize(size());
}

void SearchWidget::setWindow(Window *window)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (m_window && (!sender() || sender() != m_window) && !m_window->isAboutToClose())
	{
		m_window->detachSearchWidget(this);

		disconnect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), m_window.data(), SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), m_window.data(), SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(searchEngineChanged(QString)), m_window.data(), SLOT(setSearchEngine(QString)));
		disconnect(m_window.data(), SIGNAL(searchEngineChanged(QString)), this, SLOT(setSearchEngine(QString)));
		disconnect(m_window.data(), SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));

		setSearchEngine();
	}

	m_window = window;

	if (window)
	{
		if (mainWindow)
		{
			disconnect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
		}

		window->attachSearchWidget(this);

		setSearchEngine(window->getSearchEngine());

		connect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), m_window.data(), SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), window, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)));
		connect(this, SIGNAL(searchEngineChanged(QString)), window, SLOT(setSearchEngine(QString)));
		connect(window, SIGNAL(searchEngineChanged(QString)), this, SLOT(setSearchEngine(QString)));
		connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));
	}
	else
	{
		if (mainWindow)
		{
			connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
		}

		setSearchEngine();
	}
}

QString SearchWidget::getCurrentSearchEngine() const
{
	return currentData(Qt::UserRole + 1).toString();
}

QVariantMap SearchWidget::getOptions() const
{
	return m_options;
}

}
