#include "schrecknet.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <version_string.h>

#define SCHRECKNET_TAGNAME "wmrh_carddatabase"
#define SCHRECKNET_TAGVER 1
#define SCHRECKNET_SCHEMALOCATION                                                                                 \
    "https://raw.githubusercontent.com/Stolas/SchreckNet-WMRH-CardDB/master/cards.xsd"
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

// QVariantHash SchrecknetParser::loadCardPropertiesFromXml(QXmlStreamReader &xml)
// {
//     QVariantHash properties = QVariantHash();
//     while (!xml.atEnd()) {
//         if (xml.readNext() == QXmlStreamReader::EndElement) {
//             break;
//         }
// 
//         auto xmlName = xml.name().toString();
//         if (!xmlName.isEmpty()) {
//             QVariant xmlValue = QVariant();
//             if (xmlName == "FOR_ALL_ARRAYS") {
//                   auto isChar = xml.isCharacters();
//                  auto test = xml.readElementText(QXmlStreamReader::IncludeChildElements);
//             } else {
//                 xmlValue = xml.readElementText(QXmlStreamReader::IncludeChildElements);
//             }
//             properties.insert(xmlName, xmlValue);
//         }
//     }
//     return properties;
// }

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
            bool isToken = false;
            bool isCrypt = false;
            bool upsideDown = false;
            QList<QString> types = QList<QString>();
            QList<QString> clans = QList<QString>();
            QList<QString> disciplines = QList<QString>();

            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::EndElement) {
                    xmlName = xml.name().toString();
                    if (xmlName == "card") {
                        break;
                    }
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
                } else if (xmlName == "is_crypt") {
                    isCrypt = QVariant(xml.readElementText(QXmlStreamReader::IncludeChildElements)).toBool();
                    //isCrypt = static_cast<bool>(xml.readElementText(QXmlStreamReader::IncludeChildElements).toInt());
                    // generic properties
                } else if (xmlName == "group" || xmlName == "capacity") {
                    auto xmlValue = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                    properties.insert(xmlName, xmlValue);
                // load list properties
                } else if (xmlName == "types" || xmlName == "clans" || xmlName == "disciplines" || xmlName == "rulings") {
                    while (!xml.atEnd()) {
                        if (xml.readNext() == QXmlStreamReader::EndElement) {
                            xmlName = xml.name().toString();
                            if (xmlName == "types" || xmlName == "clans" || xmlName == "disciplines" ||
                                xmlName == "rulings") {
                                break;
                            }
                        }
                        xmlName = xml.name().toString();

                        if (xmlName == "type") {
                            auto typeValue = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                            types.append(typeValue);
                        }
                        else if (xmlName == "clan") {
                            auto typeValue = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                            clans.append(typeValue);
                        }
                        else if (xmlName == "discipline") {
                            auto typeValue = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                            disciplines.append(typeValue);
                        }
                        else if (xmlName == "ruling") {
                            /* Todo; Parse rulings. */
                        }
                        else if (!xmlName.isEmpty()) {
                            qDebug() << "[SchrecknetParser] Unknown value in a list-type" << xmlName
                                     << ", trying to continue anyway";
                            xml.skipCurrentElement();
                        }

                    }

                // positioning info
                } else if (xmlName == "tablerow") {
                    tableRow = xml.readElementText(QXmlStreamReader::IncludeChildElements).toInt();
                } else if (xmlName == "sets") {
                    while (!xml.atEnd()) {
                        if (xml.readNext() == QXmlStreamReader::EndElement) {
                            xmlName = xml.name().toString();
                            if (xmlName == "sets") { break; }
                        }

                        xmlName = xml.name().toString();
                        if (xmlName == "set") {
                            // NOTE: attributes but be read before readElementText()
                            QXmlStreamAttributes attrs = xml.attributes();
                            QString setName = "";
                            QString picUrl = "";
                            for (QXmlStreamAttribute attr : attrs) {
                                QString attrName = attr.name().toString();
                                if (attrName == "name") {
                                    setName = attr.value().toString();
                                } else if (attrName == "picURL") {
                                    picUrl =  attr.value().toString();
                                }
                            }
                            if (!setName.isEmpty() && !picUrl.isEmpty()) {
                                auto set = internalAddSet(setName);
                                if (set->getEnabled()) {
                                    CardInfoPerSet setInfo(set);
                                    setInfo.setProperty("picurl", picUrl);

                                    _sets.insert(setName, setInfo);
                                }
                            }
                        }
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
                } else if (xmlName == "printed_name" || xmlName == "url") {
                    /* Ignore this one */
                } else if (!xmlName.isEmpty()) {
                    qDebug() << "[SchrecknetParser] Unknown card property" << xmlName
                             << ", trying to continue anyway";
                    xml.skipCurrentElement();
                }
            }

            /* Todo pass isCrypt*/
            int bleed = 0;
            int votes = 0;
            int strength = 0;
            if (isCrypt) {
                static const QRegularExpression cryptBleed("\\+(?P<bleed>\\d+) bleed\\.");
                static const QRegularExpression independedVotes("(?P<votes>\\d+) votes \\(titled\\)");
                static const QRegularExpression cryptStrength("\\+(?P<strength>\\d+) strength\\.");

                QRegularExpressionMatch match;
                match = cryptBleed.match(text);
                if (match.hasMatch()) {
                    bleed = match.captured("bleed").toInt();
                }

                match = independedVotes.match(text);
                if (match.hasMatch()) {
                    votes = match.captured("votes").toInt();
                } else {
                    if (text.contains("Primogen") || text.contains("Bishop")) {
                        votes = 1;
                    }
                    if (text.contains("Prince") || text.contains("Archbishop") || text.contains("Baron") ||
                        text.contains("Magaji")) {
                        votes = 2;
                    }
                    if (text.contains("Justicar") || text.contains("Cardinal")) {
                        votes = 3;
                    }
                    if (text.contains("Inner Circle") || text.contains("Regent")) {
                        votes = 4;
                    }
                }

                match = cryptStrength.match(text);
                if (match.hasMatch()) {
                    strength = match.captured("strength").toInt();
                }

                bleed += 1;
                strength += 1;
            } else {
                static const QRegularExpression bvsAlly("with (?P<life>\\d+) life\. (?P<strength>\\d+) strength, (?P<bleed>\\d+) bleed\.");
                auto match = bvsAlly.match(text);
                if (match.hasMatch()) {
                    auto capacity = match.captured("life").toInt();
                    strength = match.captured("strength").toInt();
                    bleed = match.captured("bleed").toInt();
                    properties.insert("capacity", capacity);
                }
            }

            properties.insert("bleed", bleed);
            properties.insert("votes", votes);
            properties.insert("strength", strength);


            CardInfoPtr newCard = CardInfo::newInstance(name, text, isToken, properties, relatedCards,
                                                        reverseRelatedCards, _sets, isCrypt, tableRow, upsideDown);
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
    qDebug() << "Not Implemented.";
    /// if (info.isNull()) {
    ///     qDebug() << "operator<< info is nullptr";
    ///     return xml;
    /// }

    /// QString tmpString;

    /// xml.writeStartElement("card");

    /// // variable - assigned properties
    /// xml.writeTextElement("name", info->getName());
    /// xml.writeTextElement("text", info->getText());
    /// if (info->getIsToken()) {
    ///     xml.writeTextElement("token", "1");
    /// }

    /// // generic properties
    /// xml.writeStartElement("prop");
    /// for (QString propName : info->getProperties()) {
    ///     //properties.value(propertyName)

    ///     if (info->isPropertyAList(propName)) {
    ///         QStringList values = info->getPropertyList(propName);
    ///         xml.writeStartElement(propName);
    ///         QString slicedName = propName.sliced(0, propName.length() - 1);
    ///         for (QString propValue : values) {
    ///             xml.writeTextElement(slicedName, propValue);
    ///         }

    ///         xml.writeEndElement();
    ///     } else {
    ///         xml.writeTextElement(propName, info->getProperty(propName));
    ///     }
    /// }
    /// xml.writeEndElement();

    /// // sets
    /// for (CardInfoPerSet set : info->getSets()) {
    ///     xml.writeStartElement("set");
    ///     for (QString propName : set.getProperties()) {
    ///         xml.writeAttribute(propName, set.getProperty(propName));
    ///     }

    ///     xml.writeCharacters(set.getPtr()->getShortName());
    ///     xml.writeEndElement();
    /// }

    /// // related cards
    /// const QList<CardRelation *> related = info->getRelatedCards();
    /// for (auto i : related) {
    ///     xml.writeStartElement("related");
    ///     if (i->getDoesAttach()) {
    ///         xml.writeAttribute("attach", i->getAttachTypeAsString());
    ///     }
    ///     if (i->getIsCreateAllExclusion()) {
    ///         xml.writeAttribute("exclude", "exclude");
    ///     }
    ///     if (i->getIsPersistent()) {
    ///         xml.writeAttribute("persistent", "persistent");
    ///     }
    ///     if (i->getIsVariable()) {
    ///         if (1 == i->getDefaultCount()) {
    ///             xml.writeAttribute("count", "x");
    ///         } else {
    ///             xml.writeAttribute("count", "x=" + QString::number(i->getDefaultCount()));
    ///         }
    ///     } else if (1 != i->getDefaultCount()) {
    ///         xml.writeAttribute("count", QString::number(i->getDefaultCount()));
    ///     }
    ///     xml.writeCharacters(i->getName());
    ///     xml.writeEndElement();
    /// }
    /// const QList<CardRelation *> reverseRelated = info->getReverseRelatedCards();
    /// for (auto i : reverseRelated) {
    ///     xml.writeStartElement("reverse-related");
    ///     if (i->getDoesAttach()) {
    ///         xml.writeAttribute("attach", i->getAttachTypeAsString());
    ///     }

    ///     if (i->getIsCreateAllExclusion()) {
    ///         xml.writeAttribute("exclude", "exclude");
    ///     }

    ///     if (i->getIsPersistent()) {
    ///         xml.writeAttribute("persistent", "persistent");
    ///     }
    ///     if (i->getIsVariable()) {
    ///         if (1 == i->getDefaultCount()) {
    ///             xml.writeAttribute("count", "x");
    ///         } else {
    ///             xml.writeAttribute("count", "x=" + QString::number(i->getDefaultCount()));
    ///         }
    ///     } else if (1 != i->getDefaultCount()) {
    ///         xml.writeAttribute("count", QString::number(i->getDefaultCount()));
    ///     }
    ///     xml.writeCharacters(i->getName());
    ///     xml.writeEndElement();
    /// }

    /// // positioning
    /// xml.writeTextElement("tablerow", QString::number(info->getTableRow()));
    /// if (info->getIsCrypt()) {
    ///     xml.writeTextElement("cipt", "1");
    /// }
    /// if (info->getUpsideDownArt()) {
    ///     xml.writeTextElement("upsidedown", "1");
    /// }

    /// xml.writeEndElement(); // card

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
