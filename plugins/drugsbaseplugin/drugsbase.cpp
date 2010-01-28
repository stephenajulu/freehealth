/***************************************************************************
 *   FreeMedicalForms                                                      *
 *   Copyright (C) 2008-2009 by Eric MAEKER                                *
 *   eric.maeker@free.fr                                                   *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   This program is a free and open source software.                      *
 *   It is released under the terms of the new BSD License.                *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *   modification, are permitted provided that the following conditions    *
 *   are met:                                                              *
 *   - Redistributions of source code must retain the above copyright      *
 *   notice, this list of conditions and the following disclaimer.         *
 *   - Redistributions in binary form must reproduce the above copyright   *
 *   notice, this list of conditions and the following disclaimer in the   *
 *   documentation and/or other materials provided with the distribution.  *
 *   - Neither the name of the FreeMedForms' organization nor the names of *
 *   its contributors may be used to endorse or promote products derived   *
 *   from this software without specific prior written permission.         *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS     *
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  *
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      *
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
 *   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT    *
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN     *
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
 *   POSSIBILITY OF SUCH DAMAGE.                                           *
 ***************************************************************************/
/***************************************************************************
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/

/**
  \class DrugsBase
  \brief This class owns the drugs and dosages database and interactions mechanism.

  0. Terminology\n
  \e Substances are the chimic molecules that compose a drug.\n
  \e IamCode or \e INN are the INN codes and names.\n
  \e IamClass are the classes that regroups INNs into classes of pharmaceutics family.\n
  \e CIP : presentation code of a drug. A drug can be presented into different presentation (15 pills, 30 pills a box...).\n
  \e CIS : speciality code of a drug. Everything is base on this code.
     One CIS code can be associated to many CIP, many Substances, many INNs, and many IamClasses.

  1. Initialization\n
  This class is pure static, so you can not instanciate it. To initialize datas, just do once : init().
  isInitialized() alerts you of the state of intialization.
  These two members returns true if all is ok.

  2. Drugs retreiver\n
  You can retreive drugs using CIS ou CIP code via getDrugByCIS() and getDrufByCIP().

  3. Dosages retreiver / saver

  \todo Manage user rights when creating dosage database

  \sa database_structure
  \ingroup freediams drugswidget
*/

#include "drugsbase.h"

//include drugswidget headers
#include <drugsbaseplugin/drugsdata.h>
#include <drugsbaseplugin/drugsinteraction.h>
#include <drugsbaseplugin/constants.h>
#include <drugsbaseplugin/versionupdater.h>

#include <utils/global.h>
#include <utils/log.h>

#include <coreplugin/isettings.h>
#include <coreplugin/icore.h>

// include Qt headers
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

using namespace DrugsDB;
using namespace DrugsDB::Internal;
using namespace Trans::ConstantTranslations;

namespace DrugsDB {
namespace Internal {
/**
  \brief Private part of DrugsBase
  \internal
*/
class DrugsBasePrivate
{
public:
    DrugsBasePrivate(DrugsBase *base);
    ~DrugsBasePrivate()
    {
    }
    // connections creator
    bool createConnections(const QString & path, const QString & db, const QString & dbName, bool readwrite);

    // Link tables
    void retreiveLinkTables();

    // private members for interactions
    QStringList getIamClassDenomination(const int & code_subst);
    QSet<int> getAllInnAndIamClassesIndex(const int code_subst);

public:
    // These variables are used or speed improvments and database protection
    QHash<int, QString>       m_IamDenominations;       /*!< INN and class denominations */
    QMultiHash< int, int >    m_Lk_iamCode_substCode;   /*!< Link Iam_Id to Code_Subst */
    QMultiHash< int, int >    m_Lk_classCode_iamCode;   /*!< Link ClassIam_Id to Iam_Id */

    DrugsBase *m_DrugsBase;
    bool m_LogChrono;
};
}  // End Internal
}  // End DrugsDB


//--------------------------------------------------------------------------------------------------------
//--------------------------------- Initialization of static members -------------------------------------
//--------------------------------------------------------------------------------------------------------
DrugsBase * DrugsBase::m_Instance = 0;
bool DrugsBase::m_initialized = false;
bool DrugsBase::m_InteractionsDatabaseAvailable = false;
const QString DrugsBase::separator = "|||";

//--------------------------------------------------------------------------------------------------------
//-------------------------------------- Initializing Database -------------------------------------------
//--------------------------------------------------------------------------------------------------------
/** \brief Returns the unique instance of DrugsBase. If it does not exists, it is created */
DrugsBase *DrugsBase::instance()
{
    if (!m_Instance) {
        m_Instance = new DrugsBase(qApp);
        m_Instance->init();
    }
    return m_Instance;
}

