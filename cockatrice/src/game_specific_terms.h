#ifndef GAME_SPECIFIC_TERMS_H
#define GAME_SPECIFIC_TERMS_H

#include <QCoreApplication>
#include <QString>

namespace Vtes
{
QString const CardType("type");
QString const BloodCost("bloodcost");
QString const PoolCost("poolcost");
QString const Clans("clans");
QString const Bleed("bleed");
QString const Votes("votes");
QString const Group("group");

inline static const QString getNicePropertyName(QString key)
{
    if (key == CardType)
        return QCoreApplication::translate("Vtes", "Card Type");
    if (key == BloodCost)
        return QCoreApplication::translate("Vtes", "Blood Cost");
    if (key == PoolCost)
        return QCoreApplication::translate("Vtes", "Pool Cost");
    if (key == Clans)
        return QCoreApplication::translate("Vtes", "Clans");
    if (key == Votes)
        return QCoreApplication::translate("Vtes", "Votes");
    if (key == Group)
        return QCoreApplication::translate("Vtes", "Group");
    return key;
}
}; // namespace Vtes



/*
 * Collection of traslatable property names used in games,
 * so we can use Game::Property instead of hardcoding strings.
 * Note: Mtg = "Maybe that game"
 */

namespace Mtg
{
QString const CardType("type");
QString const ConvertedManaCost("cmc");
QString const Colors("colors");
QString const Loyalty("loyalty");
QString const MainCardType("maintype");
QString const ManaCost("manacost");
QString const PowTough("pt");
QString const Side("side");
QString const Layout("layout");
QString const ColorIdentity("coloridentity");

inline static const QString getNicePropertyName(QString key)
{
    if (key == CardType)
        return QCoreApplication::translate("Mtg", "Card Type");
    if (key == ConvertedManaCost)
        return QCoreApplication::translate("Mtg", "Mana Value");
    if (key == Colors)
        return QCoreApplication::translate("Mtg", "Color(s)");
    if (key == Loyalty)
        return QCoreApplication::translate("Mtg", "Loyalty");
    if (key == MainCardType)
        return QCoreApplication::translate("Mtg", "Main Card Type");
    if (key == ManaCost)
        return QCoreApplication::translate("Mtg", "Mana Cost");
    if (key == PowTough)
        return QCoreApplication::translate("Mtg", "P/T");
    if (key == Side)
        return QCoreApplication::translate("Mtg", "Side");
    if (key == Layout)
        return QCoreApplication::translate("Mtg", "Layout");
    if (key == ColorIdentity)
        return QCoreApplication::translate("Mtg", "Color Identity");
    return key;
}
}; // namespace Mtg

#endif
