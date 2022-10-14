#include <oks/file.h>

#include "data_editor_main_dlg.h"
#include "data_editor_data_dlg.h"
#include "data_editor_files_dlg.h"

#include <oks/xm_utils.h>
#include <oks/xm_context.h>

#include <stdlib.h>
#include <stdio.h>

#include <Xm/Text.h>
#include <Xm/TextF.h>

#include "ok.xpm"

OksDataEditorFilesDialog::OksDataEditorFilesDialog(
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
) :
  OksDataEditorDialog     (parent),
  p_ptr_files             (ptr_files),
  p_str_files             (str_files),
  p_action                (action_btn),
  p_restart_check_updated (false),
  p_is_active             (true),
  p_ptr_cb                (ptr_cb),
  p_str_cb                (str_cb),
  text_w                  (0),
  author_w                (0)
{
  Pixmap pixmap(OksXm::create_color_pixmap(get_form_widget(), picture));

  setIcon(pixmap);

  setTitle(title);

  Widget pxm_w = add(OksDlgEntity::Label, OksDlgEntity::idPixmap, (char *)"");
  XtVaSetValues(pxm_w, XmNlabelType, XmPIXMAP, XmNlabelPixmap, pixmap, NULL);

  Widget label_w = add_label(idLabel, label);
  attach_previous(label_w, pxm_w, pxm_w, 0, XmATTACH_OPPOSITE_WIDGET);

  Widget files_w = add_list(idFiles, "Files:");
  attach_right(idFiles);

  for (const auto& x : ptr_files)
    OksXm::List::add_row(files_w, x->get_full_file_name().c_str());

  for (const auto& x : str_files)
    OksXm::List::add_row(files_w, x.c_str());

  int list_height = ptr_files.size() + str_files.size();
  if(list_height > 10) list_height = 10;
  else if(list_height < 2) list_height = 2;
  XtVaSetValues(files_w, XmNvisibleItemCount, list_height, NULL);

  if(input) {
    if(show_author) {
      author_w = add_text_field(idAuthor, "Author names");
      attach_right(idAuthor);
      set_value(idAuthor, OksGC::Parameters::def_params().get_string_parameter(OksParameterNames::get_author_name())->c_str());
    }

    text_w = add_text(idText, input);
    attach_right(idText);

    if(text) XmTextSetString(text_w, const_cast<char *>(text));
  }

  add_separator();

  XtAddCallback(add_push_button(idApply, p_action), XmNactivateCallback, applyCB, (XtPointer)this);
  XtAddCallback(add_push_button(idClose, "Cancel"), XmNactivateCallback, cancelCB, (XtPointer)this);
  attach_previous(idClose);
  OksXm::set_close_cb(XtParent(get_form_widget()), cancelCB, (void *)this);

  attach_bottom(input ? idText : idFiles);

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


OksDataEditorFilesDialog::~OksDataEditorFilesDialog()
{
  parent()->remove_dlg(this);

  if(p_restart_check_updated) {
    parent()->start_check_updated();
  }
}

void
OksDataEditorFilesDialog::report_ok_action(Widget w, const char * action)
{
  std::string msg("Operation \"");
  msg.append(action);
  msg.append("\" has completed successfully.");

  XtManageChild( OksXm::create_dlg_with_pixmap( w, "Info", ok_xpm, msg.c_str(), "Ok", 0, 0, 0, 0) );
}

void
OksDataEditorFilesDialog::applyCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorFilesDialog * dlg = reinterpret_cast<OksDataEditorFilesDialog *>(client_data);

  OksXm::AutoCstring text(dlg->text_w ? XmTextGetString(dlg->text_w) : 0);
  OksXm::AutoCstring author(dlg->author_w ? XmTextGetString(dlg->author_w) : 0);

  if(author.get() && *OksGC::Parameters::def_params().get_string_parameter(OksParameterNames::get_author_name()) != author.get()) {
    OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_author_name(), author.get());
    try {
      OksGC::Parameters::def_params().save();
    }
    catch(oks::FailedSaveGuiRcFile& ex) {
      OksDataEditorMainDialog::report_error("Save OKS Data Editor Resource File", ex.what());
    }
  }

  bool status = dlg->p_ptr_files.empty()
      ? (*dlg->p_str_cb)(dlg->p_str_files, dlg->parent(), author.get(), text.get())
      : (*dlg->p_ptr_cb)(dlg->p_ptr_files, dlg->parent(), author.get(), text.get());

  dlg->p_is_active = false;
  XtRemoveGrab(XtParent(w));

  if(status) {
    report_ok_action(dlg->parent()->get_form_widget(), dlg->p_action);
  }
}

void
OksDataEditorFilesDialog::cancelCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorFilesDialog * dlg = reinterpret_cast<OksDataEditorFilesDialog *>(client_data);
  dlg->p_is_active = false;
  XtRemoveGrab(XtParent(w));
}