/**
   \brief Constructor.
   \private
*/
DrugsBase::DrugsBase(QObject *parent)
        : InteractionsBase(parent), d(0)
{
    d = new DrugsBasePrivate(this);
    setObjectName("DrugsBase");

    // DRUGS DATABASE
    addTable(Table_CIS, "CIS");
    addTable(Table_COMPO, "COMPO");
    addTable(Table_CIS_CIP, "CIS_CIP");
    addTable(Table_IAM, "IAM_IMPORT");
    addTable(Table_IAM_DENOMINATION, "IAM_DENOMINATION");

    addField(Table_CIS, CIS_CIS ,            "CIS");
    addField(Table_CIS, CIS_DENOMINATION ,   "DENOMINATION");
    addField(Table_CIS, CIS_FORME,           "FORME");
    addField(Table_CIS, CIS_ADMINISTRATION,  "ADMINISTRATION");
    addField(Table_CIS, CIS_AMM,              "AMM");
    addField(Table_CIS, CIS_AUTORISATION,     "AUTORISATION");
    addField(Table_CIS, CIS_COMMERCIALISATION,"COMMERCIALISATION");
    addField(Table_CIS, CIS_CODE_RPC,         "CODE_RPC");

    addField(Table_CIS_CIP, CISP_CIS,         "CIS");
    addField(Table_CIS_CIP, CISP_CIP,         "CIP");
    addField(Table_CIS_CIP, CISP_LIBELLE,     "LIBELLE");
    addField(Table_CIS_CIP, CISP_STATUT,      "STATUT");
    addField(Table_CIS_CIP, CISP_COMMERCIALISATION, "COMMERCIALISATION");
    addField(Table_CIS_CIP, CISP_DATE,        "DATE_STR");
    addField(Table_CIS_CIP, CISP_CIPLONG,     "CIP_LONG");

    addField(Table_COMPO, COMPO_CIS,          "CIS");
    addField(Table_COMPO, COMPO_NOM,          "NOM");
    addField(Table_COMPO, COMPO_CODE_SUBST,   "CODE_SUBST");
    addField(Table_COMPO, COMPO_DENOMINATION, "DENOMINATION");
    addField(Table_COMPO, COMPO_DOSAGE,       "DOSAGE");
    addField(Table_COMPO, COMPO_REF_DOSAGE,   "REF_DOSAGE");
    addField(Table_COMPO, COMPO_NATURE,       "NATURE");
    addField(Table_COMPO, COMPO_LK_NATURE,    "LK_NATURE");

    addField(Table_IAM_DENOMINATION, IAM_DENOMINATION_ID, "ID_DENOMINATION");
    addField(Table_IAM_DENOMINATION, IAM_DENOMINATION,    "DENOMINATION");

    addField(Table_IAM, IAM_ID,       "IAM_ID");
    addField(Table_IAM, IAM_ID1,      "ID1");
    addField(Table_IAM, IAM_ID2,      "ID2");
    addField(Table_IAM, IAM_TYPE,     "TYPE");
    addField(Table_IAM, IAM_TEXT_IAM, "TEXT_IAM");
    addField(Table_IAM, IAM_TEXT_CAT, "TEXT_CAT");
}

/** \brief Destructor. */
DrugsBase::~DrugsBase()
{
    if (d) delete d;
    d=0;
}

DrugsBasePrivate::DrugsBasePrivate(DrugsBase * base)
        : m_DrugsBase(base), m_LogChrono(false) {}

/** \brief Initializer for the database. Return the error state. */
bool DrugsBase::init()
{
    // only one base can be initialized
    if (m_initialized)
        return true;

    // test driver
     if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
         Utils::Log::addError(this, tr("FATAL ERROR : SQLite driver is not available."));
         Utils::warningMessageBox(tr("Fatal Error. Application will not work properly"),
                                  tr("The SQLite database driver is not installed or can not be found. "
                                     "Please report this error to the developper."),
                                  "", qApp->applicationName());
          return false;
      }

     QString pathToDb = "";

     if (Utils::isRunningOnMac())
         pathToDb = Core::ICore::instance()->settings()->databasePath() + QDir::separator() + QString(DRUGS_DATABASE_NAME); //QDir::cleanPath(qApp->applicationDirPath() + "/../Resources");
     else
         pathToDb = Core::ICore::instance()->settings()->databasePath() + QDir::separator() + QString(DRUGS_DATABASE_NAME); //QDir::cleanPath(qApp->applicationDirPath());

