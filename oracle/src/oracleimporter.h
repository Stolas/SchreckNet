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
    SetToDownload(QString _longName, QString _shortName,
                  QList<QVariant> _cards,
                  QString _setType = QString(),
                  const QDate &_releaseDate = QDate())
        : longName(std::move(_longName)), shortName(std::move(_shortName)), cards(std::move(_cards)),
          releaseDate(_releaseDate), setType(std::move(_setType))
    { }

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
