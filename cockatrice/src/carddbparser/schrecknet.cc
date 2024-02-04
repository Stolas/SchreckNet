#include "schrecknet.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include <version_string.h>

#define SCHRECKNET_TAGNAME "schrecknet_carddatabase"
#define SCHRECKNET_TAGVER 1
#define SCHRECKNET_SCHEMALOCATION                                                                                 \
    "https://raw.githubusercontent.com/Cockatrice/Cockatrice/master/doc/carddatabase_v4/cards.xsd"
/* Todo; set location. */

bool SchrecknetParser::getCanParseFile(const QString &fileName, QIODevice &device)
{
    qDebug() << "[SchrecknetParser] Trying to parse: " << fileName;

    if (!fileName.endsWith(".xml", Qt::CaseInsensitive)) {
        qDebug() << "[SchrecknetParser] Parsing failed: wrong extension";
        return false;
    }

    QXmlStreamReader xml(&device);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            if (xml.name().toString() == SCHRECKNET_TAGNAME) {
                int version = xml.attributes().value("version").toString().toInt();
                if (version == SCHRECKNET_TAGVER) {
                    return true;
                } else {
                    qDebug() << "[SchrecknetParser] Parsing failed: wrong version" << version;
                    return false;
                }

            } else {
                qDebug() << "[SchrecknetParser] Parsing failed: wrong element tag" << xml.name();
                return false;
            }
        }
    }

    return true;
}

void SchrecknetParser::parseFile(QIODevice &device)
{
    QXmlStreamReader xml(&device);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::EndElement) {
                    break;
                }

                auto xmlName = xml.name().toString();
                if (xmlName == "sets") {
                    loadSetsFromXml(xml);
                } else if (xmlName == "cards") {
                    loadCardsFromXml(xml);
                } else if (!xmlName.isEmpty()) {
                    qDebug() << "[SchrecknetParser] Unknown item" << xmlName << ", trying to continue anyway";
                    xml.skipCurrentElement();
                }
            }
        }
    }
}

void SchrecknetParser::loadSetsFromXml(QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::EndElement) {
            break;
        }

        auto xmlName = xml.name().toString();
        if (xmlName == "set") {
            QString shortName, longName, setType;
            QDate releaseDate;
            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::EndElement) {
                    break;
                }
                xmlName = xml.name().toString();

                if (xmlName == "name") {
                    shortName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xmlName == "longname") {
                    longName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xmlName == "settype") {
                    setType = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xmlName == "releasedate") {
                    releaseDate =
                        QDate::fromString(xml.readElementText(QXmlStreamReader::IncludeChildElements), Qt::ISODate);
                } else if (!xmlName.isEmpty()) {
                    qDebug() << "[SchrecknetParser] Unknown set property" << xmlName
                             << ", trying to continue anyway";
                    xml.skipCurrentElement();
                }
            }

            internalAddSet(shortName, longName, setType, releaseDate);
        }
    }
}

QVariantHash SchrecknetParser::loadCardPropertiesFromXml(QXmlStreamReader &xml)
{
    QVariantHash properties = QVariantHash();
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::EndElement) {
            break;
        }

        auto xmlName = xml.name().toString();
        if (!xmlName.isEmpty()) {
            properties.insert(xmlName, xml.readElementText(QXmlStreamReader::IncludeChildElements));
        }
    }
    return properties;
}

