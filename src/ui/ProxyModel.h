/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_PROXYMODEL_H
#define MEERKAT_PROXYMODEL_H

#include <QtCore/QIdentityProxyModel>
#include <QtGui/QStandardItemModel>

namespace Meerkat
{

class ProxyModel : public QIdentityProxyModel
{
	Q_OBJECT

public:
	explicit ProxyModel(QStandardItemModel *model, const QList<QPair<QString, int> > &mapping, QObject *parent = NULL);

	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex sibling(int row, int column, const QModelIndex &index) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	int columnCount(const QModelIndex &parent) const;
	int rowCount(const QModelIndex &parent) const;

private:
	QStandardItemModel *m_model;
	QList<QPair<QString, int> > m_mapping;
};

}

#endif
