#include <sstream>
#include <vector>

#include <boost/date_time/posix_time/time_formatters.hpp>

#include <string.h>

#include <oks/class.h>
#include <oks/object.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Matrix.h>
#include <Xm/TextF.h>

#include "data_editor_comment_dlg.h"
#include "data_editor_data_dlg.h"
#include "data_editor_main_dlg.h"
#include "data_editor_search_panel.h"
#include "data_editor_exceptions.h"
#include "data_editor.h"

#include "exit.xpm"

const int NumOfObjectsMatrixColumns = 2;
const int NumOfCommentsMatrixColumns = 3;

extern OksObject *clipboardObject;

static void
fill_search_dirs(OksKernel& k, std::list<std::string>& out)
{
  out = k.get_repository_dirs();

  if(!k.get_user_repository_root().empty()) {
    out.push_front(k.OksKernel::get_user_repository_root());
  }
}

static std::string
get_file_path(const OksKernel& kernel, const char * value, OksFile * file_h)
{
  try {
    return kernel.get_file_path(value, file_h);
  }
  catch(std::exception& ex) {
    static std::string empty;
    return empty;
  }
}

void
OksDataEditorDataDialog::default_ilistCB(Widget, XtPointer client_data, XtPointer call_data)
{
  XmListCallbackStruct * cbs = (XmListCallbackStruct *)call_data;
  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)client_data;
  OksXm::AutoCstring iname(OksXm::string_to_c_str(cbs->item));

  std::string s = get_file_path(*dlg->parent(), iname.get(), dlg->file_h);

  OksFile * f = (!s.empty() ? dlg->parent()->find_data_file(s) : 0);

  if(f) {
    dlg->parent()->create_data_dlg(f);
  }
  else {
    std::ostringstream text;
    text << "the file \'" << iname.get() << "\' is not loaded or it is a schema file";
    OksDataEditorMainDialog::report_error("Open Data Dialog", text.str().c_str(), dlg->get_form_widget());
  }
}


void
OksDataEditorDataDialog::olistCB(Widget w, XtPointer d, XtPointer) 
{
  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)d;
  OksObject * o = (OksObject *)OksXm::Matrix::get_selected_row_user_data(dlg->p_objects_matrix);

  long user_data = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  if(!o) return;

  switch(user_data) {
    case idAcObjectShow:
      dlg->parent()->create_object_dlg(o);
      break;

    case idAcObjectSelect:
      clipboardObject = o;
      break;

    case idAcObjectDelete:
      try {
        OksObject::destroy(o);
      }
      catch(oks::exception& ex) {
        OksDataEditorMainDialog::report_exception("Delete Object", ex, dlg->get_form_widget());
      }

      break;
  }
}

static XtPointer done_with_strip_dlg;

static void
stripCB(Widget, XtPointer d, XtPointer)
{
  done_with_strip_dlg = d;
}

void
OksDataEditorDataDialog::strip_file(std::string& name)
{
  std::string s(name);
  std::list<std::string> dirs;
  fill_search_dirs(*parent(), dirs);

  for(std::list<std::string>::const_iterator i = dirs.begin(); i != dirs.end(); ++i) {
    std::string dir(*i);
    dir += '/';
    std::string::size_type len = dir.size();
    if(s.size() > len && s.compare(0, len, dir) == 0) {
      s.erase(0, len);
      
      std::string text("Strip off database repository root directory from file path:\n\"");
      text += name;
      text += "\"\nThe result after strip is:\n\"";
      text += s;
      text += '\"';

      done_with_strip_dlg = 0;

        // create dialog and get answer
      {
        Widget w = OksXm::create_dlg_with_pixmap(get_form_widget(), "Strip file name", exit_xpm, text.c_str(), "Strip", "Skip", 0, 0, 0);
        XtVaSetValues(w, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);
        XtAddCallback(w, XmNokCallback, stripCB, reinterpret_cast<XtPointer>(1));
        XtAddCallback(w, XmNcancelCallback, stripCB, reinterpret_cast<XtPointer>(2));
        OksXm::set_close_cb(XtParent(w), stripCB, reinterpret_cast<XtPointer>(2));
      
        XtManageChild(w);

        while(done_with_strip_dlg == 0) {
          XtAppProcessEvent(XtWidgetToApplicationContext(w), XtIMAll);
        }
      
        XtDestroyWidget(XtParent(w));
      }

      if(done_with_strip_dlg == reinterpret_cast<XtPointer>(1)) {      
        name = s;
      }

      break;
    }
  }
}


