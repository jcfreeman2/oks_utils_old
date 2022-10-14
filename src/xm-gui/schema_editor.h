#ifndef __SCHEMA_EDITOR_H
#define __SCHEMA_EDITOR_H

extern const char *szClasses;
extern const char *szSchema;
extern const char *szClass;
extern const char *szIsAbstract;
extern const char *szDescription;
extern const char *szSuperClasses;
extern const char *szSubClasses;
extern const char *szAttributes;
extern const char *szRelationships;
extern const char *szMethods;
extern const char *szType;
extern const char *szCardinality;
extern const char *szNonNullValue;
extern const char *szInitialValue;
extern const char *szRange;
extern const char *szFormat;
extern const char *szName;
extern const char *szDefinedInClass;
extern const char *szCardinalityConstraint;
extern const char *szReferenceType;
extern const char *szBrowse;
extern const char *szClassType;
extern const char *szBody;
extern const char *szConditions;
extern const char *szActions;

extern const char *szYes;
extern const char *szNo;
extern const char *szSingleValue;
extern const char *szMultiValue;
extern const char *szCanBeNull;
extern const char *szNonNull;

extern const char *szCreate;
extern const char *szUpdate;
extern const char *szModify;
extern const char *szDelete;

extern const char *szNew;
extern const char *szShow;
extern const char *szSort;

enum {
  idClasses = 201,
  idSchema,
  idClass,
  idIsAbstract,
  idDescription,
  idSuperClasses,
  idSubClasses,
  idAttributes,
  idRelationships,
  idMethods,
  idTypeList,
  idFormats,
  idType,
  idCardinality,
  idNonNullValue,
  idInitialValue,
  idRange,
  idFormat,
  idName,
  idDefinedInClass,
  idBrowse,
  idCardinalityConstraint,
  idReferenceType,
  idClassType,
  idBody,
  idConditions,
  idActions,
  idClassPanedWindow,
  idMainPanedWindow
};

#endif
