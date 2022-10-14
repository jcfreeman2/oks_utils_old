#ifndef __SCHEMA_EDITOR_SCHEMA_DLG_H
#define __SCHEMA_EDITOR_SCHEMA_DLG_H 

#include <string>

#include "schema_editor_dialog.h"

class OksClass;
class OksFile;

class OksSchemaEditorSchemaDialog : public OksSchemaEditorDialog
{
  public:

    OksSchemaEditorSchemaDialog		(OksFile *, OksSchemaEditorMainDialog *);
    ~OksSchemaEditorSchemaDialog	();

    Type	 			type() const {return OksSchemaEditorDialog::SchemaDlg;}

    OksFile *				get_file() const {return file_h;}

    void		 		change_schema_file();
    void				remove_class(const OksClass *);
    void				refresh();
    void				update() {;}
    bool				is_read_only() const;


  private:

    OksFile *                           file_h;

    static void				default_clistCB(Widget, XtPointer, XtPointer);
    static void				clistCB(Widget, XtPointer, XtPointer);
    static void				clistAC(Widget, XtPointer, XEvent *, Boolean *);

    static void				default_ilistCB(Widget, XtPointer, XtPointer);
    static void				ilistCB(Widget, XtPointer, XtPointer);
    static void				ilistAC(Widget, XtPointer, XEvent *, Boolean *);

    static void				applyCB(Widget, XtPointer, XtPointer);

    enum {
      idAcSchemaClassNew = 110,
      idAcSchemaClassShow,
      idAcSchemaClassDelete,
      idAcAddFile,
      idAcRemoveFile,
      idAcRenameFile,
      idAcShowFile,
      idPanedWindow,
      idLogicalName,
      idType,
      idCreated,
      idLastModified,
      idIncludes,
      idOK,
      idApply,
      idCancel
    };
};

#endif
