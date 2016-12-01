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

#include "QtWebKitCookieJar.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../ui/AcceptCookieDialog.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/WebWidget.h"

#include <QtCore/QTimer>

namespace Meerkat
{

QtWebKitCookieJar::QtWebKitCookieJar(CookieJar *cookieJar, WebWidget *widget) : QNetworkCookieJar(widget),
	m_widget(widget),
	m_cookieJar(cookieJar),
	m_generalCookiesPolicy(CookieJar::AcceptAllCookies),
	m_thirdPartyCookiesPolicy(CookieJar::AcceptAllCookies),
	m_keepMode(CookieJar::KeepUntilExpiresMode),
	m_isDialogVisible(false)
{
}

void QtWebKitCookieJar::setup(const QStringList &thirdPartyAcceptedHosts, const QStringList &thirdPartyRejectedHosts, CookieJar::CookiesPolicy generalCookiesPolicy, CookieJar::CookiesPolicy thirdPartyCookiesPolicy, CookieJar::KeepMode keepMode)
{
	m_thirdPartyAcceptedHosts = thirdPartyAcceptedHosts;
	m_thirdPartyRejectedHosts = thirdPartyRejectedHosts;
	m_generalCookiesPolicy = generalCookiesPolicy;
	m_thirdPartyCookiesPolicy = thirdPartyCookiesPolicy;
	m_keepMode = keepMode;
}

void QtWebKitCookieJar::dialogClosed()
{
	m_isDialogVisible = false;

	showDialog();
}

void QtWebKitCookieJar::showDialog()
{
	if (m_operations.isEmpty() || m_isDialogVisible)
	{
		return;
	}

	m_isDialogVisible = true;

	const QPair<CookieJar::CookieOperation, QNetworkCookie> operation(m_operations.dequeue());
	AcceptCookieDialog *cookieDialog(new AcceptCookieDialog(operation.second, operation.first, m_cookieJar, m_widget));
	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-warning")), cookieDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, cookieDialog, m_widget);

	connect(cookieDialog, SIGNAL(finished(int)), &dialog, SLOT(close()));
	connect(cookieDialog, SIGNAL(finished(int)), this, SLOT(dialogClosed()));
	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);
}

void QtWebKitCookieJar::setWidget(WebWidget *widget)
{
	m_widget = widget;
}

QtWebKitCookieJar* QtWebKitCookieJar::clone(WebWidget *parent)
{
	return new QtWebKitCookieJar(m_cookieJar, parent);
}

CookieJar* QtWebKitCookieJar::getCookieJar()
{
	return m_cookieJar;
}

QList<QNetworkCookie> QtWebKitCookieJar::cookiesForUrl(const QUrl &url) const
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies)
	{
		return QList<QNetworkCookie>();
	}

	return m_cookieJar->getCookiesForUrl(url);
}

bool QtWebKitCookieJar::insertCookie(const QNetworkCookie &cookie)
{
	if (!canModifyCookie(cookie))
	{
		return false;
	}

	if (m_keepMode == CookieJar::AskIfKeepMode)
	{
		m_operations.enqueue(qMakePair((m_cookieJar->hasCookie(cookie) ? CookieJar::UpdateCookie : CookieJar::InsertCookie), cookie));

		if (m_operations.count() == 1)
		{
			QTimer::singleShot(250, this, SLOT(showDialog()));
		}

		return false;
	}

	if (m_keepMode == CookieJar::KeepUntilExitMode)
	{
		QNetworkCookie mutableCookie(cookie);
		mutableCookie.setExpirationDate(QDateTime());

		return m_cookieJar->forceInsertCookie(mutableCookie);
	}

	return m_cookieJar->forceInsertCookie(cookie);
}

bool QtWebKitCookieJar::updateCookie(const QNetworkCookie &cookie)
{
	return insertCookie(cookie);
}

bool QtWebKitCookieJar::deleteCookie(const QNetworkCookie &cookie)
{
	if (!canModifyCookie(cookie))
	{
		return false;
	}

	if (m_keepMode == CookieJar::AskIfKeepMode)
	{
		m_operations.enqueue(qMakePair(CookieJar::RemoveCookie, cookie));

		if (m_operations.count() == 1)
		{
			QTimer::singleShot(250, this, SLOT(showDialog()));
		}

		return false;
	}

	return m_cookieJar->forceDeleteCookie(cookie);
}

bool QtWebKitCookieJar::canModifyCookie(const QNetworkCookie &cookie) const
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies || m_generalCookiesPolicy == CookieJar::ReadOnlyCookies)
	{
		return false;
	}

	if (m_thirdPartyCookiesPolicy != CookieJar::AcceptAllCookies || !m_thirdPartyRejectedHosts.isEmpty())
	{
		QUrl url;
		url.setScheme(QLatin1String("http"));
		url.setHost(cookie.domain().startsWith(QLatin1Char('.')) ? cookie.domain().mid(1) : cookie.domain());

		if (!CookieJar::isDomainTheSame(m_widget->getUrl(), url))
		{
			if (m_thirdPartyRejectedHosts.contains(cookie.domain()))
			{
				return false;
			}

			if (m_thirdPartyAcceptedHosts.contains(cookie.domain()))
			{
				return true;
			}

			if (m_thirdPartyCookiesPolicy == CookieJar::IgnoreCookies)
			{
				return false;
			}

			if (m_thirdPartyCookiesPolicy == CookieJar::AcceptExistingCookies && !m_cookieJar->hasCookie(cookie))
			{
				return false;
			}
		}
	}

	return true;
}

bool QtWebKitCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies || m_generalCookiesPolicy == CookieJar::ReadOnlyCookies)
	{
		return false;
	}

	bool added(false);

	for (int i = 0; i < cookieList.count(); ++i)
	{
		QNetworkCookie cookie(cookieList.at(i));
		cookie.normalize(url);

		if (validateCookie(cookie, url) && canModifyCookie(cookie))
		{
			if (m_keepMode == CookieJar::AskIfKeepMode)
			{
				m_operations.enqueue(qMakePair((m_cookieJar->hasCookie(cookie) ? CookieJar::UpdateCookie : CookieJar::InsertCookie), cookie));
			}
			else
			{
				if (m_keepMode == CookieJar::KeepUntilExitMode)
				{
					cookie.setExpirationDate(QDateTime());
				}

				m_cookieJar->forceInsertCookie(cookie);

				added = true;
			}
		}
	}

	if (m_operations.count() > 0)
	{
		QTimer::singleShot(250, this, SLOT(showDialog()));
	}

	return added;
}

}
