#ifndef __DATA_EDITOR_DATA_DLG_H
#define __DATA_EDITOR_DATA_DLG_H 

#include <string>

#include "data_editor_dialog.h"

class OksObject;
class OksDataEditorSearchPanel;

namespace oks {
  class Comment;
}

class OksDataEditorDataDialog : public OksDataEditorDialog
{
  friend class OksDataEditorMainDialog;
  friend class OksDataEditorObjectDialog;


  public:

    OksDataEditorDataDialog	(OksFile *, OksDataEditorMainDialog *);

    ~OksDataEditorDataDialog	();
    OksDataEditorDialog::Type	type() const {return OksDataEditorDialog::DataDlg;}

    void			change_file_name();

    void			remove_object(const OksObject *);
    void			add_object(const OksObject *);

    OksFile *			get_file() const {return file_h;}

    static std::string          make_name(const OksObject *);

    void			refresh();
    
    void                        refresh_objects();

    void                        append_comment_row(const std::string&, const oks::Comment&);
    void                        fill_comment_row(const std::string& id, const oks::Comment& c, String *row) const;

  private:

    OksFile *                   file_h;
    Widget                      p_comments_matrix;
    Widget                      p_objects_matrix;
    Widget                      p_includes_list;

    OksDataEditorSearchPanel *  p_select_objects_panel;  // criteria to find objects
    OksDataEditorSearchPanel *  p_select_classes_panel;  // criteria to find classes

    void			strip_file(std::string& name);


      // objects callbacks and actions

    static void                 label_objectsCB(Widget, XtPointer, XtPointer);
    static void                 default_objectsCB(Widget, XtPointer, XtPointer);
    static void			olistCB(Widget, XtPointer, XtPointer);
    static void                 objectsAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                 findClassesCB(Widget, XtPointer, XtPointer);
    static void                 findObjectsCB(Widget, XtPointer, XtPointer);
    bool                        test_match(const OksObject * obj) const;

      // includes callbacks and actions

    static void			default_ilistCB(Widget, XtPointer, XtPointer);
    static void			ilistCB(Widget, XtPointer, XtPointer);
    static void			ilistAC(Widget, XtPointer, XEvent *, Boolean *);


      // comments callbacks and actions

    static void                 label_commentsCB(Widget w, XtPointer, XtPointer);
    static void                 default_commentsCB(Widget, XtPointer, XtPointer);
    static void                 commentsAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                 commentsCB(Widget, XtPointer, XtPointer);
    static void                 newCommentCB(Widget, XtPointer, XtPointer);


      // dialog callbacks

    static void			applyCB(Widget, XtPointer, XtPointer);
    static void			closeCB(Widget, XtPointer, XtPointer);

    enum {
      idAcObjectShow = 111,
      idAcObjectDelete,
      idAcObjectSelect,
      idAcAddFile,
      idAcRemoveFile,
      idAcRenameFile,
      idAcShowFile,
      // idAcDataNewComment,
      idAcDataShowComment,
      idAcDataDeleteComment,
      idPanedWindow,
      idFileName,
      idLogicalName,
      idType,
      idCreated,
      idLastModified,
      idIncludes,
      idObjects,
      idComments,
      idNewComment,
      idOK,
      idApply,
      idCancel
    };
 };

#endif
