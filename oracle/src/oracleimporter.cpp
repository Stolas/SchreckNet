#include "oracleimporter.h"

#include "carddbparser/schrecknet.h"
#include "qt-json/json.h"

#include <QtWidgets>
#include <algorithm>
#include <climits>

SplitCardPart::SplitCardPart(const QString &_name,
                             const QString &_text,
                             const QVariantHash &_properties,
                             const CardInfoPerSet _setInfo)
    : name(_name), text(_text), properties(_properties), setInfo(_setInfo)
{
}

OracleImporter::OracleImporter(const QString &_dataDir, QObject *parent) : CardDatabase(parent), dataDir(_dataDir)
{
}

bool OracleImporter::readSetsFromByteArray(const QByteArray &data)
{
    QList<SetToDownload> newSetList;
    QSet<QString> setNames;
    QList<QVariant> setCards;

    bool ok;

    auto jsonMap = QtJson::Json::parse(QString(data), ok).toMap();
    if (!ok) {
        qDebug() << "error: QtJson::Json::parse()";
        return false;
    }
    auto setList = jsonMap.value("sets").toList();
    auto cardList = jsonMap.value("cards").toList();
    auto setcnt = setList.size();
    auto cardcnt = cardList.size();

    Q_FOREACH(QVariant set, setList) {
        auto setMap = set.toMap();
        auto releaseDate = setMap.value("release_date").toString();
        auto shortName = setMap.value("short_name").toString();
        auto setName = setMap.value("name").toString();

        /* Todo; For each minion (vamps and allies) calculate Bleed, Strength  For Vamps also Votes and for allies also life. */
        /* X life. X strength, X bleed. */

        // Todo; SchreckNet should instead of for each set go over all
        //                  cards it should add all sets and then for
        //                  each card add to the correct sets.
        setCards.empty();
        Q_FOREACH (QVariant card, cardList) {
            // Todo; SchreckNet, handle cards like remove the lists..
            auto cardMap = card.toMap();
            auto name = cardMap.value("name").toString();
            
            auto cardSets = cardMap.value("sets").toList();
            if (cardSets.contains(shortName)) {
                setCards.append(card);
            }

        }

        newSetList.append(SetToDownload(setName, shortName, setCards));
    }



    // auto size = cardArray.size();
    // Q_FOREACH (auto card, cardArray) {
    //     auto cardSets = card.toObject().value("sets").toObject();
    //     // For each Set the card has.
    //     Q_FOREACH (auto setName, cardSets.keys()) {
    //         // Normalize all Promo sets.
    //         if (setName.toLower().contains("promo")) { setName = "Promo"; }

    //         // Check if in a known set.
    //         bool knownSet = false;
    //         Q_FOREACH (auto set_, newSetList) {
    //             if (set_.getLongName() == setName) {
    //                 // Set is known, and add to list.
    //                 auto num = set_.addCard(card.toObject());
    //                 knownSet = true;
    //                 break;
    //             }
    //         }
    //         if (!knownSet) {
    //             setCards.empty();
    //             setCards.append(card.toObject());
    //             newSetList.append(SetToDownload(setName, setCards));
    //         }
    //     }
    // }



    //     // Old Slow Method
    //     auto cardSets = card.toObject().value("sets").toObject();
    //     Q_FOREACH (auto setName, cardSets.keys()) {
    //         setNames.insert(setName);
    //     }
    // }

    // // For Each Set
    // Q_FOREACH (auto setName, setNames) {
    //     setCards.empty();

    //     // Check each card to be in the set
    //     Q_FOREACH (auto card, cardArray) {
    //         auto cardSets = card.toObject().value("sets").toObject();
    //         if (cardSets.keys().contains(setName)) {
    //             // The card is in the current set to add.
    //             setCards.append(card.toObject());
    //         }
    //     }

    //     // Add all the sets and cards  to the newsetList
    //     // Schrecknet Note; releaseDate is already a tad insane.
    //     newSetList.append(SetToDownload(setName, setCards));
    // }

    std::sort(newSetList.begin(), newSetList.end());

    if (newSetList.isEmpty()) {
        return false;
    }
    allSets = newSetList;
    return true;
}

