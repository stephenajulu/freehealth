/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2011 by Eric MAEKER, MD (France) <eric.maeker@free.fr>        *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/

/**
  \class DrugsDB::InteractionManager
  \todo write documentation
  \sa DrugsDB::DrugsInteractionResult, DrugsDB::DrugsInteractionQuery
*/

#include "interactionmanager.h"

#include <drugsbaseplugin/idrugengine.h>
#include <drugsbaseplugin/idrug.h>
#include <drugsbaseplugin/drugsbase.h>
#include <drugsbaseplugin/idruginteraction.h>

#include <coreplugin/icore.h>
#include <coreplugin/itheme.h>

#include <utils/log.h>
#include <translationutils/constanttranslations.h>
#include <extensionsystem/pluginmanager.h>

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QFile>
#include <QDir>
#include <QMultiHash>
#include <QMap>
#include <QMultiMap>
#include <QList>
#include <QSet>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QStandardItemModel>

namespace  {
    const char* const LIST_MASK =
            "<table border=1 cellpadding=2 cellspacing=2 width=100%>\n"
            "<tr>\n"
            "  <td align=center>\n"
            "   <span style=\"font-weight:bold\">%1\n</span>"
            "  </td>\n"
//            "<tr>\n"
//            "  <td colspan=2 align=center>\n"
//            "   <span style=\"font-weight:bold\">%2\n</span>"
//            "</td>\n"
            "</tr>\n"
            "<tr><td>%2</td></tr>\n"
            "</table>\n";
}

using namespace DrugsDB;
using namespace Trans::ConstantTranslations;

static inline ExtensionSystem::PluginManager *pluginManager() { return ExtensionSystem::PluginManager::instance(); }


DrugInteractionQuery::DrugInteractionQuery(const QVector<IDrug *> &testDrugs, QObject *parent) :
        QObject(parent), m_Drugs(testDrugs), m_TestDDI(true), m_TestPDI(true), m_StandardModel(0)
{
}

DrugInteractionQuery::DrugInteractionQuery(QObject *parent) :
        QObject(parent), m_TestDDI(true), m_TestPDI(true), m_StandardModel(0)
{
}

DrugInteractionQuery::~DrugInteractionQuery()
{
}

void DrugInteractionQuery::clearDrugsList()
{
    m_Drugs.clear();
}

void DrugInteractionQuery::setDrugsList(const QVector<IDrug *> &list)
{
    m_Drugs = list;
}

void DrugInteractionQuery::addDrug(IDrug *drug)
{
    m_Drugs.append(drug);
}

void DrugInteractionQuery::removeDrug(IDrug *drug)
{
    int id = m_Drugs.indexOf(drug);
    if (id!=-1) {
        m_Drugs.remove(id);
    }
}

void DrugInteractionQuery::removeLastInsertedDrug()
{
    if (!m_Drugs.isEmpty())
        m_Drugs.remove(m_Drugs.indexOf(m_Drugs.last()));
}

bool DrugInteractionQuery::containsDrug(const IDrug *drug) const
{
    return m_Drugs.contains((IDrug*)drug);
}

QStandardItemModel *DrugInteractionQuery::toStandardModel() const
{
    if (!m_StandardModel) {
        m_StandardModel = new QStandardItemModel;
    }

    // for all drugs
    QFont bold;
    bold.setBold(true);
    for(int i=0; i < m_Drugs.count(); ++i) {
        IDrug *drug = m_Drugs.at(i);
        // add a root item
        QStandardItem *root = new QStandardItem(drug->brandName());
        root->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_StandardModel->appendRow(root);
        /** \todo foreach component of the drug, append total daily dose */
    }
    return m_StandardModel;
}

void DrugInteractionQuery::warn() const
{
    QString tmp;
    for(int i=0; i < m_Drugs.count(); ++i) {
        tmp += "  * " + m_Drugs.at(i)->brandName() + "\n";
    }
    if (tmp.isEmpty())
        tmp = "  !! No drug\n";
    tmp = QString("DrugInteractionQuery: testing\n%1"
                  "  * TestDDI: %2 \n"
                  "  * TestPDI: %3")
          .arg(tmp)
          .arg(m_TestDDI)
          .arg(m_TestPDI);
    qWarning() << tmp;
}


DrugInteractionResult::DrugInteractionResult(const QVector<IDrugInteraction *> &interactions, QObject *parent) :
        QObject(parent), m_Interactions(interactions), m_DDITested(false), m_PDITested(false), m_StandardModel(0)
{
}

