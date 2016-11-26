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

#include "BookmarksModel.h"
#include "Console.h"
#include "HistoryManager.h"
#include "SessionsManager.h"
#include "ThemesManager.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtCore/QSaveFile>
#include <QtWidgets/QMessageBox>

namespace Meerkat
{

BookmarksItem::BookmarksItem() : QStandardItem()
{
}

void BookmarksItem::remove()
{
	BookmarksModel *model(qobject_cast<BookmarksModel*>(this->model()));

	if (model)
	{
		model->removeBookmark(this);
	}
	else
	{
		delete this;
	}
}

void BookmarksItem::setData(const QVariant &value, int role)
{
	if (model() && qobject_cast<BookmarksModel*>(model()))
	{
		model()->setData(index(), value, role);
	}
	else
	{
		QStandardItem::setData(value, role);
	}
}

void BookmarksItem::setItemData(const QVariant &value, int role)
{
	QStandardItem::setData(value, role);
}

QStandardItem* BookmarksItem::clone() const
{
	BookmarksItem *item(new BookmarksItem());
	item->setData(data(BookmarksModel::TypeRole), BookmarksModel::TypeRole);
	item->setData(data(BookmarksModel::UrlRole), BookmarksModel::UrlRole);
	item->setData(data(BookmarksModel::TitleRole), BookmarksModel::TitleRole);
	item->setData(data(BookmarksModel::DescriptionRole), BookmarksModel::DescriptionRole);
	item->setData(data(BookmarksModel::KeywordRole), BookmarksModel::KeywordRole);
	item->setData(data(BookmarksModel::TimeAddedRole), BookmarksModel::TimeAddedRole);
	item->setData(data(BookmarksModel::TimeModifiedRole), BookmarksModel::TimeModifiedRole);
	item->setData(data(BookmarksModel::TimeVisitedRole), BookmarksModel::TimeVisitedRole);
	item->setData(data(BookmarksModel::VisitsRole), BookmarksModel::VisitsRole);

	return item;
}

QVariant BookmarksItem::data(int role) const
{
	if (role == Qt::DisplayRole)
	{
		const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(data(BookmarksModel::TypeRole).toInt()));

		if (type == BookmarksModel::RootBookmark)
		{
			BookmarksModel *model(qobject_cast<BookmarksModel*>(this->model()));

			if (model && model->getFormatMode() == BookmarksModel::NotesMode)
			{
				return QCoreApplication::translate("Meerkat::BookmarksModel", "Notes");
			}

			return QCoreApplication::translate("Meerkat::BookmarksModel", "Bookmarks");
		}

		if (type == BookmarksModel::TrashBookmark)
		{
			return QCoreApplication::translate("Meerkat::BookmarksModel", "Trash");
		}
	}

	if (role == Qt::DecorationRole)
	{
		const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(data(BookmarksModel::TypeRole).toInt()));

		if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark)
		{
			return ThemesManager::getIcon(QLatin1String("inode-directory"));
		}

		if (type == BookmarksModel::TrashBookmark)
		{
			return ThemesManager::getIcon(QLatin1String("user-trash"));
		}

		if (type == BookmarksModel::UrlBookmark)
		{
			return HistoryManager::getIcon(data(BookmarksModel::UrlRole).toUrl());
		}

		return QVariant();
	}

	if (role == Qt::AccessibleDescriptionRole && static_cast<BookmarksModel::BookmarkType>(data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::SeparatorBookmark)
	{
		return QLatin1String("separator");
	}

	if (role == BookmarksModel::IsTrashedRole)
	{
		QModelIndex parent(index().parent());

		while (parent.isValid())
		{
			const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt()));

			if (type == BookmarksModel::TrashBookmark)
			{
				return true;
			}

			if (type == BookmarksModel::RootBookmark)
			{
				break;
			}

			parent = parent.parent();
		}

		return false;
	}

	return QStandardItem::data(role);
}

QList<QUrl> BookmarksItem::getUrls() const
{
	QList<QUrl> urls;

	if (static_cast<BookmarksModel::BookmarkType>(data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
	{
		urls.append(data(BookmarksModel::UrlRole).toUrl());
	}

	for (int i = 0; i < rowCount(); ++i)
	{
		BookmarksItem *bookmark(dynamic_cast<BookmarksItem*>(child(i, 0)));

		if (!bookmark)
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()));

		if (type == BookmarksModel::FolderBookmark)
		{
			urls.append(bookmark->getUrls());
		}
		else if (type == BookmarksModel::UrlBookmark)
		{
			urls.append(bookmark->data(BookmarksModel::UrlRole).toUrl());
		}
	}

	return urls;
}

