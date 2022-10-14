/**	
 *	\file src/xm-gui/schema_editor_dialog.h
 *
 *	This file is part of the OKS package
 *      and it is used by the OKS Schema Editor.
 *
 *	Author: <Igor.Soloviev@cern.ch>
 *
 *	This file contains common class used by the Attribute,
 *      relationship and method dialogs.
 */

#ifndef __SCHEMA_EDITOR_DLG_H
#define __SCHEMA_EDITOR_DLG_H

#include <oks/xm_dialog.h>

class OksSchemaEditorMainDialog;
class OksClass;

class OksSchemaEditorDialog : public OksDialog
{

  public:

    enum Type {
      ViewDlg,
      AttributeDlg,
      RelationshipDlg,
      MethodDlg,
      MethodImplementationDlg,
      ClassDlg,
      SchemaDlg
    };

    OksSchemaEditorDialog (OksSchemaEditorMainDialog *, OksClass * = 0);
    virtual ~OksSchemaEditorDialog () {;}

    OksSchemaEditorMainDialog * parent() const {return p_parent;}
    OksClass * get_class() const {return p_class;}

    virtual Type type() const = 0;
    virtual void update() = 0;
    virtual void change_schema_file() = 0;
    virtual bool is_read_only() const;


  protected:

    static void closeCB(Widget, XtPointer, XtPointer);


      /**
       *  To be used by window's popup menu.
       */

    static void make_popup(Widget, XtPointer, XEvent *, bool, XtCallbackProc);

    enum {
      idDelete = 1,
      idRename,
      idUpdate
    };


  private:

    OksSchemaEditorMainDialog * p_parent;
    OksClass * p_class;

};

#endif