DrugInteractionResult::DrugInteractionResult(QObject *parent) :
        QObject(parent), m_DDITested(false), m_PDITested(false), m_StandardModel(0)
{
}

DrugInteractionResult::~DrugInteractionResult()
{
    qDeleteAll(m_Interactions);
    m_Interactions.clear();
    if (m_StandardModel)
        delete m_StandardModel;
}

void DrugInteractionResult::clear()
{
    qDeleteAll(m_Interactions);
    m_Interactions.clear();
    m_DDITested = false;
    m_PDITested = false;
}

QVector<IDrugInteraction *> DrugInteractionResult::interactions(const QString &engineUid) const
{
    QVector<IDrugInteraction *> toReturn;
    for(int i = 0; i < m_Interactions.count(); ++i) {
        IDrugInteraction *di = m_Interactions.at(i);
        if (di->engine()->uid() == engineUid || engineUid.isEmpty()) {
            toReturn << di;
        }
    }
    return toReturn;
}

bool DrugInteractionResult::drugHaveInteraction(const IDrug *drug, const QString &engineUid) const
{
    for(int i = 0; i < m_Interactions.count(); ++i) {
        IDrugInteraction *di = m_Interactions.at(i);
        if (di->engine()->uid()==engineUid || engineUid.isEmpty()) {
            if (di->drugs().contains((IDrug*)drug))
                return true;
        }
    }
    return false;
}

QVector<IDrugInteraction *> DrugInteractionResult::getInteractions(const IDrug *drug, const QString &engineUid) const
{
    QVector<IDrugInteraction *> toReturn;
    for(int i = 0; i < m_Interactions.count(); ++i) {
        IDrugInteraction *di = m_Interactions.at(i);
        if (di->engine()->uid() == engineUid || engineUid.isEmpty()) {
            if (di->drugs().contains((IDrug*)drug))
                toReturn << di;
        }
    }
    return toReturn;
}

QIcon DrugInteractionResult::maxLevelOfInteractionIcon(const IDrug *drug, const int levelOfWarning, const int size, const QString &engineUid)
{
    QVector<IDrugInteraction *> dis = getInteractions(drug, engineUid);
    if (dis.isEmpty()) {
        return QIcon();
    }
    IDrugEngine *engine = dis.at(0)->engine();
    return engine->maximumInteractingLevelIcon(dis, drug, levelOfWarning, size);
}

QStandardItemModel *DrugInteractionResult::toStandardModel() const
{
    if (!m_StandardModel) {
        m_StandardModel = new QStandardItemModel;
    }
    m_StandardModel->clear();

    // get all engines
    QVector<IDrugEngine *> engines;
    for(int i=0; i < m_Interactions.count(); ++i) {
        IDrugEngine *engine = m_Interactions.at(i)->engine();
        if (!engines.contains(engine))
            engines << engine;
    }

    // for all engines
    QFont bold;
    bold.setBold(true);
    for(int i=0; i<engines.count();++i) {
        // add a root item
        IDrugEngine *engine = engines.at(i);
        QStandardItem *root = new QStandardItem(engine->icon(Core::ITheme::MediumIcon),engine->name());
        root->setData(-1, Qt::UserRole);
        root->setFont(bold);
        root->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_StandardModel->appendRow(root);

        // for all interactions for this engine
        QHash<QString, QStandardItem *> levels;
        for(int j=0; j < m_Interactions.count(); ++j) {
            IDrugInteraction *di = m_Interactions.at(j);
            if (di->engine()!=engine)
                continue;

//            // No double
//            if (id_di.contains(di->value(Internal::DrugsInteraction::DI_Id).toInt()))
//                continue;
//            id_di << di->value(Internal::DrugsInteraction::DI_Id).toInt();

            // Get the interaction level item
            QStandardItem *level = levels.value(di->type(), 0);
            if (!level) {
                level = new QStandardItem(di->type());
                level->setIcon(di->icon());
                level->setFont(bold);
//                level->setForeground(QBrush(Qt::red));
                level->setData(-1, Qt::UserRole);
                level->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                root->appendRow(level);
                levels.insert(di->type(), level);
            }

            // Include the interaction's datas
            QString h = di->header();
            QStringList splitters;
            splitters << "<br>" << "<br />";
            bool interactorsAdded = false;
            foreach(const QString &s, splitters) {
                if (h.contains(s)) {
                    foreach(const QString &part, h.split(s)) {
                        QStandardItem *interactor = new QStandardItem(part.trimmed());
                        interactor->setData(j, Qt::UserRole);
                        interactor->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                        level->appendRow(interactor);
                        interactorsAdded = true;
                    }
                    break;
                }
            }
            if (!interactorsAdded) {
                QStandardItem *interactors = new QStandardItem(di->header());
                interactors->setData(j, Qt::UserRole);
                interactors->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                level->appendRow(interactors);
            }

//            QStandardItem *risk = new QStandardItem(di->risk());
//            QLabel *riskLabel = new QLabel(QString("%1: %2")
//                                           .arg(QCoreApplication::translate(Constants::DRUGSBASE_TR_CONTEXT, Constants::NATURE_OF_RISK))
//                                           .arg(di->value(Internal::DrugsInteraction::DI_Risk).toString()));
//            risk->setWordWrap(true);
//            tree->setItemWidget(risk, 0, riskLabel);

//            QStandardItem *management = new QStandardItem(di->management());
//            QLabel *managementLabel = new QLabel(QString("%1: %2")
//                                                 .arg(tr("Management: "))
//                                                 .arg(di->value(Internal::DrugsInteraction::DI_Management).toString()));
//            managementLabel->setWordWrap(true);
//            managementLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//            tree->setItemWidget(management, 0, managementLabel);
//            managementLabel->setMargin(0);
            //        qWarning() << managementLabel << managementLabel->contentsMargins();
//            interactors->appendRow(risk);
//            interactors->appendRow(management);
        }
    }
    return m_StandardModel;
}