BookmarksModel::BookmarksModel(const QString &path, FormatMode mode, QObject *parent) : QStandardItemModel(parent),
	m_rootItem(new BookmarksItem()),
	m_trashItem(new BookmarksItem()),
	m_mode(mode)
{
	m_rootItem->setData(RootBookmark, TypeRole);
	m_rootItem->setData(((mode == NotesMode) ? tr("Notes") : tr("Bookmarks")), TitleRole);
	m_rootItem->setDragEnabled(false);
	m_trashItem->setData(TrashBookmark, TypeRole);
	m_trashItem->setData(tr("Trash"), TitleRole);
	m_trashItem->setDragEnabled(false);
	m_trashItem->setEnabled(false);

	appendRow(m_rootItem);
	appendRow(m_trashItem);
	setItemPrototype(new BookmarksItem());

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(((mode == NotesMode) ? tr("Failed to open notes file: %1") : tr("Failed to open bookmarks file: %1")).arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, path);

		return;
	}

	QXmlStreamReader reader(file.readAll());

	if (reader.readNextStartElement() && reader.name() == QLatin1String("xbel") && reader.attributes().value(QLatin1String("version")).toString() == QLatin1String("1.0"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("folder") || reader.name() == QLatin1String("bookmark") || reader.name() == QLatin1String("separator"))
			{
				readBookmark(&reader, m_rootItem);
			}
			else
			{
				reader.skipCurrentElement();
			}

			if (reader.hasError())
			{
				getRootItem()->removeRows(0, getRootItem()->rowCount());

				Console::addMessage(((m_mode == NotesMode) ? tr("Failed to load notes file: %1") : tr("Failed to load bookmarks file: %1")).arg(reader.errorString()), Console::OtherCategory, Console::ErrorLevel, path);

				QMessageBox::warning(nullptr, tr("Error"), ((m_mode == NotesMode) ? tr("Failed to load notes file.") : tr("Failed to load bookmarks file.")), QMessageBox::Close);

				return;
			}
		}
	}

	connect(this, SIGNAL(itemChanged(QStandardItem*)), this, SIGNAL(modelModified()));
	connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SIGNAL(modelModified()));
	connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(notifyBookmarkModified(QModelIndex)));
	connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SIGNAL(modelModified()));
	connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(notifyBookmarkModified(QModelIndex)));
	connect(this, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SIGNAL(modelModified()));
}

void BookmarksModel::trashBookmark(BookmarksItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	const BookmarkType type(static_cast<BookmarkType>(bookmark->data(TypeRole).toInt()));

	if (type != RootBookmark && type != TrashBookmark)
	{
		if (type == SeparatorBookmark || bookmark->data(IsTrashedRole).toBool())
		{
			bookmark->remove();
		}
		else
		{
			BookmarksItem *trashItem(getTrashItem());

			m_trash[bookmark] = qMakePair(bookmark->parent()->index(), bookmark->row());

			trashItem->appendRow(bookmark->parent()->takeRow(bookmark->row()));
			trashItem->setEnabled(true);

			removeBookmarkUrl(bookmark);

			emit bookmarkModified(bookmark);
			emit bookmarkTrashed(bookmark);
			emit modelModified();
		}
	}
}

void BookmarksModel::restoreBookmark(BookmarksItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	BookmarksItem *formerParent(m_trash.contains(bookmark) ? getBookmark(m_trash[bookmark].first) : getRootItem());

	if (!formerParent || static_cast<BookmarkType>(formerParent->data(TypeRole).toInt()) != FolderBookmark)
	{
		formerParent = getRootItem();
	}

	if (m_trash.contains(bookmark))
	{
		formerParent->insertRow(m_trash[bookmark].second, bookmark->parent()->takeRow(bookmark->row()));

		m_trash.remove(bookmark);
	}
	else
	{
		formerParent->appendRow(bookmark->parent()->takeRow(bookmark->row()));
	}

	readdBookmarkUrl(bookmark);

	BookmarksItem *trashItem(getTrashItem());

	trashItem->setEnabled(trashItem->rowCount() > 0);

	emit bookmarkModified(bookmark);
	emit bookmarkRestored(bookmark);
	emit modelModified();
}

