/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef MEERKAT_FILEPATHWIDGET_H
#define MEERKAT_FILEPATHWIDGET_H

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLineEdit>

namespace Meerkat
{

class FilePathWidget : public QWidget
{
	Q_OBJECT

public:
	explicit FilePathWidget(QWidget *parent = NULL);

	void setFilters(const QStringList &filters);
	void setSelectFile(bool mode);
	void setPath(const QString &path);
	QString getPath() const;

protected:
	void focusInEvent(QFocusEvent *event);

protected slots:
	void selectPath();
	void updateCompleter();

private:
	QLineEdit *m_lineEdit;
	QCompleter *m_completer;
	QString m_filter;
	bool m_selectFile;

signals:
	void pathChanged(const QString &path);
};

}

#endif
