CREATE TABLE IF NOT EXISTS MASTER (
  DID      INTEGER PRIMARY KEY,
  UID1     varchar(50) not null,
  UID2     varchar(50),
  UID3     varchar(50),
  OLD_UID  varchar(50),
  SID      integer
);
CREATE INDEX MasterIndexPk ON MASTER (DID);

CREATE TABLE IF NOT EXISTS SOURCES (
  SID             INTEGER PRIMARY KEY,
  DATABASE_UID    varchar(20) not null,
  MASTER_LID      integer,
  LANG            varchar(5),
  WEB             varchar(250),
  COPYRIGHT       varchar(500),
  DATE            date,
  DRUGS_VERSION   varchar(10),
  AUTHORS         varchar(200),
  VERSION         varchar(10),
  PROVIDER        varchar(200),
  WEBLINK         varchar(500),
  DRUG_UID_NAME   varchar(50),
  ATC             bool default true,
  INTERACTIONS    bool default true,
  COMPLEMENTARY_WEBSITE  varchar(200),
  PACK_MAIN_CODE_NAME    varchar(50),
  MOL_LINK_COMPLETION    integer default 0,
  AUTHOR_COMMENTS        varchar(10000),
  DRUGS_NAME_CONSTRUCTOR varchar(200),
  FREEMEDFORMS_COMPTA_VERSION varchar(10),
  WEBPORTAL_COMPTA_VERSION    varchar(10)
);

CREATE TABLE IF NOT EXISTS LABELS (
  LID   INTEGER PRIMARY KEY,
  LANG  varchar(5),
  LABEL varchar(250)
);
CREATE INDEX LabelsIndexPk ON LABELS (LID);

CREATE TABLE IF NOT EXISTS LABELS_LINK (
  MASTER_LID    integer not null,
  LID           integer not null,
  UNIQUE(MASTER_LID, LID),
  FOREIGN KEY(LID) REFERENCES LABELS(LID)
);
CREATE INDEX LabelsLinkK ON LABELS_LINK (MASTER_LID,LID);

CREATE TABLE IF NOT EXISTS BIBLIOGRAPHY (
  BIB_ID    INTEGER PRIMARY KEY,
  TYPE      varchar(20),
  LINK      varchar(200),
  TEXTUAL_REFERENCE varchar(1000),
  ABSTRACT  varchar(4000),
  EXPLANATION varchar(4000),
  XML varchar(10000)
);
CREATE INDEX BibsIndexPk ON BIBLIOGRAPHY (BIB_ID);

CREATE TABLE IF NOT EXISTS BIBLIOGRAPHY_LINKS (
  BIB_MASTER_ID integer,
  BIB_ID        integer,
  UNIQUE(BIB_MASTER_ID, BIB_ID),
  FOREIGN KEY(BIB_ID) REFERENCES BIBLIOGRAPHY(BIB_ID)
);
CREATE INDEX BibLinksMasterK ON BIBLIOGRAPHY_LINKS (BIB_MASTER_ID,BIB_ID);

CREATE TABLE IF NOT EXISTS DRUGS (
  ID        INTEGER NOT NULL PRIMARY KEY,
  DID       integer,
  SID       integer,
  NAME      varchar(255),
  ATC_ID    integer,
  STRENGTH  varchar(40),
  VALID     boolean,
  MARKETED  boolean,
  AID_MASTER_LID  integer,
  LINK_SPC  varchar(500),
  EXTRA_XML varchar(5000),
  FOREIGN KEY(DID) REFERENCES MASTER(DID)
);
CREATE INDEX DrugsIndexPk ON DRUGS (DID);

CREATE TABLE IF NOT EXISTS MOLS (
  MID       INTEGER PRIMARY KEY,
  SID       integer,
  NAME      varchar(150),
  WWW       varchar(200)
);
CREATE INDEX MolsIndexPk ON MOLS (MID);

CREATE TABLE IF NOT EXISTS COMPOSITION (
  DID           integer not null,
  MID           integer not null,
  STRENGTH      varchar(150),
  STRENGTH_NID  integer,
  DOSE_REF      varchar(150),
  DOSE_REF_NID  integer,
  NATURE        varchar(2),
  LK_NATURE     integer,
  FOREIGN KEY(DID) REFERENCES MASTER(DID),
  FOREIGN KEY(MID) REFERENCES MOLS(MID)
);
CREATE INDEX CompoDrugIndexPk ON  COMPOSITION (DID);
CREATE INDEX CompoMolsIndexPk ON  COMPOSITION (MID);

