#ifndef ORACLEIMPORTER_H
#define ORACLEIMPORTER_H

#include <QMap>
#include <QVariant>
#include <carddatabase.h>
#include <utility>

class SetToDownload
{
private:
    QString shortName, longName;
    QList<QVariant> cards;
    QDate releaseDate;
    QString setType;

public:
    const QString &getShortName() const
    {
        // Todo; SchreckNet might want to have shortnames. Not sure yet.
        // if (longName.contains("Promo")) {
        //     shortName = QString("Promo");
        //     // return new QString("Promo");
        // }
        return shortName;
    }
    const QString &getLongName() const
    {
        return longName;
    }
    const QList<QVariant> &getCards() const
    {
        return cards;
    }
    const QString &getSetType() const
    {
        return setType;
    }
    const QDate &getReleaseDate() const
    {
        return releaseDate;
    }
    SetToDownload(QString _longName,
                  QList<QVariant> _cards,
                  QString _setType = QString(),
                  const QDate &_releaseDate = QDate())
        : longName(std::move(_longName)), cards(std::move(_cards)),
          releaseDate(_releaseDate), setType(std::move(_setType))
    {
        /* Schrecknet: To whomever it might concern, yes this is most def not the best way to implement this. */
        if (longName == "Jyhad") { shortName = QString("Jyhad"); return; }
        if (longName == "Vampire: The Eternal Struggle") { shortName = QString("V:TES"); return; }
        if (longName == "Dark Sovereigns") { shortName = QString("DS"); return; }
        if (longName == "Ancient Hearts") { shortName = QString("AH"); return; }
        if (longName == "Sabbat") { shortName = QString("Sabbat"); return; }
        if (longName == "Sabbat War") { shortName = QString("SW"); return; }
        if (longName == "Final Nights") { shortName = QString("FN"); return; }
        if (longName == "Bloodlines") { shortName = QString("BL"); return; }
        if (longName == "Camarilla Edition") { shortName = QString("CE"); return; }
        if (longName == "Anarchs") { shortName = QString("Anarchs"); return; }
        if (longName == "Black Hand") { shortName = QString("BH"); return; }
        if (longName == "Gehenna") { shortName = QString("Gehenna"); return; }
        if (longName == "Tenth Anniversary") { shortName = QString("Tenth"); return; }
        if (longName == "Kindred Most Wanted") { shortName = QString("KMW"); return; }
        if (longName == "Legacies of Blood") { shortName = QString("LoB"); return; }
        if (longName == "Nights of Reckoning") { shortName = QString("NoR"); return; }
        if (longName == "Third Edition") { shortName = QString("Third"); return; }
        if (longName == "Sword of Caine") { shortName = QString("SoC"); return; }
        if (longName == "Lords of the Night") { shortName = QString("LotN"); return; }
        if (longName == "Blood Shadowed Court") { shortName = QString("BSC"); return; }
        if (longName == "Twilight Rebellion") { shortName = QString("TR"); return; }
        if (longName == "Keepers of Tradition") { shortName = QString("KoT"); return; }
        if (longName == "Ebony Kingdom") { shortName = QString("EK"); return; }
        if (longName == "Heirs to the Blood") { shortName = QString("HttB"); return; }
        if (longName == "Danse Macabre") { shortName = QString("DM");  return; }
        if (longName == "The Unaligned") { shortName = QString("TA"); return; }
        /* Note; Storyline Rewards is missing. */
        if (longName == "Anarch Unbound") { shortName = QString("AU"); return; }
        if (longName == "Lost Kindred") { shortName = QString("LK"); return; }
        if (longName == "Sabbat Preconstructed") { shortName = QString("SP"); return; }
        if (longName == "Fifth Edition") { shortName = QString("V5"); return; }

        /* Todo; put these in the correct order.. */
        if (longName == "Fifth Edition (Anarch)") { shortName = QString("V5A"); return; }
        if (longName == "Shadows of Berlin") { shortName = QString("SoB"); return; }
        if (longName == "New Blood") { shortName = QString("NB"); return; }
        if (longName == "Fall of London") { shortName = QString("FoL"); return; }
        if (longName == "Anthology") { shortName = QString("Ath"); return; }
        if (longName == "New Blood II") { shortName = QString("NB2"); return; }
        if (longName == "Echoes of Gehenna") { shortName = QString("EoG"); return; }
        if (longName == "Keepers of Tradition Reprint") { shortName = QString("KoTR"); return; }
        if (longName == "Heirs to the Blood Reprint") { shortName = QString("HttBR"); return; }
        if (longName == "First Blood") { shortName = QString("1e"); return; }
        if (longName == "Twenty-Fifth Anniversary") { shortName = QString("25th"); return; }
        if (longName == "Fifth Edition (Companion)") { shortName = QString("V5C"); return; } 
        if (longName == "Print on Demand") { shortName = QString("POD"); return; }
        shortName = QString("Promo");
    }

    int addCard(QVariant card)
    {
        cards.append(card);
        return cards.length();
    }

    bool operator<(const SetToDownload &set) const
    {
        return longName.compare(set.longName, Qt::CaseInsensitive) < 0;
    }
};

class SplitCardPart
{
public:
    SplitCardPart(const QString &_name, const QString &_text, const QVariantHash &_properties, CardInfoPerSet setInfo);
    inline const QString &getName() const
    {
        return name;
    }
    inline const QString &getText() const
    {
        return text;
    }
    inline const QVariantHash &getProperties() const
    {
        return properties;
    }
    inline const CardInfoPerSet &getSetInfo() const
    {
        return setInfo;
    }

private:
    QString name;
    QString text;
    QVariantHash properties;
    CardInfoPerSet setInfo;
};

class OracleImporter : public CardDatabase
{
    Q_OBJECT
private:
    //const QStringList mainCardTypes = {"Crypt", "Master", "Minion", "Event"}; // {"Crypt",  "Master", "Political Action", "Ally",  "Equipment",
                                            // "Retainer", "Action Modifier", "Reaction", "Combat", "Event",  "Conviction", "Power", "Reflex"};
    QList<SetToDownload> allSets;
    QVariantMap setsMap;
    QString dataDir;

    QString getMainCardType(const QStringList &typeList);
    CardInfoPtr addCard(QString name,
                        QString text,
                        bool isToken,
                        QVariantHash properties,
                        QList<CardRelation *> &relatedCards,
                        CardInfoPerSet setInfo);
signals:
    void setIndexChanged(int cardsImported, int setIndex, const QString &setName);
    void dataReadProgress(int bytesRead, int totalBytes);

public:
    explicit OracleImporter(const QString &_dataDir, QObject *parent = nullptr);
    bool readSetsFromByteArray(const QByteArray &data);
    int startImport();
    bool saveToFile(const QString &fileName, const QString &sourceUrl, const QString &sourceVersion);
    int importCardsFromSet(const CardSetPtr &currentSet, const QList<QVariant> &cards, bool skipSpecialNums = true);
    QList<SetToDownload> &getSets()
    {
        return allSets;
    }
    const QString &getDataDir() const
    {
        return dataDir;
    }
    void clear();

protected:
    inline QString getStringPropertyFromMap(const QVariantMap &card, const QString &propertyName);
    void sortAndReduceColors(QString &colors);
};

#endif