void DrugInteractionResult::warn() const
{
    // get all IDrugEngine names
    QStringList names;
    for(int i=0; i<m_Interactions.count();++i) {
        if (!names.contains(m_Interactions.at(i)->engine()->name()))
            names << m_Interactions.at(i)->engine()->name();
    }
    QString tmp = QString("DrugInteractionResult: %1\n"
                          "    (DDITested: %2; PDITested: %3)\n"
                          "    (NbOfInteractions: %4)")
            .arg(names.join("; "))
            .arg(m_DDITested)
            .arg(m_PDITested)
            .arg(m_Interactions.count());
    qWarning() << tmp;
}


namespace DrugsDB {
namespace Internal {

class InteractionManagerPrivate
{
public:
    InteractionManagerPrivate() :
            m_LogChrono(false)
    {
    }

    ~InteractionManagerPrivate()
    {
    }

public:
    QVector<IDrugEngine*> m_Engines;
    bool m_LogChrono;
};

}  // End Internal
}  // End Drugs

//--------------------------------------------------------------------------------------------------------
//---------------------------------------- Managing drugs interactions -----------------------------------
//--------------------------------------------------------------------------------------------------------
InteractionManager *InteractionManager::m_Instance = 0;

InteractionManager *InteractionManager::instance(QObject *parent)
{
    if (!m_Instance)
        m_Instance = new InteractionManager(parent);
    return m_Instance;
}

InteractionManager::InteractionManager(QObject *parent) :
        QObject(parent), d(0)
{
    static int handler = 0;
    ++handler;
    d = new Internal::InteractionManagerPrivate();
    setObjectName("InteractionManager" + QString::number(handler));

    // Get all engines from plugin manager
    d->m_Engines = pluginManager()->getObjects<IDrugEngine>().toVector();
    connect(pluginManager(), SIGNAL(objectAdded(QObject*)), this, SLOT(onNewObjectAddedToPluginManagerPool(QObject*)));
}

InteractionManager::~InteractionManager()
{
    if (d) delete d;
    d=0;
}

DrugInteractionResult *InteractionManager::checkInteractions(const DrugInteractionQuery &query)
{
    QTime t;
    t.start();

    int nbInteractions = 0;
    DrugInteractionResult *result = new DrugInteractionResult;
    for(int i = 0; i < d->m_Engines.count(); ++i) {
        IDrugEngine *engine = d->m_Engines.at(i);

        qWarning() << "DrugEngine" << engine->name() << "Compute" << (engine->isActive() && engine->canComputeInteractions()) << "nbDrugs" << query.drugsList().count();

        if (!engine->isActive() || !engine->canComputeInteractions())
            continue;

        nbInteractions += engine->calculateInteractions(query.drugsList());

        if (engine->isCalculatingDrugDrugInteractions())
            result->setDDITested(true);
        if (engine->isCalculatingPatientDrugInteractions())
            result->setPDITested(true);

        result->addInteractions(engine->getAllInteractionsFound());

        if (d->m_LogChrono)
            Utils::Log::logTimeElapsed(t, engine->name(), QString("calculateInteractions() : Engine %1").arg(engine->name()));
    }

    return result;
}