void BookmarksModel::removeBookmark(BookmarksItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	removeBookmarkUrl(bookmark);

	const quint64 identifier(bookmark->data(IdentifierRole).toULongLong());

	if (identifier > 0 && m_identifiers.contains(identifier))
	{
		m_identifiers.remove(identifier);
	}

	if (!bookmark->data(KeywordRole).toString().isEmpty() && m_keywords.contains(bookmark->data(KeywordRole).toString()))
	{
		m_keywords.remove(bookmark->data(KeywordRole).toString());
	}

	emit bookmarkRemoved(bookmark);

	bookmark->parent()->removeRow(bookmark->row());

	emit modelModified();
}

void BookmarksModel::readBookmark(QXmlStreamReader *reader, BookmarksItem *parent)
{
	BookmarksItem *bookmark(nullptr);

	if (reader->name() == QLatin1String("folder"))
	{
		bookmark = addBookmark(FolderBookmark, reader->attributes().value(QLatin1String("id")).toULongLong(), QUrl(), QString(), parent);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate), TimeAddedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate), TimeModifiedRole);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setData(reader->readElementText().trimmed(), TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setData(reader->readElementText().trimmed(), DescriptionRole);
				}
				else if (reader->name() == QLatin1String("folder") || reader->name() == QLatin1String("bookmark") || reader->name() == QLatin1String("separator"))
				{
					readBookmark(reader, bookmark);
				}
				else if (reader->name() == QLatin1String("info"))
				{
					while (reader->readNext())
					{
						if (reader->isStartElement())
						{
                            if (reader->name() == QLatin1String("metadata") && reader->attributes().value(QLatin1String("owner")).toString().startsWith(QLatin1String("https://www.meerkat.tk/")))
							{
								while (reader->readNext())
								{
									if (reader->isStartElement())
									{
										if (reader->name() == QLatin1String("keyword"))
										{
											bookmark->setData(reader->readElementText().trimmed(), KeywordRole);
										}
										else
										{
											reader->skipCurrentElement();
										}
									}
									else if (reader->isEndElement() && reader->name() == QLatin1String("metadata"))
									{
										break;
									}
								}
							}
							else
							{
								reader->skipCurrentElement();
							}
						}
						else if (reader->isEndElement() && reader->name() == QLatin1String("info"))
						{
							break;
						}
					}
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == QLatin1String("folder"))
			{
				break;
			}
			else if (reader->hasError())
			{
				return;
			}
		}
	}
	else if (reader->name() == QLatin1String("bookmark"))
	{
		bookmark = addBookmark(UrlBookmark,reader->attributes().value(QLatin1String("id")).toULongLong(), reader->attributes().value(QLatin1String("href")).toString(), QString(), parent);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate), TimeAddedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate), TimeModifiedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("visited")).toString(), Qt::ISODate), TimeVisitedRole);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setData(reader->readElementText().trimmed(), TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setData(reader->readElementText().trimmed(), DescriptionRole);
				}
				else if (reader->name() == QLatin1String("info"))
				{
					while (reader->readNext())
					{
						if (reader->isStartElement())
						{
                            if (reader->name() == QLatin1String("metadata") && reader->attributes().value(QLatin1String("owner")).toString().startsWith("https://www.meerkat.tk/"))
							{
								while (reader->readNext())
								{
									if (reader->isStartElement())
									{
										if (reader->name() == QLatin1String("keyword"))
										{
											bookmark->setData(reader->readElementText().trimmed(), KeywordRole);
										}
										else if (reader->name() == QLatin1String("visits"))
										{
											bookmark->setData(reader->readElementText().toInt(), VisitsRole);
										}
										else
										{
											reader->skipCurrentElement();
										}
									}
									else if (reader->isEndElement() && reader->name() == QLatin1String("metadata"))
									{
										break;
									}
								}
							}
							else
							{
								reader->skipCurrentElement();
							}
						}
						else if (reader->isEndElement() && reader->name() == QLatin1String("info"))
						{
							break;
						}
					}
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == QLatin1String("bookmark"))
			{
				break;
			}
			else if (reader->hasError())
			{
				return;
			}
		}
	}
	else if (reader->name() == QLatin1String("separator"))
	{
		addBookmark(SeparatorBookmark, 0, QUrl(), QString(), parent);

		reader->readNext();
	}
}