//     if (Utils::isDebugCompilation()) {
//         // This is the debug mode of freediams ==> use global_resources
//         pathToDb = QDir::cleanPath(QString("%1/databases/%2/").arg(FMF_GLOBAL_RESOURCES).arg(DRUGS_DATABASE_NAME));
//     }

     Utils::Log::addMessage(this, tr("Searching databases into dir %1").arg(pathToDb));

     // Connect Drugs Database
     createConnection(DRUGS_DATABASE_NAME, DRUGS_DATABASE_FILENAME, pathToDb,
                      Utils::Database::ReadOnly, Utils::Database::SQLite);

     // Connect and check Dosage Database
     createConnection(Dosages::Constants::DOSAGES_DATABASE_NAME, Dosages::Constants::DOSAGES_DATABASE_FILENAME,
                      Core::ICore::instance()->settings()->path(Core::ISettings::ReadWriteDatabasesPath) + QDir::separator() + QString(DRUGS_DATABASE_NAME),
                      Utils::Database::ReadWrite, Utils::Database::SQLite, "log", "pas", Utils::Database::CreateDatabase);
     checkDosageDatabaseVersion();

     // Initialize
     d->retreiveLinkTables();
     InteractionsBase::init();
     m_initialized = true;
     return true;
}

/**
  \brief This is for debugging purpose. Log timers for some crucial functions.
  \sa checkInteractions(), getDrugsByCIS()
*/
void DrugsBase::logChronos(bool state)
{
    d->m_LogChrono = state;
    InteractionsBase::logChronos(state);
}

QString DrugsBase::dosageCreateTableSqlQuery()
{
    return "CREATE TABLE IF NOT EXISTS `DOSAGE` ("
           "`POSO_ID`               INTEGER        PRIMARY KEY AUTOINCREMENT,"
           "`POSO_UUID`             varchar(40)    NULL,"    // put NOT NULL
           "`INN_LK`                int(11)        NULL,"
           "`INN_DOSAGE`            varchar(100)   NULL,"    // contains the dosage of the SA INN
           "`CIS_LK`                int(11)        NULL,"
           "`CIP_LK`                int(11)        NULL,"
           "`LABEL`                 varchar(300)   NULL,"    // put NOT NULL

           "`INTAKEFROM`            double         NULL,"    // put NOT NULL
           "`INTAKETO`              double         NULL,"
           "`INTAKEFROMTO`          bool           NULL,"
           "`INTAKESCHEME`          varchar(200)   NULL,"    // put NOT NULL
           "`INTAKESINTERVALOFTIME` int(10)        NULL,"
           "`INTAKESINTERVALSCHEME` varchar(200)   NULL,"

           "`DURATIONFROM`          double         NULL,"    // put NOT NULL
           "`DURATIONTO`            double         NULL,"
           "`DURATIONFROMTO`        bool           NULL,"
           "`DURATIONSCHEME`        varchar(200)   NULL,"    // put NOT NULL

           "`PERIOD`                int(10)        NULL,"    // put NOT NULL
           "`PERIODSCHEME`          varchar(200)   NULL,"    // put NOT NULL
           "`ADMINCHEME`            varchar(100)   NULL,"    // put NOT NULL
           "`DAILYSCHEME`           varchar(250)   NULL,"
           "`MEALSCHEME`            int(10)        NULL,"
           "`ISALD`                 bool           NULL,"
           "`TYPEOFTREATEMENT`      int(10)        NULL,"

           "`MINAGE`                int(10)        NULL,"
           "`MAXAGE`                int(10)        NULL,"
           "`MINAGEREFERENCE`       int(10)        NULL,"
           "`MAXAGEREFERENCE`       int(10)        NULL,"
           "`MINWEIGHT`             int(10)        NULL,"
           "`SEXLIMIT`              int(10)        NULL,"
           "`MINCLEARANCE`          int(10)        NULL,"
           "`MAXCLEARANCE`          int(10)        NULL,"
           "`PREGNANCYLIMITS`       int(10)        NULL,"
           "`BREASTFEEDINGLIMITS`   int(10)        NULL,"
           "`PHYSIOLOGICALLIMITS`   int(10)        NULL,"  // Is this really needed ?

           "`NOTE`                  varchar(500)   NULL,"

           "`CIM10_LK`              varchar(150)   NULL,"
           "`CIM10_LIMITS_LK`       varchar(150)   NULL,"
           "`EDRC_LK`               varchar(150)   NULL,"

           "`EXTRAS`                blob           NULL,"
           "`USERVALIDATOR`         varchar(200)   NULL,"
           "`CREATIONDATE`          date           NULL,"    // put NOT NULL
           "`MODIFICATIONDATE`      date           NULL,"
           "`TRANSMITTED`           date           NULL,"
           "`ORDER`                 int(10)        NULL"
           ");";
}