///** \todo move this */
//QIcon InteractionManager::interactionIcon(const int level, const int levelOfWarning, bool medium)  // static
//{
//    using namespace DrugsDB::Constants;
//    Core::ITheme *th = Core::ICore::instance()->theme();
//    Core::ITheme::IconSize size = Core::ITheme::SmallIcon;
//    if (medium)
//        size = Core::ITheme::MediumIcon;
//    if (level & Interaction::ContreIndication)
//        return th->icon(INTERACTION_ICONCRITICAL, size);
//    else if (level & Interaction::Deconseille)
//        return th->icon(INTERACTION_ICONDECONSEILLEE, size);
//    else if ((level & Interaction::APrendreEnCompte) && (levelOfWarning <= 1))
//        return th->icon(INTERACTION_ICONTAKEINTOACCOUNT, size);
//    else if ((level & Interaction::P450) && (levelOfWarning <= 1))
//        return th->icon(INTERACTION_ICONP450, size);
//    else if ((level & Interaction::GPG) && (levelOfWarning <= 1))
//        return th->icon(INTERACTION_ICONGPG, size);
//    else if ((level & Interaction::Precaution) && (levelOfWarning <= 1))
//        return th->icon(INTERACTION_ICONPRECAUTION, size);
//    else if ((level & Interaction::Information) && (levelOfWarning == 0))
//        return th->icon(INTERACTION_ICONINFORMATION, size);
//    else if ((level & Interaction::InnDuplication) && (levelOfWarning == 0))
//        return th->icon(INTERACTION_ICONINFORMATION, size);
//    else if (level & Interaction::noIAM)
//        return th->icon(INTERACTION_ICONOK, size);
//    else
//        return th->icon(INTERACTION_ICONUNKONW, size);
//}

//QIcon InteractionManager::iamIcon(const IDrug *drug, const int &levelOfWarning, bool medium) const
//{
//    /** \todo code here, move this */
////    using namespace DrugsDB::Constants;
////    Core::ITheme::IconSize size = Core::ITheme::SmallIcon;
////    if (medium)
////        size = Core::ITheme::MediumIcon;
////    Core::ITheme *th = Core::ICore::instance()->theme();
////    if (drugHaveInteraction(drug)) {
////        Interaction::TypesOfIAM r = getMaximumTypeOfIAM(drug);
////        return interactionIcon(r, levelOfWarning);
////    } else if (levelOfWarning <= 1) {
////        if (!Internal::DrugsBase::instance()->drugsINNIsKnown(drug))
////            return th->icon(INTERACTION_ICONUNKONW,size);
////        else return th->icon(INTERACTION_ICONOK,size);
////    }
//    return QIcon();
//}

QString InteractionManager::listToHtml(const QVector<IDrugInteraction *> &list, bool fullInfos) // static
{
    QString tmp, toReturn;

    // get all engines
    QVector<IDrugEngine*> engines;
    for(int i=0; i < list.count(); ++i) {
        if (!engines.contains(list.at(i)->engine()))
            engines << list.at(i)->engine();
    }

    // for all engine create the interaction list
    for(int i=0; i<engines.count(); ++i) {
        IDrugEngine *eng = engines.at(i);
        for(int j=0; j < list.count(); ++j) {
            IDrugInteraction *di = list.at(j);
            if (di->engine()==eng) {
//                tmp += di->toHtml(fullInfos);
                tmp += "-&nbsp;" + di->type() + "<br />";
            }
        }
        if (!tmp.isEmpty()) {
            tmp.chop(6);
            toReturn.append(QString(LIST_MASK)
                            .arg(eng->name())
                            .arg(tmp));
        }
    }

    return toReturn;
}

QString InteractionManager::drugInteractionSynthesisToHtml(const IDrug *drug, const QVector<IDrugInteraction *> &list, bool fullInfos)
{
    // create the interaction list
    QVector<IDrugInteraction *> interactions;
    for(int i=0; i < list.count(); ++i) {
        IDrugInteraction *interaction = list.at(i);

        if (interaction->drugs().contains((IDrug*)drug)) {
            interactions << interaction;
        }
    }

    return synthesisToHtml(interactions, fullInfos);
}

