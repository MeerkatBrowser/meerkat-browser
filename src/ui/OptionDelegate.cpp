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

#include "OptionDelegate.h"
#include "../core/SettingsManager.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QPainter>

namespace Meerkat
{

OptionDelegate::OptionDelegate(bool isSimple, QObject *parent) : QItemDelegate(parent),
	m_isSimple(isSimple)
{
}

void OptionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const SettingsManager::OptionType type(SettingsManager::getOptionDefinition(SettingsManager::getOptionIdentifier(index.data(Qt::UserRole).toString())).type);

	if (type == SettingsManager::UnknownType)
	{
		QItemDelegate::paint(painter, option, index);

		return;
	}

	drawBackground(painter, option, index);

	switch (type)
	{
		case SettingsManager::BooleanType:
			drawDisplay(painter, option, option.rect, index.data(Qt::DisplayRole).toBool() ? QCoreApplication::translate("Otter::OptionDelegate", "Yes") : QCoreApplication::translate("Otter::OptionDelegate", "No"));

			break;
		case SettingsManager::ColorType:
			{
				const QString color(index.data(Qt::DisplayRole).toString());

				if (!color.isEmpty())
				{
					painter->fillRect(option.rect, QColor(color));
				}

				QStyleOptionViewItem mutableOption(option);
				mutableOption.displayAlignment = Qt::AlignCenter;
				mutableOption.palette.setColor(QPalette::Text, Qt::white);

				drawDisplay(painter, mutableOption, option.rect, color);

				mutableOption.palette.setColor(QPalette::Text, Qt::black);

				drawDisplay(painter, mutableOption, option.rect.adjusted(-1, -1, -1, -1), color);

				break;
			}
		case SettingsManager::FontType:
			{
				const QString font(index.data(Qt::DisplayRole).toString());
				QStyleOptionViewItem mutableOption(option);
				mutableOption.font = QFont(font);

				drawDisplay(painter, mutableOption, option.rect, font);

				break;
			}
		default:
			drawDisplay(painter, option, option.rect, index.data(Qt::DisplayRole).toString());

			break;
	}
}

void OptionDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void OptionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (m_isSimple)
	{
		OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

		if (widget)
		{
			model->setData(index, widget->getValue());
		}
	}
}

QWidget* OptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	const QString name(index.data(Qt::UserRole).toString());
	const int identifier(SettingsManager::getOptionIdentifier(name));
	const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(identifier));
	QVariant value(index.data(Qt::EditRole));

	if (value.isNull())
	{
		value = SettingsManager::getValue(identifier);
	}

	OptionWidget *widget(new OptionWidget(name, value, definition.type, parent));
	widget->setIndex(index);
	widget->setControlsVisible(!m_isSimple);

	if (definition.type == SettingsManager::EnumerationType)
	{
		widget->setChoices(definition.choices);
	}

	connect(widget, SIGNAL(commitData(QWidget*)), this, SIGNAL(commitData(QWidget*)));

	return widget;
}

QSize OptionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());
	size.setHeight(option.fontMetrics.height() * 1.25);

	return size;
}

}
