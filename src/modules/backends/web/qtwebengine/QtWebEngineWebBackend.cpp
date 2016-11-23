/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebEngineWebBackend.h"
#include "QtWebEngineTransfer.h"
#include "QtWebEngineUrlRequestInterceptor.h"
#include "QtWebEngineWebWidget.h"
#include "../../../../core/HandlersManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/TransfersManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/TransferDialog.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineSettings>

namespace Meerkat
{

QString QtWebEngineWebBackend::m_engineVersion;
QMap<QString, QString> QtWebEngineWebBackend::m_userAgentComponents;
QMap<QString, QString> QtWebEngineWebBackend::m_userAgents;

QtWebEngineWebBackend::QtWebEngineWebBackend(QObject *parent) : WebBackend(parent),
	m_requestInterceptor(NULL),
	m_isInitialized(false)
{
	const QString userAgent(QWebEngineProfile::defaultProfile()->httpUserAgent());
	QRegularExpression platformExpression(QLatin1String("(\\([^\\)]+\\))"));
	QRegularExpression engineExpression(QLatin1String("Chrome/([\\d\\.]+)"));

	m_userAgentComponents[QLatin1String("platform")] = platformExpression.match(userAgent).captured(1);
	m_userAgentComponents[QLatin1String("engineVersion")] = QLatin1String("AppleWebKit/537.36 (KHTML, like Gecko) Chrome/") + engineExpression.match(userAgent).captured(1);
	m_userAgentComponents[QLatin1String("applicationVersion")] = QCoreApplication::applicationName() + QLatin1Char('/') + QCoreApplication::applicationVersion();

	m_engineVersion = engineExpression.match(userAgent).captured(1);
}

void QtWebEngineWebBackend::optionChanged(int identifier)
{
	if (identifier == SettingsManager::Network_AcceptLanguageOption)
	{
		QWebEngineProfile::defaultProfile()->setHttpAcceptLanguage(NetworkManagerFactory::getAcceptLanguage());
	}
	else if (identifier == SettingsManager::Network_UserAgentOption)
	{
		QWebEngineProfile::defaultProfile()->setHttpUserAgent(getUserAgent());
	}
	else if (!(SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Browser/")) || SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Content/"))))
	{
		return;
	}

	QWebEngineSettings *globalSettings(QWebEngineSettings::globalSettings());
	globalSettings->setAttribute(QWebEngineSettings::AutoLoadImages, (SettingsManager::getValue(SettingsManager::Browser_EnableImagesOption).toString() != QLatin1String("onlyCached")));
	globalSettings->setAttribute(QWebEngineSettings::PluginsEnabled, SettingsManager::getValue(SettingsManager::Browser_EnablePluginsOption).toString() != QLatin1String("disabled"));
	globalSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, SettingsManager::getValue(SettingsManager::Browser_EnableJavaScriptOption).toBool());
	globalSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, SettingsManager::getValue(SettingsManager::Browser_JavaScriptCanAccessClipboardOption).toBool());
	globalSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, SettingsManager::getValue(SettingsManager::Browser_JavaScriptCanOpenWindowsOption).toBool());
#if QT_VERSION >= 0x050700
	globalSettings->setAttribute(QWebEngineSettings::WebGLEnabled, SettingsManager::getValue(SettingsManager::Browser_EnableWebglOption).toBool());
#endif
	globalSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, SettingsManager::getValue(SettingsManager::Browser_EnableLocalStorageOption).toBool());
	globalSettings->setFontSize(QWebEngineSettings::DefaultFontSize, SettingsManager::getValue(SettingsManager::Content_DefaultFontSizeOption).toInt());
	globalSettings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, SettingsManager::getValue(SettingsManager::Content_DefaultFixedFontSizeOption).toInt());
	globalSettings->setFontSize(QWebEngineSettings::MinimumFontSize, SettingsManager::getValue(SettingsManager::Content_MinimumFontSizeOption).toInt());
	globalSettings->setFontFamily(QWebEngineSettings::StandardFont, SettingsManager::getValue(SettingsManager::Content_StandardFontOption).toString());
	globalSettings->setFontFamily(QWebEngineSettings::FixedFont, SettingsManager::getValue(SettingsManager::Content_FixedFontOption).toString());
	globalSettings->setFontFamily(QWebEngineSettings::SerifFont, SettingsManager::getValue(SettingsManager::Content_SerifFontOption).toString());
	globalSettings->setFontFamily(QWebEngineSettings::SansSerifFont, SettingsManager::getValue(SettingsManager::Content_SansSerifFontOption).toString());
	globalSettings->setFontFamily(QWebEngineSettings::CursiveFont, SettingsManager::getValue(SettingsManager::Content_CursiveFontOption).toString());
	globalSettings->setFontFamily(QWebEngineSettings::FantasyFont, SettingsManager::getValue(SettingsManager::Content_FantasyFontOption).toString());
}

