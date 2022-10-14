create table OKSSCHEMA (
  VERSION        mediumint not null,
  RELEASE        varchar(48) not null,
  TIME           varchar(24) not null,
  CREATEDBY      varchar(16) not null,
  HOST           varchar(256) not null,
  DESCRIPTION    varchar(2000),
  constraint     OSTAG_PK primary key (VERSION),
  constraint     OSTAG_CK check (VERSION > 0)
);

create table OKSDATA (
  SCHEMAVERSION  mediumint not null,
  BASEVERSION    mediumint,
  VERSION        mediumint not null,
  TIME           varchar(24) not null,
  CREATEDBY      varchar(16) not null,
  HOST           varchar(256) not null,
  DESCRIPTION    varchar(2000),
  constraint     ODTAG_PK primary key (SCHEMAVERSION, VERSION),
  constraint     ODTAG_SCHEMA_FK foreign key (SCHEMAVERSION) references OKSSCHEMA (VERSION),
  constraint     ODTAG_CK check (VERSION > 0)
);

alter table OKSDATA add constraint ODTAG_BASE_FK foreign key (SCHEMAVERSION, BASEVERSION) references OKSDATA (SCHEMAVERSION, VERSION);

create table OKSTAG (
  NAME           varchar(256) not null,
  SCHEMAVERSION  mediumint not null,
  DATAVERSION    mediumint not null,
  TIME           varchar(24) not null,
  CREATEDBY      varchar(16) not null,
  HOST           varchar(256) not null,
  constraint     OKSTAG_PK primary key (NAME),
  constraint     OKSTAG_FK foreign key (SCHEMAVERSION, DATAVERSION) references OKSDATA (SCHEMAVERSION, VERSION)
);

create table OKSARCHIVE (
  TIME           varchar(24) not null,
  PARTITION      varchar(256) not null,
  SCHEMAVERSION  mediumint not null,
  DATAVERSION    mediumint not null,
  CREATEDBY      varchar(16) not null,
  HOST           varchar(256) not null,
  RUNNUMBER      mediumint,
  constraint     OKSARCHIVE_PK primary key (TIME,PARTITION),
  constraint     OKSARCHIVE_FK foreign key (SCHEMAVERSION, DATAVERSION) references OKSDATA (SCHEMAVERSION, VERSION)
);

create table OKSCLASS (
  VERSION      mediumint not null,
  NAME         varchar(64) not null,
  DESCRIPTION  varchar(2000),
  ISABSTRACT   tinyint(1),
  constraint   OC_PK primary key (VERSION, NAME),
  constraint   OC_FK foreign key (VERSION) references OKSSCHEMA (VERSION),
  constraint   OC_ISABSTRACT_CK check (ISABSTRACT in (0,1))
);

create table OKSSUPERCLASS (
  VERSION         mediumint not null,
  PARENTNAME      varchar(64) not null,
  PARENTPOSITION  mediumint,
  VALUE           varchar(64) not null,
  constraint      OSC_VALUE_PK primary key (VERSION, PARENTNAME, VALUE),
  constraint      OSC_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      OSC_VALUE_FK foreign key (VERSION, VALUE) references OKSCLASS (VERSION, NAME),
  constraint      OSC_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION)
);

create table OKSATTRIBUTE (
  VERSION         mediumint not null,
  PARENTNAME      varchar(64) not null,
  PARENTPOSITION  mediumint,
  ISDIRECT        tinyint(1),
  NAME            varchar(128) not null,
  DESCRIPTION     varchar(2000),
  TYPE            varchar(6) not null,
  RANGE           varchar(1024),
  FORMAT          varchar(3) not null,
  ISMULTIVALUE    tinyint(1),
  MULTIVALUEIMPL  varchar(6) not null,
  INITVALUE       varchar(1024),
  ISNOTNULL       tinyint(1),
  constraint      OA_NAME_PK primary key (VERSION, PARENTNAME, NAME),
  constraint      OA_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      OA_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION),
  constraint      OA_TYPE_CK check (TYPE in ('bool','s8','u8','s16','u16','s32','u32','s64','u64','float','double','date','time','string','enum','class')),
  constraint      OA_FORMAT_CK check (FORMAT in ('dec','hex','oct')),
  constraint      OA_IsMV_CK check (ISMULTIVALUE in (0,1)),
  constraint      OA_MVIMPL_CK check ( MULTIVALUEIMPL in ('list','vector')),
  constraint      OA_ISNOTNULL_CK check (ISNOTNULL in (0,1)),
  constraint      OA_ISDIRECT_CK check (ISDIRECT in (0,1))
);

