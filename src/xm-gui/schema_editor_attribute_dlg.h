#ifndef __SCHEMA_EDITOR_ATTRIBUTE_DLG_H
#define __SCHEMA_EDITOR_ATTRIBUTE_DLG_H 

#include "schema_editor_dialog.h"

class OksAttribute;
class OksClass;

class OksSchemaEditorAttributeDialog : public OksSchemaEditorDialog
{

  public:

    OksSchemaEditorAttributeDialog	(OksAttribute*, OksClass*, OksSchemaEditorMainDialog *);
    ~OksSchemaEditorAttributeDialog	();

    Type	 			type() const {return OksSchemaEditorDialog::AttributeDlg;}

    OksAttribute *			get_attribute() const {return a;}

    void				update();
    void				change_schema_file();


  private:

    OksAttribute *			a;

    static void				typeCB(Widget, XtPointer, XtPointer);

    static void				iconCB(Widget, XtPointer, XtPointer);
    static void				iconAC(Widget, XtPointer, XEvent *, Boolean *);

};

#endif
