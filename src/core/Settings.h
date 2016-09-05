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

#ifndef MEERKAT_SETTINGS_H
#define MEERKAT_SETTINGS_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

namespace Meerkat
{

class Settings : public QObject
{
	Q_OBJECT

public:
	explicit Settings(QObject *parent = NULL);
	Settings(const QString &path, QObject *parent = NULL);

	void clear();
	void beginGroup(const QString &group);
	void removeGroup(const QString &group);
	void endGroup();
	void setComment(const QString &comment);
	void setValue(const QString &key, const QVariant &value);
	QString getComment() const;
	QVariant getValue(const QString &key, const QVariant &fallback = QVariant()) const;
	QStringList getGroups() const;
	QStringList getKeys() const;
	bool save(const QString &path = QString());
	bool hasError() const;

private:
	QString m_path;
	QString m_group;
	QString m_comment;
	QMap<QString, QVariantMap> m_data;
	bool m_hasError;
};

}

#endif