void
OksDataEditorDataDialog::ilistCB(Widget w, XtPointer d, XtPointer) 
{
  long user_data = (long)OksXm::get_user_data(w);

  char * name = 0; // name-of-selected-file or name-of-button (in case of add file)

  if(user_data != idAcAddFile) {
    name = OksXm::List::get_selected_value(XtParent(XtParent(XtParent(w))));
  }
  else {
    XmString xms;
    XtVaGetValues(w, XmNlabelString, &xms, NULL);
    name = OksXm::string_to_c_str(xms);
  }

  OksPopupMenu::destroy(w);

  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)d;

  switch (user_data) {
    case idAcShowFile:
      if(name) {
        std::string s = get_file_path(*dlg->parent(), name, dlg->file_h);

	if(!s.empty()) {
	  OksFile * f = dlg->parent()->find_data_file(s);

	  if(f) {
	    dlg->parent()->create_data_dlg(f);
	  }
	}
      }
      break;

    case idAcAddFile: {
      std::string s = OksXm::ask_file(
        dlg->get_form_widget(),
	"Input name of schema or data file to be included",
	"Add include file",
	(strcmp(name, "working directory") ? name : 0),
	"*.xml"
      );

      if(s.length()) {
        std::string s2(s);
        dlg->strip_file(s);
        try {
          dlg->file_h->add_include_file(s);
	  dlg->refresh();
          dlg->parent()->refresh_schemes();
          dlg->parent()->refresh_data();
          dlg->parent()->refresh_classes();
          dlg->parent()->update_data_file_row(dlg->get_file());
        }
        catch (oks::exception & ex) {
          OksDataEditorMainDialog::report_exception("Add Include", ex, dlg->get_form_widget());
        }
      }
      break; }

    case idAcRemoveFile:
    case idAcRenameFile:
      if(user_data == idAcRenameFile) {
        const char * dir_s = 0;
	std::string dir_name = get_file_path(*dlg->parent(), name, dlg->file_h);
	std::string::size_type pos = dir_name.find_last_of('/');

        if(pos != std::string::npos) {
          dir_name.erase(pos);
	  dir_s = dir_name.c_str();
        }

        std::string s = OksXm::ask_file(
          dlg->get_form_widget(),
	  "Input new name of file",
	  "Rename include file",
	  dir_s, "*.xml"
        );

        if(s.length()) {
          try {
            dlg->strip_file(s);
            dlg->file_h->rename_include_file(name, s);
            dlg->refresh();
            dlg->parent()->refresh_schemes();
            dlg->parent()->refresh_data();
            dlg->parent()->refresh_classes();
            dlg->parent()->update_data_file_row(dlg->get_file());
          }
          catch (oks::exception & ex) {
            OksDataEditorMainDialog::report_exception("Rename Include File", ex, dlg->get_form_widget());
          }
        }
      }
      else {
        try {
          dlg->file_h->remove_include_file(name);
          dlg->refresh();
          dlg->parent()->refresh_schemes();
          dlg->parent()->refresh_data();
          dlg->parent()->refresh_classes();
          dlg->parent()->update_data_file_row(dlg->get_file());
        }
        catch (oks::exception & ex) {
          OksDataEditorMainDialog::report_exception("Remove Include File", ex, dlg->get_form_widget());
        }
      }

      break;
  }

  if(name) XtFree(name);
}


