rem set termout off

drop table TempOksDataVal;
drop table TempOksDataRel;
drop table TempOksObject;

drop table OksDataVal;
drop table OksDataRel;
drop table OksObject;
drop table OksMethodImplementation;
drop table OksMethod;
drop table OksRelationship;
drop table OksAttribute;
drop table OksSuperClass;
drop table OksClass;
drop table OksArchive;
drop table OksTag;
drop table OksData;
drop table OksSchema;

create table OksSchema (
  Version        number(10) not null,
  Release        varchar2(48) not null,
  Time           varchar2(24) not null,
  CreatedBy      varchar2(16) not null,
  Host           varchar2(256) not null,
  Description    varchar2(2000),
  constraint     OSTAG_PK primary key (Version),
  constraint     OSTAG_CK check (Version > 0)
);

create table OksData (
  SchemaVersion  number(10) not null,
  BaseVersion    number(10),
  Version        number(10) not null,
  Time           varchar2(24) not null,
  CreatedBy      varchar2(16) not null,
  Host           varchar2(256) not null,
  Description    varchar2(2000),
  constraint     ODTAG_PK primary key (SchemaVersion, Version),
  constraint     ODTAG_Schema_FK foreign key (SchemaVersion) references OksSchema (Version),
  constraint     ODTAG_CK check (Version > 0)
);

alter table OksData add constraint ODTAG_Base_FK foreign key (SchemaVersion, BaseVersion) references OksData (SchemaVersion, Version);

create table OksTag (
  Name           varchar2(256) not null,
  SchemaVersion  number(10) not null,
  DataVersion    number(10) not null,
  Time           varchar2(24) not null,
  CreatedBy      varchar2(16) not null,
  Host           varchar2(256) not null,
  constraint     OksTag_PK primary key (Name),
  constraint     OksTag_FK foreign key (SchemaVersion, DataVersion) references OksData (SchemaVersion, Version)
);

create table OksArchive (
  Time           varchar2(24) not null,
  Partition      varchar2(256) not null,
  SchemaVersion  number(10) not null,
  DataVersion    number(10) not null,
  CreatedBy      varchar2(16) not null,
  Host           varchar2(256) not null,
  RunNumber      number(10),
  constraint     OksArchive_PK primary key (Time,Partition),
  constraint     OksArchive_FK foreign key (SchemaVersion, DataVersion) references OksData (SchemaVersion, Version)
);

create table OksClass (
  Version      number(10) not null,
  Name         varchar2(64) not null,
  Description  varchar2(2000),
  IsAbstract   number(1),
  constraint   OC_PK primary key (Version, Name),
  constraint   OC_FK foreign key (Version) references OksSchema (Version),
  constraint   OC_IsAbstract_CK check (IsAbstract in (0,1))
);

create table OksSuperClass (
  Version         number(10) not null,
  ParentName      varchar2(64) not null,
  ParentPosition  number(10),
  Value           varchar2(64) not null,
  constraint      OSC_Value_PK primary key (Version, ParentName, Value),
  constraint      OSC_Parent_FK foreign key (Version, ParentName) references OksClass (Version, Name),
  constraint      OSC_Value_FK foreign key (Version, Value) references OksClass (Version, Name),
  constraint      OSC_Position_UN unique (Version, ParentName, ParentPosition)
);

create table OksAttribute (
  Version         number(10) not null,
  ParentName      varchar2(64) not null,
  ParentPosition  number(10),
  IsDirect        number(1),
  Name            varchar2(128) not null,
  Description     varchar2(2000),
  Type            varchar2(6) not null,
  Range           varchar2(1024),
  Format          varchar2(3) not null,
  IsMultiValue    number(1),
  MultiValueImpl  varchar2(6) not null,
  InitValue       varchar2(1024),
  IsNotNull       number(1),
  constraint      OA_Name_PK primary key (Version, ParentName, Name),
  constraint      OA_Parent_FK foreign key (Version, ParentName) references OksClass (Version, Name),
  constraint      OA_Position_UN unique (Version, ParentName, ParentPosition),
  constraint      OA_Type_CK check (Type in ('bool','s8','u8','s16','u16','s32','u32','s64','u64','float','double','date','time','string','enum','class')),
  constraint      OA_Format_CK check (Format in ('dec','hex','oct')),
  constraint      OA_IsMV_CK check (IsMultiValue in (0,1)),
  constraint      OA_MVImpl_CK check ( MultiValueImpl in ('list','vector')),
  constraint      OA_IsNotNull_CK check (IsNotNull in (0,1)),
  constraint      OA_IsDirect_CK check (IsDirect in (0,1))
);