/**
  \brief
  \todo documentation
*/
bool DrugsBase::createDatabase( const QString &connectionName , const QString &dbName,
                                   const QString &pathOrHostName,
                                   TypeOfAccess /*access*/, AvailableDrivers driver,
                                   const QString & /*login*/, const QString & /*pass*/,
                                   CreationOption /*createOption*/
                                  )
{
    // TODO ask user if he wants :
    // 1. an empty dosage base
    // 2. to retreive dosages from internet FMF website
    if (connectionName != Dosages::Constants::DOSAGES_DATABASE_NAME)
        return false;
    Utils::Log::addMessage(this, tkTr(Trans::Constants::TRYING_TO_CREATE_1_PLACE_2)
                           .arg(dbName).arg(pathOrHostName));
        // create an empty database and connect
    QSqlDatabase DB;
    if (driver == SQLite) {
        DB = QSqlDatabase::addDatabase("QSQLITE" , connectionName);
        if (!QDir(pathOrHostName).exists())
            if (!QDir().mkpath(pathOrHostName))
                tkTr(Trans::Constants::_1_ISNOT_AVAILABLE_CANNOTBE_CREATED).arg(pathOrHostName);
        DB.setDatabaseName(QDir::cleanPath(pathOrHostName + QDir::separator() + dbName));
        DB.open();
    }
    else if (driver == MySQL) {
        /** \todo how to create a new mysql database ??? */
    }

    // create db structure
    // before we need to inform Utils::Database of the connectionName to use
    setConnectionName(connectionName);
    // The SQL scheme MUST BE synchronized with the Dosages::Constants Model Enumerator !!!
    if (executeSQL(QStringList()
        << dosageCreateTableSqlQuery()
        << "CREATE TABLE IF NOT EXISTS `VERSION` ("
           "`ACTUAL`                varchar(10)    NULL"
           ");"
        << QString("INSERT INTO `VERSION` (`ACTUAL`) VALUES('%1');").arg(VersionUpdater::instance()->lastDosageDabaseDosage())
        , DB)) {
        Utils::Log::addMessage(this, tr("Database %1 %2 correctly created").arg(connectionName, dbName));
        return true;
    } else {
        Utils::Log::addError(this, tr("ERROR : database can not be created %1 %2 %3")
                         .arg(connectionName, dbName, DB.lastError().text()));
    }
    return false;
}

/**
  \brief Check the version of the doage database. Do the necessary updates for that database according to the application version number.
  Added from freediams version 0.0.8 stable
*/
void DrugsBase::checkDosageDatabaseVersion()
{
    if (!DrugsDB::VersionUpdater::instance()->isDosageDatabaseUpToDate())
        if (!DrugsDB::VersionUpdater::instance()->updateDosageDatabase())
            Utils::Log::addError(this, "Dosage database can not be correctly updated");
}

/** \brief Returns hash that contains dosage uuid has key and the xml'd dosage to transmit as value */
QHash<QString, QString> DrugsBase::getDosageToTransmit()
{
    QHash<QString, QString> toReturn;
    QSqlDatabase DB = QSqlDatabase::database(Dosages::Constants::DOSAGES_DATABASE_NAME);
    if (!DB.open()) {
        Utils::Log::addError(this, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                             .arg(Dosages::Constants::DOSAGES_DATABASE_NAME).arg(DB.lastError().text()));
        return toReturn;
    }
    QString req = QString("SELECT * FROM `DOSAGE` WHERE (`TRANSMITTED` IS NULL);");
    {
        QSqlQuery query(req,DB);
        if (query.isActive()) {
            while (query.next()) {
                QHash<QString,QString> toXml;
                int i=0;
                for (i=0;i<query.record().count();++i) {
                    // create a XML of the dosage
                    toXml.insert(query.record().field(i).name(), query.value(i).toString());
                }
                toReturn.insert(toXml.value("POSO_UUID"), Utils::createXml(Dosages::Constants::DOSAGES_TABLE_NAME,toXml,4,false));
            }
        } else
            Utils::Log::addQueryError(this, query);
    }

    req = QString("SELECT * FROM `DOSAGE` WHERE (`TRANSMITTED`<`MODIFICATIONDATE`);");
    {
        QSqlQuery query(req,DB);
        if (query.isActive()) {
            while (query.next()) {
                QHash<QString,QString> toXml;
                int i=0;
                for (i=0;i<query.record().count();++i) {
                    // create a XML of the dosage
                    toXml.insert(query.record().field(i).name(), query.value(i).toString());
                }
                toReturn.insert(toXml.value("POSO_UUID"), Utils::createXml(Dosages::Constants::DOSAGES_TABLE_NAME,toXml,4,false));
            }
        } else {
            Utils::Log::addQueryError(this, query);
        }
    }
    return toReturn;
}