CREATE TABLE IF NOT EXISTS UNITS (
  NID     INTEGER PRIMARY KEY,
  VALUE   varchar(20)
);
CREATE INDEX UnitsIndexPk ON UNITS (NID);

CREATE TABLE IF NOT EXISTS LK_MOL_ATC (
  MID     integer not null,
  ATC_ID  integer not null,
  SID     integer not null,
  UNIQUE(MID,ATC_ID,SID)
);
CREATE INDEX LkMolAtcK ON LK_MOL_ATC (MID,ATC_ID,SID);

CREATE TABLE IF NOT EXISTS PACKAGING (
  DID integer not null,
  SID integer,
  PACKAGE_UID int(20) NOT NULL,
  LABEL varchar(500) NOT NULL,
  STATUS varchar(1),
  MARKETING int(1) NOT NULL DEFAULT 1,
  DATE varchar(25),
  OPTIONAL_CODE int(20)
);

CREATE TABLE IF NOT EXISTS DRUG_ROUTES (
  DID  integer not null,
  RID  integer not null,
  UNIQUE(DID,RID),
  FOREIGN KEY(DID) REFERENCES MASTER(DID),
  FOREIGN KEY(RID) REFERENCES ROUTES(RID)
);
CREATE INDEX DrugRoutesDidFk ON DRUG_ROUTES (DID,RID);

CREATE TABLE IF NOT EXISTS DRUG_FORMS (
  DID           integer not null,
  MASTER_LID    integer not null,
  UNIQUE(DID,MASTER_LID),
  FOREIGN KEY(DID) REFERENCES MASTER(DID),
  FOREIGN KEY(MASTER_LID) REFERENCES LABELS_LINK(MASTER_LID)
);
CREATE INDEX DrugsFormsK ON DRUG_FORMS (DID,MASTER_LID);

CREATE TABLE IF NOT EXISTS ROUTES (
  RID           INTEGER PRIMARY KEY,
  MASTER_LID    integer not null,
  SYSTEMIC_STATUS varchar(10),
  UNIQUE(RID, MASTER_LID),
  FOREIGN KEY(MASTER_LID) REFERENCES LABELS_LINK(MASTER_LID)
);
CREATE INDEX RoutesK ON ROUTES (RID, MASTER_LID);

CREATE TABLE IF NOT EXISTS SEARCH_ENGINES (
  ID integer NOT NULL AUTO_INCREMENT primary key,
  LABEL varchar(25),
  URL varchar(1000)
);

DELETE FROM SEARCH_ENGINES;
INSERT INTO SEARCH_ENGINES VALUES (NULL, "WHO ATC description", "http://www.whocc.no/atc_ddd_index/?&code=[[ONE_ATC_CODE]]&showdescription=yes");
INSERT INTO SEARCH_ENGINES VALUES (NULL, "Search drug name @ NIH", "http://vsearch.nlm.nih.gov/vivisimo/cgi-bin/query-meta?v%3Aproject=medlineplus&query=[[DRUG_NAME]]&x=0&y=0");
INSERT INTO SEARCH_ENGINES VALUES (NULL, "Search INN name @ NIH", "http://vsearch.nlm.nih.gov/vivisimo/cgi-bin/query-meta?v%3Aproject=medlineplus&query=[[ONE_ATC_CODE]]&x=0&y=0");

CREATE TABLE IF NOT EXISTS DB_SCHEMA_VERSION (
  ID INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
  VERSION varchar(10),
  DATE  date,
  COMMENT varchar(500)
);

CREATE TABLE IF NOT EXISTS ATC (
  ATC_ID INTEGER PRIMARY KEY,
  CODE VARCHAR(7) NULL,
  WARNDUPLICATES BOOLEAN
);
CREATE INDEX AtcIndexPk ON ATC (ATC_ID);
CREATE INDEX AtcCodePk ON ATC (CODE);

