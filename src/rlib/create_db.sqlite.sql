create table OKSSCHEMA (
  VERSION        INT not null,
  RELEASE        TEXT not null,
  TIME           TEXT not null,
  CREATEDBY      TEXT not null,
  HOST           TEXT not null,
  DESCRIPTION    TEXT,
  constraint     OSTAG_PK primary key (VERSION),
  constraint     OSTAG_CK check (VERSION > 0)
);

create table OKSDATA (
  SCHEMAVERSION  INT not null,
  BASEVERSION    INT,
  VERSION        INT not null,
  TIME           TEXT not null,
  CREATEDBY      TEXT not null,
  HOST           TEXT not null,
  DESCRIPTION    TEXT,
  constraint     ODTAG_PK primary key (SCHEMAVERSION, VERSION),
  constraint     ODTAG_SCHEMA_FK foreign key (SCHEMAVERSION) references OKSSCHEMA (VERSION),
  constraint     ODTAG_CK check (VERSION > 0),
  constraint	 ODTAG_BASE_FK foreign key (SCHEMAVERSION, BASEVERSION) references OKSDATA (SCHEMAVERSION, VERSION)
);


create table OKSTAG (
  NAME           TEXT not null,
  SCHEMAVERSION  INT not null,
  DATAVERSION    INT not null,
  TIME           TEXT not null,
  CREATEDBY      TEXT not null,
  HOST           TEXT not null,
  constraint     OKSTAG_PK primary key (NAME),
  constraint     OKSTAG_FK foreign key (SCHEMAVERSION, DATAVERSION) references OKSDATA (SCHEMAVERSION, VERSION)
);

create table OKSARCHIVE (
  TIME           TEXT not null,
  PARTITION      TEXT not null,
  SCHEMAVERSION  INT not null,
  DATAVERSION    INT not null,
  CREATEDBY      TEXT not null,
  HOST           TEXT not null,
  RUNNUMBER      INT,
  constraint     OKSARCHIVE_PK primary key (TIME,PARTITION),
  constraint     OKSARCHIVE_FK foreign key (SCHEMAVERSION, DATAVERSION) references OKSDATA (SCHEMAVERSION, VERSION)
);

create table OKSCLASS (
  VERSION      INT not null,
  NAME         TEXT not null,
  DESCRIPTION  TEXT,
  ISABSTRACT   BOOLEAN,
  constraint   OC_PK primary key (VERSION, NAME),
  constraint   OC_FK foreign key (VERSION) references OKSSCHEMA (VERSION)
);

create table OKSSUPERCLASS (
  VERSION         INT not null,
  PARENTNAME      TEXT not null,
  PARENTPOSITION  INT,
  VALUE           TEXT not null,
  constraint      OSC_VALUE_PK primary key (VERSION, PARENTNAME, VALUE),
  constraint      OSC_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      OSC_VALUE_FK foreign key (VERSION, VALUE) references OKSCLASS (VERSION, NAME),
  constraint      OSC_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION)
);

create table OKSATTRIBUTE (
  VERSION         INT not null,
  PARENTNAME      TEXT not null,
  PARENTPOSITION  INT,
  ISDIRECT        BOOLEAN,
  NAME            TEXT not null,
  DESCRIPTION     TEXT,
  TYPE            TEXT not null,
  RANGE           TEXT,
  FORMAT          TEXT not null,
  ISMULTIVALUE    BOOLEAN,
  MULTIVALUEIMPL  TEXT not null,
  INITVALUE       TEXT,
  ISNOTNULL       BOOLEAN,
  constraint      OA_NAME_PK primary key (VERSION, PARENTNAME, NAME),
  constraint      OA_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      OA_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION),
  constraint      OA_TYPE_CK check (TYPE in ('bool','s8','u8','s16','u16','s32','u32','s64','u64','float','double','date','time','string','enum','class')),
  constraint      OA_FORMAT_CK check (FORMAT in ('dec','hex','oct')),
  constraint      OA_MVIMPL_CK check ( MULTIVALUEIMPL in ('list','vector'))
);

create table OKSRELATIONSHIP (
  VERSION         INT not null,
  PARENTNAME      TEXT not null,
  PARENTPOSITION  INT,
  ISDIRECT        BOOLEAN,
  NAME            TEXT not null,
  DESCRIPTION     TEXT,
  CLASSTYPE       TEXT not null,
  LOWCC           TEXT not null,
  HIGHCC          TEXT not null,
  ISCOMPOSITE     BOOLEAN,
  ISEXCLUSIVE     BOOLEAN,
  ISDEPENDENT     BOOLEAN,
  MULTIVALUEIMPL  TEXT not null,
  constraint      OR_NAME_PK primary key (VERSION, PARENTNAME, NAME),
  constraint      OR_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      OR_CLASSTYPE_FK foreign key (VERSION, CLASSTYPE) references OKSCLASS (VERSION, NAME),
  constraint      OR_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION),
  constraint      OR_LOWCC_CK check ( LOWCC in ('zero','one')),
  constraint      OR_HIGHCC_CK check ( HIGHCC in ('one','many')),
  constraint      OR_MVIMPL_CK check ( MULTIVALUEIMPL in ('list','vector'))
);