QString OracleImporter::getMainCardType(const QStringList &typeList)
{
    if (typeList.isEmpty()) {
        return {};
    }

    if (typeList.contains("Master")) {
        return QString("Master");
    }

    if (typeList.contains("Event")) {
        return QString("Event");
    }

    if (typeList.contains("Vampire") || typeList.contains("Imbeud")) {
        return QString("Crypt");
    }
    return QString("Minion");
}

CardInfoPtr OracleImporter::addCard(QString name,
                                    QString text,
                                    bool isToken,
                                    QVariantHash properties,
                                    QList<CardRelation *> &relatedCards,
                                    CardInfoPerSet setInfo)
{
    // Workaround for card name weirdness
    if (cards.contains(name)) {
        CardInfoPtr card = cards.value(name);
        card->addToSet(setInfo.getPtr(), setInfo);
        return card;
    }


    /* Fix Properties here.. */

    // DETECT CARD POSITIONING INFO

    // cards that enter the field tapped
    bool cipt = text.contains(" it enters the battlefield tapped") ||
                (text.contains(name + " enters the battlefield tapped") &&
                 !text.contains(name + " enters the battlefield tapped unless"));

    // table row
    int tableRow = 1;
    QString mainCardType = properties.value("maintype").toString();
    if ((mainCardType == "Crypt")) {
        tableRow = 0;
    } else if (mainCardType == "Minion") {
        tableRow = 2;
    } else {
        tableRow = 3;
    }


    // insert the card and its properties
    QList<CardRelation *> reverseRelatedCards;
    CardInfoPerSetMap setsInfo;
    setsInfo.insert(setInfo.getPtr()->getShortName(), setInfo);
    CardInfoPtr newCard = CardInfo::newInstance(name, text, isToken, properties, relatedCards, reverseRelatedCards,
                                                setsInfo, cipt, tableRow);

    if (name.isEmpty()) {
        qDebug() << "warning: an empty card was added to set" << setInfo.getPtr()->getShortName();
    }
    cards.insert(name, newCard);

    return newCard;
}

QString OracleImporter::getStringPropertyFromMap(const QVariantMap &card, const QString &propertyName)
{
    return card.contains(propertyName) ? card.value(propertyName).toString() : QString("");
}

