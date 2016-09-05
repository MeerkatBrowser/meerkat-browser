/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "FileSystemCompleterModel.h"

namespace Meerkat
{

FileSystemCompleterModel::FileSystemCompleterModel(QObject *parent) : QFileSystemModel(parent)
{
	setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
	setRootPath(QString());
}

QVariant FileSystemCompleterModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0)
	{
		return QDir::toNativeSeparators(filePath(index));
	}

	return QFileSystemModel::data(index, role);
}

}
