#include "phasestoolbar.h"

#include "pb/command_draw_cards.pb.h"
#include "pb/command_next_turn.pb.h"
#include "pb/command_set_active_phase.pb.h"
#include "pb/command_set_card_attr.pb.h"
#include "pixmapgenerator.h"

#include <QAction>
#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QTimer>

PhaseButton::PhaseButton(const QString &_name, QGraphicsItem *parent, QAction *_doubleClickAction, bool _highlightable)
    : QObject(), QGraphicsItem(parent), name(_name), active(false), highlightable(_highlightable),
      activeAnimationCounter(0), doubleClickAction(_doubleClickAction), width(50)
{
    if (highlightable) {
        activeAnimationTimer = new QTimer(this);
        connect(activeAnimationTimer, SIGNAL(timeout()), this, SLOT(updateAnimation()));
        activeAnimationTimer->setSingleShot(false);
    } else
        activeAnimationCounter = 9;

    setCacheMode(DeviceCoordinateCache);
}

QRectF PhaseButton::boundingRect() const
{
    return {0, 0, width, width};
}

void PhaseButton::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    QRectF iconRect = boundingRect().adjusted(3, 3, -3, -3);
    QRectF translatedIconRect = painter->combinedTransform().mapRect(iconRect);
    qreal scaleFactor = translatedIconRect.width() / iconRect.width();
    QPixmap iconPixmap = PhasePixmapGenerator::generatePixmap(qRound(translatedIconRect.height()), name);

    painter->setBrush(QColor(static_cast<int>(220 * (activeAnimationCounter / 10.0)),
                             static_cast<int>(220 * (activeAnimationCounter / 10.0)),
                             static_cast<int>(220 * (activeAnimationCounter / 10.0))));
    painter->setPen(Qt::gray);
    painter->drawRect(0, 0, static_cast<int>(width - 1), static_cast<int>(width - 1));
    painter->save();
    resetPainterTransform(painter);
    painter->drawPixmap(iconPixmap.rect().translated(qRound(3 * scaleFactor), qRound(3 * scaleFactor)), iconPixmap,
                        iconPixmap.rect());
    painter->restore();

    painter->setBrush(QColor(0, 0, 0, static_cast<int>(255 * ((10 - activeAnimationCounter) / 15.0))));
    painter->setPen(Qt::gray);
    painter->drawRect(0, 0, static_cast<int>(width - 1), static_cast<int>(width - 1));
}

void PhaseButton::setWidth(double _width)
{
    prepareGeometryChange();
    width = _width;
}

void PhaseButton::setActive(bool _active)
{
    if ((active == _active) || !highlightable)
        return;

    active = _active;
    activeAnimationTimer->start(25);
}

void PhaseButton::updateAnimation()
{
    if (!highlightable)
        return;

    if (active) {
        if (++activeAnimationCounter >= 10)
            activeAnimationTimer->stop();
    } else {
        if (--activeAnimationCounter <= 0)
            activeAnimationTimer->stop();
    }
    update();
}

void PhaseButton::mousePressEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    emit clicked();
}

void PhaseButton::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    triggerDoubleClickAction();
}

void PhaseButton::triggerDoubleClickAction()
{
    if (doubleClickAction)
        doubleClickAction->trigger();
}

PhasesToolbar::PhasesToolbar(QGraphicsItem *parent)
    : QGraphicsItem(parent), width(100), height(100), ySpacing(1), symbolSize(8)
{
    auto *aUnlockAll = new QAction(this);
    connect(aUnlockAll, SIGNAL(triggered()), this, SLOT(actUntapAll()));
    auto *aDrawCard = new QAction(this);
    connect(aDrawCard, SIGNAL(triggered()), this, SLOT(actDrawCard()));

    /* Schrecknet Phases; Unlock, Master, Minion, Influence, Discard */
    PhaseButton *untapButton = new PhaseButton("untap", this, aUnlockAll);
    PhaseButton *masterButton = new PhaseButton("master", this);
    PhaseButton *minionButton = new PhaseButton("minion", this);
    PhaseButton *influenceButton = new PhaseButton("influence", this);
    PhaseButton *discardButton = new PhaseButton("discard", this);

    buttonList << untapButton << masterButton << minionButton << influenceButton << discardButton;

    for (auto &i : buttonList)
        connect(i, SIGNAL(clicked()), this, SLOT(phaseButtonClicked()));

    nextTurnButton = new PhaseButton("nextturn", this, nullptr, false);
    connect(nextTurnButton, SIGNAL(clicked()), this, SLOT(actNextTurn()));

    rearrangeButtons();

    retranslateUi();
}