void QtWebEngineWebBackend::downloadFile(QWebEngineDownloadItem *item)
{
#if QT_VERSION >= 0x050700
	if (item->savePageFormat() != QWebEngineDownloadItem::UnknownSaveFormat)
	{
		const QStringList filters({tr("HTML file (*.html *.htm)"), tr("HTML file with all resources (*.html *.htm)"), tr("Web archive (*.mht)")});
		const SaveInformation result(Utils::getSavePath(QFileInfo(item->path()).baseName() + QLatin1String(".html"), QString(), filters));

		if (result.path.isEmpty())
		{
			item->cancel();
			item->deleteLater();
		}
		else
		{
			const int index(filters.indexOf(result.filter));

			if (index == 1)
			{
				item->setSavePageFormat(QWebEngineDownloadItem::CompleteHtmlSaveFormat);
			}
			else if (index == 2)
			{
				item->setSavePageFormat(QWebEngineDownloadItem::MimeHtmlSaveFormat);
			}
			else
			{
				item->setSavePageFormat(QWebEngineDownloadItem::SingleHtmlSaveFormat);
			}

			item->setPath(result.path);
			item->accept();
		}

		return;
	}
#endif

	QWebEngineProfile *profile(qobject_cast<QWebEngineProfile*>(sender()));
	QtWebEngineTransfer *transfer(new QtWebEngineTransfer(item, (Transfer::CanNotifyOption | ((profile && profile->isOffTheRecord()) ? Transfer::IsPrivateOption : Transfer::NoOption))));

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return;
	}

	const HandlerDefinition handler(HandlersManager::getHandler(transfer->getMimeType().name()));

	switch (handler.transferMode)
	{
		case IgnoreTransferMode:
			transfer->cancel();
			transfer->deleteLater();

			break;
		case AskTransferMode:
			TransferDialog(transfer).exec();

			break;
		case OpenTransferMode:
			transfer->setOpenCommand(handler.openCommand);

			TransfersManager::addTransfer(transfer);

			break;
		case SaveTransferMode:
			transfer->setTarget(handler.downloadsPath + QDir::separator() + transfer->getSuggestedFileName());

			if (transfer->getState() == Transfer::CancelledState)
			{
				TransfersManager::addTransfer(transfer);
			}
			else
			{
				transfer->deleteLater();
			}

			break;
		case SaveAsTransferMode:
			{
				const QString path(Utils::getSavePath(transfer->getSuggestedFileName(), handler.downloadsPath, QStringList(), true).path);

				if (path.isEmpty())
				{
					transfer->cancel();
					transfer->deleteLater();

					return;
				}

				transfer->setTarget(path);

				TransfersManager::addTransfer(transfer);
			}

			break;
	}
}

WebWidget* QtWebEngineWebBackend::createWidget(bool isPrivate, ContentsWidget *parent)
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;
		m_requestInterceptor = new QtWebEngineUrlRequestInterceptor(this);

		QWebEngineProfile::defaultProfile()->setHttpAcceptLanguage(NetworkManagerFactory::getAcceptLanguage());
		QWebEngineProfile::defaultProfile()->setHttpUserAgent(getUserAgent());
		QWebEngineProfile::defaultProfile()->setRequestInterceptor(m_requestInterceptor);

		QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

		const QString cachePath(SessionsManager::getCachePath());

		if (cachePath.isEmpty())
		{
			QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
		}
		else
		{
			QDir().mkpath(cachePath);

			QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
			QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize(SettingsManager::getValue(SettingsManager::Cache_DiskCacheLimitOption).toInt() * 1024);
			QWebEngineProfile::defaultProfile()->setCachePath(cachePath);
			QWebEngineProfile::defaultProfile()->setPersistentStoragePath(cachePath + QLatin1String("/persistentStorage/"));
		}

		optionChanged(SettingsManager::Browser_EnableFullScreenOption);

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int)));
		connect(QWebEngineProfile::defaultProfile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)), this, SLOT(downloadFile(QWebEngineDownloadItem*)));
	}

	return new QtWebEngineWebWidget(isPrivate, this, parent);
}

QString QtWebEngineWebBackend::getTitle() const
{
	return tr("Blink Backend");
}

QString QtWebEngineWebBackend::getDescription() const
{
	return tr("Backend utilizing QtWebEngine module");
}

QString QtWebEngineWebBackend::getVersion() const
{
	return QCoreApplication::applicationVersion();
}

QString QtWebEngineWebBackend::getEngineVersion() const
{
	return m_engineVersion;
}

QString QtWebEngineWebBackend::getSslVersion() const
{
	return tr("unknown");
}

QString QtWebEngineWebBackend::getUserAgent(const QString &pattern) const
{
	if (!pattern.isEmpty())
	{
		if (m_userAgents.contains(pattern))
		{
			return (m_userAgents[pattern].isEmpty() ? pattern : m_userAgents[pattern]);
		}

		QString userAgent(pattern);
		QMap<QString, QString>::iterator iterator;

		for (iterator = m_userAgentComponents.begin(); iterator != m_userAgentComponents.end(); ++iterator)
		{
			userAgent = userAgent.replace(QStringLiteral("{%1}").arg(iterator.key()), iterator.value());
		}

		m_userAgents[pattern] = ((pattern == userAgent) ? QString() : userAgent);

		return userAgent;
	}

	const UserAgentInformation userAgent(NetworkManagerFactory::getUserAgent(SettingsManager::getValue(SettingsManager::Network_UserAgentOption).toString()));

	return ((userAgent.value.isEmpty()) ? QString() : getUserAgent(userAgent.value));
}

QStringList QtWebEngineWebBackend::getBlockedElements(const QString &domain) const
{
	return (m_requestInterceptor ? m_requestInterceptor->getBlockedElements(domain) : QStringList());
}

QUrl QtWebEngineWebBackend::getHomePage() const
{
	return QUrl(QLatin1String("http://meerkat-browser.sourceforge.net/"));
}

QIcon QtWebEngineWebBackend::getIcon() const
{
	return QIcon();
}

bool QtWebEngineWebBackend::requestThumbnail(const QUrl &url, const QSize &size)
{
	Q_UNUSED(url)
	Q_UNUSED(size)

	return false;
}

}