/** Marks all dosages as transmitted now. \e dosageUuids contains dosage uuid. */
bool DrugsBase::markAllDosageTransmitted(const QStringList &dosageUuids)
{
    if (dosageUuids.count()==0)
        return true;
    QSqlDatabase DB = QSqlDatabase::database(Dosages::Constants::DOSAGES_DATABASE_NAME);
    if (!DB.open()) {
        Utils::Log::addError(this, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                             .arg(Dosages::Constants::DOSAGES_DATABASE_NAME).arg(DB.lastError().text()));
        return false;
    }
    QStringList reqs;
    foreach(const QString &s, dosageUuids) {
        QString req = QString("UPDATE `DOSAGE` SET `TRANSMITTED`='%1' WHERE %2")
                      .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                      .arg(QString("`POSO_UUID`='%1'").arg(s));
        reqs << req;
    }
    if (!executeSQL(reqs,DB)) {
        Utils::Log::addError(this, tr("Unable to update transmission date dosage"));
        return false;
    }
    return true;
}


struct minimalCompo {
    bool isLike(const int _inn, const QString &_dosage) const {
        return (inn == _inn) && (dosage == _dosage);
    }
    int inn;
    QString dosage;
};

/**
  \brief Returns all CIS that have a recorded dosage. Manages INN dosage type.
  \todo put this in a thread...
*/
QList<int> DrugsBase::getAllCISThatHaveRecordedDosages() const
{
    QList<int> toReturn;
    QSqlDatabase DosageDB = QSqlDatabase::database(Dosages::Constants::DOSAGES_DATABASE_NAME);
    if ((DosageDB.isOpen()) && (!DosageDB.open())) {
        Utils::Log::addError(this, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                             .arg(Dosages::Constants::DOSAGES_DATABASE_NAME).arg(DosageDB.lastError().text()));
        return toReturn;
    }
    QString req = QString("SELECT DISTINCT CIS_LK FROM `DOSAGE`;");
    {
        QSqlQuery query(req,DosageDB);
        if (query.isActive()) {
            while (query.next()) {
                toReturn << query.value(0).toInt();
            }
        } else {
            Utils::Log::addQueryError(this, query);
        }
    }

    // Get all CIS that contains INN that have available dosage
    QMultiHash<int, QString> inn_dosageRef = getAllINNThatHaveRecordedDosages();

    // get all code_subst from INNs
    QHash<int, QString> where;
    QString tmp;
    QList<int> code_subst;
    QSqlDatabase DrugsDB = QSqlDatabase::database(Constants::DRUGS_DATABASE_NAME);
    if ((DrugsDB.isOpen()) && (!DrugsDB.open())) {
        Utils::Log::addError(this, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                             .arg(Constants::DRUGS_DATABASE_NAME).arg(DrugsDB.lastError().text()));
        return toReturn;
    }

    // get all needed datas from database
    QMultiHash<int, minimalCompo> cis_compo;
    foreach(int inn, inn_dosageRef.keys()) {
        foreach(int code, d->m_Lk_iamCode_substCode.values(inn)) {
            if (!code_subst.contains(code))
                code_subst << code;
        }
    }
    foreach(int code, code_subst) {
        tmp += QString::number(code) + ", ";
    }
    tmp.chop(2);

    // Get all CIS that contains the substance + dosage
    where.clear();
    req.clear();
    where.insert(Constants::COMPO_CODE_SUBST, QString("IN (%1)").arg(tmp));
    req = select(Constants::Table_COMPO,
                 QList<int>() << Constants::COMPO_CIS
                 << Constants::COMPO_CODE_SUBST
                 << Constants::COMPO_DOSAGE,
                 where);
    QSqlQuery query(req, DrugsDB);
    if (query.isActive()) {
        while (query.next()) {
            int cis = query.value(0).toInt();
            minimalCompo compo;
            compo.inn = d->m_Lk_iamCode_substCode.key(query.value(1).toInt());
            compo.dosage = query.value(2).toString();
            cis_compo.insertMulti(cis, compo);
        }
    } else {
        Utils::Log::addQueryError(this, query);
    }

    // now check every drugs
    foreach(const int cis, cis_compo.uniqueKeys()) {
        QList<int> innsOfThisDrug;
        foreach(const minimalCompo &compo, cis_compo.values(cis)) {
            if (!innsOfThisDrug.contains(compo.inn))
                innsOfThisDrug << compo.inn;
            QString d = compo.dosage;
            foreach(const QString &r, inn_dosageRef.values(compo.inn)) {
                // remove unneeded strings
                if (d == r)
                    toReturn << cis;
                else if (d.remove(",000") == r)
                    toReturn << cis;
                else if (d.remove(",00") == r)
                    toReturn << cis;

                // try unit conversion
                if (d.replace("000 mg", " g") == r) {
                    toReturn << cis;
                }
                QString t = r;
                if (t.replace("000 mg", " g") == d) {
                    toReturn << cis;
                }
            }
        }
        if (innsOfThisDrug.count() > 1)
            toReturn.removeAll(cis);
    }
    return toReturn;
}

