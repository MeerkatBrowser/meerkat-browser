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

#include "ItemDelegate.h"

#include <QtWidgets/QApplication>

namespace Meerkat
{

ItemDelegate::ItemDelegate(bool forceIcon, QObject *parent) : QItemDelegate(parent),
	m_forceIcon(forceIcon)
{
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	drawBackground(painter, option, index);

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		QStyleOptionFrame frameOption;
		frameOption.palette = option.palette;
		frameOption.rect = option.rect;
		frameOption.state = option.state;
		frameOption.frameShape = QFrame::HLine;

		QApplication::style()->drawControl(QStyle::CE_ShapedFrame, &frameOption, painter, 0);

		return;
	}

	QStyleOptionViewItem mutableOption(option);

	if (index.data(Qt::FontRole).isValid())
	{
		mutableOption.font = index.data(Qt::FontRole).value<QFont>();
	}

	if (m_forceIcon && index.column() == 0)
	{
		if (!index.data(Qt::DecorationRole).value<QIcon>().isNull())
		{
			QRect decorationRectangle(option.rect);
			decorationRectangle.setRight(option.rect.left() + option.rect.height());
			decorationRectangle = decorationRectangle.marginsRemoved(QMargins(1, 1, 1, 1));

			index.data(Qt::DecorationRole).value<QIcon>().paint(painter, decorationRectangle);
		}

		QRect titleRectangle(option.rect);
		titleRectangle.setLeft(option.rect.left() + option.rect.height());

		drawDisplay(painter, mutableOption, titleRectangle, index.data(Qt::DisplayRole).toString());
	}
	else
	{
		drawDisplay(painter, mutableOption, mutableOption.rect, index.data(Qt::DisplayRole).toString());
	}
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.data(Qt::FontRole).isValid())
	{
		size.setHeight(QFontMetrics(index.data(Qt::FontRole).value<QFont>()).height() * 1.25);
	}
	else
	{
		size.setHeight(option.fontMetrics.height() * 1.25);
	}

	return size;
}

}