int OracleImporter::importCardsFromSet(const CardSetPtr &currentSet,
                                       const QList<QVariant> &cardsList,
                                       bool skipSpecialCards)
{
    int numCards = 0;
    QVariantMap card;
    QString name, printedName, picUrl, text, mainType;
    static const bool isToken = false;
    QVariantHash properties;
    CardInfoPerSet setInfo;
    QList<CardRelation *> relatedCards;
    const QStringList IGNORE_KEYS{"name", "card_text", "name_variants", "id", "artists", "sets", "_set", "_name", "ordered_sets"};
    const QStringList ARRAY_KEYS{"disciplines", "rulings", "clans", "types", "scans"};

    setInfo = CardInfoPerSet(currentSet);

    for (const QVariant &cardVar : cardsList) {
        card = cardVar.toMap();
        auto keys = card.keys();
        // 'id', '_name', 'url', 'types', 'clans', 'capacity', 'disciplines', 'card_text', '_set'
        // 'sets', 'scans', 'artists', 'group', 'ordered_sets', 'name_variants', 'name', 'printed_name'
        // Types: {'Combat', 'Equipment', 'Political Action', 'Reaction', 'Conviction', 'Master', 'Ally', 'Vampire',
        // 'Action Modifier', 'Power', 'Event', 'Action', 'Imbued', 'Retainer'}

        name = getStringPropertyFromMap(card, "name");
        text = getStringPropertyFromMap(card, "card_text");

        // card properties
        properties.clear();
        Q_FOREACH (auto key, card.keys()) {
            if (IGNORE_KEYS.contains(key)) { continue; }
            if (ARRAY_KEYS.contains(key)) {
                properties.insert(key, card[key].toStringList());
                continue;
            }

            properties.insert(key, card[key]);
        }

        const auto &mainCardType = getMainCardType(card.value("types").toStringList());
        if (mainCardType.isEmpty()) {
            qDebug() << "warning: no mainCardType for card:" << name;
        } else {
            properties.insert("maintype", mainCardType);
        }

        // Todo; Schrecknet, add relatedCards such as Viri and Zizi or Edith Blount and Enid Blount. (/XXX/), the code below is MTG code as a reminder.
        // relatedCards.clear();
        // static const QRegularExpression meldNameRegex{"then meld them into ([^\\.]*)"};
        // QString additionalName = meldNameRegex.match(text).captured(1);
        // if (!additionalName.isNull()) {
        //     relatedCards.append(new CardRelation(additionalName, CardRelation::TransformInto));
        // }

        CardInfoPtr newCard = addCard(name, text, isToken, properties, relatedCards, setInfo);
        numCards++;
    }

    
    //     auto legalities = card.value("legalities").toMap();
    //     for (const QString &fmtName : legalities.keys()) {
    //         properties.insert(QString("format-%1").arg(fmtName), legalities.value(fmtName).toString().toLower());
    //     }

    //     // split cards are considered a single card, enqueue for later merging
    //     if (layout == "split" || layout == "aftermath" || layout == "adventure") {
    //         auto _faceName = getStringPropertyFromMap(card, "faceName");
    //         SplitCardPart split(_faceName, text, properties, setInfo);
    //         auto found_iter = splitCards.find(name);
    //         if (found_iter == splitCards.end()) {
    //             splitCards.insert(name, {split});
    //         } else if (layout == "adventure") {
    //             found_iter->insert(0, split);
    //         } else {
    //             found_iter->append(split);
    //         }
    //     } else {
    //         // relations
    //         relatedCards.clear();

    //         // add other face for split cards as card relation
    //         if (!getStringPropertyFromMap(card, "side").isEmpty()) {
    //             properties["cmc"] = getStringPropertyFromMap(card, "faceManaValue");
    //             if (layout == "meld") { // meld cards don't work
    //                 static const QRegularExpression meldNameRegex{"then meld them into ([^\\.]*)"};
    //                 QString additionalName = meldNameRegex.match(text).captured(1);
    //                 if (!additionalName.isNull()) {
    //                     relatedCards.append(new CardRelation(additionalName, CardRelation::TransformInto));
    //                 }
    //             } else {
    //                 for (const QString &additionalName : name.split(" // ")) {
    //                     if (additionalName != faceName) {
    //                         relatedCards.append(new CardRelation(additionalName, CardRelation::TransformInto));
    //                     }
    //                 }
    //             }
    //             name = faceName;
    //         }

    //         // mtgjon related cards
    //         if (card.contains("relatedCards")) {
    //             QVariantMap givenRelated = card.value("relatedCards").toMap();
    //             // conjured cards from a spellbook
    //             if (givenRelated.contains("spellbook")) {
    //                 auto spbk = givenRelated.value("spellbook").toStringList();
    //                 for (const QString &spbkName : spbk) {
    //                     relatedCards.append(
    //                         new CardRelation(spbkName, CardRelation::DoesNotAttach, false, false, 1, true));
    //                 }
    //             }
    //         }

    //         CardInfoPtr newCard = addCard(name + numComponent, text, isToken, properties, relatedCards, setInfo);
    //         numCards++;
    //     }
    // }

    // // split cards handling
    // static const QString splitCardPropSeparator = QString(" // ");
    // static const QString splitCardTextSeparator = QString("\n\n---\n\n");
    // for (const QString &nameSplit : splitCards.keys()) {
    //     // get all parts for this specific card
    //     QList<SplitCardPart> splitCardParts = splitCards.value(nameSplit);
    //     QSet<QString> done{};

    //     text.clear();
    //     properties.clear();
    //     relatedCards.clear();

    //     for (const SplitCardPart &tmp : splitCardParts) {
    //         // some sets have 2 different variations of the same split card,
    //         // eg. Fire // Ice in WC02. Avoid adding duplicates.
    //         QString splitName = tmp.getName();
    //         if (done.contains(splitName)) {
    //             continue;
    //         }
    //         done.insert(splitName);

    //         if (!text.isEmpty()) {
    //             text.append(splitCardTextSeparator);
    //         }
    //         text.append(tmp.getText());

    //         if (properties.isEmpty()) {
    //             properties = tmp.getProperties();
    //             setInfo = tmp.getSetInfo();
    //         } else {
    //             const QVariantHash &tmpProps = tmp.getProperties();
    //             for (const QString &prop : tmpProps.keys()) {
    //                 QString originalPropertyValue = properties.value(prop).toString();
    //                 QString thisCardPropertyValue = tmpProps.value(prop).toString();
    //                 if (!thisCardPropertyValue.isEmpty() && originalPropertyValue != thisCardPropertyValue) {
    //                     if (originalPropertyValue.isEmpty()) { // don't create //es if one field is empty
    //                         properties.insert(prop, thisCardPropertyValue);
    //                     } else if (prop == "colors") { // the card is both colors
    //                         properties.insert(prop, originalPropertyValue + thisCardPropertyValue);
    //                     } else if (prop == "maintype") { // don't create maintypes with //es in them
    //                         continue;
    //                     } else {
    //                         properties.insert(prop,
    //                                           originalPropertyValue + splitCardPropSeparator + thisCardPropertyValue);
    //                     }
    //                 }
    //             }
    //         }
    //     }
    //     CardInfoPtr newCard = addCard(nameSplit, text, isToken, properties, relatedCards, setInfo);
    //     numCards++;
    // }

    // // only add the unique promo cards that didn't already exist in the set
    // if (skipSpecialCards) {
    //     QList<QVariant> nonDuplicatePromos;
    //     for (auto cardIter = specialPromoCards.constBegin(); cardIter != specialPromoCards.constEnd(); ++cardIter) {
    //         if (!allNameProps.contains(cardIter.key())) {
    //             nonDuplicatePromos.append(cardIter.value());
    //         }
    //     }
    //     if (!nonDuplicatePromos.isEmpty()) {
    //         numCards += importCardsFromSet(currentSet, nonDuplicatePromos, false);
    //     }
    // }
    return numCards;
}