CREATE TABLE IF NOT EXISTS INTERACTIONS (
  IAID INTEGER PRIMARY KEY,
  ATC_ID1 integer not null,
  ATC_ID2 integer not null,
  FOREIGN KEY(ATC_ID1) REFERENCES ATC(ATC_ID),
  FOREIGN KEY(ATC_ID2) REFERENCES ATC(ATC_ID)
 );
CREATE INDEX IaIndexPk ON INTERACTIONS (IAID);
CREATE INDEX IaAtcId1Fk ON INTERACTIONS (ATC_ID1,ATC_ID2);

CREATE TABLE IF NOT EXISTS IAKNOWLEDGE (
  IAKID               INTEGER PRIMARY KEY,
  TYPE                varchar(5),
  RISK_MASTER_LID     integer,
  MAN_MASTER_LID      integer,
  BIB_MASTER_ID       integer,
  WWW                 varchar(100),
  FOREIGN KEY(RISK_MASTER_LID) REFERENCES LABELS_LINK(MASTER_LID),
  FOREIGN KEY(MAN_MASTER_LID) REFERENCES LABELS_LINK(MASTER_LID),
  FOREIGN KEY(BIB_MASTER_ID) REFERENCES BIBLIOGRAPHY_LINKS(BIB_MASTER_ID)
);
CREATE INDEX IakIndexPk ON IAKNOWLEDGE (IAKID,RISK_MASTER_LID,MAN_MASTER_LID,BIB_MASTER_ID);

CREATE TABLE IF NOT EXISTS IAFORMALIZED (
  IAFID               INTEGER PRIMARY KEY,
  BLEEDINGSCORE       integer,
  RENALFAILSCORE      integer,
  PULMFAILSCORE       integer,
  HEPATICFAILSCORE    integer,
  HEARTFAILSCORE      integer,
  HEARTRYTHMFAILSCORE integer,
  MAINDOSEFROM        varchar(25),
  MAINDOSETO          varchar(25),
  IDOSEFROM           varchar(25),
  IDOSETO             varchar(25),
  MAINROUTE           integer,
  INTERACTORROUTE     integer,
  MAINADESCORE        integer,
  IADESCORE           integer
);
CREATE INDEX IaFIndexPk ON IAFORMALIZED (IAFID);

CREATE TABLE IF NOT EXISTS IA_IAK (
  IAID   integer not null,
  IAKID  integer not null,
  UNIQUE(IAID, IAKID),
  FOREIGN KEY(IAID) REFERENCES INTERACTIONS(IAID),
  FOREIGN KEY(IAKID) REFERENCES IAKNOWLEDGE(IAKID)
 );
CREATE INDEX IaToIakK ON IA_IAK (IAID,IAKID);

CREATE TABLE IF NOT EXISTS ATC_LABELS (
  ATC_ID        integer not null,
  MASTER_LID    integer not null,
  UNIQUE(ATC_ID, MASTER_LID),
  FOREIGN KEY(ATC_ID) REFERENCES ATC(ATC_ID),
  FOREIGN KEY(MASTER_LID) REFERENCES LABELS_LINK(MASTER_LID)
);
CREATE INDEX AtcLabelsK ON ATC_LABELS (ATC_ID, MASTER_LID);

CREATE TABLE IF NOT EXISTS IAM_TREE (
  ID_CLASS       integer not null,
  ID_ATC         integer not null,
  BIB_MASTER_ID  integer default -1,
  FOREIGN KEY(ID_CLASS) REFERENCES ATC(ATC_ID),
  FOREIGN KEY(ID_ATC) REFERENCES ATC(ATC_ID),
  FOREIGN KEY(BIB_MASTER_ID) REFERENCES BIBLIOGRAPHY_LINKS(BIB_MASTER_ID)
);
CREATE INDEX AtcTreeClassK ON IAM_TREE (ID_CLASS,ID_ATC,BIB_MASTER_ID);

CREATE TABLE IF NOT EXISTS PIM_SOURCES (
  PIM_SID        INTEGER PRIMARY KEY,
  UID            varchar(50),
  NAME           varchar(255),
  PMID           integer,
  COUNTRY        varchar(20),
  WWW            varchar(500)
);
CREATE INDEX PimSourceIndex ON PIM_SOURCES (PIM_SID);

CREATE TABLE IF NOT EXISTS PIM_TYPES (
  PIM_TID        INTEGER PRIMARY KEY,
  UID            varchar(50),
  MASTER_LID     integer,
  FOREIGN KEY(MASTER_LID) REFERENCES LABELS_LINK(MASTER_LID)
);
CREATE INDEX PimTypesIndex ON PIM_TYPES (PIM_TID);

