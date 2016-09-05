/**************************************************************************
* Meerkat Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010 - 2014 David Rosca <nowrep@gmail.com>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentBlockingProfile.h"
#include "Console.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "SessionsManager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Meerkat
{

ContentBlockingProfile::ContentBlockingProfile(const QString &name, QObject *parent) : QObject(parent),
	m_root(NULL),
	m_networkReply(NULL),
	m_category(OtherCategory),
	m_name(name),
	m_languages({QLocale::AnyLanguage}),
	m_enableWildcards(SettingsManager::getValue(SettingsManager::ContentBlocking_EnableWildcardsOption).toBool()),
	m_isUpdating(false),
	m_isEmpty(true),
	m_wasLoaded(false)
{
	load(true);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
}

void ContentBlockingProfile::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::ContentBlocking_EnableWildcardsOption)
	{
		m_enableWildcards = value.toBool();

		clear();
	}
}

void ContentBlockingProfile::clear()
{
	if (!m_wasLoaded)
	{
		return;
	}

	if (m_root)
	{
		QtConcurrent::run(this, &ContentBlockingProfile::deleteNode, m_root);
	}

	m_styleSheet.clear();
	m_styleSheetWhiteList.clear();
	m_styleSheetBlackList.clear();

	m_wasLoaded = false;
}

void ContentBlockingProfile::load(bool onlyHeader)
{
	QFile file(getPath());

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to open content blocking profile file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	QTextStream stream(&file);

	if (!stream.readLine().trimmed().startsWith(QLatin1String("[Adblock Plus"), Qt::CaseInsensitive))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to load content blocking profile file: invalid header"), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		file.close();

		return;
	}

	while (!stream.atEnd())
	{
		QString line(stream.readLine().trimmed());

		if (!line.startsWith(QLatin1Char('!')))
		{
			m_isEmpty = false;

			break;
		}

		if (line.startsWith(QLatin1String("! Title: ")))
		{
			m_title = line.remove(QLatin1String("! Title: "));

			continue;
		}

		line.remove(QLatin1Char(' '));

		if (line.startsWith(QLatin1String("!URL:")))
		{
			m_updateUrl = QUrl(line.remove(QLatin1String("!URL:")));

			continue;
		}
	}

	file.close();

	const QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	const QDateTime lastUpdate(QDateTime::fromString(profilesSettings.value(m_name + QLatin1String("/lastUpdate")).toString(), Qt::ISODate));
	const int updateInterval(profilesSettings.value(m_name + QLatin1String("/updateInterval")).toInt());

	if (!m_isUpdating && updateInterval > 0 && (!lastUpdate.isValid() || lastUpdate.daysTo(QDateTime::currentDateTime()) > updateInterval))
	{
		downloadRules();
	}

	if (!onlyHeader)
	{
		loadRules();
	}
}

void ContentBlockingProfile::parseRuleLine(QString line)
{
	if (line.indexOf(QLatin1Char('!')) == 0 || line.isEmpty())
	{
		return;
	}

	if (line.startsWith(QLatin1String("##")))
	{
		m_styleSheet.append(line.mid(2));

		return;
	}

	if (line.contains(QLatin1String("##")))
	{
		parseStyleSheetRule(line.split(QLatin1String("##")), m_styleSheetBlackList);

		return;
	}

	if (line.contains(QLatin1String("#@#")))
	{
		parseStyleSheetRule(line.split(QLatin1String("#@#")), m_styleSheetWhiteList);

		return;
	}

	const int optionSeparator(line.indexOf(QLatin1Char('$')));
	QStringList options;

	if (optionSeparator >= 0)
	{
		options = line.mid(optionSeparator + 1).split(QLatin1Char(','), QString::SkipEmptyParts);

		line = line.left(optionSeparator);
	}

	while (line.endsWith(QLatin1Char('|')) || line.endsWith(QLatin1Char('*')) || line.endsWith(QLatin1Char('^')))
	{
		line = line.left(line.length() - 1);
	}

	if (line.startsWith(QLatin1Char('*')))
	{
		line = line.mid(1);
	}

	if (line.contains(QLatin1Char('^')) || (!m_enableWildcards && line.contains(QLatin1Char('*'))))
	{
		// TODO - '^'
		return;
	}

	ContentBlockingRule *rule(new ContentBlockingRule());
	rule->ruleOption = NoOption;
	rule->exceptionRuleOption = NoOption;
	rule->isException = false;
	rule->needsDomainCheck = false;

	if (line.startsWith(QLatin1String("@@")))
	{
		line = line.mid(2);

		rule->isException = true;
	}

	if (line.startsWith(QLatin1String("||")))
	{
		line = line.mid(2);

		rule->needsDomainCheck = true;
	}

	for (int i = 0; i < options.count(); ++i)
	{
		const bool optionException(options.at(i).startsWith(QLatin1Char('~')));

		if (options.at(i).contains(QLatin1String("third-party")))
		{
			rule->ruleOption |= ThirdPartyOption;
			rule->exceptionRuleOption |= (optionException ? ThirdPartyOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("stylesheet")))
		{
			rule->ruleOption |= StyleSheetOption;
			rule->exceptionRuleOption |= (optionException ? StyleSheetOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("image")))
		{
			rule->ruleOption |= ImageOption;
			rule->exceptionRuleOption |= (optionException ? ImageOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("script")))
		{
			rule->ruleOption |= ScriptOption;
			rule->exceptionRuleOption |= (optionException ? ScriptOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("object")))
		{
			rule->ruleOption |= ObjectOption;
			rule->exceptionRuleOption |= (optionException ? ObjectOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("object-subrequest")) || options.at(i).contains(QLatin1String("object_subrequest")))
		{
			rule->ruleOption |= ObjectSubRequestOption;
			rule->exceptionRuleOption |= (optionException ? ObjectSubRequestOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("subdocument")))
		{
			rule->ruleOption |= SubDocumentOption;
			rule->exceptionRuleOption |= (optionException ? SubDocumentOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("xmlhttprequest")))
		{
			rule->ruleOption |= XmlHttpRequestOption;
			rule->exceptionRuleOption |= (optionException ? XmlHttpRequestOption : NoOption);
		}
		else if (options.at(i).contains(QLatin1String("domain")))
		{
			const QStringList parsedDomains(options.at(i).mid(options.at(i).indexOf(QLatin1Char('=')) + 1).split(QLatin1Char('|'), QString::SkipEmptyParts));

			for (int j = 0; j < parsedDomains.count(); ++j)
			{
				if (parsedDomains.at(j).startsWith(QLatin1Char('~')))
				{
					rule->allowedDomains.append(parsedDomains.at(j).mid(1));

					continue;
				}

				rule->blockedDomains.append(parsedDomains.at(j));
			}
		}
		else
		{
			// TODO - document, elemhide
			delete rule;

			return;
		}
	}

	addRule(rule, line);

	return;
}

void ContentBlockingProfile::parseStyleSheetRule(const QStringList &line, QMultiHash<QString, QString> &list)
{
	const QStringList domains(line.at(0).split(QLatin1Char(',')));

	for (int i = 0; i < domains.count(); ++i)
	{
		list.insert(domains.at(i), line.at(1));
	}
}

void ContentBlockingProfile::addRule(ContentBlockingRule *rule, const QString &ruleString)
{
	Node *node(m_root);

	for (int i = 0; i < ruleString.length(); ++i)
	{
		const QChar value(ruleString.at(i));
		bool childrenExists(false);

		for (int j = 0; j < node->children.count(); ++j)
		{
			Node *nextNode(node->children.at(j));

			if (nextNode->value == value)
			{
				node = nextNode;

				childrenExists = true;

				break;
			}
		}

		if (!childrenExists)
		{
			Node *newNode(new Node());
			newNode->value = value;

			node->children.append(newNode);

			node = newNode;
		}
	}

	node->rule = rule;
}

void ContentBlockingProfile::deleteNode(Node *node)
{
	for (int i = 0; i < node->children.count(); ++i)
	{
		deleteNode(node->children.at(i));
	}

	delete node->rule;
	delete node;
}

void ContentBlockingProfile::replyFinished()
{
	m_isUpdating = false;

	if (!m_networkReply)
	{
		return;
	}

	m_networkReply->deleteLater();

	const QByteArray downloadedDataHeader(m_networkReply->readLine());
	const QByteArray downloadedDataChecksum(m_networkReply->readLine());
	const QByteArray downloadedData(m_networkReply->readAll());

	if (m_networkReply->error() != QNetworkReply::NoError || !downloadedDataHeader.trimmed().startsWith(QByteArray("[Adblock Plus")))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(m_networkReply->errorString()), Console::OtherCategory, Console::ErrorLevel, getPath());

		return;
	}

	if (downloadedDataChecksum.contains(QByteArray("! Checksum: ")))
	{
		QByteArray checksum(downloadedDataChecksum);
		const QByteArray verifiedChecksum(QCryptographicHash::hash(downloadedDataHeader + QString(downloadedData).replace(QRegExp(QLatin1String("^*\n{2,}")), QLatin1String("\n")).toStdString().c_str(), QCryptographicHash::Md5));

		if (verifiedChecksum.toBase64().replace(QByteArray("="), QByteArray()) != checksum.replace(QByteArray("! Checksum: "), QByteArray()).replace(QByteArray("\n"), QByteArray()))
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: checksum mismatch"), Console::OtherCategory, Console::ErrorLevel, getPath());

			return;
		}
	}

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("blocking")));

	const bool isFirstDownload(getPath().startsWith(QLatin1String(":/")));
	QFile file(SessionsManager::getWritableDataPath(QLatin1String("blocking") + QLatin1String("/") + m_name + QLatin1String(".txt")));

	if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	profilesSettings.setValue(m_name + QLatin1String("/lastUpdate"), QDateTime::currentDateTime().toString(Qt::ISODate));

	file.write(downloadedDataHeader);

	if (!isFirstDownload)
	{
		file.write(QStringLiteral("! URL: %1\n").arg(m_updateUrl.toString()).toUtf8());
	}

	file.write(downloadedDataChecksum);
	file.write(downloadedData);
	file.close();

	if (file.error() != QFile::NoError)
	{
// TODO
	}

	const bool wasLoaded(m_wasLoaded);

	clear();
	load(wasLoaded);

	emit profileModified(m_name);
}

QString ContentBlockingProfile::getName() const
{
	return m_name;
}

QString ContentBlockingProfile::getTitle() const
{
	return (m_title.isEmpty() ? tr("(Unknown)") : m_title);
}

QString ContentBlockingProfile::getPath() const
{
	return SessionsManager::getReadableDataPath(QLatin1String("blocking") + QLatin1String("/") + m_name + QLatin1String(".txt"));
}

QUrl ContentBlockingProfile::getUpdateUrl() const
{
	return m_updateUrl;
}

ContentBlockingManager::CheckResult ContentBlockingProfile::checkUrl(const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType)
{
	ContentBlockingManager::CheckResult result;

	if (!m_wasLoaded && !loadRules())
	{
		return result;
	}

	m_baseUrlHost = baseUrl.host();
	m_requestUrl = requestUrl.url(QUrl::RemoveScheme);
	m_requestHost = requestUrl.host();

	if (m_requestUrl.startsWith(QLatin1String("//")))
	{
		m_requestUrl = m_requestUrl.mid(2);
	}

	for (int i = 0; i < m_requestUrl.length(); ++i)
	{
		if (checkUrlSubstring(m_root, m_requestUrl.right(m_requestUrl.length() - i), QString(), resourceType))
		{
			result.profile = m_name;
			result.isBlocked = true;

			return result;
		}
	}

	return result;
}

QStringList ContentBlockingProfile::getStyleSheet()
{
	if (!m_wasLoaded)
	{
		loadRules();
	}

	return m_styleSheet;
}

QStringList ContentBlockingProfile::getStyleSheetBlackList(const QString &domain)
{
	if (!m_wasLoaded)
	{
		loadRules();
	}

	return m_styleSheetBlackList.values(domain);
}

QStringList ContentBlockingProfile::getStyleSheetWhiteList(const QString &domain)
{
	if (!m_wasLoaded)
	{
		loadRules();
	}

	return m_styleSheetWhiteList.values(domain);
}

QList<QLocale::Language> ContentBlockingProfile::getLanguages() const
{
	return m_languages;
}

ContentBlockingProfile::ProfileCategory ContentBlockingProfile::getCategory() const
{
	return m_category;
}

bool ContentBlockingProfile::downloadRules()
{
	if (m_isUpdating)
	{
		return false;
	}

	if (!m_updateUrl.isValid())
	{
		const QString path(getPath());

		if (m_updateUrl.isEmpty())
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile, update URL is empty"), Console::OtherCategory, Console::ErrorLevel, path);
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile, update URL (%1) is invalid").arg(m_updateUrl.toString()), Console::OtherCategory, Console::ErrorLevel, path);
		}

		return false;
	}

	QNetworkRequest request(m_updateUrl);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

	m_networkReply = NetworkManagerFactory::getNetworkManager()->get(request);

	connect(m_networkReply, SIGNAL(finished()), this, SLOT(replyFinished()));

	m_isUpdating = true;

	return true;
}

bool ContentBlockingProfile::loadRules()
{
	if (m_isEmpty)
	{
		downloadRules();

		return false;
	}

	m_wasLoaded = true;

	if (m_domainExpression.pattern().isEmpty())
	{
		m_domainExpression = QRegularExpression(QLatin1String("[:\?&/=]"));
#if QT_VERSION >= 0x050400
		m_domainExpression.optimize();
#endif
	}

	QFile file(getPath());
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.readLine(); // header

	m_root = new Node();

	while (!stream.atEnd())
	{
		parseRuleLine(stream.readLine());
	}

	file.close();

	return true;
}

bool ContentBlockingProfile::resolveDomainExceptions(const QString &url, const QStringList &ruleList)
{
	for (int i = 0; i < ruleList.count(); ++i)
	{
		if (url.contains(ruleList.at(i)))
		{
			return true;
		}
	}

	return false;
}

bool ContentBlockingProfile::checkUrlSubstring(Node *node, const QString &subString, QString currentRule, NetworkManager::ResourceType resourceType)
{
	for (int i = 0; i < subString.length(); ++i)
	{
		const QChar treeChar(subString.at(i));

		if (node->rule && checkRuleMatch(node->rule, currentRule, resourceType))
		{
			return true;
		}

		bool childrenExists(false);

		for (int j = 0; j < node->children.count(); ++j)
		{
			Node *nextNode(node->children.at(j));

			if (nextNode->value == QLatin1Char('*'))
			{
				const QString wildcardSubString(subString.mid(i));

				for (int k = 0; k < wildcardSubString.length(); ++k)
				{
					if (checkUrlSubstring(nextNode, wildcardSubString.right(wildcardSubString.length() - k), currentRule + wildcardSubString.left(k), resourceType))
					{
						return true;
					}
				}
			}

			if (nextNode->value == treeChar)
			{
				node = nextNode;

				childrenExists = true;

				break;
			}
		}

		if (!childrenExists)
		{
			return false;
		}

		currentRule += treeChar;
	}

	if (node->rule && checkRuleMatch(node->rule, currentRule, resourceType))
	{
		return true;
	}

	return false;
}

bool ContentBlockingProfile::checkRuleMatch(ContentBlockingRule *rule, const QString &currentRule, NetworkManager::ResourceType resourceType)
{
	if (!m_requestUrl.contains(currentRule))
	{
		return false;
	}

	const QStringList requestSubdomainList(ContentBlockingManager::createSubdomainList(m_requestHost));

	if (rule->needsDomainCheck && !requestSubdomainList.contains(currentRule.left(currentRule.indexOf(m_domainExpression))))
	{
		return false;
	}

	const bool hasBlockedDomains(!rule->blockedDomains.isEmpty());
	const bool hasAllowedDomains(!rule->allowedDomains.isEmpty());
	bool isBlocked(hasBlockedDomains ? resolveDomainExceptions(m_baseUrlHost, rule->blockedDomains) : true);
	isBlocked = (hasAllowedDomains ? !resolveDomainExceptions(m_baseUrlHost, rule->allowedDomains) : isBlocked);

	if (rule->ruleOption.testFlag(ThirdPartyOption))
	{
		if (m_baseUrlHost.isEmpty() || requestSubdomainList.contains(m_baseUrlHost))
		{
			isBlocked = rule->exceptionRuleOption.testFlag(ThirdPartyOption);
		}
		else if (!hasBlockedDomains && !hasAllowedDomains)
		{
			isBlocked = !rule->exceptionRuleOption.testFlag(ThirdPartyOption);
		}
	}

	QList<QPair<RuleOption, NetworkManager::ResourceType> > options({qMakePair(ImageOption, NetworkManager::ImageType), qMakePair(ScriptOption, NetworkManager::ScriptType), qMakePair(StyleSheetOption, NetworkManager::StyleSheetType), qMakePair(ObjectOption, NetworkManager::ObjectType), qMakePair(XmlHttpRequestOption, NetworkManager::XmlHttpRequestType), qMakePair(SubDocumentOption, NetworkManager::SubFrameType), qMakePair(ObjectSubRequestOption, NetworkManager::ObjectSubrequestType)});

	for (int i = 0; i < options.count(); ++i)
	{
		if (rule->ruleOption.testFlag(options.at(i).first))
		{
			if (resourceType == options.at(i).second)
			{
				isBlocked = (isBlocked ? !rule->exceptionRuleOption.testFlag(options.at(i).first) : isBlocked);
			}
			else
			{
				isBlocked = (isBlocked ? rule->exceptionRuleOption.testFlag(options.at(i).first) : isBlocked);
			}
		}
	}

	return (isBlocked ? !rule->isException : false);
}

}