QString InteractionManager::synthesisToHtml(const QVector<IDrugInteraction *> &list, bool fullInfos) // static
{
    QString display;

    // get all engines
    QVector<IDrugEngine*> engines;
    for(int i=0; i < list.count(); ++i) {
        if (!engines.contains(list.at(i)->engine()))
            engines << list.at(i)->engine();
    }

    // for all engine create the interaction list
    for(int j=0; j<engines.count(); ++j) {
        IDrugEngine *eng = engines.at(j);
        QList<IDrug *> concernedDrugs;
        // retrieve all tested drugs
        for(int i=0; i < list.count(); ++i) {
            IDrugInteraction *interaction = list.at(i);

            if (interaction->engine()!=eng)
                continue;

            foreach(IDrug *drg, interaction->drugs()) {
                if (!concernedDrugs.contains(drg))
                    concernedDrugs.append(drg);
            }
        }

        // add tested drug brand names
        display.append(QString("<p><center>%1</center></p><p>").arg(eng->name()));
        for(int i = 0; i < concernedDrugs.count(); ++i) {
            IDrug *drg = concernedDrugs.at(i);
            display.append(QString("%1&nbsp;.&nbsp;%2<br />")
                           .arg(i)
                           .arg(drg->brandName()));
        }
        display.append("</p><p>");

        if (concernedDrugs.count() > 0) {
            for(int i=0; i<list.count(); ++i)
                display.append(list.at(i)->toHtml(true));
        } else {
            display = tkTr(Trans::Constants::NO_1_FOUND).arg(tkTr(Trans::Constants::INTERACTION));
        }
        display.append("</p>");
    }
    return display;
}

void InteractionManager::synthesisToTreeWidget(const QList<IDrugInteraction *> &list, QTreeWidget *tree) // static
{
    /** \todo code here */
    Q_ASSERT(tree);
//    using namespace DrugsDB::Constants;
//    QString tmp, toReturn;
//    QList<int> id_di;
//    QHash<QString, QTreeWidgetItem *> categories;
//    QFont bold;
//    bold.setBold(true);

//    foreach(Internal::DrugsInteraction *di, list) {

//        // No double
//        if (id_di.contains(di->value(Internal::DrugsInteraction::DI_Id).toInt()))
//            continue;
//        id_di << di->value(Internal::DrugsInteraction::DI_Id).toInt();

//        // Get the category
//        QTreeWidgetItem *category;
//        const QString &catName = di->value(Internal::DrugsInteraction::DI_TypeId).toString();
//        if (!categories.value(catName)) {
//            category = new QTreeWidgetItem(tree, QStringList() << catName);
//            category->setExpanded(true);
//            category->setFont(0, bold);
//            category->setForeground(0, QBrush(Qt::red));
//            categories.insert(catName, category);
//        }

//        // Include the interaction's datas
//        QTreeWidgetItem *interactors = new QTreeWidgetItem(category, QStringList()
//                                                           << QString("%1 <-> %2").arg(di->value(Internal::DrugsInteraction::DI_ATC1_Label).toString()).arg(di->value(Internal::DrugsInteraction::DI_ATC2_Label).toString()));
//        interactors->setFont(0, bold);
//        QTreeWidgetItem *risk = new QTreeWidgetItem(interactors);
//        QLabel *riskLabel = new QLabel(QString("%1: %2")
//                                       .arg(QCoreApplication::translate(Constants::DRUGSBASE_TR_CONTEXT, Constants::NATURE_OF_RISK))
//                                       .arg(di->value(Internal::DrugsInteraction::DI_Risk).toString()));
//        riskLabel->setWordWrap(true);
//        tree->setItemWidget(risk, 0, riskLabel);

//        QTreeWidgetItem *management = new QTreeWidgetItem(interactors);
//        QLabel *managementLabel = new QLabel(QString("%1: %2")
//                                             .arg(tr("Management: "))
//                                             .arg(di->value(Internal::DrugsInteraction::DI_Management).toString()));
//        managementLabel->setWordWrap(true);
//        managementLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//        tree->setItemWidget(management, 0, managementLabel);
//        managementLabel->setMargin(0);
//        //        qWarning() << managementLabel << managementLabel->contentsMargins();
//    }
}

void InteractionManager::onNewObjectAddedToPluginManagerPool(QObject *object)
{
    IDrugEngine *engine = qobject_cast<IDrugEngine*>(object);
    if (!engine)
        return;
    d->m_Engines.append(engine);
}
