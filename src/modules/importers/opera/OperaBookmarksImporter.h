/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef MEERKAT_OPERABOOKMARKSIMPORTER_H
#define MEERKAT_OPERABOOKMARKSIMPORTER_H

#include "../../../core/BookmarksImporter.h"
#include "../../../ui/BookmarksImporterWidget.h"

#include <QtCore/QFile>

namespace Meerkat
{

class OperaBookmarksImporter : public BookmarksImporter
{
	Q_OBJECT

public:
	explicit OperaBookmarksImporter(QObject *parent = nullptr);
	~OperaBookmarksImporter();

	QWidget* getOptionsWidget();
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getSuggestedPath(const QString &path = QString()) const;
	QString getBrowser() const;
	QUrl getHomePage() const;
	QIcon getIcon() const;
	QStringList getFileFilters() const;

public slots:
	bool import(const QString &path);

protected:
	enum OperaBookmarkEntry
	{
		NoEntry = 0,
		UrlEntry = 1,
		FolderStartEntry = 2,
		FolderEndEntry = 3,
		SeparatorEntry = 4
	};

private:
	BookmarksImporterWidget *m_optionsWidget;
};

}

#endif