void SchrecknetParser::loadCardsFromXml(QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::EndElement) {
            break;
        }

        auto xmlName = xml.name().toString();

        if (xmlName == "card") {
            QString name = QString("");
            QString text = QString("");
            QVariantHash properties = QVariantHash();
            QList<CardRelation *> relatedCards, reverseRelatedCards;
            auto _sets = CardInfoPerSetMap();
            int tableRow = 0;
            bool cipt = false;
            bool isToken = false;
            bool upsideDown = false;

            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::EndElement) {
                    break;
                }

                xmlName = xml.name().toString();
                // Todo; Schrecknet, read what we need.

                // variable - assigned properties
                if (xmlName == "name") {
                    name = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xmlName == "text") {
                    text = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xmlName == "token") {
                    isToken = static_cast<bool>(xml.readElementText(QXmlStreamReader::IncludeChildElements).toInt());
                    // generic properties
                } else if (xmlName == "prop") {
                    properties = loadCardPropertiesFromXml(xml);
                    // positioning info
                } else if (xmlName == "tablerow") {
                    tableRow = xml.readElementText(QXmlStreamReader::IncludeChildElements).toInt();
                } else if (xmlName == "cipt") {
                    cipt = (xml.readElementText(QXmlStreamReader::IncludeChildElements) == "1");
                } else if (xmlName == "upsidedown") {
                    upsideDown = (xml.readElementText(QXmlStreamReader::IncludeChildElements) == "1");
                    // sets
                } else if (xmlName == "set") {
                    // NOTE: attributes but be read before readElementText()
                    QXmlStreamAttributes attrs = xml.attributes();
                    QString setName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                    auto set = internalAddSet(setName);
                    if (set->getEnabled()) {
                        CardInfoPerSet setInfo(set);
                        for (QXmlStreamAttribute attr : attrs) {
                            QString attrName = attr.name().toString();
                            if (attrName == "picURL")
                                attrName = "picurl";
                            setInfo.setProperty(attrName, attr.value().toString());
                        }
                        _sets.insert(setName, setInfo);
                    }
                    // related cards
                } else if (xmlName == "related" || xmlName == "reverse-related") {
                    CardRelation::AttachType attachType = CardRelation::DoesNotAttach;
                    bool exclude = false;
                    bool variable = false;
                    bool persistent = false;
                    int count = 1;
                    QXmlStreamAttributes attrs = xml.attributes();
                    QString cardName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                    if (attrs.hasAttribute("count")) {
                        if (attrs.value("count").toString().indexOf("x=") == 0) {
                            variable = true;
                            count = attrs.value("count").toString().remove(0, 2).toInt();
                        } else if (attrs.value("count").toString().indexOf("x") == 0) {
                            variable = true;
                        } else {
                            count = attrs.value("count").toString().toInt();
                        }

                        if (count < 1) {
                            count = 1;
                        }
                    }

                    if (attrs.hasAttribute("attach")) {
                        attachType = attrs.value("attach").toString() == "transform" ? CardRelation::TransformInto
                                                                                     : CardRelation::AttachTo;
                    }

                    if (attrs.hasAttribute("exclude")) {
                        exclude = true;
                    }

                    if (attrs.hasAttribute("persistent")) {
                        persistent = true;
                    }

                    auto *relation = new CardRelation(cardName, attachType, exclude, variable, count, persistent);
                    if (xmlName == "reverse-related") {
                        reverseRelatedCards << relation;
                    } else {
                        relatedCards << relation;
                    }
                } else if (!xmlName.isEmpty()) {
                    qDebug() << "[SchrecknetParser] Unknown card property" << xmlName
                             << ", trying to continue anyway";
                    xml.skipCurrentElement();
                }
            }

            CardInfoPtr newCard = CardInfo::newInstance(name, text, isToken, properties, relatedCards,
                                                        reverseRelatedCards, _sets, cipt, tableRow, upsideDown);
            emit addCard(newCard);
        }
    }
}

static QXmlStreamWriter &operator<<(QXmlStreamWriter &xml, const CardSetPtr &set)
{
    if (set.isNull()) {
        qDebug() << "&operator<< set is nullptr";
        return xml;
    }

    xml.writeStartElement("set");
    xml.writeTextElement("name", set->getShortName());
    xml.writeTextElement("longname", set->getLongName());
    xml.writeTextElement("settype", set->getSetType());
    xml.writeTextElement("releasedate", set->getReleaseDate().toString(Qt::ISODate));
    xml.writeEndElement();

    return xml;
}