QRectF PhasesToolbar::boundingRect() const
{
    return {0, 0, width, height};
}

void PhasesToolbar::retranslateUi()
{
    for (int i = 0; i < buttonList.size(); ++i)
        buttonList[i]->setToolTip(getLongPhaseName(i));
}

QString PhasesToolbar::getLongPhaseName(int phase) const
{
    switch (phase) {
        case 0:
            return tr("Unlock phase");
        case 1:
            return tr("Master phase");
        case 2:
            return tr("Minion phase");
        case 3:
            return tr("Influence phase");
        case 4:
            return tr("Discard phase");
        default:
            return QString();
    }
}

void PhasesToolbar::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->fillRect(boundingRect(), QColor(50, 50, 50));
}

const double PhasesToolbar::marginSize = 3;

void PhasesToolbar::rearrangeButtons()
{
    for (auto &i : buttonList)
        i->setWidth(symbolSize);
    nextTurnButton->setWidth(symbolSize);

    double y = marginSize;
    buttonList[0]->setPos(marginSize, y); /* Unlock */
    y += ySpacing;
    buttonList[1]->setPos(marginSize, y += symbolSize); /* Master */
    buttonList[2]->setPos(marginSize, y += symbolSize); /* Minion */
    y += ySpacing;
    buttonList[3]->setPos(marginSize, y += symbolSize); /* Influence*/
    buttonList[4]->setPos(marginSize, y += symbolSize); /* Discard */
    y += ySpacing;
    nextTurnButton->setPos(marginSize, y + symbolSize);
}

void PhasesToolbar::setHeight(double _height)
{
    prepareGeometryChange();

    height = _height;
    ySpacing = (height - 2 * marginSize) / (buttonCount * 5 + spaceCount);
    symbolSize = ySpacing * 5;
    width = symbolSize + 2 * marginSize;

    rearrangeButtons();
}

void PhasesToolbar::setActivePhase(int phase)
{
    if (phase >= buttonList.size())
        return;

    for (int i = 0; i < buttonList.size(); ++i)
        buttonList[i]->setActive(i == phase);
}

void PhasesToolbar::triggerPhaseAction(int phase)
{
    if (0 <= phase && phase < buttonList.size()) {
        buttonList[phase]->triggerDoubleClickAction();
    }
}

void PhasesToolbar::phaseButtonClicked()
{
    auto *button = qobject_cast<PhaseButton *>(sender());
    if (button->getActive())
        button->triggerDoubleClickAction();

    Command_SetActivePhase cmd;
    cmd.set_phase(static_cast<google::protobuf::uint32>(buttonList.indexOf(button)));

    emit sendGameCommand(cmd, -1);
}

void PhasesToolbar::actNextTurn()
{
    emit sendGameCommand(Command_NextTurn(), -1);
}

void PhasesToolbar::actUntapAll()
{
    Command_SetCardAttr cmd;
    cmd.set_zone("table");
    cmd.set_attribute(AttrTapped);
    cmd.set_attr_value("0");

    emit sendGameCommand(cmd, -1);
}

/* Todo; Schrecknet, this is not used. */
void PhasesToolbar::actDrawCard()
{
    Command_DrawCards cmd;
    cmd.set_number(1);

    emit sendGameCommand(cmd, -1);
}