void
OksDataEditorDataDialog::ilistAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);
  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)client_data;
  bool is_file_read_only = OksKernel::check_read_only(dlg->file_h);
  OksXm::AutoCstring value(OksXm::List::get_selected_value(w));

  {
    std::string s;
    if(value.get()) s = get_file_path(*dlg->parent(), value.get(), dlg->file_h);

    if(!s.empty() && dlg->parent()->find_data_file(s))
      popup.addItem("Show", idAcShowFile, ilistCB, client_data);
    else
      popup.addDisableItem("Show");
  }

  popup.add_separator();

  if(is_file_read_only) {
    popup.addDisableItem("Add");
  }
  else {
    Widget cascade = popup.addCascade("Add from");

    std::list<std::string> dirs;
    fill_search_dirs(*dlg->parent(), dirs);

    for(std::list<std::string>::const_iterator i = dirs.begin(); i != dirs.end(); ++i) {
      popup.addItem((*i).c_str(), idAcAddFile, ilistCB, client_data, cascade);
    }

    popup.addItem("working directory", idAcAddFile, ilistCB, client_data, cascade);
  }

  if(value.get())
    popup.addItem("Remove", idAcRemoveFile, ilistCB, client_data);
  else
    popup.addDisableItem("Remove");

  if(value.get() && (is_file_read_only == false))
    popup.addItem("Rename", idAcRenameFile, ilistCB, client_data);
  else
    popup.addDisableItem("Rename");

  popup.show((XButtonPressedEvent *)event);
}


void
OksDataEditorDataDialog::applyCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)client_data;

  OksXm::AutoCstring new_logical_name(XmTextFieldGetString(dlg->get_widget(idLogicalName)));
  OksXm::AutoCstring new_type(XmTextFieldGetString(dlg->get_widget(idType)));

  try {
    dlg->file_h->set_logical_name(new_logical_name.get());
    dlg->file_h->set_type(new_type.get());
  }
  catch(oks::exception& ex) {
    OksDataEditorMainDialog::report_exception("Update File Attributes", ex, dlg->get_form_widget());
  }

  dlg->refresh();
  dlg->parent()->update_data_file_row(dlg->file_h);
}

void
OksDataEditorDataDialog::closeCB(Widget, XtPointer client_data, XtPointer)
{
  delete (OksDataEditorDataDialog *)client_data;
}

void
OksDataEditorDataDialog::findClassesCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorDataDialog * dlg(reinterpret_cast<OksDataEditorDataDialog *>(client_data));
  dlg->p_select_classes_panel->refresh();
  dlg->refresh_objects();
}

void
OksDataEditorDataDialog::findObjectsCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorDataDialog * dlg(reinterpret_cast<OksDataEditorDataDialog *>(client_data));
  dlg->p_select_objects_panel->refresh();
  dlg->refresh_objects();
}