void BookmarksModel::writeBookmark(QXmlStreamWriter *writer, QStandardItem *bookmark) const
{
	if (!bookmark)
	{
		return;
	}

	switch (static_cast<BookmarkType>(bookmark->data(TypeRole).toInt()))
	{
		case FolderBookmark:
			writer->writeStartElement(QLatin1String("folder"));
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->data(IdentifierRole).toULongLong()));

			if (bookmark->data(TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->data(TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->data(TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->data(TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			writer->writeTextElement(QLatin1String("title"), bookmark->data(TitleRole).toString());

			if (!bookmark->data(DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->data(DescriptionRole).toString());
			}

			if (m_mode == BookmarksMode && !bookmark->data(KeywordRole).toString().isEmpty())
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
                writer->writeAttribute(QLatin1String("owner"), QLatin1String("https://www.meerkat.tk/meerkat-xbel-bookmark"));
				writer->writeTextElement(QLatin1String("keyword"), bookmark->data(KeywordRole).toString());
				writer->writeEndElement();
				writer->writeEndElement();
			}

			for (int i = 0; i < bookmark->rowCount(); ++i)
			{
				writeBookmark(writer, bookmark->child(i, 0));
			}

			writer->writeEndElement();

			break;
		case UrlBookmark:
			writer->writeStartElement(QLatin1String("bookmark"));
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->data(IdentifierRole).toULongLong()));

			if (!bookmark->data(UrlRole).toString().isEmpty())
			{
				writer->writeAttribute(QLatin1String("href"), bookmark->data(UrlRole).toString());
			}

			if (bookmark->data(TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->data(TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->data(TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->data(TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			if (m_mode != NotesMode)
			{
				if (bookmark->data(TimeVisitedRole).toDateTime().isValid())
				{
					writer->writeAttribute(QLatin1String("visited"), bookmark->data(TimeVisitedRole).toDateTime().toString(Qt::ISODate));
				}

				writer->writeTextElement(QLatin1String("title"), bookmark->data(TitleRole).toString());
			}

			if (!bookmark->data(DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->data(DescriptionRole).toString());
			}

			if (m_mode == BookmarksMode && (!bookmark->data(KeywordRole).toString().isEmpty() || bookmark->data(VisitsRole).toInt() > 0))
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
                writer->writeAttribute(QLatin1String("owner"), QLatin1String("https://www.meerkat.tk/meerkat-xbel-bookmark"));

				if (!bookmark->data(KeywordRole).toString().isEmpty())
				{
					writer->writeTextElement(QLatin1String("keyword"), bookmark->data(KeywordRole).toString());
				}

				if (bookmark->data(VisitsRole).toInt() > 0)
				{
					writer->writeTextElement(QLatin1String("visits"), QString::number(bookmark->data(VisitsRole).toInt()));
				}

				writer->writeEndElement();
				writer->writeEndElement();
			}

			writer->writeEndElement();

			break;
		default:
			writer->writeEmptyElement(QLatin1String("separator"));

			break;
	}
}

void BookmarksModel::removeBookmarkUrl(BookmarksItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	if (static_cast<BookmarkType>(bookmark->data(TypeRole).toInt()) == FolderBookmark)
	{
		for (int i = 0; i < bookmark->rowCount(); ++i)
		{
			removeBookmarkUrl(dynamic_cast<BookmarksItem*>(bookmark->child(i, 0)));
		}
	}
	else if (!bookmark->data(UrlRole).toUrl().isEmpty())
	{
		const QUrl url(Utils::normalizeUrl(bookmark->data(UrlRole).toUrl()));

		if (m_urls.contains(url))
		{
			m_urls[url].removeAll(bookmark);

			if (m_urls[url].isEmpty())
			{
				m_urls.remove(url);
			}
		}
	}
}

void BookmarksModel::readdBookmarkUrl(BookmarksItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	if (static_cast<BookmarkType>(bookmark->data(TypeRole).toInt()) == FolderBookmark)
	{
		for (int i = 0; i < bookmark->rowCount(); ++i)
		{
			readdBookmarkUrl(dynamic_cast<BookmarksItem*>(bookmark->child(i, 0)));
		}
	}
	else if (!bookmark->data(UrlRole).toUrl().isEmpty())
	{
		const QUrl url(Utils::normalizeUrl(bookmark->data(UrlRole).toUrl()));

		if (!m_urls.contains(url))
		{
			m_urls[url] = QList<BookmarksItem*>();
		}

		m_urls[url].append(bookmark);
	}
}

void BookmarksModel::emptyTrash()
{
	BookmarksItem *trashItem(getTrashItem());
	trashItem->removeRows(0, trashItem->rowCount());
	trashItem->setEnabled(false);

	m_trash.clear();

	emit modelModified();
}

void BookmarksModel::notifyBookmarkModified(const QModelIndex &index)
{
	BookmarksItem *bookmark(getBookmark(index));

	if (bookmark)
	{
		emit bookmarkModified(bookmark);
	}
}

BookmarksItem* BookmarksModel::addBookmark(BookmarkType type, quint64 identifier, const QUrl &url, const QString &title, BookmarksItem *parent, int index)
{
	BookmarksItem *bookmark(new BookmarksItem());

	if (parent)
	{
		parent->insertRow(((index < 0) ? parent->rowCount() : index), bookmark);
	}
	else
	{
		getRootItem()->insertRow(((index < 0) ? getRootItem()->rowCount() : index), bookmark);
	}

	if (type == UrlBookmark || type == SeparatorBookmark)
	{
		bookmark->setDropEnabled(false);
	}

	setData(bookmark->index(), type, TypeRole);
	setData(bookmark->index(), url, UrlRole);
	setData(bookmark->index(), title, TitleRole);

	if (type != RootBookmark && type != TrashBookmark && type != FolderBookmark)
	{
		bookmark->setFlags(bookmark->flags() | Qt::ItemNeverHasChildren);
	}

	if (type != TrashBookmark && type != UnknownBookmark)
	{
		if (identifier == 0 || m_identifiers.contains(identifier))
		{
			identifier = (m_identifiers.isEmpty() ? 1 : (m_identifiers.keys().last() + 1));
		}

		setData(bookmark->index(), identifier, IdentifierRole);

		m_identifiers[identifier] = bookmark;
	}

	emit bookmarkAdded(bookmark);
	emit modelModified();

	return bookmark;
}

BookmarksItem* BookmarksModel::getBookmark(const QString &keyword) const
{
	if (m_keywords.contains(keyword))
	{
		return m_keywords[keyword];
	}

	return nullptr;
}

BookmarksItem* BookmarksModel::getBookmark(const QModelIndex &index) const
{
	return getBookmark(index.data(IdentifierRole).toULongLong());
}

BookmarksItem* BookmarksModel::getBookmark(quint64 identifier) const
{
	if (identifier == 0)
	{
		return getRootItem();
	}

	if (m_identifiers.contains(identifier))
	{
		return m_identifiers[identifier];
	}

	return nullptr;
}

BookmarksItem* BookmarksModel::getRootItem() const
{
	return m_rootItem;
}

BookmarksItem* BookmarksModel::getTrashItem() const
{
	return m_trashItem;
}

BookmarksItem* BookmarksModel::getItem(const QString &path) const
{
	if (path == QLatin1String("/"))
	{
		return getRootItem();
	}

	if (path.startsWith(QLatin1Char('#')))
	{
		return getBookmark(path.mid(1).toULongLong());
	}

	QStandardItem *item(getRootItem());
	const QStringList directories(path.split(QLatin1Char('/'), QString::SkipEmptyParts));

	for (int i = 0; i < directories.count(); ++i)
	{
		bool hasFound(false);

		for (int j = 0; j < item->rowCount(); ++j)
		{
			if (item->child(j) && item->child(j)->data(Qt::DisplayRole) == directories.at(i))
			{
				item = item->child(j);

				hasFound = true;

				break;
			}
		}

		if (!hasFound)
		{
			return nullptr;
		}
	}

	return dynamic_cast<BookmarksItem*>(item);
}

QMimeData* BookmarksModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData(new QMimeData());
	QStringList texts;
	QList<QUrl> urls;

	if (indexes.count() == 1)
	{
		mimeData->setProperty("x-item-index", indexes.at(0));
	}

	for (int i = 0; i < indexes.count(); ++i)
	{
		if (indexes.at(i).isValid() && static_cast<BookmarkType>(indexes.at(i).data(TypeRole).toInt()) == UrlBookmark)
		{
			texts.append(indexes.at(i).data(UrlRole).toString());
			urls.append(indexes.at(i).data(UrlRole).toUrl());
		}
	}

	mimeData->setText(texts.join(QLatin1String(", ")));
	mimeData->setUrls(urls);

	return mimeData;
}

QStringList BookmarksModel::mimeTypes() const
{
	return QStringList(QLatin1String("text/uri-list"));
}

QStringList BookmarksModel::getKeywords() const
{
	return m_keywords.keys();
}

QList<BookmarksModel::BookmarkMatch> BookmarksModel::findBookmarks(const QString &prefix) const
{
	QList<BookmarksItem*> matchedBookmarks;
	QList<BookmarksModel::BookmarkMatch> allMatches;
	QList<BookmarksModel::BookmarkMatch> currentMatches;
	QMultiMap<QDateTime, BookmarksModel::BookmarkMatch> matchesMap;
	QHash<QString, BookmarksItem*>::const_iterator keywordsIterator;

	for (keywordsIterator = m_keywords.constBegin(); keywordsIterator != m_keywords.constEnd(); ++keywordsIterator)
	{
		if (keywordsIterator.key().startsWith(prefix, Qt::CaseInsensitive))
		{
			BookmarksModel::BookmarkMatch match;
			match.bookmark = keywordsIterator.value();
			match.match = keywordsIterator.key();

			matchesMap.insert(match.bookmark->data(TimeVisitedRole).toDateTime(), match);

			matchedBookmarks.append(match.bookmark);
		}
	}

	currentMatches = matchesMap.values();

	matchesMap.clear();

	for (int i = (currentMatches.count() - 1); i >= 0; --i)
	{
		allMatches.append(currentMatches.at(i));
	}

	QHash<QUrl, QList<BookmarksItem*> >::const_iterator urlsIterator;

	for (urlsIterator = m_urls.constBegin(); urlsIterator != m_urls.constEnd(); ++urlsIterator)
	{
		if (urlsIterator.value().isEmpty() || matchedBookmarks.contains(urlsIterator.value().first()))
		{
			continue;
		}

		const QString result(Utils::matchUrl(urlsIterator.key(), prefix));

		if (!result.isEmpty())
		{
			BookmarkMatch match;
			match.bookmark = urlsIterator.value().first();
			match.match = result;

			matchesMap.insert(match.bookmark->data(TimeVisitedRole).toDateTime(), match);

			matchedBookmarks.append(match.bookmark);
		}
	}

	currentMatches = matchesMap.values();

	matchesMap.clear();

	for (int i = (currentMatches.count() - 1); i >= 0; --i)
	{
		allMatches.append(currentMatches.at(i));
	}

	return allMatches;
}

QList<BookmarksItem*> BookmarksModel::findUrls(const QUrl &url, QStandardItem *branch) const
{
	if (!branch)
	{
		branch = item(0, 0);
	}

	QList<BookmarksItem*> items;

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		BookmarksItem *item(dynamic_cast<BookmarksItem*>(branch->child(i)));

		if (item)
		{
			const BookmarkType type(static_cast<BookmarkType>(item->data(TypeRole).toInt()));

			if (type == FolderBookmark)
			{
				items.append(findUrls(url, item));
			}
			else if (type == UrlBookmark && item->data(UrlRole).toUrl() == url)
			{
				items.append(item);
			}
		}
	}

	return items;
}

QList<BookmarksItem*> BookmarksModel::getBookmarks(const QUrl &url) const
{
	const QUrl adjustedUrl(Utils::normalizeUrl(url));

	if (m_urls.contains(adjustedUrl))
	{
		return m_urls[adjustedUrl];
	}

	return QList<BookmarksItem*>();
}

BookmarksModel::FormatMode BookmarksModel::getFormatMode() const
{
	return m_mode;
}

bool BookmarksModel::moveBookmark(BookmarksItem *bookmark, BookmarksItem *newParent, int newRow)
{
	if (!bookmark || !newParent)
	{
		return false;
	}

	BookmarksItem *previousParent(dynamic_cast<BookmarksItem*>(bookmark->parent()));

	if (!previousParent)
	{
		if (newRow < 0)
		{
			newParent->appendRow(bookmark);
		}
		else
		{
			newParent->insertRow(newRow, bookmark);
		}

		emit modelModified();

		return true;
	}

	const int previousRow(bookmark->row());

	if (newRow < 0)
	{
		newParent->appendRow(bookmark->parent()->takeRow(bookmark->row()));

		emit bookmarkMoved(bookmark, previousParent, previousRow);
		emit modelModified();

		return true;
	}

	int targetRow(newRow);

	if (bookmark->parent() == newParent && bookmark->row() < newRow)
	{
		--targetRow;
	}

	newParent->insertRow(targetRow, bookmark->parent()->takeRow(bookmark->row()));

	emit bookmarkMoved(bookmark, previousParent, previousRow);
	emit modelModified();

	return true;
}

bool BookmarksModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	const BookmarkType type(static_cast<BookmarkType>(parent.data(TypeRole).toInt()));

	if (type == FolderBookmark || type == RootBookmark || type == TrashBookmark)
	{
		const QModelIndex index(data->property("x-item-index").toModelIndex());

		if (index.isValid())
		{
			return moveBookmark(getBookmark(index), getBookmark(parent), row);
		}

		if (data->hasUrls())
		{
			const QList<QUrl> urls(data->urls());

			for (int i = 0; i < urls.count(); ++i)
			{
				addBookmark(UrlBookmark, 0, urls.at(i), (data->property("x-url-title").toString().isEmpty() ? urls.at(i).toString() : data->property("x-url-title").toString()), getBookmark(parent), row);
			}

			return true;
		}

		return QStandardItemModel::dropMimeData(data, action, row, column, parent);
	}

	return false;
}

bool BookmarksModel::save(const QString &path) const
{
	if (SessionsManager::isReadOnly())
	{
		return false;
	}

	QSaveFile file(path);

	if (!file.open(QIODevice::WriteOnly))
	{
		return false;
	}

	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	writer.setAutoFormattingIndent(-1);
	writer.writeStartDocument();
	writer.writeDTD(QLatin1String("<!DOCTYPE xbel>"));
	writer.writeStartElement(QLatin1String("xbel"));
	writer.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));

	QStandardItem *rootItem(item(0, 0));

	for (int i = 0; i < rootItem->rowCount(); ++i)
	{
		writeBookmark(&writer, rootItem->child(i, 0));
	}

	writer.writeEndDocument();

	return file.commit();
}