create table OKSRELATIONSHIP (
  VERSION         mediumint not null,
  PARENTNAME      varchar(64) not null,
  PARENTPOSITION  mediumint,
  ISDIRECT        tinyint(1),
  NAME            varchar(128) not null,
  DESCRIPTION     varchar(2000),
  CLASSTYPE       varchar(64) not null,
  LOWCC           varchar(4) not null,
  HIGHCC          varchar(4) not null,
  ISCOMPOSITE     tinyint(1),
  ISEXCLUSIVE     tinyint(1),
  ISDEPENDENT     tinyint(1),
  MULTIVALUEIMPL  varchar(6) not null,
  constraint      or_NAME_PK primary key (VERSION, PARENTNAME, NAME),
  constraint      or_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      or_CLASSTYPE_FK foreign key (VERSION, CLASSTYPE) references OKSCLASS (VERSION, NAME),
  constraint      or_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION),
  constraint      or_LOWCC_CK check ( LOWCC in ('zero','one')),
  constraint      or_HIGHCC_CK check ( HIGHCC in ('one','many')),
  constraint      or_ISCOMPOSITE_CK check ( ISCOMPOSITE in (0,1)),
  constraint      or_ISEXCLUSIVE_CK check ( ISEXCLUSIVE in (0,1)),
  constraint      or_ISDEPENDENT_CK check ( ISDEPENDENT in (0,1)),
  constraint      or_MVIMPL_CK check ( MULTIVALUEIMPL in ('list','vector')),
  constraint      or_ISDIRECT_CK check (ISDIRECT in (0,1))
);

create table OKSMETHOD (
  VERSION         mediumint not null,
  PARENTNAME      varchar(64) not null,
  PARENTPOSITION  mediumint,
  ISDIRECT        tinyint(1),
  NAME            varchar(128) not null,
  DESCRIPTION     varchar(2000),
  constraint      OM_NAME_PK primary key (VERSION, PARENTNAME, NAME),
  constraint      OM_PARENT_FK foreign key (VERSION, PARENTNAME) references OKSCLASS (VERSION, NAME),
  constraint      OM_POSITION_UN unique (VERSION, PARENTNAME, PARENTPOSITION),
  constraint      OM_ISDIRECT_CK check (ISDIRECT in (0,1))
);

create table OKSMETHODIMPLEMENTATION (
  VERSION           mediumint not null,
  PARENTCLASSNAME   varchar(64) not null,
  PARENTMETHODNAME  varchar(128) not null,
  PARENTPOSITION    mediumint,
  LANGUAGE          varchar(16) not null,
  PROTOTYPE         varchar(1024) not null,
  BODY              varchar(2048),
  constraint        OMI_LANGUAGE_PK primary key (VERSION, PARENTCLASSNAME, PARENTMETHODNAME, LANGUAGE),
  constraint        OMI_PARENT_FK foreign key (VERSION, PARENTCLASSNAME, PARENTMETHODNAME) references OKSMETHOD (VERSION, PARENTNAME, NAME),
  constraint        OMI_POSITION_UN unique (VERSION, PARENTCLASSNAME, PARENTMETHODNAME, PARENTPOSITION)
);

create table OKSOBJECT (
  SCHEMAVERSION     mediumint not null,
  DATAVERSION       mediumint not null,
  CREATIONVERSION   mediumint not null,
  CLASSNAME         varchar(64) not null,
  ID                varchar(64) not null,
  STATE             smallint,
  constraint        OO_PK primary key (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID),
  constraint        OO_TAG_FK foreign key (SCHEMAVERSION, DATAVERSION) references OKSDATA (SCHEMAVERSION, VERSION),
  constraint        OO_CLASS_FK foreign key (SCHEMAVERSION, CLASSNAME) references OKSCLASS (VERSION, NAME),
  constraint        OO_STATE_CK check (STATE in (0,1,2))
);