OksDataEditorDataDialog::OksDataEditorDataDialog(OksFile * f, OksDataEditorMainDialog *p) :
  OksDataEditorDialog    (p),
  file_h		 (f),
  p_select_objects_panel (0),
  p_select_classes_panel (0)
{
  setIcon((OksDialog *)p);

  add_label(idFileName, "full name");
  add_label(idCreated, "created");
  add_label(idLastModified, "last modified");

  add_separator();

  add_text_field(idLogicalName, "Logical Name");
  add_text_field(idType, "  Type");
  attach_previous(idType);

  add_paned_window(idPanedWindow);
  attach_right(idPanedWindow);

  {
    OksForm * f = new OksForm(get_widget(idPanedWindow));

    p_includes_list = f->add_list(idIncludes, "Include files");
    XtAddCallback(p_includes_list, XmNdefaultActionCallback, default_ilistCB, (XtPointer)this);
    XtAddEventHandler(p_includes_list, ButtonPressMask, False, ilistAC, (XtPointer)this);

    f->attach_right(idIncludes);
    f->attach_bottom(idIncludes);

    get_paned_window(idPanedWindow)->add_form(f, idIncludes);
  }

  {
    OksForm * f = new OksForm(get_widget(idPanedWindow));

    p_objects_matrix = f->add_matrix(idObjects, (void *)"Objects:");
    f->attach_right(idObjects);
    p_select_objects_panel = new OksDataEditorSearchPanel(*f, "Matching object UIDs:", "objects", findObjectsCB, (XtPointer)this);
    p_select_classes_panel = new OksDataEditorSearchPanel(*f, "Matching class names:", "classes", findClassesCB, (XtPointer)this, 1);
    f->attach_bottom(idObjects);

    get_paned_window(idPanedWindow)->add_form(f, idObjects);


      // set objects matrix specific parameters

    char const * labels[] = {"Class", "UID"};
    unsigned char label_alinments[NumOfObjectsMatrixColumns];
    short widths[NumOfObjectsMatrixColumns];

    for(int i=0; i<NumOfObjectsMatrixColumns; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }

    XtVaSetValues(
      p_objects_matrix,
      XmNcolumns, NumOfObjectsMatrixColumns,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      NULL
    );

    XtAddCallback(p_objects_matrix, XmNdefaultActionCallback, default_objectsCB, (XtPointer)this);
    XtAddCallback(p_objects_matrix, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
    XtAddCallback(p_objects_matrix, XmNlabelActivateCallback, label_objectsCB, 0);
    XtAddEventHandler(p_objects_matrix, ButtonPressMask, False, objectsAC, (XtPointer)this);
  }

  {
    OksForm * f = new OksForm(get_widget(idPanedWindow));
    if(!file_h->is_read_only()) {
      XtAddCallback(f->add_push_button(idNewComment, "Add Comment"), XmNactivateCallback, newCommentCB, (XtPointer)this);
    }
    p_comments_matrix = f->add_matrix(idComments, (void *)"Comments:");
    f->attach_right(idComments);
    f->attach_bottom(idComments);

    get_paned_window(idPanedWindow)->add_form(f, idComments);


      // set comments matrix specific parameters

    char const * labels[] = {"Creation Time (UTC)", "Author", "Text"};
    unsigned char label_alinments[NumOfCommentsMatrixColumns];
    short widths[NumOfCommentsMatrixColumns];

    for(int i=0; i<NumOfCommentsMatrixColumns; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }

    XtVaSetValues(
      p_comments_matrix,
      XmNcolumns, NumOfCommentsMatrixColumns,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      NULL
    );

    XtAddCallback(p_comments_matrix, XmNdefaultActionCallback, default_commentsCB, (XtPointer)this);
    XtAddCallback(p_comments_matrix, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
    XtAddCallback(p_comments_matrix, XmNlabelActivateCallback, label_commentsCB, 0);
    XtAddEventHandler(p_comments_matrix, ButtonPressMask, False, commentsAC, (XtPointer)this);
  }

  {
    Widget w = add_push_button(idOK, "OK");
    XtAddCallback(w, XmNactivateCallback, applyCB, (XtPointer)this);
    XtAddCallback(w, XmNactivateCallback, closeCB, (XtPointer)this);
  }

  XtAddCallback(add_push_button(idApply, "Apply"), XmNactivateCallback, applyCB, (XtPointer)this);
  attach_previous(idApply);

  XtAddCallback(add_push_button(idCancel, "Close"), XmNactivateCallback, closeCB, (XtPointer)this);
  attach_previous(idCancel);

  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_bottom(idPanedWindow);

  change_file_name();
  refresh();
  show();

  XtAddCallback(p_comments_matrix, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);
  XtAddCallback(p_objects_matrix, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);

  p_select_objects_panel->init();
  p_select_classes_panel->init();

  setMinSize();
  setCascadePos();
}


OksDataEditorDataDialog::~OksDataEditorDataDialog()
{
  parent()->remove_dlg(this);
}


void
OksDataEditorDataDialog::refresh_objects()
{
  const char *item = OksXm::Matrix::get_selected_value(p_objects_matrix);

  if(item) {
    XbaeMatrixDeselectAll(p_objects_matrix);	// it looks like it may crash without this
  }

  XbaeMatrixDeleteRows(p_objects_matrix, 0, XbaeMatrixNumRows(p_objects_matrix));
  XtVaSetValues(p_objects_matrix, XmNfill, false, NULL);

  if(std::list<OksObject *> * objects = parent()->create_list_of_data_objects(file_h)) {
    std::vector<OksObject *> vec;
    vec.reserve(objects->size());

    for(std::list<OksObject *>::const_iterator i = objects->begin(); i != objects->end(); ++i) {
      if(test_match(*i)) vec.push_back(*i);
    }

    if(vec.empty()) {
      XtVaSetValues(p_objects_matrix, XmNfill, true, NULL);
    }
    else {
      const int num_of_rows(vec.size());
      String ** cells = new String * [ num_of_rows ];
      int idx(0);

      for(std::vector<OksObject *>::const_iterator i = vec.begin(); i != vec.end(); ++i, ++idx) {
        cells[idx] = new String [2];
        cells[idx][0] = const_cast<String>((*i)->GetClass()->get_name().c_str());
        cells[idx][1] = const_cast<String>((*i)->GetId().c_str());
      }

      XtVaSetValues(p_objects_matrix, XmNrows, num_of_rows, NULL);
      XtVaSetValues(p_objects_matrix, XmNcells, cells, NULL);

      idx = 0;
      for(std::vector<OksObject *>::const_iterator i = vec.begin(); i != vec.end(); ++i) {
        XbaeMatrixSetRowUserData(p_objects_matrix, idx++, (XtPointer)(*i));
      }
    }

    delete objects;
  }
  else {
    XtVaSetValues(p_objects_matrix, XmNfill, true, NULL);
  }

  OksXm::Matrix::adjust_column_widths(p_objects_matrix);
  OksXm::Matrix::set_visible_height(p_objects_matrix);
  OksXm::Matrix::set_widths(p_objects_matrix, 2);
  OksXm::Matrix::set_cell_background(p_objects_matrix, p_includes_list);

  if(!OksXm::Matrix::resort_rows(p_objects_matrix)) {
    OksXm::Matrix::sort_rows(p_objects_matrix, 0, OksXm::Matrix::SortByString);
  }
}


void
OksDataEditorDataDialog::refresh()
{
    // put logical name, type and creation/modification info

  {
    size_t logical_name_width = file_h->get_logical_name().size();
    size_t type_width = file_h->get_type().size();
  
    if(logical_name_width < 5) logical_name_width = 5;
    if(type_width < 5) type_width = 5;

    OksXm::TextField::set_width(set_value(idLogicalName, file_h->get_logical_name().c_str()), logical_name_width);
    OksXm::TextField::set_width(set_value(idType, file_h->get_type().c_str()), type_width);

    set_value(idLogicalName, file_h->get_logical_name().c_str());
    set_value(idType, file_h->get_type().c_str());
  }

  {
    std::string created_at = boost::posix_time::to_simple_string(file_h->get_creation_time());
    std::string s("Created by \"");
    s += file_h->get_created_by();
    s += "\" on \"";
    s += file_h->get_created_on();
    s += "\" at \"";
    s += created_at;
    s += '\"';

    set_value(idCreated, s.c_str());
  }

  {
    std::string last_modified_at = boost::posix_time::to_simple_string(file_h->get_last_modification_time());
    std::string s("Last modified by \"");
    s += file_h->get_last_modified_by();
    s += "\" on \"";
    s += file_h->get_last_modified_on();
    s += "\" at \"";
    s += last_modified_at;
    s += '\"';

    set_value(idLastModified, s.c_str());
  }


    // refresh list of includes

  {
    OksForm * f = get_paned_window(idPanedWindow)->get_form(idIncludes);
    OksXm::AutoCstring selected(OksXm::List::get_selected_value(p_includes_list));

    XmListDeleteAllItems(p_includes_list);

    const std::list<std::string> & ilist = file_h->get_include_files();
    for(std::list<std::string>::const_iterator i = ilist.begin(); i != ilist.end(); ++i) {
      f->set_value(idIncludes, (*i).c_str(), 0, false);
    }

    if(selected.get()) {
      OksXm::List::select_row(p_includes_list, selected.get());
    }
  }


    // refresh list of objects

  refresh_objects();


    // refresh list of comments

  {
    const char *item = OksXm::Matrix::get_selected_value(p_comments_matrix);

    if(item) {
      XbaeMatrixDeselectAll(p_comments_matrix);	// it looks like it may crash without this
    }

    XbaeMatrixDeleteRows(p_comments_matrix, 0, XbaeMatrixNumRows(p_comments_matrix));
    XtVaSetValues(p_comments_matrix, XmNfill, false, NULL);

    size_t count = 0;

    if(!file_h->get_comments().empty()) {
      for(std::map<std::string, oks::Comment *>::const_iterator i = file_h->get_comments().begin(); i != file_h->get_comments().end(); ++i) {
        append_comment_row(i->first, *i->second);
        XbaeMatrixSetRowUserData(p_comments_matrix, count++, (XtPointer)(&(i->first)));
      }
    }
    else {
      XtVaSetValues(p_comments_matrix, XmNfill, true, NULL);
    }

    static Dimension max_width = (WidthOfScreen(XtScreen(p_comments_matrix)) / 4) * 3;

    OksXm::Matrix::adjust_column_widths(p_comments_matrix, max_width);
    OksXm::Matrix::set_visible_height(p_comments_matrix);
    OksXm::Matrix::set_widths(p_comments_matrix, 2);
    OksXm::Matrix::set_cell_background(p_comments_matrix, p_includes_list);

    if(!OksXm::Matrix::resort_rows(p_comments_matrix)) {
      OksXm::Matrix::sort_rows(p_comments_matrix, 0, OksXm::Matrix::SortByTime);
    }
  }
}

void
OksDataEditorDataDialog::append_comment_row(const std::string& id, const oks::Comment& c)
{
    // set XmNfill to false if it was true
    // (this happens when add first row)
  {
    Boolean fill;
    XtVaGetValues(p_comments_matrix, XmNfill, &fill, NULL);

    if(fill == true) {
      XtVaSetValues(p_comments_matrix, XmNfill, false, NULL);
    }
  }

  String row[NumOfCommentsMatrixColumns];
  fill_comment_row(id, c, row);
  int num_of_rows = XbaeMatrixNumRows(p_comments_matrix);
  XbaeMatrixAddRows(p_comments_matrix, num_of_rows, row, 0, 0, 1);
}

void
OksDataEditorDataDialog::fill_comment_row(const std::string& id, const oks::Comment& c, String *row) const
{
  static std::string time_str; // is static since it's c-string is returned
  static std::string text_str; // is static since it's c-string is returned

  time_str = OksDataEditorMainDialog::iso2simple(id);

    // replace new-line symbols by spaces
  text_str = c.get_text();
  for(std::string::size_type idx = 0; idx < text_str.size(); ++idx) {
    if(text_str[idx] == '\n') text_str[idx] = ' ';
  }

  row[0] = const_cast<String>(time_str.c_str());
  row[1] = const_cast<String>(c.get_author().c_str());
  row[2] = const_cast<String>(text_str.c_str());
}

bool
OksDataEditorDataDialog::test_match(const OksObject * obj) const
{
  return(
    p_select_objects_panel->test_match(obj->GetId()) &&
    p_select_classes_panel->test_match(obj->GetClass()->get_name())
  );
}

void
OksDataEditorDataDialog::add_object(const OksObject * obj)
{
  if(!test_match(obj)) return;

    // set XmNfill to false if it was true (this happens when add first row)
  {
    Boolean fill;
    XtVaGetValues(p_objects_matrix, XmNfill, &fill, NULL);

    if(fill == true) {
      XtVaSetValues(p_objects_matrix, XmNfill, false, NULL);
    }
  }

  String row[NumOfObjectsMatrixColumns];
  row[0] = const_cast<String>(obj->GetClass()->get_name().c_str());
  row[1] = const_cast<String>(obj->GetId().c_str());
  int num_of_rows = XbaeMatrixNumRows(p_objects_matrix);
  XbaeMatrixAddRows(p_objects_matrix, num_of_rows, row, 0, 0, 1);
  XbaeMatrixSetRowUserData(p_objects_matrix, num_of_rows, (XtPointer)obj);
  OksXm::Matrix::set_cell_background(p_objects_matrix, p_includes_list);
}

void
OksDataEditorDataDialog::remove_object(const OksObject * o)
{
  OksXm::Matrix::delete_row_by_user_data(p_objects_matrix, (XtPointer)o);
}

void
OksDataEditorDataDialog::change_file_name()
{
    // set title

  {
    std::string s(file_h->get_full_file_name());
    if(file_h->is_read_only()) s += " (read-only)";
    setTitle("Oks Data File", s.c_str());
  }


    // set name in the header

  {
    std::string s("Full name: \"");
    s += file_h->get_full_file_name();
    s += '\"';
    set_value(idFileName, s.c_str());
  }
}


std::string
OksDataEditorDataDialog::make_name(const OksObject *o)
{
  std::ostringstream out_s;
  out_s << o->GetId() << '@' << o->GetClass()->get_name();
  return out_s.str();
}


void
OksDataEditorDataDialog::default_commentsCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorDataDialog * dlg = ((OksDataEditorDataDialog *)client_data);
  std::string * s = (std::string *)OksXm::Matrix::get_selected_row_user_data(w);
  std::map<std::string, oks::Comment *>::const_iterator i = dlg->file_h->get_comments().find(*s);
  if(i != dlg->file_h->get_comments().end()) {
    dlg->parent()->create_comment_dlg(dlg->file_h, i->first, i->second);
  }
}

void
OksDataEditorDataDialog::commentsAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)client_data;
  bool is_file_read_only = OksKernel::check_read_only(dlg->file_h);
  std::string * value = (std::string *)OksXm::Matrix::get_selected_row_user_data(w);

  OksPopupMenu popup(w);

  if(value) {
    popup.addItem("Show ", idAcDataShowComment, commentsCB, client_data);
  }
  else {
    popup.addDisableItem("Show");
  }
  
  if(is_file_read_only || !value) {
    popup.addDisableItem("Delete");
  }
  else {
    popup.addItem("Delete ", idAcDataDeleteComment, commentsCB, client_data);
  }

  popup.show((XButtonPressedEvent *)event);
}