create table OksRelationship (
  Version         number(10) not null,
  ParentName      varchar2(64) not null,
  ParentPosition  number(10),
  IsDirect        number(1),
  Name            varchar2(128) not null,
  Description     varchar2(2000),
  ClassType       varchar2(64) not null,
  LowCC           varchar2(4) not null,
  HighCC          varchar2(4) not null,
  IsComposite     number(1),
  IsExclusive     number(1),
  IsDependent     number(1),
  MultiValueImpl  varchar2(6) not null,
  constraint      or_Name_PK primary key (Version, ParentName, Name),
  constraint      or_Parent_FK foreign key (Version, ParentName) references OksClass (Version, Name),
  constraint      or_ClassType_FK foreign key (Version, ClassType) references OksClass (Version, Name),
  constraint      or_Position_UN unique (Version, ParentName, ParentPosition),
  constraint      or_LowCC_CK check ( LowCC in ('zero','one')),
  constraint      or_HighCC_CK check ( HighCC in ('one','many')),
  constraint      or_IsComposite_CK check ( IsComposite in (0,1)),
  constraint      or_IsExclusive_CK check ( IsExclusive in (0,1)),
  constraint      or_IsDependent_CK check ( IsDependent in (0,1)),
  constraint      or_MVImpl_CK check ( MultiValueImpl in ('list','vector')),
  constraint      or_IsDirect_CK check (IsDirect in (0,1))
);

create table OksMethod (
  Version         number(10) not null,
  ParentName      varchar2(64) not null,
  ParentPosition  number(10),
  IsDirect        number(1),
  Name            varchar2(128) not null,
  Description     varchar2(2000),
  constraint      OM_Name_PK primary key (Version, ParentName, Name),
  constraint      OM_Parent_FK foreign key (Version, ParentName) references OksClass (Version, Name),
  constraint      OM_Position_UN unique (Version, ParentName, ParentPosition),
  constraint      OM_IsDirect_CK check (IsDirect in (0,1))
);

create table OksMethodImplementation (
  Version           number(10) not null,
  ParentClassName   varchar2(64) not null,
  ParentMethodName  varchar2(128) not null,
  ParentPosition    number(10),
  Language          varchar2(16) not null,
  Prototype         varchar2(1024) not null,
  Body              varchar2(2048),
  constraint        OMI_Language_PK primary key (Version, ParentClassName, ParentMethodName, Language),
  constraint        OMI_Parent_FK foreign key (Version, ParentClassName, ParentMethodName) references OksMethod (Version, ParentName, Name),
  constraint        OMI_Position_UN unique (Version, ParentClassName, ParentMethodName, ParentPosition)
);

create table OksObject (
  SchemaVersion      number(10) not null,
  DataVersion        number(10) not null,
  CreationVersion    number(10) not null,
  ClassName          varchar2(64) not null,
  Id                 varchar2(64) not null,
  State              number(2),
  constraint         OO_PK primary key (SchemaVersion, DataVersion, ClassName, Id),
  constraint         OO_Parent_FK foreign key (SchemaVersion, DataVersion) references OksData (SchemaVersion, Version),
  constraint         OO_Creation_FK foreign key (SchemaVersion, CreationVersion) references OksData (SchemaVersion, Version),
  constraint         OO_Class_FK foreign key (SchemaVersion, ClassName) references OksClass (Version, Name),
  constraint         OO_State_CK check (State in (0,1,2))
);

create table OksDataRel (
  SchemaVersion       number(10) not null,
  DataVersion         number(10) not null,
  CreationVersion     number(10) not null,
  ClassName           varchar2(64) not null,
  ObjectId            varchar2(64) not null,
  Name                varchar2(128) not null,
  ParentPosition      number(10),
  ValueClassName      varchar2(64) not null,
  ValueObjectId       varchar2(64) not null,
  ValueVersion        number(10),
  constraint          ODR_Position_PK primary key (SchemaVersion, DataVersion, ClassName, ObjectId, Name, ParentPosition),
  constraint          ODR_Parent_FK foreign key (SchemaVersion, DataVersion, ClassName, ObjectId) references OksObject (SchemaVersion, DataVersion, ClassName, Id),
  constraint          ODR_Creation_FK foreign key (SchemaVersion, CreationVersion) references OksData (SchemaVersion, Version),
  constraint          ODR_Name_FK foreign key (SchemaVersion, ClassName, Name) references OksRelationship (Version, ParentName, Name),
  constraint          ODR_Value_FK foreign key (SchemaVersion, ValueVersion, ValueClassName, ValueObjectId) references OksObject (SchemaVersion, DataVersion, ClassName, Id)
);