CREATE TABLE IF NOT EXISTS PIMS (
  PIM_ID          INTEGER PRIMARY KEY,
  PIM_SID         integer,
  PIM_TID         integer,
  LEVEL           integer,
  RISK_MASTER_LID integer,
  FOREIGN KEY(PIM_SID) REFERENCES PIM_SOURCES(PIM_SID),
  FOREIGN KEY(PIM_TID) REFERENCES PIM_TYPES(PIM_TID)
);
CREATE INDEX PimsIndex ON PIMS (PIM_ID);

CREATE TABLE IF NOT EXISTS PIMS_RELATED_ATC (
  PIM_RMID         INTEGER PRIMARY KEY,
  PIM_ID           integer,
  ATC_ID           integer,
  MAXDAYDOSE       double,
  MAXDAYDOSEUNIT   integer,
  FOREIGN KEY(PIM_ID) REFERENCES PIMS(PIM_ID),
  FOREIGN KEY(ATC_ID) REFERENCES ATC(ATC_ID)
);
CREATE INDEX PimsAtcIndex ON PIMS_RELATED_ATC (PIM_RMID);
CREATE INDEX PimsAtcPimIdIndex ON PIMS_RELATED_ATC (PIM_ID);
CREATE INDEX PimsAtcAtcIdIndex ON PIMS_RELATED_ATC (ATC_ID);

CREATE TABLE IF NOT EXISTS PIMS_RELATED_ICD (
  PIM_RMID         INTEGER PRIMARY KEY,
  PIM_ID           integer,
  ICD10_ID         integer,
  FOREIGN KEY(PIM_ID) REFERENCES PIMS(PIM_ID)
);
CREATE INDEX PimsIcdIndex ON PIMS_RELATED_ICD (PIM_RMID);
CREATE INDEX PimsIcdPimIdIndex ON PIMS_RELATED_ICD (PIM_ID);
CREATE INDEX PimsIcdAtcIdIndex ON PIMS_RELATED_ICD (ICD10_ID);

INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.3.0","2010-03-01","Drugs: Translated all fields");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.0","2010-03-15","Drugs: DRUGS table : Adding GLOBAL_STRENGTH");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.0","2010-06-01","Drugs: Adding the information table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-16","Drugs: Adding LK_MOL_ATC table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-18","Drugs: INFORMATIONS table : Adding the COMPLEMENTARY_WEBSITE");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-22","Drugs: COMPOSITION table : Adding ATC Molecule code");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-22","Drugs: Adding DB_SCHEMA_VERSION table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.0","2010-09-22","Drugs: Replacing UID from int to varchar");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.0","2010-09-23","Drugs: Adding molecule links completion percent to INFORMATION table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.2","2010-09-23","Drugs: Adding routes tables and link table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.5","2011-01-10","Drugs: Totally redefined schema");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.0.8","2009-01-01","Interactions: First version of the database");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.3.0","2010-03-02","Interactions: First SVN publication");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-02","Interactions: Adding the ATC table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-16","Interactions: Adding the IAM_TREE table : 1 interacting class <-> N ATC");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-22","Interactions: Adding INTERACTIONS and INTERACTION_KNOWLEDGE tables");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-22","Interactions: Removing IAM_DENOMINATION and IAM_IMPORT tables");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-22","Interactions: Adding DB_SCHEMA_VERSION table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.4.4","2010-07-22","Interactions: First english translations of INTERACTION_KNOWLEDGE are available");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.2","2010-12-10","Interactions: Addind REFERENCES_LINK to knowledge");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.2","2010-14-10","Interactions: Addind SOURCES_LINK to IAM_TREE");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.2","2010-14-10","Interactions: Addind SOURCES table");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.5.5","2011-01-10","Interactions: Totally redefined schema");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.6.0","2011-04-21","Interactions: Adding duplication warnings for all ATC including interacting classes");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.6.0","2011-04-22","Interactions: Starting implementation of DDI formalized risks");
INSERT INTO DB_SCHEMA_VERSION VALUES (NULL,"0.6.0","2011-09-27","Biblio: adding XML formatted bibliography from pubmed");