static QXmlStreamWriter &operator<<(QXmlStreamWriter &xml, const CardInfoPtr &info)
{
    if (info.isNull()) {
        qDebug() << "operator<< info is nullptr";
        return xml;
    }

    QString tmpString;

    xml.writeStartElement("card");

    // variable - assigned properties
    xml.writeTextElement("name", info->getName());
    xml.writeTextElement("text", info->getText());
    if (info->getIsToken()) {
        xml.writeTextElement("token", "1");
    }

    // generic properties
    xml.writeStartElement("prop");
    for (QString propName : info->getProperties()) {
        //properties.value(propertyName)

        if (info->isPropertyAList(propName)) {
            QStringList values = info->getPropertyList(propName);
            xml.writeStartElement(propName);
            QString slicedName = propName.sliced(0, propName.length() - 1);
            for (QString propValue : values) {
                xml.writeTextElement(slicedName, propValue);
            }

            xml.writeEndElement();
        } else {
            xml.writeTextElement(propName, info->getProperty(propName));
        }
    }
    xml.writeEndElement();

    // sets
    for (CardInfoPerSet set : info->getSets()) {
        xml.writeStartElement("set");
        for (QString propName : set.getProperties()) {
            xml.writeAttribute(propName, set.getProperty(propName));
        }

        xml.writeCharacters(set.getPtr()->getShortName());
        xml.writeEndElement();
    }

    // related cards
    const QList<CardRelation *> related = info->getRelatedCards();
    for (auto i : related) {
        xml.writeStartElement("related");
        if (i->getDoesAttach()) {
            xml.writeAttribute("attach", i->getAttachTypeAsString());
        }
        if (i->getIsCreateAllExclusion()) {
            xml.writeAttribute("exclude", "exclude");
        }
        if (i->getIsPersistent()) {
            xml.writeAttribute("persistent", "persistent");
        }
        if (i->getIsVariable()) {
            if (1 == i->getDefaultCount()) {
                xml.writeAttribute("count", "x");
            } else {
                xml.writeAttribute("count", "x=" + QString::number(i->getDefaultCount()));
            }
        } else if (1 != i->getDefaultCount()) {
            xml.writeAttribute("count", QString::number(i->getDefaultCount()));
        }
        xml.writeCharacters(i->getName());
        xml.writeEndElement();
    }
    const QList<CardRelation *> reverseRelated = info->getReverseRelatedCards();
    for (auto i : reverseRelated) {
        xml.writeStartElement("reverse-related");
        if (i->getDoesAttach()) {
            xml.writeAttribute("attach", i->getAttachTypeAsString());
        }

        if (i->getIsCreateAllExclusion()) {
            xml.writeAttribute("exclude", "exclude");
        }

        if (i->getIsPersistent()) {
            xml.writeAttribute("persistent", "persistent");
        }
        if (i->getIsVariable()) {
            if (1 == i->getDefaultCount()) {
                xml.writeAttribute("count", "x");
            } else {
                xml.writeAttribute("count", "x=" + QString::number(i->getDefaultCount()));
            }
        } else if (1 != i->getDefaultCount()) {
            xml.writeAttribute("count", QString::number(i->getDefaultCount()));
        }
        xml.writeCharacters(i->getName());
        xml.writeEndElement();
    }

    // positioning
    xml.writeTextElement("tablerow", QString::number(info->getTableRow()));
    if (info->getCipt()) {
        xml.writeTextElement("cipt", "1");
    }
    if (info->getUpsideDownArt()) {
        xml.writeTextElement("upsidedown", "1");
    }

    xml.writeEndElement(); // card

    return xml;
}

bool SchrecknetParser::saveToFile(SetNameMap _sets,
                                      CardNameMap cards,
                                      const QString &fileName,
                                      const QString &sourceUrl,
                                      const QString &sourceVersion)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QXmlStreamWriter xml(&file);

    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(SCHRECKNET_TAGNAME);
    xml.writeAttribute("version", QString::number(SCHRECKNET_TAGVER));
    xml.writeAttribute("xmlns:xsi", SCHRECKNET_XML_XSI_NAMESPACE);
    xml.writeAttribute("xsi:schemaLocation", SCHRECKNET_SCHEMALOCATION);

    xml.writeStartElement("info");
    xml.writeTextElement("author", QCoreApplication::applicationName() + QString(" %1").arg(VERSION_STRING));
    xml.writeTextElement("createdAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    xml.writeTextElement("sourceUrl", sourceUrl);
    xml.writeTextElement("sourceVersion", sourceVersion);
    xml.writeEndElement();

    if (_sets.count() > 0) {
        xml.writeStartElement("sets");
        for (CardSetPtr set : _sets) {
            xml << set;
        }
        xml.writeEndElement();
    }

    if (cards.count() > 0) {
        xml.writeStartElement("cards");
        for (CardInfoPtr card : cards) {
            xml << card;
        }
        xml.writeEndElement();
    }

    xml.writeEndElement(); // cockatrice_carddatabase
    xml.writeEndDocument();

    return true;
}