QMultiHash<int,QString> DrugsBase::getAllINNThatHaveRecordedDosages() const
{
    QMultiHash<int,QString> toReturn;
    QSqlDatabase DB = QSqlDatabase::database(Dosages::Constants::DOSAGES_DATABASE_NAME);
    if (!DB.open()) {
        Utils::Log::addError(this, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                             .arg(Dosages::Constants::DOSAGES_DATABASE_NAME).arg(DB.lastError().text()));
        return toReturn;
    }
    QString req = QString("SELECT DISTINCT `INN_LK`, `INN_DOSAGE` FROM `DOSAGE`;");
    {
        QSqlQuery query(req,DB);
        if (query.isActive()) {
            while (query.next()) {
                toReturn.insertMulti(query.value(0).toInt(), query.value(1).toString());
            }
        } else {
            Utils::Log::addQueryError(this, query);
        }
    }
    return toReturn;
}


//--------------------------------------------------------------------------------------------------------
//----------------------------------------- Managing Link Tables -----------------------------------------
//--------------------------------------------------------------------------------------------------------
/**
  \brief Retrieve many values into cached lists.
  Get the link tables that are protected from the SVN diffusion.\n
  Retrieve all iam denomination for speed improvments. \n
  Retrieve all iamids from IAM_EXPORT fro speed improvments.\n
*/
void DrugsBasePrivate::retreiveLinkTables()
{
     if ((!m_Lk_iamCode_substCode.isEmpty()) && (!m_Lk_classCode_iamCode.isEmpty()))
         return;

     QString tmp;
     {
          QFile file(":/inns_molecules_link.csv");
          if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
               return;
          tmp = file.readAll();
          QStringList lines = tmp.split("\n");
          int i = 0;
          foreach(QString l, lines) {
               if (!l.contains(DrugsBase::separator)) continue;
               QStringList val = l.split(DrugsBase::separator);
               m_Lk_iamCode_substCode.insertMulti(val[0].toInt(), val[1].toInt());
               i++;
          }
     }

     {
          QFile file(":/link-class-substances.csv");
          if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
               return;
          tmp = file.readAll();
          QStringList lines = tmp.split("\n");
          int i = 0;
          foreach(QString l, lines) {
               if (!l.contains(DrugsBase::separator)) continue;
               QStringList val = l.split(DrugsBase::separator);
               m_Lk_classCode_iamCode.insertMulti(val[0].toInt(), val[1].toInt());
               i++;
          }
     }
     m_DrugsBase->m_InteractionsDatabaseAvailable = m_Lk_classCode_iamCode.count() && m_Lk_iamCode_substCode.count();

     /** \todo release these resources files to limit memory usage */

     QSqlDatabase DB = QSqlDatabase::database(DRUGS_DATABASE_NAME);
     if (!DB.isOpen())
          DB.open();

     // get m_IamDenominations : all iam denominations known
     {
         QHashWhere where;
         QString req = m_DrugsBase->select(Table_IAM_DENOMINATION);

         QSqlQuery q(req , DB);
         if (q.isActive()) {
             while (q.next())
                 m_IamDenominations.insert(q.value(0).toInt(), q.value(1).toString());
         }
         else
             Utils::Log::addQueryError(m_DrugsBase, q);
     }


     /*
          // get m_Lk_iamCode_substCode : link table for iamCode <-> codeSubst
          {
               QString req = "SELECT `ID_IAM_SUBST`, `ID_CODE_SUBST` "
                             " FROM `IAM_SUBST_L_CODE_SUBST` "
                             " ORDER BY `ID_IAM_SUBST`;";

               QSqlQuery q(req , DB);
               if (q.isActive())
               {
                    while (q.next())
                         m_Lk_iamCode_substCode.insertMulti(q.value(0).toInt(), q.value(1).toInt());
               }
               else
                    qWarning() << q.lastError().driverText() << q.lastError().databaseText() << q.lastQuery();
          }


          // get m_Lk_classCode_iamCode : link table for iamCode <-> codeSubst
          {
               QString req = "SELECT `ID_IAM_CLASS`, `ID_IAM_SUBST` "
                             "FROM `IAM_CLASS_L_SUBST` "
                             "ORDER BY `ID_IAM_CLASS`;";

               QSqlQuery q(req , DB);
               if (q.isActive())
               {
                    while (q.next())
                         m_Lk_classCode_iamCode.insertMulti(q.value(0).toInt(), q.value(1).toInt());
               }
               else
                    qWarning() << q.lastError().driverText() << q.lastError().databaseText() << q.lastQuery();
          }
     */
 }


