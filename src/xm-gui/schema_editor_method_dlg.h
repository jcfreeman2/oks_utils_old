#ifndef __SCHEMA_EDITOR_METHOD_DLG_H
#define __SCHEMA_EDITOR_METHOD_DLG_H 

#include "schema_editor_dialog.h"

class OksMethod;
class OksMethodImplementation;

class OksSchemaEditorMethodImplementationDialog : public OksSchemaEditorDialog
{

  public:

    OksSchemaEditorMethodImplementationDialog (OksMethodImplementation*, OksMethod*, OksClass*, OksSchemaEditorMainDialog *);
    ~OksSchemaEditorMethodImplementationDialog();

    Type type() const {return OksSchemaEditorDialog::MethodImplementationDlg;}

    OksMethodImplementation * get_method_implementation() const {return p_implementation;}

    void update();
    void change_schema_file();


  private:

    OksMethodImplementation * p_implementation;
    OksMethod * p_parent_method;

    static void applyCB(Widget, XtPointer, XtPointer);

    enum {
      idDefinedInMethod = 100,
      idApply,
      idClose,
      idLanguage,
      idPrototype,
      idBody
    };

};

class OksSchemaEditorMethodDialog : public OksSchemaEditorDialog
{

  public:

    OksSchemaEditorMethodDialog (OksMethod*, OksClass*, OksSchemaEditorMainDialog *);
    ~OksSchemaEditorMethodDialog ();

    Type type() const {return OksSchemaEditorDialog::MethodDlg;}

    OksMethod * get_method() const {return m;}
	
    void update();
    void change_schema_file();
    void show_implementations();


  private:

    OksMethod * m;

    static void parse_action(char *, char **, int *);

    static void showCB(Widget, XtPointer, XtPointer);
    static void implementationCB(Widget, XtPointer, XtPointer);
    static void implementationAC(Widget, XtPointer, XEvent *, Boolean *);

    static void iconCB(Widget, XtPointer, XtPointer);
    static void iconAC(Widget, XtPointer, XEvent *, Boolean *);

    enum {
      idAcMethodAddImplementation = 100,
      idAcMethodModifyImplementation,
      idAcMethodRemoveImplementation,
      
      idImplementations,
      idDescription
    };

};

#endif
