#include <oks/file.h>

#include "data_editor_main_dlg.h"
#include "data_editor_data_dlg.h"
#include "data_editor_merge_conflicts_dlg.h"

#include <stdlib.h>
#include <stdio.h>

#include <oks/xm_utils.h>
#include <oks/xm_context.h>

#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/List.h>

#include "update.xpm"

void OksDataEditorMergeConflictsDialog::refresh()
{
  XmListDeleteAllItems(files_w);

  for (const auto& x : p_files)
    OksXm::List::add_row(files_w, x.c_str());

  int list_height = p_files.size();
  if(list_height > 10) list_height = 10;
  else if(list_height < 2) list_height = 2;
  XtVaSetValues(files_w, XmNvisibleItemCount, list_height, NULL);
}


OksDataEditorMergeConflictsDialog::OksDataEditorMergeConflictsDialog(
  OksDataEditorMainDialog * parent,
  std::list<std::string> files) :
  OksDataEditorDialog     (parent),
  p_files                 (files),
  p_restart_check_updated (false),
  p_is_active             (true)
{
  Pixmap pixmap(OksXm::create_color_pixmap(get_form_widget(), update_xpm));

  setIcon(pixmap);

  setTitle("Fix merged conflict");

  Widget pxm_w = add(OksDlgEntity::Label, OksDlgEntity::idPixmap, (char *)"");
  XtVaSetValues(pxm_w, XmNlabelType, XmPIXMAP, XmNlabelPixmap, pixmap, NULL);

  Widget label_w = add_label(idLabel, "The following files were merged with conflicts:");
  attach_previous(label_w, pxm_w, pxm_w, 0, XmATTACH_OPPOSITE_WIDGET);

  files_w = add_list(idFiles, "Files:");
  attach_right(idFiles);

  Widget text_w = add_text(idText, "Fix the conflicts using a merge tool to reload the files.\nOtherwise your changes will be lost.\nRead git manual for more information.\nUse your favorite editor or try:");
  attach_right(idText);

  XtVaSetValues(get_previous(idText), XmNalignment, XmALIGNMENT_BEGINNING, NULL);

  std::string out;
  out.append("cd ");
  out.append(parent->get_user_repository_root());
  out.append("\n"
      "git config merge.tool vimdiff\n"
      "git config merge.conflictstyle diff3\n"
      "git config mergetool.prompt false\n"
      "git mergetool");

  XmTextSetString(text_w, (char *)out.c_str());
  XtVaSetValues(text_w, XmNrows, 5, NULL);
  OksXm::TextField::set_not_editable(text_w);

  refresh();

  add_separator();

  XtAddCallback(add_push_button(idApply, "Done, try again"), XmNactivateCallback, tryAgainCB, (XtPointer)this);
//  OksXm::set_close_cb(XtParent(get_form_widget()), runVimDiffCB, (void *)this);

  attach_bottom(idFiles);

  show(XtGrabExclusive);

  setMinSize();
  setCascadePos();

  if(parent->p_check_updated_timer) {
    parent->stop_check_updated();
    p_restart_check_updated = true;
  }

  parent->dialogs.push_back(this);

  while(p_is_active) {
    XtAppProcessEvent(parent->app_context(), XtIMAll);
  }

  delete this;
}


OksDataEditorMergeConflictsDialog::~OksDataEditorMergeConflictsDialog()
{
  parent()->remove_dlg(this);

  if(p_restart_check_updated) {
    parent()->start_check_updated();
  }
}


void
OksDataEditorMergeConflictsDialog::tryAgainCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorMergeConflictsDialog * dlg = reinterpret_cast<OksDataEditorMergeConflictsDialog *>(client_data);

  try
    {
      dlg->p_files = dlg->parent()->get_repository_unmerged_files();
    }
  catch(std::exception& ex)
    {
      std::cerr << "Failed to read information about unmerged files: " << ex.what() << std::endl;
      return;
    }

  if(!dlg->p_files.empty()) return;

  dlg->p_is_active = false;
  XtRemoveGrab(XtParent(w));
}
