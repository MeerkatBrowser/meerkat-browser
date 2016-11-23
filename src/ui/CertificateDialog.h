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

#ifndef MEERKAT_CERTIFICATEDIALOG_H
#define MEERKAT_CERTIFICATEDIALOG_H

#include "Dialog.h"

#include <QtGui/QStandardItem>
#include <QtNetwork/QSslCertificate>

namespace Meerkat
{

namespace Ui
{
	class CertificateDialog;
}

class CertificateDialog : public Dialog
{
	Q_OBJECT

public:
	enum CertificateField
	{
		VersionField = 0,
		SerialNumberField,
		SignatureAlgorithmField,
		IssuerField,
		ValidityField,
		ValidityNotBeforeField,
		ValidityNotAfterField,
		SubjectField,
		PublicKeyField,
		PublicKeyAlgorithmField,
		PublicKeyValueField,
		ExtensionsField,
		ExtensionField,
		DigestField,
		DigestSha1Field,
		DigestSha256Field
	};

	explicit CertificateDialog(QList<QSslCertificate> certificates, QWidget *parent = nullptr);
	~CertificateDialog();

protected:
	void changeEvent(QEvent *event);
	QStandardItem* createField(CertificateField field, QStandardItem *parent = nullptr, const QMap<int, QVariant> &data = QMap<int, QVariant>());
	QString formatHex(const QString &source, const QChar &separator = QLatin1Char(' '));

protected slots:
	void exportCertificate();
	void updateCertificate();
	void updateValue();

private:
	QList<QSslCertificate> m_certificates;
	Ui::CertificateDialog *m_ui;
};

}

#endif
