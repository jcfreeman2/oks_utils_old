#ifndef __SCHEMA_EDITOR_RELATIONSHIP_DLG_H
#define __SCHEMA_EDITOR_RELATIONSHIP_DLG_H 

#include "schema_editor_dialog.h"

class OksRelationship;
class OksClass;

class OksSchemaEditorRelationshipDialog : public OksSchemaEditorDialog
{

  public:

    OksSchemaEditorRelationshipDialog (OksRelationship*, OksClass*, OksSchemaEditorMainDialog *);
    ~OksSchemaEditorRelationshipDialog ();

    Type type() const {return OksSchemaEditorDialog::RelationshipDlg;}

    OksRelationship * get_relationship() const {return p_rel;}

    void update();
    void change_schema_file();
	
    void refresh();


  private:

    OksRelationship * p_rel;

    static void setCB(Widget, XtPointer, XtPointer);
    static void chooseCB(Widget, XtPointer, XtPointer);

    static void iconCB(Widget, XtPointer, XtPointer);
    static void iconAC(Widget, XtPointer, XEvent *, Boolean *);

};

#endif
