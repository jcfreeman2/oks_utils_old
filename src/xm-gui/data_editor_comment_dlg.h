#ifndef __DATA_EDITOR_COMMENT_DLG_H
#define  __DATA_EDITOR_COMMENT_DLG_H

#include <oks/file.h>
#include <oks/xm_dialog.h>

#include "data_editor_dialog.h"

namespace oks {
  class Comment;
}

class OksDataEditorCommentDialog : public OksDataEditorDialog
{
  friend class OksDataEditorMainDialog;

  public:

    OksDataEditorCommentDialog(OksDataEditorMainDialog *, OksFile *, const std::string&, oks::Comment * = 0);
    ~OksDataEditorCommentDialog();

    OksDataEditorDialog::Type	type() const {return OksDataEditorDialog::CommentDlg;}

  private:

    OksFile * p_file;
    oks::Comment * p_comment;
    std::string p_creation_time;
    bool p_is_modal;
    bool p_restart_check_updated;

    void set_author(const char *);

    static void applyCB(Widget, XtPointer, XtPointer);
    static void createCB(Widget, XtPointer, XtPointer);
    static void cancelCB(Widget, XtPointer, XtPointer);
    static void exitCB(Widget, XtPointer, XtPointer);

    enum {
      idCreation = 100,
      idAuthor,
      idText,
      idApply,
      idDelete,
      idClose
    };

};

#endif