create table OKSDATAREL (
  SCHEMAVERSION       mediumint not null,
  DATAVERSION         mediumint not null,
  CREATIONVERSION     mediumint not null,
  CLASSNAME           varchar(64) not null,
  OBJECTID            varchar(64) not null,
  NAME                varchar(128) not null,
  PARENTPOSITION      mediumint,
  VALUECLASSNAME      varchar(64) not null,
  VALUEOBJECTID       varchar(64) not null,
  VALUEVERSION        mediumint,
  constraint          ODR_POSITION_PK primary key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID, NAME, PARENTPOSITION),
  constraint          ODR_PARENT_FK foreign key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID) references OKSOBJECT (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID),
  constraint          ODR_NAME_FK foreign key (SCHEMAVERSION, CLASSNAME, NAME) references OKSRELATIONSHIP (VERSION, PARENTNAME, NAME),
  constraint          ODR_VALUE_FK foreign key (SCHEMAVERSION, VALUEVERSION, VALUECLASSNAME, VALUEOBJECTID) references OKSOBJECT (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID)
);

create table OKSDATAVAL (
  SCHEMAVERSION       mediumint not null,
  DATAVERSION         mediumint not null,
  CREATIONVERSION     mediumint not null,
  CLASSNAME           varchar(64) not null,
  OBJECTID            varchar(64) not null,
  NAME                varchar(128) not null,
  PARENTPOSITION      mediumint,
  INTEGERVALUE        bigint,
  NUMBERVALUE         double,
  STRINGVALUE         varchar(4000),
  constraint          ODV_POSITION_PK primary key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID, NAME, PARENTPOSITION),
  constraint          ODV_PARENT_FK foreign key (SCHEMAVERSION, DATAVERSION, CLASSNAME, OBJECTID) references OKSOBJECT (SCHEMAVERSION, DATAVERSION, CLASSNAME, ID),
  constraint          ODV_NAME_FK foreign key (SCHEMAVERSION, CLASSNAME, NAME) references OKSATTRIBUTE (VERSION, PARENTNAME, NAME),
  constraint          ODV_OneVALUE check (
                        ((INTEGERVALUE is not null) and (NUMBERVALUE is null) and (STRINGVALUE is null)) or
                        ((INTEGERVALUE is null) and (NUMBERVALUE is not null) and (STRINGVALUE is null)) or
                        ((INTEGERVALUE is null) and (NUMBERVALUE is null) and (STRINGVALUE is not null)) or
                        ((INTEGERVALUE is null) and (NUMBERVALUE is null) and (STRINGVALUE is null))
                      )
);

create temporary table TEMPOKSOBJECT (
  SCHEMAVERSION     mediumint not null,
  DATAVERSION       mediumint not null,
  CREATIONVERSION   mediumint not null,
  CLASSNAME         varchar(64) not null,
  ID                varchar(64) not null,
  STATE             smallint
);

create temporary table TEMPOKSDATAREL (
  SCHEMAVERSION       mediumint not null,
  DATAVERSION         mediumint not null,
  CREATIONVERSION     mediumint not null,
  CLASSNAME           varchar(64) not null,
  OBJECTID            varchar(64) not null,
  NAME                varchar(128) not null,
  PARENTPOSITION      mediumint,
  VALUECLASSNAME      varchar(64) not null,
  VALUEOBJECTID       varchar(64) not null,
  VALUEVERSION        mediumint
);

create temporary table TEMPOKSDATAVAL (
  SCHEMAVERSION       mediumint not null,
  DATAVERSION         mediumint not null,
  CREATIONVERSION     mediumint not null,
  CLASSNAME           varchar(64) not null,
  OBJECTID            varchar(64) not null,
  NAME                varchar(128) not null,
  PARENTPOSITION      mediumint,
  INTEGERVALUE        bigint,
  NUMBERVALUE         double,
  STRINGVALUE         varchar(4000)
);