create table OKSMETHOD (
  VERSION         INT not null,
  PARENTNAME      TEXT not null,
  PARENTPOSITION  INT,
  ISDIRECT        BOOLEAN,
  NAME            TEXT not null,
  DESCRIPTION     TEXT,
  constraint      OM_NAME_PK primary key (VERSION, PARENTNAME, NAME),
  constraint      OM_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      OM_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION)
);

create table OKSMETHODIMPLEMENTATION (
  VERSION           INT not null,
  PARENTCLASSNAME   TEXT not null,
  PARENTMETHODNAME  TEXT not null,
  PARENTPOSITION    INT,
  LANGUAGE          TEXT not null,
  PROTOTYPE         TEXT not null,
  BODY              TEXT,
  constraint        OMI_LANGUAGE_PK primary key (VERSION, PARENTCLASSNAME, PARENTMETHODNAME, LANGUAGE),
  constraint        OMI_PARENT_FK foreign key (VERSION, PARENTCLASSNAME, PARENTMETHODNAME) references OKSMETHOD (VERSION, PARENTNAME, NAME),
  constraint        OMI_POSITION_UN unique (VERSION, PARENTCLASSNAME, PARENTMETHODNAME, PARENTPOSITION)
);

create table OKSOBJECT (
  SCHEMAVERSION     INT not null,
  DATAVERSION       INT not null,
  CLASSNAME         TEXT not null,
  ID                TEXT not null,
  STATE             SHORT,
  constraint        OO_PK primary key (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID),
  constraint        OO_TAG_FK foreign key (SCHEMAVERSION, DATAVERSION) references OKSDATA (SCHEMAVERSION, VERSION),
  constraint        OO_CLASS_FK foreign key (SCHEMAVERSION, CLASSNAME) references OKSCLASS (VERSION, NAME),
  constraint        OO_STATE_CK check (STATE in (0,1,2))
);

create table OKSDATAREL (
  SCHEMAVERSION       INT not null,
  DATAVERSION         INT not null,
  CLASSNAME           TEXT not null,
  OBJECTID            TEXT not null,
  NAME                TEXT not null,
  PARENTPOSITION      INT,
  VALUECLASSNAME      TEXT not null,
  VALUEOBJECTID       TEXT not null,
  VALUEVERSION        INT,
  constraint          ODR_POSITION_PK primary key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID, NAME, PARENTPOSITION),
  constraint          ODR_PARENT_FK foreign key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID) references OKSOBJECT (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID),
  constraint          ODR_NAME_FK foreign key (SCHEMAVERSION, CLASSNAME, NAME) references OKSRELATIONSHIP (VERSION, PARENTNAME, NAME),
  constraint          ODR_VALUE_FK foreign key (SCHEMAVERSION, VALUEVERSION, VALUECLASSNAME, VALUEOBJECTID) references OKSOBJECT (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID)
);

create table OKSDATAVAL (
  SCHEMAVERSION       INT not null,
  DATAVERSION         INT not null,
  CLASSNAME           TEXT not null,
  OBJECTID            TEXT not null,
  NAME                TEXT not null,
  PARENTPOSITION      INT,
  INTEGERVALUE        SLONGLONG,
  NUMBERVALUE         DOUBLE,
  STRINGVALUE         TEXT,
  constraint          ODV_POSITION_PK primary key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID, NAME, PARENTPOSITION),
  constraint          ODV_PARENT_FK foreign key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID) references OKSOBJECT (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID),
  constraint          ODV_NAME_FK foreign key (SCHEMAVERSION, CLASSNAME, NAME) references OKSATTRIBUTE (VERSION, PARENTNAME, NAME),
  constraint          ODV_OneVALUE check (
                        ((INTEGERVALUE is not null) and (NUMBERVALUE is null) and (StringVALUE is null)) or
                        ((INTEGERVALUE is null) and (NUMBERVALUE is not null) and (StringVALUE is null)) or
                        ((INTEGERVALUE is null) and (NUMBERVALUE is null) and (StringVALUE is not null)) or
                        ((INTEGERVALUE is null) and (NUMBERVALUE is null) and (StringVALUE is null))
                      )
);