/** \brief Return the list of the code of the substances linked to the INN code \e code_iam. */
QList<int> DrugsBase::getLinkedCodeSubst(QList<int> & code_iam) const
{
    QList<int> toReturn;
    foreach(int i, code_iam)
        toReturn << d->m_Lk_iamCode_substCode.values(i);
    return toReturn;
}

/** \brief Return the list of the INN code linked to the substances code \e code_subst. */
QList<int> DrugsBase::getLinkedIamCode(QList<int> & code_subst) const
{
    QList<int> toReturn;
    foreach(int i, code_subst)
        toReturn << d->m_Lk_iamCode_substCode.keys(i);
    return toReturn;
}

/** \brief Retreive from database the Substances Codes where denomination of the INN is like 'iamDenomination%' */
QList<int> DrugsBase::getLinkedSubstCode(const QString & iamDenomination)
{
     QSqlDatabase DB = QSqlDatabase::database(DRUGS_DATABASE_NAME);
     if (!DB.isOpen())
          DB.open();

     // get iamSubstCode
     QString tmp = iamDenomination;
     QHash<int, QString> where;
     where.insert(IAM_DENOMINATION, QString("LIKE '%1%'").arg(tmp.replace("'", "?")));
     QList<int> iamCode;
     QString req = this->select(Table_IAM_DENOMINATION, IAM_DENOMINATION_ID, where);
     QSqlQuery q(req , DB);
     if (q.isActive())
         while (q.next())
             iamCode << q.value(0).toInt();
     return getLinkedCodeSubst(iamCode);
}

/** \brief Return true if all the INNs of the drug are known. */
bool DrugsBase::drugsINNIsKnown(const DrugsData *drug)
{
    const QList<QVariant> & list = drug->listOfCodeMolecules().toList(); //->value(Table_COMPO, COMPO_CODE_SUBST).toList();
    foreach(QVariant q, list)
        if (d->m_Lk_iamCode_substCode.keys(q.toInt()).count() == 0)
            return false;
    return true;
}

//--------------------------------------------------------------------------------------------------------
//-------------------------------- Retreive drugs from database ------------------------------------------
//--------------------------------------------------------------------------------------------------------
/** \brief Returns the name of the INN for the substance code \e code_subst. */
QString DrugsBase::getInnDenominationFromSubstanceCode(const int code_subst)
{
     // if molecule is not associated with a dci exit
     if (!d->m_Lk_iamCode_substCode.values().contains(code_subst))
          return QString::null;
     return d->m_IamDenominations.value(d->m_Lk_iamCode_substCode.key(code_subst));
}

QString DrugsBase::getInnDenomination(const int inncode) const
{
     return d->m_IamDenominations.value(inncode);
}

/** \brief Returns the name of the INN for the substance code \e code_subst. */
QStringList DrugsBasePrivate::getIamClassDenomination(const int & code_subst)
{
     // if molecule is not associated with a dci exit
     if (!m_Lk_iamCode_substCode.values().contains(code_subst))
          return QStringList();

     // if molecule is not associated with a class exit
     QList<int> list = m_Lk_classCode_iamCode.keys(m_Lk_iamCode_substCode.key(code_subst));
     if (list.isEmpty())
          return QStringList();
     QStringList toReturn;
     foreach(int i, list)
         toReturn << m_IamDenominations.value(i);

     return toReturn;
}

/**
  \brief Returns all INN and IAM classes of MOLECULE \e code_subst.
  \sa getDrugByCIS()
*/
QSet<int> DrugsBasePrivate::getAllInnAndIamClassesIndex(const int code_subst)
{
    QSet<int> toReturn;
    toReturn = m_Lk_classCode_iamCode.keys(m_Lk_iamCode_substCode.key(code_subst)).toSet();
    if (m_Lk_iamCode_substCode.values().contains(code_subst))
        toReturn << m_Lk_iamCode_substCode.key(code_subst);
    return toReturn;
}

/** \brief Return the Inn code linked to the molecule code. Returns -1 if no inn is linked to that molecule. */
int DrugsBase::getInnCodeForCodeMolecule(const int code) const
{
    if (d->m_Lk_iamCode_substCode.values().contains(code))
        return d->m_Lk_iamCode_substCode.key(code);
    return -1;
}