void
OksDataEditorDataDialog::commentsCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)client_data;

  switch((long)OksXm::get_user_data(w)) {
    case idAcDataShowComment: {
      std::string * s = (std::string *)OksXm::Matrix::get_selected_row_user_data(dlg->p_comments_matrix);
      std::map<std::string, oks::Comment *>::const_iterator i = dlg->file_h->get_comments().find(*s);
      if(i != dlg->file_h->get_comments().end()) {
        dlg->parent()->create_comment_dlg(dlg->file_h, i->first, i->second);
      }
      break; }

    case idAcDataDeleteComment:
      try {
        std::string * s = (std::string *)OksXm::Matrix::get_selected_row_user_data(dlg->p_comments_matrix);
        dlg->file_h->remove_comment(*s);
        dlg->parent()->update_data_file_row(dlg->file_h);
        dlg->refresh();
      }
      catch(oks::FailedRemoveComment& ex) {
        OksDataEditorMainDialog::report_exception("Delete Comment", ex, dlg->get_form_widget());
      }
      break;
  }
}

void
OksDataEditorDataDialog::newCommentCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorDataDialog * dlg = ((OksDataEditorDataDialog *)client_data);
  dlg->parent()->comment(dlg->file_h, false);
  dlg->refresh();
}

void
OksDataEditorDataDialog::label_objectsCB(Widget w, XtPointer, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct *cbs = (XbaeMatrixLabelActivateCallbackStruct *)cb;

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, OksXm::Matrix::SortByString);
}

