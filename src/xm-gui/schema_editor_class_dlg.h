#ifndef __SCHEMA_EDITOR_CLASS_DLG_H
#define __SCHEMA_EDITOR_CLASS_DLG_H 

#include "schema_editor_dialog.h"

class OksClass;

class OksSchemaEditorClassDialog : public OksSchemaEditorDialog
{
  public:

    OksSchemaEditorClassDialog	(OksClass *, OksSchemaEditorMainDialog *);
    ~OksSchemaEditorClassDialog	();

    Type	 		type() const {return OksSchemaEditorDialog::ClassDlg;}

    void			show_superclasses();
    void			show_subclasses();
    void			show_attributes();
    void			show_relationships();
    void			show_methods();

    void			update();
    void			refresh();
    void			change_schema_file();

    void			add_paned_list(short, const char *, XtCallbackProc, XtEventHandler);
    void			select_super_class(const std::string&);


  private:

    bool			showAllSuperClasses;
    bool			showAllAttributes;
    bool			showAllRelationships;
    bool			showAllMethods;

    bool			is_read_only() const;

    void			create_popup(Widget, XEvent *, bool, const char *, int, int, int, int, int, int, int);

    static void			actionCB(Widget, XtPointer, XtPointer);
    static void			classesCB(Widget, XtPointer, XtPointer);
    static void			attributesCB(Widget, XtPointer, XtPointer);
    static void			relationshipsCB(Widget, XtPointer, XtPointer);
    static void			methodsCB(Widget, XtPointer, XtPointer);
    static void			addSuperclassCB(Widget, XtPointer, XtPointer);

    static void			dlgAC(Widget, XtPointer, XEvent *, Boolean *);
    static void			superclassesAC(Widget, XtPointer, XEvent *, Boolean *);
    static void			attributesAC(Widget, XtPointer, XEvent *, Boolean *);
    static void			relationshipsAC(Widget, XtPointer, XEvent *, Boolean *);
    static void			methodsAC(Widget, XtPointer, XEvent *, Boolean *);

    enum {
      idAcClassDelete = 109,
      idAcClassProperties,
      idAcClassChangeSchemaFile,
      idAcClassSuperClassShowDirect,
      idAcClassSuperClassShowAll,
      idAcClassSuperClassAdd,
      idAcClassSuperClassShow,
      idAcClassSuperClassRemove,
      idAcClassSuperClassMoveUp,
      idAcClassSuperClassMoveDown,
      idAcClassAttributeShowDirect,
      idAcClassAttributeShowAll,
      idAcClassAttributeAdd,
      idAcClassAttributeShow,
      idAcClassAttributeRemove,
      idAcClassAttributeMoveUp,
      idAcClassAttributeMoveDown,
      idAcClassRelationshipShowDirect,
      idAcClassRelationshipShowAll,
      idAcClassRelationshipAdd,
      idAcClassRelationshipShow,
      idAcClassRelationshipRemove,
      idAcClassRelationshipMoveUp,
      idAcClassRelationshipMoveDown,
      idAcClassMethodShowDirect,
      idAcClassMethodShowAll,
      idAcClassMethodAdd,
      idAcClassMethodShow,
      idAcClassMethodRemove,
      idAcClassMethodMoveUp,
      idAcClassMethodMoveDown
    };
};


#endif