create index ODR_Value_FK_IDX on OksDataRel ("SCHEMAVERSION", "VALUEVERSION", "VALUECLASSNAME", "VALUEOBJECTID");

create table OksDataVal (
  SchemaVersion       number(10) not null,
  DataVersion         number(10) not null,
  CreationVersion     number(10) not null,
  ClassName           varchar2(64) not null,
  ObjectId            varchar2(64) not null,
  Name                varchar2(128) not null,
  ParentPosition      number(10),
  IntegerValue        number(20),
  NumberValue         number,
  StringValue         varchar2(4000),
  constraint          ODV_Position_PK primary key (SchemaVersion, DataVersion, ClassName, ObjectId, Name, ParentPosition),
  constraint          ODV_Parent_FK foreign key (SchemaVersion, DataVersion, ClassName, ObjectId) references OksObject (SchemaVersion, DataVersion, ClassName, Id),
  constraint          ODV_Creation_FK foreign key (SchemaVersion, CreationVersion) references OksData (SchemaVersion, Version),
  constraint          ODV_Name_FK foreign key (SchemaVersion, ClassName, Name) references OksAttribute (Version, ParentName, Name),
  constraint          ODV_OneValue check (
                        ((IntegerValue is not null) and (NumberValue is null) and (StringValue is null)) or
                        ((IntegerValue is null) and (NumberValue is not null) and (StringValue is null)) or
                        ((IntegerValue is null) and (NumberValue is null) and (StringValue is not null)) or
                        ((IntegerValue is null) and (NumberValue is null) and (StringValue is null))
                      )
);

rem #######################################################################################################
rem #### The 3 tables below are used to avoid problems with Oracle stream replication in case of abort ####
rem #######################################################################################################

create global temporary table TempOksObject (
  SchemaVersion      number(10) not null,
  DataVersion        number(10) not null,
  CreationVersion    number(10) not null,
  ClassName          varchar2(64) not null,
  Id                 varchar2(64) not null,
  State              number(2)
) on commit delete rows;

create global temporary table TempOksDataRel (
  SchemaVersion       number(10) not null,
  DataVersion         number(10) not null,
  CreationVersion     number(10) not null,
  ClassName           varchar2(64) not null,
  ObjectId            varchar2(64) not null,
  Name                varchar2(128) not null,
  ParentPosition      number(10),
  ValueClassName      varchar2(64) not null,
  ValueObjectId       varchar2(64) not null,
  ValueVersion        number(10)
) on commit delete rows;

create global temporary table TempOksDataVal (
  SchemaVersion       number(10) not null,
  DataVersion         number(10) not null,
  CreationVersion     number(10) not null,
  ClassName           varchar2(64) not null,
  ObjectId            varchar2(64) not null,
  Name                varchar2(128) not null,
  ParentPosition      number(10),
  IntegerValue        number(20),
  NumberValue         number,
  StringValue         varchar2(4000)
) on commit delete rows;

rem describe OksObject;

rem insert into OksSchema     values (25, 'Test Schema', 'now', 'test 0');
rem insert into OksData       values (25, 1, 1, 'Test Data', 'again now', 'test 1');

rem insert into OksClass      values (25, 'A', 'it is a test application class', 1);
rem insert into OksClass      values (25, 'B',  'it is a test RC app class', 0);
rem insert into OksSuperClass values (25, 'B', 1, 'A');
rem insert into OksAttribute  values (25, 'A', 1, 1, 'Name', 'just a name', 'string', '', 'dec', 0, 'list', '?', 1);

rem insert into OksObject     values (21, 1, 1, 'TTCviECR', 'XXX', 1);
rem insert into OksObject     values (25, 1, 1, 'A', 'b', 1);
rem insert into OksDataVal    values (25, 1, 'A', 'a', 'Name', 1, null, null, 'Peter');
rem insert into OksDataVal    values (25, 1, 'A', 'a', 'Name', 2, 1, null, null);
rem insert into OksDataVal    values (25, 1, 'A', 'a', 'Name', 3, null, 3.14, null);
rem insert into OksDataVal    values (25, 1, 'A', 'a', 'Name', 4, 5, null, null);

rem set termout on
