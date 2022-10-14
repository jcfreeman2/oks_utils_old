#ifndef __DATA_EDITOR_H
#define __DATA_EDITOR_H

#include <string>

	//
	// Identifiers of action callbacks for pop-up menu
	// and items from different windows
	//

enum {
  idSchema = 200,
  idData,
  idDataFile,
  idClasses,
  idObjects,
  idClass,
  idObject,
  idMainPanedWindow,
  idAttributes,
  idRelationships,
  idQuery,
  idQueryPanedWindow,
  idQueryForm,
  idQueryResult,
  idSearchInSubClasses,
  idQueryRelationshipName,
  idAllRelsObjects,
  idRelationshipTreeForm,

  // ADD NEW IDs ABOVE idSelectPanel$$

  idSelectPanelBase

  // DO NOT ADD ANYTHING HERE
};

class OksObject;
class OksClass;
class OksAttribute;
class OksRelationship;

OksObject* findOksObject(char *);
OksClass*  findOksClass(const char *);

std::string make_description(const OksAttribute * a);
std::string make_description(const OksRelationship * r);

char * strip_brackets(const OksAttribute * a, char * s);

#endif
