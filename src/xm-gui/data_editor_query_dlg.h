#ifndef __DATA_EDITOR_QUERY_DLG_H
#define __DATA_EDITOR_QUERY_DLG_H 

#include <oks/query.h>

#include "data_editor_dialog.h"

class OksObject;
class OksClass;

class OksDataEditorQueryDialog : public OksDataEditorDialog, public OksQuery
{
  friend class OksDataEditorMainDialog;


  public:

    OksDataEditorQueryDialog	(OksClass*, OksDataEditorMainDialog*, OksQuery *);

    ~OksDataEditorQueryDialog	();
    OksDataEditorDialog::Type	type() const {return OksDataEditorDialog::QueryDlg;}

    OksClass * get_class() const {return p_class;}
    Widget tree_widget() const {return treeWidget;}

    static OksQueryExpression * create_expression(Widget, OksClass *, OksQuery::QueryType, Widget = 0, void * = 0);


  private:

    OksClass * p_class;
    Widget treeWidget;

    static void searchSubClassesCB(Widget, XtPointer, XtPointer);
    static void formCB(Widget, XtPointer, XtPointer);
    static void objectCB(Widget, XtPointer, XtPointer);
    static void closeCB(Widget, XtPointer, XtPointer);

    static void formAC(Widget, XtPointer, XEvent *, Boolean *);

    enum {
      idAddAttribute = 100,
      idAddRelationship,
      idAddNot,
      idAddAnd,
      idAddOr,
      idExecute,
      idSelectQuery,
      idSave
    };

};


class OksQueryAttributeExpressionForm : public OksComparator, public OksForm
{
  public:

    OksQueryAttributeExpressionForm(OksClass *, Widget, OksComparator * = 0);

    OksClass*	get_class() const {return p_class;}
    void	change_value(Widget);

	
  private:

    OksClass * p_class;
    const OksAttribute * p_saved_attribute;
    Widget p_use_attr_value_widget;
    Widget p_object_id_widget;
    Widget p_value_widget;
    Widget p_name_widget;

    static void	nameCB(Widget, XtPointer, XtPointer);
    static void	valueCB(Widget, XtPointer, XtPointer);
    static void	functionCB(Widget, XtPointer, XtPointer);
    static void switchTypeCB(Widget, XtPointer, XtPointer);

    enum {
      idName = 777,
      idValue,
      idComparator,
      idUseAttributeValue,
      idUseObjectID
    };
};


class OksQueryRelationshipExpressionForm : public OksRelationshipExpression, public OksForm
{
  public:

    OksQueryRelationshipExpressionForm(OksClass *, Widget, OksRelationshipExpression * = 0);

    OksClass * get_class() const {return p_class;}
    Widget tree_widget() const {return treeWidget;}


  private:

    OksClass * p_class;
    Widget treeWidget;
    
    static void	nameCB(Widget, XtPointer, XtPointer);
    static void	classesCB(Widget, XtPointer, XtPointer);
    static void	formCB(Widget, XtPointer, XtPointer);
    static void formAC(Widget, XtPointer, XEvent *, Boolean *);

    enum {
      idAcQueryRelationshipFormAddAttribute = 100,
      idAcQueryRelationshipFormAddRelationship,
      idAcQueryRelationshipFormAddNot,
      idAcQueryRelationshipFormAddAnd,
      idAcQueryRelationshipFormAddOr
    };
};


class OksQuerySimpleExpressionForm
{
  public:

    OksQuerySimpleExpressionForm(Widget, const char *, OksClass *, Widget);

    Widget	getLabelFrame() const {return labelFrame;}
    OksClass*	getSimpleClass() const {return simpleClass;}
    Widget	tree_widget() const {return treeWidget;}


  private:

    Widget	labelFrame;
    OksClass	*simpleClass;
    Widget	treeWidget;

    static void	formCB(Widget, XtPointer, XtPointer); 
    static void	formAC(Widget, XtPointer, XEvent *, Boolean *);
    
    enum {
      idAcQueryNotAddAttribute = 100,
      idAcQueryNotAddRelationship,
      idAcQueryNotAddNot,
      idAcQueryNotAddAnd,
      idAcQueryNotAddOr,
      idAcQueryAndAddAttribute,
      idAcQueryAndAddRelationship,
      idAcQueryAndAddNot,
      idAcQueryAndAddAnd,
      idAcQueryAndAddOr,
      idAcQueryOrAddAttribute,
      idAcQueryOrAddRelationship,
      idAcQueryOrAddNot,
      idAcQueryOrAddAnd,
      idAcQueryOrAddOr
    };
};


class OksQueryAndExpressionForm : public OksAndExpression, public OksQuerySimpleExpressionForm
{
  public:

    OksQueryAndExpressionForm(Widget p, OksClass *c, Widget w) : OksQuerySimpleExpressionForm (p, "AND", c, w) {
      OksXm::set_user_data(p, (XtPointer)this);
    };
};


class OksQueryOrExpressionForm : public OksOrExpression, public OksQuerySimpleExpressionForm
{
  public:

    OksQueryOrExpressionForm(Widget p, OksClass *c, Widget w) : OksQuerySimpleExpressionForm (p, "OR", c, w) {
      OksXm::set_user_data(p, (XtPointer)this);
    };
};


class OksQueryNotExpressionForm : public OksNotExpression, public OksQuerySimpleExpressionForm
{
  public:

    OksQueryNotExpressionForm(Widget p, OksClass *c, Widget w) : OksNotExpression (0), OksQuerySimpleExpressionForm (p, "NOT", c, w) {
      OksXm::set_user_data(p, (XtPointer)this);
    };
};

#endif