bool BookmarksModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	BookmarksItem *bookmark(dynamic_cast<BookmarksItem*>(itemFromIndex(index)));

	if (!bookmark)
	{
		return QStandardItemModel::setData(index, value, role);
	}

	if (role == UrlRole && value.toUrl() != index.data(UrlRole).toUrl())
	{
		const QUrl oldUrl(Utils::normalizeUrl(index.data(UrlRole).toUrl()));
		const QUrl newUrl(Utils::normalizeUrl(value.toUrl()));

		if (!oldUrl.isEmpty() && m_urls.contains(oldUrl))
		{
			m_urls[oldUrl].removeAll(bookmark);

			if (m_urls[oldUrl].isEmpty())
			{
				m_urls.remove(oldUrl);
			}
		}

		if (!newUrl.isEmpty())
		{
			if (!m_urls.contains(newUrl))
			{
				m_urls[newUrl] = QList<BookmarksItem*>();
			}

			m_urls[newUrl].append(bookmark);
		}
	}
	else if (role == KeywordRole && value.toString() != index.data(KeywordRole).toString())
	{
		const QString oldKeyword(index.data(KeywordRole).toString());
		const QString newKeyword(value.toString());

		if (!oldKeyword.isEmpty() && m_keywords.contains(oldKeyword))
		{
			m_keywords.remove(oldKeyword);
		}

		if (!newKeyword.isEmpty())
		{
			m_keywords[newKeyword] = bookmark;
		}
	}
	else if (m_mode == NotesMode && role == DescriptionRole)
	{
		const QString title(value.toString().section(QLatin1Char('\n'), 0, 0).left(100));

		setData(index, ((title == value.toString().trimmed()) ? title : title + QStringLiteral("…")), TitleRole);
	}

	bookmark->setItemData(value, role);

	switch (role)
	{
		case TitleRole:
		case UrlRole:
		case DescriptionRole:
		case IdentifierRole:
		case TypeRole:
		case KeywordRole:
		case TimeAddedRole:
		case TimeModifiedRole:
		case TimeVisitedRole:
		case VisitsRole:
			emit bookmarkModified(bookmark);
			emit modelModified();

			break;
	}

	return true;
}

bool BookmarksModel::hasBookmark(const QUrl &url) const
{
	return m_urls.contains(Utils::normalizeUrl(url));
}

bool BookmarksModel::hasKeyword(const QString &keyword) const
{
	return m_keywords.contains(keyword);
}

}
