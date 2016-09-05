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

#ifndef MEERKAT_CONTENTSWIDGET_H
#define MEERKAT_CONTENTSWIDGET_H

#include "WebWidget.h"
#include "Window.h"

#include <QtCore/QPointer>

namespace Meerkat
{

class ContentsDialog;

class ContentsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ContentsWidget(Window *window);

	virtual void setParent(Window *window);
	virtual ContentsWidget* clone(bool cloneHistory = true);
	virtual Action* getAction(int identifier);
	Window* getParent();
	virtual QString getTitle() const = 0;
	virtual QString getVersion() const;
	virtual QString getActiveStyleSheet() const;
	virtual QString getStatusMessage() const;
	virtual QLatin1String getType() const = 0;
	virtual QVariant getOption(int identifier) const;
	virtual QVariant getPageInformation(WebWidget::PageInformation key) const;
	virtual QUrl getUrl() const = 0;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() const;
	virtual WindowHistoryInformation getHistory() const;
	virtual QStringList getStyleSheets() const;
	virtual QList<LinkUrl> getFeeds() const;
	virtual QList<LinkUrl> getSearchEngines() const;
	virtual QList<NetworkManager::ResourceInformation> getBlockedRequests() const;
	virtual WindowsManager::ContentStates getContentState() const;
	virtual WindowsManager::LoadingState getLoadingState() const;
	virtual int getZoom() const;
	virtual bool canClone() const;
	virtual bool canZoom() const;
	virtual bool isPrivate() const;

public slots:
	virtual void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	virtual void print(QPrinter *printer) = 0;
	virtual void goToHistoryIndex(int index);
	virtual void removeHistoryIndex(int index, bool purge = false);
	void showDialog(ContentsDialog *dialog, bool lockEventLoop = true);
	virtual void setOption(int identifier, const QVariant &value);
	virtual void setActiveStyleSheet(const QString &styleSheet);
	virtual void setHistory(const WindowHistoryInformation &history);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url, bool typed = true);

protected:
	void timerEvent(QTimerEvent *event);
	void showEvent(QShowEvent *event);
	void resizeEvent(QResizeEvent *event);

protected slots:
	void triggerAction();
	void close();
	void cleanupDialog();

private:
	QWidget *m_layer;
	QList<QPointer<ContentsDialog> > m_dialogs;
	int m_layerTimer;

signals:
	void aboutToNavigate();
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedAddBookmark(const QUrl &url, const QString &title, const QString &description);
	void requestedEditBookmark(const QUrl &url);
	void requestedNewWindow(ContentsWidget *widget, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &search, WindowsManager::OpenHints hints);
	void requestedGeometryChange(const QRect &geometry);
	void webWidgetChanged();
	void canZoomChanged(bool can);
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void contentStateChanged(WindowsManager::ContentStates state);
	void loadingStateChanged(WindowsManager::LoadingState);
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void requestBlocked(const NetworkManager::ResourceInformation &request);
	void zoomChanged(int zoom);
};

}

#endif
