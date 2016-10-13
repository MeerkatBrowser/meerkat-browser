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

#include <QtCore/QDateTime>
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
	enum FieldType
	{
		UnknownField = 0,
		PasswordField,
		TextField
	};

	enum PasswordMatch
	{
		NoMatch = 0,
		PartialMatch,
		FullMatch
	};

	enum PasswordType
	{
		UnknownPassword = 0,
		FormPassword = 1,
		AuthPassword = 2,
		AnyPassword = (FormPassword | AuthPassword)
	};

	Q_DECLARE_FLAGS(PasswordTypes, PasswordType)

	struct FieldInformation
	{
		QString name;
		QString value;
		FieldType type;

		FieldInformation() : type(UnknownField) {}
	};

	struct PasswordInformation
	{
		QUrl url;
		QDateTime timeAdded;
		QDateTime timeUsed;
		QList<FieldInformation> fields;
		PasswordType type;

		PasswordInformation() : type(FormPassword) {}
	};

	static void createInstance(QObject *parent = NULL);
	static void clearPasswords(const QString &host = QString());
	static void addPassword(const PasswordInformation &password);
	static void removePassword(const PasswordInformation &password);
	static PasswordsManager* getInstance();
	static QStringList getHosts();
	static QList<PasswordInformation> getPasswords(const QUrl &url, PasswordTypes types = AnyPassword);
	static PasswordMatch hasPassword(const PasswordInformation &password);
	static bool hasPasswords(const QUrl &url, PasswordTypes types = AnyPassword);

protected:
	explicit PasswordsManager(QObject *parent = NULL);

private:
	static PasswordsManager *m_instance;
	static PasswordsStorageBackend *m_backend;

signals:
	void passwordsModified();
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Meerkat::PasswordsManager::PasswordTypes)

#endif