void
OksDataEditorDataDialog::label_commentsCB(Widget w, XtPointer, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct *cbs = (XbaeMatrixLabelActivateCallbackStruct *)cb;

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, (cbs->column == 0 ? OksXm::Matrix::SortByTime : OksXm::Matrix::SortByString));
}

void
OksDataEditorDataDialog::default_objectsCB(Widget w, XtPointer client_data, XtPointer)
{
  OksObject * obj = (OksObject *)OksXm::Matrix::get_selected_row_user_data(w);
  OksDataEditorDataDialog * dlg = ((OksDataEditorDataDialog *)client_data);
  dlg->parent()->create_object_dlg(obj);
}


void
OksDataEditorDataDialog::objectsAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksDataEditorDataDialog * dlg = (OksDataEditorDataDialog *)client_data;
  bool is_file_read_only = OksKernel::check_read_only(dlg->file_h);
  OksObject * obj = (OksObject *)OksXm::Matrix::get_selected_row_user_data(w);

  OksPopupMenu popup(w);

  if(obj) {
    popup.addItem("Show", idAcObjectShow, olistCB, client_data);
    popup.addItem("Select", idAcObjectSelect, olistCB, client_data);
  }
  else {
    popup.addDisableItem("Show");
    popup.addDisableItem("Select");
  }

  if(obj && (is_file_read_only == false))
    popup.addItem("Delete", idAcObjectDelete, olistCB, client_data);
  else
    popup.addDisableItem("Delete");

  popup.show((XButtonPressedEvent *)event);
}
