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

#ifndef MEERKAT_BOOKMARKSMODEL_H
#define MEERKAT_BOOKMARKSMODEL_H

#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QStandardItemModel>

namespace Meerkat
{

class BookmarksItem : public QStandardItem
{
public:
	void remove();
	void setData(const QVariant &value, int role) override;
	void setItemData(const QVariant &value, int role);
	QStandardItem* clone() const override;
	QVariant data(int role) const override;
	QList<QUrl> getUrls() const;

protected:
	explicit BookmarksItem();

friend class BookmarksModel;
};

class BookmarksModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum BookmarkType
	{
		UnknownBookmark = 0,
		RootBookmark = 1,
		TrashBookmark = 2,
		FolderBookmark = 3,
		UrlBookmark = 4,
		SeparatorBookmark = 5
	};

	enum BookmarkRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		DescriptionRole = Qt::ToolTipRole,
		IdentifierRole = Qt::UserRole,
		TypeRole = (Qt::UserRole + 1),
		KeywordRole = (Qt::UserRole + 2),
		TimeAddedRole = (Qt::UserRole + 3),
		TimeModifiedRole = (Qt::UserRole + 4),
		TimeVisitedRole = (Qt::UserRole + 5),
		VisitsRole = (Qt::UserRole + 6),
		IsTrashedRole = (Qt::UserRole + 7),
		UserRole
	};

	enum FormatMode
	{
		BookmarksMode = 0,
		NotesMode = 1
	};

	struct BookmarkMatch
	{
		BookmarksItem *bookmark = nullptr;
		QString match;
	};

	explicit BookmarksModel(const QString &path, FormatMode mode, QObject *parent = nullptr);

	void trashBookmark(BookmarksItem *bookmark);
	void restoreBookmark(BookmarksItem *bookmark);
	void removeBookmark(BookmarksItem *bookmark);
	BookmarksItem* addBookmark(BookmarkType type, quint64 identifier = 0, const QUrl &url = QUrl(), const QString &title = QString(), BookmarksItem *parent = nullptr, int index = -1);
	BookmarksItem* getBookmark(const QString &keyword) const;
	BookmarksItem* getBookmark(const QModelIndex &index) const;
	BookmarksItem* getBookmark(quint64 identifier) const;
	BookmarksItem* getRootItem() const;
	BookmarksItem* getTrashItem() const;
	BookmarksItem* getItem(const QString &path) const;
	QMimeData* mimeData(const QModelIndexList &indexes) const override;
	QStringList mimeTypes() const override;
	QStringList getKeywords() const;
	QList<BookmarkMatch> findBookmarks(const QString &prefix) const;
	QList<BookmarksItem*> findUrls(const QUrl &url, QStandardItem *branch = nullptr) const;
	QList<BookmarksItem*> getBookmarks(const QUrl &url) const;
	FormatMode getFormatMode() const;
	bool moveBookmark(BookmarksItem *bookmark, BookmarksItem *newParent, int newRow = -1);
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	bool save(const QString &path) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	bool hasBookmark(const QUrl &url) const;
	bool hasKeyword(const QString &keyword) const;

public slots:
	void emptyTrash();

protected:
	void readBookmark(QXmlStreamReader *reader, BookmarksItem *parent);
	void writeBookmark(QXmlStreamWriter *writer, QStandardItem *bookmark) const;
	void removeBookmarkUrl(BookmarksItem *bookmark);
	void readdBookmarkUrl(BookmarksItem *bookmark);

protected slots:
	void notifyBookmarkModified(const QModelIndex &index);

private:
	BookmarksItem *m_rootItem;
	BookmarksItem *m_trashItem;
	QHash<BookmarksItem*, QPair<QModelIndex, int> > m_trash;
	QHash<QUrl, QList<BookmarksItem*> > m_urls;
	QHash<QString, BookmarksItem*> m_keywords;
	QMap<quint64, BookmarksItem*> m_identifiers;
	FormatMode m_mode;

signals:
	void bookmarkAdded(BookmarksItem *bookmark);
	void bookmarkModified(BookmarksItem *bookmark);
	void bookmarkMoved(BookmarksItem *bookmark, BookmarksItem *previousParent, int previousRow);
	void bookmarkTrashed(BookmarksItem *bookmark);
	void bookmarkRestored(BookmarksItem *bookmark);
	void bookmarkRemoved(BookmarksItem *bookmark);
	void modelModified();

friend class BookmarksItem;
};

}

#endif
