/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_LOCALLISTINGNETWORKREPLY_H
#define MEERKAT_LOCALLISTINGNETWORKREPLY_H

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

namespace Meerkat
{

class LocalListingNetworkReply : public QNetworkReply
{
public:
	LocalListingNetworkReply(QObject *parent, const QNetworkRequest &request);

	qint64 bytesAvailable() const;
	qint64 readData(char *data, qint64 maxSize);
	bool isSequential() const;

public slots:
	void abort();

private:
	QByteArray m_content;
	qint64 m_offset;
};

}

#endif
