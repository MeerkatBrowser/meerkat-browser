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

#ifndef MEERKAT_PASSWORDSMANAGER_H
#define MEERKAT_PASSWORDSMANAGER_H

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

namespace Meerkat
{

class PasswordsStorageBackend;

class PasswordsManager : public QObject
{
	Q_OBJECT

public:
	enum PasswordType
	{
		FormPassword = 0,
		AuthPassword = 1
	};

	struct PasswordInformation
	{
		QUrl url;
		QStringList passwords;
		QMap<QString, QString> fields;
		PasswordType type;

		PasswordInformation() : type(FormPassword) {}
	};

	static void createInstance(QObject *parent = NULL);
	static void addPassword(const PasswordInformation &password);
	static PasswordsManager* getInstance();
	static QList<PasswordInformation> getPasswords(const QUrl &url);
	static bool isEncryptionAvailable();

protected:
	explicit PasswordsManager(QObject *parent = NULL);

private:
	static PasswordsManager *m_instance;
	static PasswordsStorageBackend *m_backend;
};

}

#endif