/** \brief Returns the unique code CIS for the CIP code \e CIP. */
int DrugsBase::getCISFromCIP(int CIP)
{
    QSqlDatabase DB = QSqlDatabase::database(DRUGS_DATABASE_NAME);
    if (!DB.isOpen())
        DB.open();
    // prepare where clause
    QHash<int, QString> where;
    where.insert(CISP_CIP, "=" + QString::number(CIP));
    // prepare query
    QString req = this->select(Table_CIS_CIP, CISP_CIS, where);
    QSqlQuery q(req , DB);
    if (q.isActive())
        if (q.next())
            return q.value(0).toInt();
    return -1;
}

/** \brief Retrieve and return the drug designed by the CIP code \e CIP_id. This function is not used. */
DrugsData *DrugsBase::getDrugByCIP(const QVariant & CIP_id)
{
    // retreive corresponding CIS
    int CIS = getCISFromCIP(CIP_id.toInt());
    if (CIS == -1) return 0;

    // retreive drugs from CIS
    return getDrugByCIS(CIS);
}

/** \brief Retrieve and return the drug designed by the CIS code \e CIS_id. */
DrugsData *DrugsBase::getDrugByCIS(const QVariant &CIS_id)
{
     if (CIS_id.toInt() < 10000000)
         return 0;
     QTime t;
     t.start();

     QSqlDatabase DB = QSqlDatabase::database(DRUGS_DATABASE_NAME);
     if ((!DB.open()) && (!DB.isOpen())) {
         Utils::Log::addError(this, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                             .arg(Constants::DRUGS_DATABASE_NAME).arg(DB.lastError().text()));
          return 0;
      }

     // construct the where clause
     QHash<int, QString> where;
     where.insert(CIS_CIS, QString("=%1").arg(CIS_id.toString()));

     // get CIS table
     QString req = select(Table_CIS, where);
     DrugsData * toReturn = 0;
     {
          QSqlQuery q(req , DB);
          if (q.isActive()) {
               if (q.next()) {
                    if (q.record().count() != CIS_MaxParam)
                         Utils::Log::addError("DrugsBase", QCoreApplication::translate("DrugsBase",
                                   "ERROR : will retreiving %1. Wrong number of fields")
                                          .arg("drugs/CIS"));
                    int i = 0;
                    toReturn = new DrugsData();
                    for (i = 0; i < CIS_MaxParam; ++i)
                         toReturn->setValue(Table_CIS, i, q.value(i));
               }
          }
      }

     // get COMPO table
     where.clear();
     where.insert(COMPO_CIS, QString("=%1").arg(CIS_id.toString()));
     QString sort = QString(" ORDER BY %1 ASC").arg(field(Table_COMPO,COMPO_LK_NATURE));
     req = select(Table_COMPO, where) + sort;
     QSet<int> codeMols;
     {
         DrugComposition *compo = 0;
         DrugComposition *precedent = 0;
          QSqlQuery q(req , DB);
          if (q.isActive()) {
              QList<DrugComposition*> list;
               while (q.next()) {
                    if (q.record().count() != COMPO_MaxParam)
                         Utils::Log::addError("DrugsBase", QCoreApplication::translate("DrugsBase",
                                   "ERROR : will retreiving %1. Wrong number of fields")
                                          .arg("drugs composition"));
                    compo = new DrugComposition();
                    int i = 0;
                    for (i = 0; i < COMPO_MaxParam; ++i)
                         compo->setValue(i, q.value(i));
                    compo->setValue(COMPO_IAM_DENOMINATION, getInnDenominationFromSubstanceCode(compo->m_CodeMolecule));
                    compo->setValue(COMPO_IAM_CLASS_DENOMINATION, d->getIamClassDenomination(compo->m_CodeMolecule));
                    compo->setInnCode(getInnCodeForCodeMolecule(q.value(COMPO_CODE_SUBST).toInt()));
                    list << compo;
                    codeMols << q.value(COMPO_CODE_SUBST).toInt();
                    if (precedent) {
                        if (q.value(COMPO_LK_NATURE) == precedent->linkId()) {
                            compo->setLinkedSubstance(list.at(list.count()-2));
                        }
                    }
                    precedent = compo;
               }
               foreach(DrugComposition *c, list)
                   toReturn->addComposition(c);
          }
      }
     foreach(const int i, codeMols) {
         toReturn->addInnAndIamClasses(d->getAllInnAndIamClassesIndex(i)) ;
     }

//     toReturn->warn();

     if (d->m_LogChrono)
         Utils::Log::logTimeElapsed(t, "DrugsBase", "getDrugByCIS");

     return toReturn;
}

