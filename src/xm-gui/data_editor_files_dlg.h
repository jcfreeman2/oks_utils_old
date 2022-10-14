#ifndef __DATA_EDITOR_FILES_DLG_H
#define  __DATA_EDITOR_FILES_DLG_H

#include <list>

#include <oks/file.h>
#include <oks/xm_dialog.h>

#include "data_editor_dialog.h"


class OksDataEditorFilesDialog : public OksDataEditorDialog
{
  friend class OksDataEditorMainDialog;

  public:

    typedef bool (*ApplyPtrCB)(const std::list<OksFile *>& files, OksDataEditorMainDialog * parent, const char * author, const char * text);
    typedef bool (*ApplyStrCB)(const std::set<std::string>& files, OksDataEditorMainDialog * parent, const char * author, const char * text);

    OksDataEditorFilesDialog(
      OksDataEditorMainDialog * parent,
      const std::list<OksFile *>& files,
      const char * title,
      const char * label,
      const char ** picture,
      const char * input,
      const char * text,
      bool show_author,
      const char * action_btn,
      ApplyPtrCB cb
    ) : OksDataEditorFilesDialog(parent, files, std::set<std::string>(), title, label, picture, input, text, show_author, action_btn, cb, nullptr) {}

    OksDataEditorFilesDialog(
      OksDataEditorMainDialog * parent,
      const std::set<std::string>& files,
      const char * title,
      const char * label,
      const char ** picture,
      const char * input,
      const char * text,
      bool show_author,
      const char * action_btn,
      ApplyStrCB cb
    ) : OksDataEditorFilesDialog(parent, std::list<OksFile *>(), files, title, label, picture, input, text, show_author, action_btn, nullptr, cb) {}

    ~OksDataEditorFilesDialog();

    OksDataEditorDialog::Type type() const {return OksDataEditorDialog::FilesDlg;}

    static void report_ok_action(Widget w, const char * action);

  private:

    std::list<OksFile *> p_ptr_files;
    std::set<std::string> p_str_files;
    const char * p_action;
    bool p_restart_check_updated;
    bool p_is_active;
    ApplyPtrCB p_ptr_cb;
    ApplyStrCB p_str_cb;
    Widget text_w;
    Widget author_w;

    static void applyCB(Widget, XtPointer, XtPointer);
    static void cancelCB(Widget, XtPointer, XtPointer);

    OksDataEditorFilesDialog(
        OksDataEditorMainDialog * parent,
        const std::list<OksFile *>& ptr_files,
        const std::set<std::string>& str_files,
        const char * title,
        const char * label,
        const char ** picture,
        const char * input,
        const char * text,
        bool show_author,
        const char * action_btn,
        ApplyPtrCB ptr_cb,
        ApplyStrCB str_cb
      );


    enum {
      idLabel = 100,
      idAuthor,
      idFiles,
      idText,
      idApply,
      idClose
    };

};

#endif