// void OracleImporter::sortAndReduceColors(QString &colors)
// {
//     // sort
//     const QHash<QChar, unsigned int> colorOrder{{'W', 0}, {'U', 1}, {'B', 2}, {'R', 3}, {'G', 4}};
//     std::sort(colors.begin(), colors.end(), [&colorOrder](const QChar a, const QChar b) {
//         return colorOrder.value(a, INT_MAX) < colorOrder.value(b, INT_MAX);
//     });
//     // reduce
//     QChar lastChar = '\0';
//     for (int i = 0; i < colors.size(); ++i) {
//         if (colors.at(i) == lastChar)
//             colors.remove(i, 1);
//         else
//             lastChar = colors.at(i);
//     }
// }

int OracleImporter::startImport()
{
    int setCards = 0, setIndex = 0;
    // add an empty set for tokens
    CardSetPtr tokenSet = CardSet::newInstance(TOKENS_SETNAME, tr("Dummy set containing tokens"), "Tokens");
    sets.insert(TOKENS_SETNAME, tokenSet);

    for (const SetToDownload &curSetToParse : allSets) {
        CardSetPtr newSet = CardSet::newInstance(curSetToParse.getShortName(), curSetToParse.getLongName(),
                                                 curSetToParse.getSetType(), curSetToParse.getReleaseDate());
        if (!sets.contains(newSet->getShortName()))
            sets.insert(newSet->getShortName(), newSet);

        int numCardsInSet = importCardsFromSet(newSet, curSetToParse.getCards());

        ++setIndex;

        emit setIndexChanged(numCardsInSet, setIndex, curSetToParse.getLongName());
    }

    emit setIndexChanged(setCards, setIndex, QString());

    // total number of sets
    return setIndex;
}

bool OracleImporter::saveToFile(const QString &fileName, const QString &sourceUrl, const QString &sourceVersion)
{
    SchrecknetParser parser;
    return parser.saveToFile(sets, cards, fileName, sourceUrl, sourceVersion);
}

void OracleImporter::clear()
{
    CardDatabase::clear();
    allSets.clear();
}
