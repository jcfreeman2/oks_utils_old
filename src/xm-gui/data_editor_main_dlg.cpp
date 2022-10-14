#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/scoped_array.hpp>

#include <ers/ers.h>

#include <oks/relationship.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/profiler.h>

#include <oks/xm_popup.h>
#include <oks/xm_msg_window.h>
#include <oks/xm_help_window.h>
#include <oks/xm_utils.h>
#include <oks/xm_context.h>

#include <oks/access.h>

#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <Xm/Matrix.h>
#include <Xm/Label.h>
#include <Xm/FileSB.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/CascadeB.h>
#include <Xm/Text.h>


#include "data_editor_main_dlg.h"
#include "data_editor_data_dlg.h"
#include "data_editor_class_dlg.h"
#include "data_editor_comment_dlg.h"
#include "data_editor_files_dlg.h"
#include "data_editor_object_dlg.h"
#include "data_editor_query_dlg.h"
#include "data_editor_replace_dlg.h"
#include "data_editor_search_panel.h"
#include "data_editor_versions_query_dlg.h"
#include "data_editor_repository_versions_dlg.h"
#include "data_editor_object_ref_by_dlg.h"
#include "data_editor_exceptions.h"
#include "data_editor.h"

#include "g_window.h"
#include "g_class.h"
#include "g_window_info.h"

#include "exit.xpm"
#include "info.xpm"
#include "xbae.xpm"
#include "xmhtml.xpm"
#include "tree.xpm"
#include "data_editor_about.xpm"
#include "default-obj.xpm"
#include "warning.xpm"
#include "error.xpm"
#include "fail.xpm"
#include "save_all.xpm"
#include "commit.xpm"


bool OksDataEditorMainDialog::p_verbose_mode;

Widget about_dialog;
Widget edit_pane;

extern OksDataEditorMainDialog * mainDlg;
extern OksObject *               clipboardObject;


static OksDataEditorMainDialog::DialogReturn done_with_dialog;

const char * szReload       = "Reload        ";
const char * szClose        = "Close         ";
const char * szCloseAll     = "Close All";
const char * szSave         = "Save          ";
const char * szForceSave    = "Force Save    ";
const char * szSaveAs       = "Save As...    ";
const char * szSaveAll      = "Save All";
const char * szSetActive    = "Set Active    ";
const char * szUnsetActive  = "Unset Active  ";
const char * szDetails      = "Details       ";

const size_t NumOfDataFilesMatrixColumns = 4;
const size_t NumOfClassesMatrixColumns   = 4;

static void
real_signal_handler (XtPointer data, XtIntervalId *)
{
  int num = (int) (long) data;

  std::cout << "got signal " << num << (num == SIGINT ? " (interrupt from keyboard)" : num == SIGTERM ? " (termination signal)" : "") << std::endl;
  std::cout.flush();

  if (num == SIGINT)
    OksDataEditorMainDialog::exit_app("interrupt");
  else
    delete mainDlg;
}

extern "C" void
signal_handler(int num)
{
  if (mainDlg)
    XtAppAddTimeOut(mainDlg->app_context(), 0, real_signal_handler, (XtPointer) (long) num);
}


OksDataEditorDialog::OksDataEditorDialog(OksDataEditorMainDialog *p) :
  OksDialog	("", p),
  p_parent	(p)
{
}

void
OksDataEditorMainDialog::show_help_window(const char * url)
{
  static const char *env_url = getenv("OKS_GUI_HELP_URL");

  std::string s;

  if(strncmp("http://", url, 7)) {
    if(env_url) {
      s = env_url;
    }
    else {
      s = "file:";
      s += OKS_DATA_EDITOR_ONLINE_HELP_DIR ; // defined during compilation
    }
  }
  s += url;

  
  get_help_window()->showHelpWindow();
  get_help_window()->displayURL(s.c_str());
}


    //
    // This method creates or remove object string in the Data Dialog (if exists),
    // sets the object's file changed in the main dialog and updates number
    // of objects in the class' matrix of the main window
    //

void
OksDataEditorMainDialog::update_object_info(const OksObject *o, bool add)
{
  OksFile * file_h = o->get_file();

  if(file_h) {

      // update state of files in the main window

    update_data_file_row(file_h);


      // find object's related Data Dialog (if exists)

    OksDataEditorDataDialog * odedd = find_data_dlg(file_h);

    if(odedd) {
      if(add) {
        odedd->add_object(o);
      }
      else {
        odedd->remove_object(o);
      }
    }


      // updates number of objects in the class matrix of the main window

    const char * obj_class_name = o->GetClass()->get_name().c_str();
    size_t row = OksXm::Matrix::get_row_by_value(classes_matrix, obj_class_name);

    if(row != (size_t)-1) {
      size_t number;
      std::istringstream s_in(XbaeMatrixGetCell(classes_matrix, row, 1));

      s_in >> number;

      if(add) number++;
      else number--;

      std::ostringstream s_out;
      s_out << number;

      std::string buf = s_out.str();

      XbaeMatrixSetCell(classes_matrix, row, 1, const_cast<char *>(buf.c_str()));
    }
  }
}


    //
    // Is called when an object is deleted
    //

void
OksDataEditorMainDialog::DeleteNotifyFN(OksObject *o, void * p)
{
  OksDataEditorMainDialog * dlg = (OksDataEditorMainDialog *)p;
  dlg->update_object_info(o, false);


    // find Class Dialog (if exists)

  if(OksDataEditorClassDialog * odecd = dlg->find_class_dlg(o->GetClass())) {
    Widget matrix = odecd->get_widget(idObjects);
    int	rowsNumber = XbaeMatrixNumRows(matrix);

    for(int i=0; i<rowsNumber; i++) {
      const char *uid = XbaeMatrixGetCell(matrix, i, 0);
      if(uid && (o->GetId() == uid)) {
        if(OksXm::Matrix::get_selected_value(matrix) == uid) XbaeMatrixDeselectRow(matrix, i);
        XbaeMatrixDeleteRows(matrix, i, 1);
        break;
      }
    }
  }


    // delete Object Dialog if it exists

  delete dlg->find_object_dlg(o);


    // delete Object Referenced By Dialog if it exists

  delete dlg->find_object_referenced_by_dlg(o);


    // update graphical windows

  for(std::list<G_Window *>::iterator i = dlg->p_windows.begin(); i != dlg->p_windows.end(); ++i) {
    (*i)->delete_object(o);
  }
  
    // update referenced by dialogues
    
  dlg->refresh_object_referenced_by_dialogs();


  dlg->reset_used();


    // clean Clipboard if it contains deleted object

  if(o == clipboardObject) clipboardObject = 0;
}


void
OksDataEditorMainDialog::refresh_object_referenced_by_dialogs()
{
  for(std::list<OksDataEditorDialog *>::const_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    if((*i)->type() == OksDataEditorDialog::ObjectReferencedByDlg) {
      OksDataEditorObjectReferencedByDialog *dlg = static_cast<OksDataEditorObjectReferencedByDialog *>(*i);
      dlg->refresh();
    }
  }
}


    //
    // Is called when an object is created
    //

void
OksDataEditorMainDialog::CreateNotifyFN(OksObject *o, void * p)
{
  OksDataEditorMainDialog * dlg = (OksDataEditorMainDialog *)p;
  dlg->update_object_info(o, true);

    // find Class Dialog (if exists)

  if(OksDataEditorClassDialog *odecd = dlg->find_class_dlg(o->GetClass())) {
    if(odecd->p_select_objects_panel->test_match(o->GetId()) == true) {
      Widget matrix = odecd->get_widget(idObjects);

      OksDataEditorClassDialog::add_row(matrix, o); // FIXME: check if true

      OksXm::Matrix::set_cell_background(matrix, odecd->get_widget(idClass));
    }
  }

    // update graphical windows

  for(std::list<G_Window *>::iterator i = dlg->p_windows.begin(); i != dlg->p_windows.end(); ++i) {
    if((*i)->create_top_level_object(o)) {
      (*i)->draw(0);
    }
  }


    // update referenced by dialogues

  dlg->refresh_object_referenced_by_dialogs();
}


    //
    // Is called when an object is created
    //

void
OksDataEditorMainDialog::ChangeNotifyFN(OksObject *o, void * p)
{
  OksDataEditorMainDialog * dlg = (OksDataEditorMainDialog *)p;

  dlg->update_data_file_row(o->get_file());

  {
    OksDataEditorObjectDialog * odeod = dlg->find_object_dlg(o);
    if(odeod) {
      odeod->refresh();
    }
  }

  if(!dlg->p_windows.empty()) {
    for(std::list<G_Window *>::iterator i = dlg->p_windows.begin(); i != dlg->p_windows.end(); ++i) {
      if(!(*i)->objects.empty()) {
        for(std::list<G_Object *>::iterator j = (*i)->objects.begin(); j != (*i)->objects.end(); ++j) {
	  if((*j)->get_oks_object() == o) {
	    (*i)->draw(0);
	    break;
	  }
	}
      }
    }
  }

  const OksClass *c = o->GetClass();
  OksDataEditorClassDialog *odecd = dlg->find_class_dlg(c);

  if(odecd) {
    Widget matrix = odecd->get_widget(idObjects);
    int rowsNumber = XbaeMatrixNumRows(matrix);

    for(int i=0; i<rowsNumber; i++) {
      if(o == (OksObject *)XbaeMatrixGetRowUserData(matrix, i)) {
        XbaeMatrixSetCell(matrix, i, 0, const_cast<char *>(o->GetId().c_str()));

        unsigned short sizeOfMatrix = c->number_of_all_attributes() + c->number_of_all_relationships();
        OksDataInfo odi(0, (const OksAttribute *)0);
	std::list<OksAttribute *>::const_iterator ai = c->all_attributes()->begin();
	size_t numOfAttrs = c->number_of_all_attributes();
	const OksAttribute * a = 0;
        OksData *d;

        for(unsigned int j=0; j<sizeOfMatrix;) {
            // if process an attribute, we need to know it's format
	  if(j < numOfAttrs) {
	    a = *ai;
	    ++ai;
	  }
	  else {
	    a = 0;
	  }
          odi.offset = j++;
          d = o->GetAttributeValue(&odi);

	  std::string value;
	  if(a) {
            value = d->str(a->is_integer() ? (int)a->get_format() : (int)0);
	  }
	  else {
            value = d->str(c->get_kernel());
	  }

          XbaeMatrixSetCell(matrix, i, j, const_cast<char *>(value.c_str()));
        }

        XbaeMatrixRefresh(matrix);

        break;
      }
    }
  }


    // update referenced by dialogues

  dlg->refresh_object_referenced_by_dialogs();
}


void
OksDataEditorMainDialog::exit_app(const char * message)
{
  if (mainDlg)
    {
      for (const auto& i : mainDlg->data_files())
        if (mainDlg->confirm_changes(message, i.second) == CancelExit)
          return;

      if (!mainDlg->get_user_repository_root().empty())
        {
          std::set<std::string> updated, added, removed;

          try
            {
              mainDlg->get_updated_repository_files(updated, added, removed);
            }
          catch(const oks::exception& ex)
            {
              report_exception("Get repository status", ex);
            }

          if (int count = updated.size() + added.size() + removed.size())
            {
              std::ostringstream text;
              if (count == 1)
                text << "There is 1 uncommitted file";
              else
                text << "There are " << count << " uncommitted files";
              text << ":\n";

              auto add_message = [](std::ostringstream& text, std::set<std::string>& files, const char * name)
                {
                  if (!files.empty())
                    {
                      text << '(' << name << "):";
                      bool first = false;
                      for (const auto&x : files)
                        {
                          if (first)
                            first = false;
                          else
                            text << ',';

                          text << ' ' << x;
                        }

                      text << '\n';
                    }
                };

              add_message(text, added, "added");
              add_message(text, removed, "removed");
              add_message(text, updated, "updated");

              const char ** xpm = warning_xpm;

              if (mainDlg->is_user_repository_created())
                {
                  xpm = error_xpm;
                  text << "\nThe uncommitted files are stored on temporal area, that will be removed on exit.\n"
                      "All uncommitted changes will be lost!\n";
                }

              text << "\nPress [OK] to exit anyway or [Cancel] to continue work.";

              if (!mainDlg->confirm_dialog(message, text.str().c_str(), xpm))
                return;
            }
        }

      delete mainDlg;
    }

  exit(0);
}


void
OksDataEditorMainDialog::classCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  OksDataEditorMainDialog *dlg = (OksDataEditorMainDialog *)client_data;
  char *name = XbaeMatrixGetCell(w, ((XbaeMatrixDefaultActionCallbackStruct *)call_data)->row, 0);
  OksClass *cl = (name && *name != '\0' ? dlg->find_class(name) : 0);

  if(cl && !cl->get_is_abstract()) dlg->create_class_dlg(cl);
}


void
OksDataEditorMainDialog::dataCB(Widget w, XtPointer client_data, XtPointer)
{
  ((OksDataEditorMainDialog *)client_data)->create_data_dlg(
    (OksFile *)(OksXm::Matrix::get_selected_row_user_data(w))
  );
}


void
OksDataEditorMainDialog::OpenDataCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmFileSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksDataEditorMainDialog *dlg = (OksDataEditorMainDialog *)client_data;
    OksXm::AutoCstring name(OksXm::string_to_c_str(((XmFileSelectionBoxCallbackStruct *)call_data)->value));

    try {
      mainDlg->load_data(name.get());
      dlg->refresh_schemes();
      dlg->refresh_data();
      dlg->refresh_classes();
      dlg->refresh_class_dialogs();
      OksXm::Matrix::select_row((Widget)OksXm::get_user_data(w), name.get());
      mainDlg->test_bind_classes_status();
    }
    catch (oks::exception & ex) {
      report_exception("Load Data File", ex);
    }
  }

  XtUnmanageChild(w);
  XtDestroyWidget(w);
}


void
OksDataEditorMainDialog::OpenSchemaCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmFileSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksDataEditorMainDialog *dlg = (OksDataEditorMainDialog *)client_data;
    OksXm::AutoCstring name(OksXm::string_to_c_str(((XmFileSelectionBoxCallbackStruct *)call_data)->value));

    try {
      mainDlg->load_schema(name.get());
      Widget list = (Widget)OksXm::get_user_data(w);
      dlg->refresh_schemes();
      dlg->refresh_classes();

      int itemCount;
      XtVaGetValues(list, XmNitemCount, &itemCount, NULL);
      XmListSelectPos(list, itemCount, true);

      mainDlg->test_bind_classes_status();
    }
    catch (oks::exception & ex) {
      report_exception("Load Schema File", ex);
    }
  }
  
  XtUnmanageChild(w);
  XtDestroyWidget(w);
}


void
OksDataEditorMainDialog::SaveAsDataCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    Widget matrix = (Widget)OksXm::get_user_data(w); 

    OksXm::AutoCstring newDataName(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));

    if(OksXm::ask_overwrite_file(matrix, newDataName.get()) == false) return;

    int row = static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(XmSelectionBoxGetChild(w, XmDIALOG_OK_BUTTON))));
    std::string oldDataName(XbaeMatrixGetCell(matrix, row, 0));

    OksDataEditorMainDialog *dlg = (OksDataEditorMainDialog *)client_data;
    OksFile * f = dlg->find_data_file(oldDataName);

    if(f) {
      try {
        dlg->comment(f);
        dlg->save_as_data(newDataName.get(), f);
        dlg->remove_backup(f);

        dlg->refresh_data();
        OksXm::Matrix::select_row(matrix, newDataName.get());


          // change name of datafile in Data Dialog (if exists)

        OksDataEditorDataDialog * odedd = dlg->find_data_dlg(f);
        if(odedd) {
          odedd->change_file_name();
	  odedd->refresh();
	  dlg->update_menu_bar();
        }


          // change name of datafile in all Object Dialog
	  // (if exists and an object belongs to the renamed datafile)

        for(std::list<OksDataEditorDialog *>::iterator i = dlg->dialogs.begin(); i != dlg->dialogs.end();++i) {
          OksDataEditorDialog *oded = *i;

          if(oded->type() == OksDataEditorDialog::ObjectDlg) {
            OksDataEditorObjectDialog *dlg = static_cast<OksDataEditorObjectDialog *>(oded);

            if(dlg->getObject()->get_file() == f) {
	      dlg->refresh_file_name();
	    }
          }
        }
      }
      catch (oks::exception& ex) {
        report_exception("Save As Data File", ex);
      }
    }
  }

  XtDestroyWidget(w);
}


void
OksDataEditorMainDialog::OpenQueryCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmFileSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksXm::AutoCstring queryName(OksXm::string_to_c_str(((XmFileSelectionBoxCallbackStruct *)call_data)->value));
    OksClass *cl = (OksClass *)OksXm::get_user_data(w);

    if(!queryName.is_non_empty()) {
      report_error("Open Query", "there is no file name");
      return;
    }

    try
      {
        if (std::filesystem::is_regular_file(queryName.get()) == false)
          {
            std::ostringstream text;
            text << "is not a regular file \"" << queryName.get() << '\"';
            report_error("Open Query", text.str().c_str());
            return;
          }
      }
    catch (std::exception & ex)
      {
        std::ostringstream text;
        text << "cannot get information about file \"" << queryName.get() << "\": " << ex.what();
        report_error("Open Query", text.str().c_str());
        return;
      }

    std::ifstream f(queryName.get());

    if(!f.good()) {
      std::ostringstream text;
      text << "cannot open file \"" << queryName.get() << '\"';
      report_error("Open Query", text.str().c_str());
      return;
    }

    f.seekg(0, std::ios::end);
    long len = f.tellg();
    f.seekg(0, std::ios::beg);

    boost::scoped_array<char> query_buf(new char[len + 1]);

    f.read(query_buf.get(), len);
    query_buf[len] = '\0';

    if(f.fail() || f.bad()) {
      std::ostringstream text;
      text << "cannot read file \"" << queryName.get() << '\"';
      report_error("Open Query", text.str().c_str());
      return;
    }

    std::unique_ptr<OksQuery> qe(new OksQuery(cl, query_buf.get()));

    if(qe->good())
      ((OksDataEditorMainDialog *)client_data)->create_query_dlg(cl, qe.release());
  }

  XtUnmanageChild(w);
  XtDestroyWidget(w);
}

void
OksDataEditorMainDialog::reload(OksFile * f, const std::list<OksFile *> * ff)
{
  p_skip_menu_update = true;  // do not redraw menu, while files are reloaded

  OksXm::destroy_modal_dialogs();

  try {
    std::set<OksFile*> files;
    if(f) {
      files.insert(f);
    }
    else {
      for(std::list<OksFile *>::const_iterator i = ff->begin(); i != ff->end(); ++i) {
        files.insert(*i);
      }
    }
    reload_data(files);
  }
  catch (oks::exception & e) {
    report_exception("Reload Data File", e);
  }

  refresh_schemes();
  refresh_data();
  refresh_classes();

    // refresh or close all open data file dialogs

  for(std::list<OksDataEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end();) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::DataDlg) {
      OksDataEditorDataDialog * dlg(static_cast<OksDataEditorDataDialog *>(oded));

      bool found(false);

      for(OksFile::Map::const_iterator j = data_files().begin(); j != data_files().end(); ++j) {
        if(j->second == dlg->get_file()) {
	  dlg->refresh();
	  found = true;
	  break;
	}
      }

      if(!found) {
        i = dialogs.erase(i);
	delete oded;
	continue;
      }
    }
    else if(oded->type() == OksDataEditorDialog::ObjectDlg) {
      static_cast<OksDataEditorObjectDialog *>(oded)->refresh_file_name();
    }
    else if(oded->type() == OksDataEditorDialog::CommentDlg) {
      i = dialogs.erase(i);
      delete oded;
      continue;
    }
    
    ++i;
  }

  p_skip_menu_update = false;  // refresh menu bar once all obsolete dialogs were closed
  update_menu_bar();
}

void
OksDataEditorMainDialog::comment(OksFile * f, bool test_parameter)
{
  if(test_parameter == false || OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_ask_comment_name())) {
    new OksDataEditorCommentDialog(this, f, "", 0);
  }
}

void
OksDataEditorMainDialog::save(OksFile * f, bool force)
{
  try {
    comment(f);
    save_data(f, force);
    update_data_file_row(f);
    remove_backup(f);
  }
  catch (oks::exception & ex) {
    report_exception("Save Data File", ex);
  }
}

void
OksDataEditorMainDialog::create_backup(OksFile * pf)
{
  try {
    backup_data(pf, ".saved");
    p_backup_files.insert(pf);
  }
  catch (oks::exception & ex) {
    report_exception("Create Backup File", ex);
  }
}

void
OksDataEditorMainDialog::remove_backup(OksFile * pf)
{
  std::string name = pf->get_full_file_name() + ".saved";

    // check file existence
  {
    std::ifstream f(name.c_str());

    if(!f) {
      ERS_DEBUG(2, "file \"" << name << "\" was not found; nothing to remove");
      return;
    }
  }

  if(int code = unlink(name.c_str()) != 0) {
    std::ostringstream text;
    text << "failed to unlink backup file \"" << name << "\": unlink() failed with code " << code << ", reason = \'" << strerror(errno) << '\'';
    report_error("Remove Backup File", text.str().c_str());
  }
  else {
    p_backup_files.erase(pf);
  }
}

void
OksDataEditorMainDialog::close_graphical_windows()
{
  while(!p_windows.empty()) {
    G_Window *win = p_windows.front();
    p_windows.pop_front();
    delete win;
  }
}

std::string
OksDataEditorMainDialog::get_commit_comments(const std::set<std::string>& created, const std::set<std::string>& updated)
{
  std::map<std::string, std::string> comments_sorted_by_text;        // ["text"] => min("iso_time")
  std::multimap<std::string, std::string> comments_sorted_by_time;   // ["iso_time"] => "text1", "text2", ...

  std::vector<const std::set<std::string> *> files = {&created, &updated};

  for (auto& fs : files)
    {
      for (auto& f : *fs)
        {
          std::time_t last_modified = get_repository_checkout_ts();

          try
            {
              std::string cmdl = " -n 1 -- ";
              cmdl.append(f);

              std::vector<OksRepositoryVersion> version = get_repository_versions(false, cmdl);
              if (version.empty() == false)
                {
                  last_modified = version[0].m_date;
                }
            }
          catch (oks::exception& ex)
            {
              std::cerr << "ERROR [get_commit_comments]: failed to get last commit timestamp of file \"" << f << "\":\n" << ex.what() << std::endl;
            }

          std::string filename(OksKernel::get_user_repository_root());
          filename.push_back('/');
          filename.append(f);

          OksFile * file = find_data_file(filename);

          if (file == nullptr)
            file = find_schema_file(filename);

          if (file)
            {
              for (const auto& j : file->get_comments())
                {
                  if (boost::posix_time::to_time_t(boost::posix_time::from_iso_string(j.first)) >= last_modified)
                    {
                      std::map<std::string, std::string>::iterator x = comments_sorted_by_text.find(j.second->get_text());
                      if (x == comments_sorted_by_text.end())
                        comments_sorted_by_text[j.second->get_text()] = j.first;
                      else if (x->second > j.first)
                        x->second = j.first;
                    }
                }
            }
        }
    }


  for (const auto& i : comments_sorted_by_text)
    comments_sorted_by_time.insert(std::multimap<std::string, std::string>::value_type(i.second, i.first));

  std::string out;

  for (const auto& i : comments_sorted_by_time)
    {
      if (!out.empty())
        out += "\n---\n";
      out += i.second;
    }

  return out;
}

static const char*
default_fileop_directory (const OksKernel *k)
{
  return (!k->get_user_repository_root ().empty () ? k->get_user_repository_root ().c_str () :
          !k->get_repository_dirs ().empty () ? k->get_repository_dirs ().front ().c_str () : nullptr);
}


void
OksDataEditorMainDialog::anyCB(Widget w, XtPointer client_data, XtPointer) 
{
  long user_data = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  OksDataEditorMainDialog * dlg = (OksDataEditorMainDialog *)client_data;
  Widget list = XtParent(XtParent(XtParent(w)));
  Widget matrix = 0;
  const char * itemName = 0;
  OksFile * pf = 0;

  if(
    user_data == idAcMainSchemaOpen	||
    user_data == idAcMainSchemaClose	||
    user_data == idAcMainSchemaCloseAll
  ) {
    itemName = OksXm::List::get_selected_value(list);
    if(itemName) pf = dlg->find_schema_file(itemName);
  }
  else {
    matrix = list;
    itemName = OksXm::Matrix::get_selected_value(matrix);
    pf = (OksFile *)(OksXm::Matrix::get_selected_row_user_data(matrix));
  }

  switch(user_data) {
     case idAcMainSchemaOpen:
       XtVaSetValues(
         OksXm::create_file_selection_box(
           dlg->get_form_widget(), "Open OKS Schema", "Selection of file with OKS schema",
           default_fileop_directory(dlg), "*.schema.xml", true, OksDataEditorMainDialog::OpenSchemaCB, (XtPointer)client_data, 0, 0
         ),
	 XmNuserData, (XtPointer)list,
	 NULL
       );

       break;

     case idAcMainDataOpen:
       XtVaSetValues(
         OksXm::create_file_selection_box(
           dlg->get_form_widget(), "Open OKS Data", "Selection of file with OKS data",
           default_fileop_directory(dlg), "*.data.xml", true, OksDataEditorMainDialog::OpenDataCB, (XtPointer)client_data, 0, 0
         ),
	 XmNuserData, (XtPointer)list,
	 NULL
       );

       break;

    case idAcMainSchemaClose:
      dlg->close_graphical_windows();
      dlg->remove_g_classes();
      dlg->close_schema(pf);
      dlg->refresh_schemes();
      dlg->refresh_classes();

      break;

    case idAcMainSchemaCloseAll:
      for(OksFile::Map::const_iterator i = dlg->data_files().begin(); i != dlg->data_files().end(); ++i) {
        if(dlg->confirm_changes("Close", i->second) == OksDataEditorMainDialog::CancelExit) return;
      }

      while(!dlg->dialogs.empty()) {
        OksDataEditorDialog * d = dlg->dialogs.front();
        dlg->dialogs.pop_front();
        delete d;
      }

      dlg->close_graphical_windows();
      dlg->remove_g_classes();
      dlg->close_all_schema();
      dlg->refresh_schemes();
      dlg->refresh_classes();

      break;

    case idAcMainDataNew: {
      std::string s = OksXm::ask_file(
        dlg->get_form_widget(),
	"Create new OKS data file",
	"New OKS data file name",
	default_fileop_directory(dlg),
	"*.data.xml"
      );

      if(s.length() && OksXm::ask_overwrite_file(mainDlg->get_form_widget(), s.c_str())) {
        try {
          dlg->new_data(s);
          dlg->refresh_data();
          OksXm::Matrix::select_row(list, s.c_str());
          XtSetSensitive(dlg->classes_matrix, True);
        }
        catch (oks::exception & ex) {
          report_exception("New Data File", ex);
        }
      }

      break;
    }

    case idAcMainDataReload:
      if(pf && dlg->confirm_changes("Reload", pf) != OksDataEditorMainDialog::CancelExit) {
        dlg->reload(pf);
      }

      break;

    case idAcMainDataClose:
      if(pf && dlg->confirm_changes("Close", pf) != OksDataEditorMainDialog::CancelExit) {
        if(OksDataEditorDataDialog * odedd = dlg->find_data_dlg(pf)) {
	  delete odedd;
	}
        dlg->close_graphical_windows();
        dlg->close_data(pf);
        dlg->refresh_data();
      }

      break;

    case idAcMainDataCloseAll:
      for(OksFile::Map::const_iterator i = dlg->data_files().begin(); i != dlg->data_files().end(); ++i) {
        if(dlg->confirm_changes("Close", i->second) == OksDataEditorMainDialog::CancelExit) return;
      }

      dlg->close_graphical_windows();
      dlg->close_data_dialogs();
      dlg->close_all_data();
      dlg->refresh_data();

      break;

    case idAcMainDataSave:
      dlg->save(pf);
      break;

    case idAcMainDataSaveForce: {
      std::ostringstream text;
      text << "OKS: save file \"" << pf->get_full_file_name() << "\" warning";
      XtManageChild(
        OksXm::create_dlg_with_pixmap(
          mainDlg->get_form_widget(), text.str().c_str(), warning_xpm,
          "The file was saving with \'force\' option ingoring\n"
          "possible violations of the OKS consistency rules.\n"
          "The reading of such file by OKS tools may cause problems.\n"
	  "Check and fix any problems during saving of this file.",
          "Ok", 0, 0, 0, 0
        )
      );
      dlg->save(pf, true);
      break; }

    case idAcMainDataSaveAll:
      try {
        dlg->save_all_data();
	for(std::set<OksFile *>::iterator i = dlg->p_backup_files.begin(); i != dlg->p_backup_files.end();) {
	  dlg->remove_backup(*(i++));  // note, the current OksFile will be removed from the set
	}
      }
      catch (oks::exception & ex) {
        report_exception("Save All Data Files", ex);
      }
      dlg->refresh_data();
      break;

    case idAcMainDataSaveAs: {

        // calculate directory where to save files

      const char * dir_name = 0;
      std::string olds(itemName);
      std::string::size_type pos = olds.find_last_of('/');

      if(pos != std::string::npos) {
        olds.erase(pos);
	dir_name = olds.c_str();
      }

      Widget dialog = OksXm::create_file_selection_box(
        dlg->get_form_widget(),
	"Save OKS Data As", "Selection of file to save data", dir_name, "*.data.xml", true,
	OksDataEditorMainDialog::SaveAsDataCB, client_data
      );

      XmTextSetString(XmFileSelectionBoxGetChild(dialog, XmDIALOG_VALUE_TEXT), const_cast<char *>(itemName));

      int row = -1, column = -1;
      XbaeMatrixFirstSelectedCell(matrix, &row, &column);

      XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON), XmNuserData, (XtPointer)(long)row, NULL);
      XtVaSetValues(dialog, XmNuserData, (XtPointer)matrix, NULL);

      break;
    }

    case idAcMainDataSetActive:
    case idAcMainDataUnsetActive:
        try {
          OksFile * old = dlg->get_active_data();
          dlg->set_active_data(user_data == idAcMainDataSetActive ? pf : 0);
          if(old) dlg->update_data_file_row(old);
          dlg->update_data_file_row(pf);
        }
        catch(oks::exception& ex) {
          report_exception("Change Active File", ex);
        }
        break;

    case idAcMainDataShowDetails:
	dlg->create_data_dlg(pf);
	break;

    case idAcMainClassQueryShow:
    case idAcMainClassQueryLoad:
    case idAcMainClassNew:
    case idAcMainClassShow: {
	OksClass *cl = dlg->find_class(itemName);
	if(cl) {
		if(user_data == idAcMainClassShow)
			dlg->create_class_dlg(cl);
		else if(user_data == idAcMainClassQueryShow)
			dlg->create_query_dlg(cl);
		else if(user_data == idAcMainClassNew)
			dlg->create_object_dlg(cl);
		else {
			Arg	args[5];
			Widget	dialog;
	
			XtSetArg(args[0], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
			dialog = XmCreateFileSelectionDialog(dlg->get_form_widget(), (char *)"file selection", args, 1);
	
			XtVaSetValues(XtParent(dialog), XmNtitle, "Open OKS Query", NULL);
			XtAddCallback (dialog, XmNokCallback, OksDataEditorMainDialog::OpenQueryCB, client_data);
			XtAddCallback (dialog, XmNcancelCallback, OksDataEditorMainDialog::OpenQueryCB, client_data);

			XtVaSetValues(dialog, XmNuserData, (XtPointer)cl, NULL);
			XtManageChild(dialog);
		}
	}
	break;
    }
  }
  
  if(itemName && !matrix) XtFree(const_cast<char *>(itemName));
}


void
OksDataEditorMainDialog::dataAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  const char *dataName = OksXm::Matrix::get_selected_value(w);
  OksDataEditorMainDialog *dlg = (OksDataEditorMainDialog *)client_data;
  OksFile * pf = (OksFile *)(OksXm::Matrix::get_selected_row_user_data(w));

  OksPopupMenu popup(w);

  popup.addItem("New...", idAcMainDataNew, anyCB, client_data);
  popup.addItem("Open...", idAcMainDataOpen, anyCB, client_data);
  popup.add_separator();

  if(dataName) {
    popup.addItem(szReload, dataName, idAcMainDataReload, anyCB, client_data);
    popup.addItem(szClose, dataName, idAcMainDataClose, anyCB, client_data);
    popup.addItem(szCloseAll, idAcMainDataCloseAll, anyCB, client_data);
    popup.add_separator();
    popup.addItem(szSave, dataName, idAcMainDataSave, anyCB, client_data);
    popup.addItem(szForceSave, dataName, idAcMainDataSaveForce, anyCB, client_data);
    popup.addItem(szSaveAs, dataName, idAcMainDataSaveAs, anyCB, client_data);
    popup.addItem(szSaveAll, idAcMainDataSaveAll, anyCB, client_data);
    popup.add_separator();

    Widget btn;
    if(pf == dlg->get_active_data()) {
      btn = popup.addItem(szUnsetActive, dataName, idAcMainDataUnsetActive, anyCB, client_data);
    }
    else {
      btn = popup.addItem(szSetActive, dataName, idAcMainDataSetActive, anyCB, client_data);
    }
    popup.addItem(szDetails, dataName, idAcMainDataShowDetails, anyCB, client_data);

    if(!pf || OksKernel::check_read_only(pf) == true) XtSetSensitive(btn, false);
  }
  else {
    int itemCount = XbaeMatrixNumRows(w);

    popup.addDisableItem("Reload");
    popup.addDisableItem(szClose);
    if(itemCount)
      popup.addItem(szCloseAll, idAcMainDataCloseAll, anyCB, client_data);
    else
      popup.addDisableItem(szCloseAll);
    popup.add_separator();
    popup.addDisableItem(szSave);
    popup.addDisableItem(szForceSave);
    popup.addDisableItem(szSaveAs);
    if(itemCount)
      popup.addItem(szSaveAll, idAcMainDataSaveAll, anyCB, client_data);
    else
      popup.addDisableItem(szSaveAll);
    popup.add_separator();

    popup.addDisableItem(szSetActive);
    popup.addDisableItem(szDetails);
  }

  popup.show((XButtonPressedEvent *)event);
}


void
OksDataEditorMainDialog::schemaAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;
  
  OksXm::AutoCstring schemaName(OksXm::List::get_selected_value(w));
  
  OksPopupMenu popup(w);

  popup.addItem("Open...", idAcMainSchemaOpen, anyCB, client_data);
  popup.add_separator();
  
  if(schemaName.get())
    popup.addItem(szClose, schemaName.get(), idAcMainSchemaClose, anyCB, client_data);
  else
    popup.addDisableItem(szClose);

  popup.addItem(szCloseAll, idAcMainSchemaCloseAll, anyCB, client_data);

  popup.show((XButtonPressedEvent *)event);
}


void
OksDataEditorMainDialog::classAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);

  int			row = -1, column;
  const char		*class_name = 0;
  const OksClass	*c = 0;

  XbaeMatrixFirstSelectedCell(w, &row, &column);

  if(row != -1 && column != -1) {
	class_name = XbaeMatrixGetCell(w, row, column);
	c = findOksClass(class_name);
  }

  if(row != -1 && c && !c->get_is_abstract())
	popup.addItem("Show   ", idAcMainClassShow, anyCB, client_data);
  else
	popup.addDisableItem("Show   ");

  popup.add_separator();


  if(row != -1 && c && !c->get_is_abstract() && OksDataEditorMainDialog::is_valid_active_data())
	popup.addItem("New Object", idAcMainClassNew, anyCB, client_data);
  else
	popup.addDisableItem("New Object");

  popup.add_separator();


  if(row != -1) {
	popup.addItem("Query", idAcMainClassQueryShow, anyCB, client_data);
	popup.addItem("Load Query", idAcMainClassQueryLoad, anyCB, client_data);
  }
  else {
	popup.addDisableItem("Query");
	popup.addDisableItem("Load Query");
  }

  popup.show((XButtonPressedEvent *)event);
}


void
OksDataEditorMainDialog::classesLabelCB(Widget w, XtPointer, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct *cbs = (XbaeMatrixLabelActivateCallbackStruct *)cb;

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, ((cbs->column == 1) ? OksXm::Matrix::SortByInteger : OksXm::Matrix::SortByString));
}

void
OksDataEditorMainDialog::dataLabelCB(Widget w, XtPointer, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct *cbs = (XbaeMatrixLabelActivateCallbackStruct *)cb;

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, OksXm::Matrix::SortByString);
}

void
OksDataEditorMainDialog::init(
  bool is_v, const char *init_dir,
  const char *init_d, const char *pxm_dir, const char *bmp_dir)
{
  if(init_dir) p_base_init_dir = init_dir;

  p_verbose_mode = is_v;


    // suppose, XmList has the same background as XmFrame

  G_Class::init(schema_list);


    // calculate init path prefix:
    //  * "${OKS_GUI_PATH}/"
    //  * "${TDAQ_INST_PATH}/share/data/"

  {
    static const char * base_path = (
      p_base_init_dir.empty() ? getenv("OKS_GUI_PATH") : p_base_init_dir.c_str()
    );

    static const char * tdaq_path = getenv("TDAQ_INST_PATH");

    if(base_path && *base_path) {
      p_init_path = base_path;
    }
    else if(tdaq_path && *tdaq_path) {
      p_init_path = tdaq_path;
      p_init_path += "/share/data";
    }

    if(is_verbose()) {
      std::cout << "p_init_path search prefix = \'" << p_init_path << "\'\n";
    }
  }


    // read configuration information

  p_init_data_files  = get_conf_param("OKS_GUI_INIT_DATA",   init_d);
  p_pixmaps_dirs     = get_conf_param("OKS_GUI_XPM_DIRS",    pxm_dir);
  p_bitmaps_dirs     = get_conf_param("OKS_GUI_XBM_DIRS",    bmp_dir);


    // init default graphical class

  p_default_class = new G_Class(
    0,
    "Default Graphical Class",
    default_obj_xpm,
    (const char **)0,
    G_Class::G_Bitmap((unsigned char *)0, 0, 0),
    G_Class::G_Bitmap((unsigned char *)0, 0, 0)
  );


    // init other classes from database

  init_g_classes();
  
    // update menu bar

  set_edit_menu();
}

void
OksDataEditorMainDialog::findClassesCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorMainDialog * dlg(reinterpret_cast<OksDataEditorMainDialog *>(client_data));
  dlg->p_select_classes_panel->refresh();
  dlg->refresh_classes();
}

void
OksDataEditorMainDialog::findDataFilesCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorMainDialog * dlg(reinterpret_cast<OksDataEditorMainDialog *>(client_data));
  dlg->p_select_data_files_panel->refresh();
  dlg->refresh_data();
}

void
OksDataEditorMainDialog::configureAC (Widget shell, XtPointer /*client_data*/, XEvent *event, Boolean * /*unused*/)
{
  XConfigureEvent *cevent = (XConfigureEvent *) event;
  if(cevent->type == ConfigureNotify) {
    OksDialog::resetCascadePos(shell, cevent->x, cevent->y);
  }
}

OksDataEditorMainDialog::OksDataEditorMainDialog(int *app_argc, char **app_argv, const char * fallback_resources[][2], bool no_msg_dlg, bool msg_dlg_auto_scroll) :
  OksTopDialog	                      ("Oks Data Editor", app_argc, app_argv, "OksEditor", fallback_resources),
  OksKernel	                      (),
  replaceDialog	                      (0),
  findDialog	                      (0),
  p_new_repository_versions_dlg       (nullptr),
  p_archived_repository_versions_dlg  (nullptr),
  p_readonly_mode                     (false),
  p_versions_query_dlg                (nullptr),
  msg_window	                      (0),
  no_msg_window	                      (no_msg_dlg),
  helpWindow	                      (0),
  p_skip_menu_update                  (false),
  p_check_updated_timer               (0),
  p_check_need_backup_timer           (0),
  p_base_init_dir	              (""),
  p_init_data_files	              (""),
  p_pixmaps_dirs	              (""),
  p_bitmaps_dirs	              (""),
  p_default_class                     (0),
  p_root_class                        (0),
  p_g_classes_created                 (false),
  p_red_pixel                         (OksXm::string_to_pixel(app_shell(), "dark red")),
  p_green_pixel                       (OksXm::string_to_pixel(app_shell(), "dark green")),
  p_blue_pixel                        (OksXm::string_to_pixel(app_shell(), "dark blue")),
  p_select_classes_panel              (0),
  p_select_data_files_panel           (0)
{
  set_allow_duplicated_classes_mode(false);

  bool restore_pos(static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_restore_main_window_pos())));

  if(restore_pos) {
    Dimension x(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_main_window_x_pos()));
    Dimension y(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_main_window_y_pos()));
    XtVaSetValues(p_app_shell, XmNx, x, XmNy, y, NULL);
  }

  mainDlg = this;
  
  menuBar = add_menubar();
  fill_menu_bar(menuBar);
  XtManageChild(menuBar);

  Widget panedWidget = add_paned_window(idMainPanedWindow);
  OksPanedWindow *panedWindow = get_paned_window(idMainPanedWindow);
  attach_right(idMainPanedWindow);

  OksForm *f = new OksForm(panedWidget);
  schema_list = f->add_list(idSchema, "Schema Files:");
  f->attach_right(idSchema);
  f->attach_bottom(idSchema);
  panedWindow->add_form(f, idSchema);
  XtAddEventHandler(schema_list, ButtonPressMask, False, schemaAC, (XtPointer)this);

  f = new OksForm(panedWidget);
  data_matrix = f->add_matrix(idData, (void *)"Data Files:");
  f->attach_right(idData);
  p_select_data_files_panel = new OksDataEditorSearchPanel(*f, "Matching names of files:", "data files", findDataFilesCB, (XtPointer)this);
  f->attach_bottom(idData);
  panedWindow->add_form(f, idData);



    // set data files matrix specific parameters

  {
    char const * labels[] = {"File", "Access", "Repository", "Status"};
    unsigned char label_alinments[NumOfDataFilesMatrixColumns];
    short widths[NumOfDataFilesMatrixColumns];

    for(size_t i=0; i<NumOfDataFilesMatrixColumns; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }
  
    XtVaSetValues(
      data_matrix,
      XmNcolumns, NumOfDataFilesMatrixColumns,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      NULL
    );

    XtAddCallback(data_matrix, XmNdefaultActionCallback, dataCB, (XtPointer)this);
    XtAddCallback(data_matrix, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
    XtAddCallback(data_matrix, XmNlabelActivateCallback, dataLabelCB, 0);
    XtAddEventHandler(data_matrix, ButtonPressMask, False, dataAC, (XtPointer)this);
  }

  f = new OksForm(panedWidget);
  classes_matrix = f->add_matrix(idClasses, (void *)"Classes:");
  f->attach_right(idClasses);
  p_select_classes_panel = new OksDataEditorSearchPanel(*f, "Matching names of classes:", "classes", findClassesCB, (XtPointer)this);
  f->attach_bottom(idClasses);
  panedWindow->add_form(f, idClasses);


    // set classes matrix specific parameters

  {
    char const *labels[] = {"Name", "Number of instances", "Is abstract", ""};
    unsigned char label_alinments[NumOfClassesMatrixColumns];
    short widths[NumOfClassesMatrixColumns];

    for(size_t i=0; i<NumOfClassesMatrixColumns; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }

    XtVaSetValues(
      classes_matrix,
      XmNcolumns, NumOfClassesMatrixColumns,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      NULL
    );

    XtAddCallback(classes_matrix, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
    XtAddCallback(classes_matrix, XmNlabelActivateCallback, classesLabelCB, 0);
    XtAddCallback(classes_matrix, XmNdefaultActionCallback, classCB, (XtPointer)this);
    XtAddEventHandler(classes_matrix, ButtonPressMask, False, classAC, (XtPointer)this);
  }

  OksXm::set_close_cb(p_app_shell, (XtCallbackProc)MenuCB, (void *)idMenuFileExit);

  attach_bottom(idMainPanedWindow);

  show();

  refresh_classes();
  refresh_schemes();
  refresh_data();

    // adjust right column width

  XtAddCallback(data_matrix, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);
  XtAddCallback(classes_matrix, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);

  subscribe();

  if(no_msg_window == false) {
    msg_window = new OksXmMessageWindow("Oks Data Editor Message Log", this, msg_dlg_auto_scroll);

    if(restore_pos) {
      Dimension x(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_message_window_x_pos()));
      Dimension y(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_message_window_y_pos()));
      Widget msg_window_w = XtParent(msg_window->get_form_widget());
      XtVaSetValues(msg_window_w, XmNx, x, XmNy, y, NULL);
    }
  }

  setIcon(OksXm::create_color_pixmap(form, data_editor_about_xpm));

  p_select_data_files_panel->init();
  p_select_classes_panel->init();

    // register handlers for user's signals

  signal(SIGINT,signal_handler);
  signal(SIGTERM,signal_handler);

  if(static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_ext_modified_files_check_name()))) {
    start_check_updated();
  }

  if(static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_backup_modified_files_check_name()))) {
    start_check_need_backup();
  }

  XtAddEventHandler (p_app_shell, StructureNotifyMask, False, configureAC, NULL);
}


OksXmHelpWindow*
OksDataEditorMainDialog::get_help_window()
{
  if(!helpWindow) {
    helpWindow = new OksXmHelpWindow("Oks Data Editor Help", this);
  }

  return helpWindow;
}


OksDataEditorMainDialog::~OksDataEditorMainDialog()
{
  if(mainDlg == 0) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, "Oops, the OksDataEditorMainDialog was already destroyed"));
    return;
  }

  if(static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_restore_main_window_pos()))) {
    Dimension x(0), y(0);
    XtVaGetValues(p_app_shell, XmNx, &x, XmNy, &y, NULL);
    OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_main_window_x_pos(), static_cast<unsigned short>(x));
    OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_main_window_y_pos(), static_cast<unsigned short>(y));
    if(msg_window) {
      Widget msg_window_w = XtParent(msg_window->get_form_widget());
      XtVaGetValues(msg_window_w, XmNx, &x, XmNy, &y, NULL);
      OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_message_window_x_pos(), static_cast<unsigned short>(x));
      OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_message_window_y_pos(), static_cast<unsigned short>(y));
    }

    try {
      OksGC::Parameters::def_params().save();
    }
    catch(oks::FailedSaveGuiRcFile& ex) {
      report_exception("Save OKS Data Editor Resource File", ex);
    }
  }

  mainDlg = nullptr;

  stop_check_updated();

  if(get_verbose_mode()) {
    std::cout << "Exiting OKS data editor ...\n";
  }

  subscribe_create_object(0, 0);
  subscribe_delete_object(0, 0);
  subscribe_change_object(0, 0);

    // remove possible locks

  for(OksFile::Map::const_iterator i = data_files().begin(); i != data_files().end(); ++i) {
    if((*i).second->is_locked()) {
      if(get_silence_mode() == false) {
        std::cout << " * unlock file \'" << (*i).second->get_full_file_name() << "\'\n";
      }

      try {
        (*i).second->unlock();
      }
      catch(oks::exception& ex) {
        ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
      }
    }
  }

    // print out if profiling information is needed

  if(get_profiling_mode()) {
    if(OksProfiler * p = GetOksProfiler()) {
      std::cout << *p << std::endl;
    }
    else {
      std::cout << "The OKS profiler was now inited, run in debug mode" << std::endl;
    }
  }


    // if save-on-exit is needed

  if(msg_window) {
    delete msg_window;
    msg_window = 0;
  }


    // real exit

  exit(0);
}


void
OksDataEditorMainDialog::refresh_classes() const
{
  const char *item = OksXm::Matrix::get_selected_value(classes_matrix);

  if(item)
    XbaeMatrixDeselectAll(classes_matrix);	// it looks like it may crash without this

  XbaeMatrixDeleteRows(classes_matrix, 0, XbaeMatrixNumRows(classes_matrix));

  if(!classes().empty()) {
    String row[NumOfClassesMatrixColumns];
    size_t cnt = 0;

    for(OksClass::Map::const_iterator i = classes().begin(); i != classes().end(); ++i) {
      const OksClass * c = i->second;

      if(p_select_classes_panel->test_match(c->get_name()) == false) continue;

      if(cnt == 0) {
        XtVaSetValues(classes_matrix, XmNfill, false, NULL);
      }

      std::ostringstream s;
      s << c->number_of_objects();

      std::string buf = s.str();

      row[0] = const_cast<String>(c->get_name().c_str());
      row[1] = const_cast<String>(buf.c_str());
      row[2] = const_cast<String>(c->get_is_abstract() ? "Yes" : "No");
      row[3] = const_cast<String>("");  // fake cell

      XbaeMatrixAddRows(classes_matrix, cnt++, row, 0, 0, 1);
    }

    if(cnt == 0) {
      XtVaSetValues(classes_matrix, XmNfill, true, NULL);
    }
  }
  else {
    const_cast<OksDataEditorMainDialog *>(this)->close_all_data();
    refresh_data();
    XtVaSetValues(classes_matrix, XmNfill, true, NULL);
  }

  if(item)
    OksXm::Matrix::select_row(classes_matrix, item);

  OksXm::Matrix::adjust_column_widths(classes_matrix);
  OksXm::Matrix::set_visible_height(classes_matrix);
  OksXm::Matrix::set_widths(classes_matrix, 3);
  OksXm::Matrix::set_cell_background(classes_matrix, schema_list);

  if(!OksXm::Matrix::resort_rows(classes_matrix)) {
    OksXm::Matrix::sort_rows(classes_matrix, 0, OksXm::Matrix::SortByString);
  }
}

void
OksDataEditorMainDialog::refresh_schemes() const
{
  OksXm::AutoCstring item(OksXm::List::get_selected_value(schema_list));

  XmListDeleteAllItems(schema_list);

  if(!schema_files().empty()) {
    for(OksFile::Map::const_iterator i = schema_files().begin(); i != schema_files().end(); ++i)
      OksXm::List::add_row(schema_list, (*i).second->get_full_file_name().c_str());
  }

  if(item.get()) {
    OksXm::List::select_row(schema_list, item.get());
  }
}


void
OksDataEditorMainDialog::refresh_data() const
{
  const char *selection = OksXm::Matrix::get_selected_value(data_matrix);

  std::string s; // keeps copy of selected item: it will be destroyed
  if(selection) {
    s = selection;
    XbaeMatrixDeselectAll(data_matrix);	// it looks like it may crash without this
  }

  XbaeMatrixDeleteRows(data_matrix, 0, XbaeMatrixNumRows(data_matrix));
  XtVaSetValues(data_matrix, XmNfill, true, NULL);

  bool data_are_loaded = !data_files().empty();

  {
    for(std::list<Widget>::const_iterator i = p_edit_btns.begin(); i != p_edit_btns.end(); ++i) {
      XtSetSensitive(*i, data_are_loaded);
    }
  }

  if(data_are_loaded) {
    for(OksFile::Map::const_iterator i = data_files().begin(); i != data_files().end(); ++i) {
      if(p_select_data_files_panel->test_match(*i->first) == false) continue;
      append_data_file_row(i->second);
    }

    XtSetSensitive(classes_matrix, True);
  }
  else
    XtSetSensitive(classes_matrix, False);

	//
	// Do not allow matrix capture all screen's width
	// if filenames are too long (set 75% limit)
	//

  static Dimension max_width = (WidthOfScreen(XtScreen(data_matrix)) / 4) * 3;

  OksXm::Matrix::adjust_column_widths(data_matrix, max_width);
  OksXm::Matrix::set_visible_height(data_matrix);
  OksXm::Matrix::set_widths(data_matrix, 2);
  OksXm::Matrix::set_cell_background(data_matrix, schema_list);

  if(!OksXm::Matrix::resort_rows(data_matrix)) {
    OksXm::Matrix::sort_rows(data_matrix, 0, OksXm::Matrix::SortByString);
  }

  if(!s.empty())
    OksXm::Matrix::select_row(data_matrix, s.c_str());
}

void
OksDataEditorMainDialog::remove_dlg(OksDataEditorDialog *dlg)
{
  dialogs.remove(dlg);
  update_menu_bar();
}


OksDataEditorDataDialog *
OksDataEditorMainDialog::find_data_dlg(const OksFile * f)
{
  for(std::list<OksDataEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::DataDlg) {
      OksDataEditorDataDialog *dlg = static_cast<OksDataEditorDataDialog *>(oded);
      if(dlg->get_file() == f) return dlg;
    }
  }

  return 0;
}

OksDataEditorClassDialog *
OksDataEditorMainDialog::find_class_dlg(const OksClass * c)
{
  for(std::list<OksDataEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::ClassDlg) {
      OksDataEditorClassDialog *dlg = static_cast<OksDataEditorClassDialog *>(oded);
      if(dlg->get_class() == c) return dlg;
    }
  }
  	
  return 0;
}

OksDataEditorObjectDialog*
OksDataEditorMainDialog::find_object_dlg(const OksObject *o)
{
  for(std::list<OksDataEditorDialog *>::const_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::ObjectDlg) {
      OksDataEditorObjectDialog *dlg = static_cast<OksDataEditorObjectDialog *>(oded);
      if(dlg->getObject() == o) return dlg;
    }
  }

  return 0;
}

OksDataEditorObjectReferencedByDialog *
OksDataEditorMainDialog::find_object_referenced_by_dlg(const OksObject *o)
{
  for(std::list<OksDataEditorDialog *>::const_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::ObjectReferencedByDlg) {
      OksDataEditorObjectReferencedByDialog *dlg = static_cast<OksDataEditorObjectReferencedByDialog *>(oded);
      if(dlg->get_object() == o) return dlg;
    }
  }

  return 0;
}

OksDataEditorCommentDialog *
OksDataEditorMainDialog::find_comment_dlg(const oks::Comment * c)
{
  for(std::list<OksDataEditorDialog *>::const_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::CommentDlg) {
      OksDataEditorCommentDialog *dlg = static_cast<OksDataEditorCommentDialog *>(oded);
      if(dlg->p_comment == c) return dlg;
    }
  }

  return 0;
}


void
OksDataEditorMainDialog::raise_dlg(OksDialog * dlg) const
{
  XMapRaised(display(), XtWindow(XtParent(dlg->get_form_widget())));
}

OksDataEditorDataDialog*
OksDataEditorMainDialog::create_data_dlg(OksFile * f)
{
  OksDataEditorDataDialog * odedd = find_data_dlg(f);

  if(odedd) 
    raise_dlg(odedd);
  else {
    dialogs.push_back(odedd = new OksDataEditorDataDialog(f, this));
    update_menu_bar();
  }

  return odedd;
}

OksDataEditorClassDialog*
OksDataEditorMainDialog::create_class_dlg(const OksClass *c)
{  
  OksDataEditorClassDialog *odecd = find_class_dlg(c);

  if(odecd) 
    raise_dlg(odecd);
  else {
    dialogs.push_back(odecd = new OksDataEditorClassDialog((OksClass *)c, this));
    update_menu_bar();
  }
  
  return odecd;
}

OksDataEditorObjectDialog*
OksDataEditorMainDialog::create_object_dlg(const OksObject *o)
{
  OksDataEditorObjectDialog * odeod = find_object_dlg(o);

  if(odeod)
    raise_dlg(odeod);
  else {
    dialogs.push_back(odeod = new OksDataEditorObjectDialog((OksObject *)o, this));
    update_menu_bar();
  }

  return odeod;
}

OksDataEditorObjectReferencedByDialog*
OksDataEditorMainDialog::create_object_referenced_by_dlg(const OksObject *o)
{
  OksDataEditorObjectReferencedByDialog * dlg = find_object_referenced_by_dlg(o);

  if(dlg)
    raise_dlg(dlg);
  else {
    dialogs.push_back(dlg = new OksDataEditorObjectReferencedByDialog(o, this));
    update_menu_bar();
  }

  return dlg;
}

OksDataEditorQueryDialog*
OksDataEditorMainDialog::create_query_dlg(const OksClass *c, OksQuery *q)
{  
  OksDataEditorQueryDialog *odecd = new OksDataEditorQueryDialog((OksClass *)c, this, q);
  dialogs.push_back(odecd);

  update_menu_bar();

  return odecd;
}

void
OksDataEditorMainDialog::create_object_dlg(const OksClass *c)
{
  Arg           args[10];
  XmString      string = OksXm::create_string("Input object's ID:");
  XmString	string2 = OksXm::create_string("(empty string means anonymous object)");
  Widget        dialog;

  XtSetArg(args[0], XmNallowShellResize, true);
  //XtSetArg(args[1], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);

  dialog = XmCreatePromptDialog(get_form_widget(), (char *)"prompt dialog", args, 1);
  XtVaSetValues(XtParent(dialog), XmNtitle, "Input object's id:", NULL);
  XtVaSetValues(dialog,
	XmNselectionLabelString, string,
	XmNchildPlacement, XmPLACE_BELOW_SELECTION,
	NULL
  );

  XtSetArg(args[0], XmNlabelString, string2);
  XtManageChild(XmCreateLabel(dialog, (char *)"simple", args, 1));

  XtAddCallback(dialog, XmNokCallback, newObjectCB, (XtPointer)c);
  XtAddCallback(dialog, XmNcancelCallback, newObjectCB, (XtPointer)c);
  XtAddCallback(dialog, XmNhelpCallback, newObjectCB, (XtPointer)c);

  XtManageChild(dialog);
  XmStringFree(string);
  XmStringFree(string2);
}

OksDataEditorCommentDialog*
OksDataEditorMainDialog::create_comment_dlg(OksFile * f, const std::string& id, oks::Comment * c)
{
  OksDataEditorCommentDialog * cdeod = find_comment_dlg(c);

  if(cdeod)
    raise_dlg(cdeod);
  else {
    dialogs.push_back(cdeod = new OksDataEditorCommentDialog(this, f, id, c));
    update_menu_bar();
  }

  return cdeod;
}

void
OksDataEditorMainDialog::close_data_dialogs()
{
  for(std::list<OksDataEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end();) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::DataDlg) {
      OksDataEditorDataDialog *dlg = static_cast<OksDataEditorDataDialog *>(oded);

      i = dialogs.erase(i);

      delete dlg;
    }
    else
      ++i;
  }
}


void
OksDataEditorMainDialog::newObjectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksClass * c = (OksClass *)client_data;
    OksXm::AutoCstring value(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));
    try {
      mainDlg->create_object_dlg(
        (value.is_non_empty())
          ? new OksObject(c, value.get())
          : new OksObject(c)
      );
    }
    catch(oks::exception& ex) {
      report_exception("New Object", ex);
    }
  }
  else if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_HELP) {
    mainDlg->show_help_window("ObjectCreation.html");
  }
}

bool
OksDataEditorMainDialog::save_all_files_cb(const std::list<OksFile *>& files, OksDataEditorMainDialog * dlg, const char * author, const char * text)
{
  bool status(true);
  for(std::list<OksFile *>::const_iterator i = files.begin(); i != files.end(); ++i) {
    try {
      OksFile * f(*i);
      if(text && *text && author && *author) f->add_comment(text, author);
      dlg->save_data(f);
      dlg->update_data_file_row(f);
      dlg->remove_backup(f);
    }
    catch (oks::exception & ex) {
      report_exception("Save All Updated Files", ex);
      status = false;
    }
  }

  return status;
}


bool OksDataEditorMainDialog::commit_all_files_cb(const std::set<std::string>& files, OksDataEditorMainDialog * dlg, const char * author, const char * text)
{
  try {
    std::set<Widget> widgets;
    dlg->get_all_dialogs(widgets);
    OksXm::ShowWatchCursor _wc(XtDisplay(dlg->app_shell()), widgets);

    dlg->commit_repository(text);
    dlg->p_last_repository_version = dlg->get_repository_version();
  }
  catch (oks::exception & ex) {
    report_exception("Commit All User Repository Files", ex);
    return false;
  }

  return true;
}


void
OksDataEditorMainDialog::MenuCB (Widget w, XtPointer client_data, XtPointer call_data) 
{
  switch (reinterpret_cast<long>(client_data)) {
    case idMenuFileExit:
      OksDataEditorMainDialog::exit_app("Exit");
      break;

    case idMenuEditFind:
      mainDlg->create_find_replace_dialog(OksDataEditorReplaceDialog::find);
      break;

    case idMenuEditReplace:
      mainDlg->create_find_replace_dialog(OksDataEditorReplaceDialog::replace);
      break;
 
    case idMenuEditRefreshMainWindow:
      mainDlg->refresh_main_window();
      break;

    case idMenuFileSaveAllModified:
      {
      std::list<OksFile *> files;

      for (const auto& i : mainDlg->data_files())
        if (i.second->is_updated())
          files.push_back(i.second);

      if (!files.empty())
        new OksDataEditorFilesDialog(
          mainDlg, files, "Save Modified Files",
          "The following files have been modified.\nWhen saved, a comment can be added to all files\n(fill \"Author names\" and \"Comment\" text below).",
          save_all_xpm,
          "Comment",
          0,
          true,
          "Save",
          save_all_files_cb
        );
      break;
      }

    case idMenuFileShowVersions:
      if (!mainDlg->get_user_repository_root().empty())
        mainDlg->show_versions_query_dialog();
      break;


    case idMenuFileUpdateRepository: //{
      if (!mainDlg->get_user_repository_root().empty())
        mainDlg->show_new_repository_versions_dlg(mainDlg->get_repository_version());
      break;

    case idMenuFileCommitRepository:
      if (!mainDlg->get_user_repository_root().empty()) {
          std::set<std::string> updated, added, removed;

          try
            {
              mainDlg->get_updated_repository_files(updated, added, removed);
            }
          catch(const oks::exception& ex)
            {
              report_exception("Get repository status", ex);
            }

          std::set<std::string> files;

          for(const auto&x : added)
            files.insert(std::string("(add) ") + x);

          for(const auto&x : removed)
            files.insert(std::string("(remove) ") + x);

          for(const auto&x : updated)
            files.insert(std::string("(update) ") + x);

          if(!files.empty())
            new OksDataEditorFilesDialog(
              mainDlg, files, "Commit User Repository Files",
              "The following files are stored in user repository\nand are different from oks server repository.\nOn commit a comment has to be added.\nFill \"GIT Comment\" text below, avoid single quotes.\n",
              commit_xpm,
              "GIT Comment",
              mainDlg->get_commit_comments(added, updated).c_str(),
              false,
              "Commit",
              commit_all_files_cb
            );
          else
            std::cout << "[COMMIT REPOSITORY]: there are no updated files stored in repository." << std::endl;
      }
      else {
        std::cout << "[COMMIT REPOSITORY]: there are no repository files." << std::endl;
      }
      break;

    case idMenuFileCheckConsistency:
      mainDlg->check_objects_and_files();
      break;

    case idMenuEdit:
      mainDlg->add_window(new G_Window((G_WindowInfo *)OksXm::get_user_data(w), mainDlg));
      mainDlg->update_menu_bar();
      break;

    case idMenuOptionSilenceMode:
      mainDlg->set_silence_mode(
	( ((XmToggleButtonCallbackStruct *)call_data)->set ? True : False )
      );
      break;

    case idMenuOptionVerboseMode:
      mainDlg->set_verbose_mode(
	( ((XmToggleButtonCallbackStruct *)call_data)->set ? True : False )
      );
      break;

    case idMenuOptionProfilingMode:
      mainDlg->set_profiling_mode(
	( ((XmToggleButtonCallbackStruct *)call_data)->set ? True : False )
      );
      break;

    case idMenuOptionCheckFileMode:
      if(((XmToggleButtonCallbackStruct *)call_data)->set) {
	mainDlg->start_check_updated();
	XtSetSensitive(mainDlg->checkPeriodBtn, true);
        OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_ext_modified_files_check_name(), static_cast<unsigned short>(true));
      }
      else {
	mainDlg->stop_check_updated();
	XtSetSensitive(mainDlg->checkPeriodBtn, false);
        OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_ext_modified_files_check_name(), static_cast<unsigned short>(false));
      }

      break;

    case idMenuOptionCheckFilePeriod: {
      mainDlg->stop_check_updated();

      std::ostringstream ss;
      ss << OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_ext_modified_files_value_name());

      std::string buf = ss.str();
      std::string s = OksXm::ask_name(
        mainDlg->get_form_widget(),
        "Period of check externally modified files",
        "Input period (from 5 to 3600 seconds):",
        buf.c_str()
      );

      if(s.length()) {
        int check_updated_timeout = atoi(s.c_str());

        if(check_updated_timeout < 5) {
          std::ostringstream text;
          text << "WARNING: period " << check_updated_timeout << " is too small.\n";
          check_updated_timeout = 5;
          text << "Set the period equal " << check_updated_timeout << " seconds.\n";
          report_error("Set Check File Period", text.str().c_str());
        }
        else if(check_updated_timeout > 3600) {
          std::ostringstream text;
          text << "WARNING: period " << check_updated_timeout << " is too big.\n";
          check_updated_timeout = 3600;
          text << "Set the period equal " << check_updated_timeout << " seconds.\n";
          report_error("Set Check File Period", text.str().c_str());
	}

        OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_ext_modified_files_value_name(), static_cast<unsigned short>(check_updated_timeout));
      }

      mainDlg->start_check_updated();

      break; }

    case idMenuOptionCheckNeedBackupMode:
      if(((XmToggleButtonCallbackStruct *)call_data)->set) {
	mainDlg->start_check_need_backup();
	XtSetSensitive(mainDlg->backupPeriodBtn, true);
        OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_backup_modified_files_check_name(), static_cast<unsigned short>(true));
      }
      else {
	mainDlg->stop_check_need_backup();
	XtSetSensitive(mainDlg->backupPeriodBtn, false);
        OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_backup_modified_files_check_name(), static_cast<unsigned short>(false));
      }

      break;

    case idMenuOptionCheckNeedBackupPeriod: {
      mainDlg->stop_check_need_backup();

      std::ostringstream ss;
      ss << OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_backup_modified_files_value_name());

      std::string buf = ss.str();
      std::string s = OksXm::ask_name(
        mainDlg->get_form_widget(),
        "Time interval to save recovery files for modified ones",
        "Input period (from 5 to 3600 seconds):",
        buf.c_str()
      );

      if(s.length()) {
        int check_need_backup_timeout = atoi(s.c_str());

        if(check_need_backup_timeout < 5) {
          std::ostringstream text;
          text << "WARNING: period " << check_need_backup_timeout << " is too small.\n";
          check_need_backup_timeout = 5;
          text << "Set the period equal " << check_need_backup_timeout << " seconds.\n";
          report_error("Set Backup Period", text.str().c_str());
        }
        else if(check_need_backup_timeout > 3600) {
          std::ostringstream text;
          text << "WARNING: period " << check_need_backup_timeout << " is too big.\n";
          check_need_backup_timeout = 3600;
          text << "Set the period equal " << check_need_backup_timeout << " seconds.\n";
          report_error("Set Backup Period", text.str().c_str());
	}

        OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_backup_modified_files_value_name(), static_cast<unsigned short>(check_need_backup_timeout));
      }

      mainDlg->start_check_need_backup();

      break; }

    case idMenuOptionAskCommentOnSave:
      OksGC::Parameters::def_params().set_generic_parameter(
        OksParameterNames::get_ask_comment_name(), (((XmToggleButtonCallbackStruct *)call_data)->set)
      );
      break;

    case idMenuOptionRestorePosition:
      OksGC::Parameters::def_params().set_generic_parameter(
        OksParameterNames::get_restore_main_window_pos(), (((XmToggleButtonCallbackStruct *)call_data)->set)
      );
      break;

    case idMenuOptionSaveOptions:
      try {
        OksGC::Parameters::def_params().save();
      }
      catch(oks::FailedSaveGuiRcFile& ex) {
        report_exception("Save OKS Data Editor Resource File", ex);
      }
      break;

    case idMenuWindowHelp:
      mainDlg->get_help_window()->showHelpWindow();
      break;

    case idMenuWindowMessages:
      mainDlg->get_msg_window()->show_window();
      break;

    case idMenuHelpGeneral:
      mainDlg->show_help_window("OksDataEditor.html");
      break;

    case idMenuHelpIndex:
      mainDlg->show_help_window("Index.html");
      break;

    case idMenuHelpMainWindow:
      mainDlg->show_help_window("MainWindow.html");
      break;

    case idMenuHelpMessageLog:
      mainDlg->show_help_window("MessageLogWindow.html");
      break;

    case idMenuHelpDataFileWindow:
      mainDlg->show_help_window("DataFileWindow.html");
      break;

    case idMenuHelpClassWindow:
      mainDlg->show_help_window("ClassWindow.html");
      break;

    case idMenuHelpObjectWindow:
      mainDlg->show_help_window("ObjectWindow.html");
      break;

    case idMenuHelpQueryWindow:
      mainDlg->show_help_window("QueryWindow.html");
      break;

    case idMenuHelpGraphicalWindow:
      mainDlg->show_help_window("GraphicalWindow.html");
      break;

    case idMenuHelpReplaceWindow:
      mainDlg->show_help_window("ReplaceWindow.html");
      break;

    case idMenuHelpAbout:
      if(!about_dialog) {
        about_dialog = OksXm::create_dlg_with_pixmap(w, "About", data_editor_about_xpm, (
            std::string("OKS Data Editor (OKS version ") + OksKernel::GetVersion() + ")\n\n"
            "The editor provides an interactive GUI to graphically\n"
            "manipulate objects stored in the OKS data files.\n\n"
            "Copyright (C) 2001-2022 CERN for the benefit of the ATLAS collaboration.\n"
            "Licensed under the Apache License, version 2.0.\n"
            "https://www.apache.org/licenses/LICENSE-2.0\n\n"
            "Author: Igor Soloviev <Igor.Soloviev@cern.ch>\n"
            ).c_str(), "Ok", 0, 0, 0, 0);

        Widget work_area = (Widget)OksXm::get_user_data(about_dialog);


          // Xbae matrix

        XtManageChild(XmCreateSeparator(work_area, (char *)"", 0, 0));

        OksXm::RowColumn::create_pixmap_and_text(
          work_area,
          xbae_xpm,
          "Uses free XbaeMatrix widget version 4.6.2.\n"
          "Copyright (c) 1991, 1992 Bell Communications Research, Inc. (Bellcore)\n"
          "Copyright (c) 1995-97 Andrew Lister\nAll Rights Reserved."
        );


          // XmHTML widget

        XtManageChild(XmCreateSeparator(work_area, (char *)"", 0, 0));

        OksXm::RowColumn::create_pixmap_and_text(
          work_area,
          xmhtml_xpm,
          "Uses free XmHTML widget beta version 1.1.4.\n"
          "(C) Copyright 1993-1997 Ripley Software Development."
        );


          // XmTree widget

        XtManageChild(XmCreateSeparator(work_area, (char *)"", 0, 0));

        OksXm::RowColumn::create_pixmap_and_text(
          work_area,
          tree_xpm,
          "Uses free XmTree widget version 1.0 code example from\n"
          "\"The X Window System: Programming and Applications with Xt\".\n"
          "Second OSF/Motif Edition by Douglas Young, Prentice Hall, 1994\n"
          "Copyright (c) 1994 by Prentice Hall, All Rights Reserved."
        );
      }

      XtManageChild(about_dialog);
      break;
  }
}


void
OksDataEditorMainDialog::fill_menu_bar (Widget menu_bar)
{
  Widget menu_pane = 0;	


	//
	// File menu
	//

  menu_pane = XmCreatePulldownMenu (menu_bar, (char *)"File", 0, 0);

  OksXm::MenuBar::add_push_button(menu_pane, "Save All Updated", idMenuFileSaveAllModified, MenuCB);
  {
    Widget pb_w = OksXm::MenuBar::add_push_button(menu_pane, "Show Repository Versions", idMenuFileShowVersions, MenuCB);
    if(OksKernel::get_repository_root().empty()) { XtSetSensitive(pb_w, false); }
  }

  OksXm::MenuBar::add_push_button(menu_pane, "Update Repository", idMenuFileUpdateRepository, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Commit Repository", idMenuFileCommitRepository, MenuCB);
  OksXm::MenuBar::add_separator(menu_pane);
  OksXm::MenuBar::add_push_button(menu_pane, "Check consistency", idMenuFileCheckConsistency, MenuCB);
  OksXm::MenuBar::add_separator(menu_pane);
  OksXm::MenuBar::add_push_button(menu_pane, "Exit", idMenuFileExit, MenuCB);

  OksXm::MenuBar::add_cascade_button(menu_bar, menu_pane, "File", 'F');


	//
	// Edit menu
	//

  edit_pane = XmCreatePulldownMenu (menu_bar, (char *)"Edit", 0, 0);

  OksXm::MenuBar::add_push_button(edit_pane, "Find", idMenuEditFind, MenuCB);
  OksXm::MenuBar::add_push_button(edit_pane, "Replace", idMenuEditReplace, MenuCB);
  OksXm::MenuBar::add_separator(edit_pane);
  OksXm::MenuBar::add_push_button(edit_pane, "Refresh", idMenuEditRefreshMainWindow, MenuCB);
  OksXm::MenuBar::add_cascade_button(menu_bar, edit_pane, "Edit", 'E');


	//
	// Options menu
	//

  menu_pane = XmCreatePulldownMenu (menu_bar, (char *)"Options", 0, 0);

  OksXm::MenuBar::add_toggle_button(menu_pane, "Silence mode", idMenuOptionSilenceMode, get_silence_mode(), MenuCB);
  OksXm::MenuBar::add_toggle_button(menu_pane, "Verbose mode", idMenuOptionVerboseMode, get_verbose_mode(), MenuCB);
  OksXm::MenuBar::add_toggle_button(menu_pane, "Profiling mode", idMenuOptionProfilingMode, get_profiling_mode(), MenuCB);

  OksXm::MenuBar::add_separator(menu_pane);

  {
    bool check_ext_modified_files(static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_ext_modified_files_check_name())));
    OksXm::MenuBar::add_toggle_button(menu_pane, "Check externally modified files", idMenuOptionCheckFileMode, check_ext_modified_files, MenuCB);
    checkPeriodBtn = OksXm::MenuBar::add_push_button(menu_pane, "Set period of check", idMenuOptionCheckFilePeriod, MenuCB);
    if(check_ext_modified_files == false) XtSetSensitive(checkPeriodBtn, false);
  }

  OksXm::MenuBar::add_separator(menu_pane);

  {
    bool backup_modified_files(static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_backup_modified_files_check_name())));
    OksXm::MenuBar::add_toggle_button(menu_pane, "Recovery mode for modified files", idMenuOptionCheckNeedBackupMode, backup_modified_files, MenuCB);
    backupPeriodBtn = OksXm::MenuBar::add_push_button(menu_pane, "Set backup period", idMenuOptionCheckNeedBackupPeriod, MenuCB);
    if(backup_modified_files == false) XtSetSensitive(backupPeriodBtn, false);
  }

  OksXm::MenuBar::add_separator(menu_pane);

  OksXm::MenuBar::add_toggle_button(
    menu_pane, "Ask comment on file save", idMenuOptionAskCommentOnSave,
    static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_ask_comment_name())),
    MenuCB
  );

  OksXm::MenuBar::add_separator(menu_pane);

  OksXm::MenuBar::add_toggle_button(
    menu_pane, "Restore position on restart", idMenuOptionRestorePosition,
    static_cast<bool>(OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_restore_main_window_pos())),
    MenuCB
  );


  OksXm::MenuBar::add_separator(menu_pane);

  OksXm::MenuBar::add_push_button(menu_pane, "Save Options", idMenuOptionSaveOptions, MenuCB);

  OksXm::MenuBar::add_cascade_button(menu_bar, menu_pane, "Options", 'O');


	//
	// Window menu
	//

  windowsPane = XmCreatePulldownMenu (menu_bar, (char *)"Windows", 0, 0);

  start_menu_bar_pane();

  OksXm::MenuBar::add_cascade_button(menu_bar, windowsPane, "Windows", 'W');


	//
	// Help menu
	//

  menu_pane = XmCreatePulldownMenu (menu_bar, (char *)"Help", 0, 0);

  OksXm::MenuBar::add_push_button(menu_pane, "General", idMenuHelpGeneral, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Index", idMenuHelpIndex, MenuCB);

  XtManageChild(XmCreateSeparator(menu_pane, (char *)"", 0, 0));

  OksXm::MenuBar::add_push_button(menu_pane, "Main Window", idMenuHelpMainWindow, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Message Log", idMenuHelpMessageLog, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Data File Window", idMenuHelpDataFileWindow, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Class Window", idMenuHelpClassWindow, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Object Window", idMenuHelpObjectWindow, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Query Window", idMenuHelpQueryWindow, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Graphical Window", idMenuHelpGraphicalWindow, MenuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Find/Replace Window", idMenuHelpReplaceWindow, MenuCB);

  XtManageChild(XmCreateSeparator(menu_pane, (char *)"", 0, 0));

  OksXm::MenuBar::add_push_button(menu_pane, "About...", idMenuHelpAbout, MenuCB);

  XtVaSetValues(menu_bar,
    XmNmenuHelpWidget, OksXm::MenuBar::add_cascade_button(menu_bar, menu_pane, "Help", 'H'),
    NULL
  );
}


void
OksDataEditorMainDialog::raiseCB(Widget w, XtPointer d, XtPointer)
{
  XMapRaised(XtDisplay(w), XtWindow((Widget)d));
}

void
OksDataEditorMainDialog::add_window_button(Widget parent, Widget form)
{
  char	 *title;
  Widget shell = XtParent(form);

  XtVaGetValues(shell, XmNtitle, &title, NULL);
  Widget w = XmCreatePushButton(parent, title, 0, 0);
  XtAddCallback(w, XmNactivateCallback, raiseCB, (XtPointer)shell);
  XtManageChild(w);
}

void
OksDataEditorMainDialog::set_edit_menu()
{
  if(!p_win_infos.empty()) {
    OksXm::MenuBar::add_separator(edit_pane);

    for(std::list<G_WindowInfo *>::iterator i = p_win_infos.begin(); i != p_win_infos.end(); ++i) {
      G_WindowInfo * w_info = *i;
      Widget edit_btn = OksXm::MenuBar::add_push_button(edit_pane, w_info->title().c_str(), idMenuEdit, MenuCB);
      OksXm::set_user_data(edit_btn, (XtPointer)w_info);
      XtSetSensitive(edit_btn, false);
      p_edit_btns.push_back(edit_btn);
    }
  }
}

void
OksDataEditorMainDialog::start_menu_bar_pane() const
{
  OksXm::MenuBar::add_push_button(windowsPane, "Help", idMenuWindowHelp, MenuCB);

  Widget w = OksXm::MenuBar::add_push_button(windowsPane, "Message Log", idMenuWindowMessages, MenuCB);

  if(no_msg_window == true) XtSetSensitive(w, false);
}

void
OksDataEditorMainDialog::update_menu_bar()
{
  if(p_skip_menu_update) return;


    // destroy present menu ites except "Help" and "Message Log" ones
  {
    WidgetList  items;
    Cardinal    nItems;
    int         n;

    XtVaGetValues(windowsPane, XmNchildren, &items, XmNnumChildren, &nItems, NULL);

    for(n=nItems-1; n >= 2; n--) {
      XtUnmanageChild(items[n]);
      XtDestroyWidget(items[n]);
    }
  }

  {
    std::vector<OksDialog *> udialogs {findDialog, replaceDialog, p_versions_query_dlg, p_new_repository_versions_dlg, p_archived_repository_versions_dlg};

    for (auto& x : udialogs)
      if (x)
        {
          XtManageChild(XmCreateSeparator(windowsPane, (char *) "", 0, 0));
          add_window_button(windowsPane, x->get_form_widget());
        }
  }

  if(!dialogs.empty()) {
    OksDataEditorDialog::Type types[] = {
      OksDataEditorDialog::DataDlg,
      OksDataEditorDialog::ClassDlg,
      OksDataEditorDialog::ObjectDlg,
      OksDataEditorDialog::QueryDlg,
      OksDataEditorDialog::CommentDlg,
      OksDataEditorDialog::FindReplaceResultDlg,
      OksDataEditorDialog::ObjectReferencedByDlg
    };

    for(size_t j = 0; j < sizeof(types)/sizeof(OksDataEditorDialog::Type); ++j) {
      bool first = true;

      for(std::list<OksDataEditorDialog *>::const_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
        if((*i)->type() == types[j]) {
	  if(first == true) {
	    XtManageChild(XmCreateSeparator(windowsPane, (char *)"", 0, 0));
	    first = false;
	  }

	  add_window_button(windowsPane, (*i)->get_form_widget());
        }
      }
    }
  }

  if(!p_windows.empty()) {
    XtManageChild(XmCreateSeparator(windowsPane, (char *)"", 0, 0));

    std::set<std::string, std::less<std::string> > titles;
	
    for(std::list<G_Window *>::const_iterator i = p_windows.begin(); i != p_windows.end();++i) {
      G_Window *win = *i;
      std::string win_title = win->get_title();
      
      if(titles.find(win_title) != titles.end()) {
        size_t title_count = 2L;

	while(true) {
	  std::ostringstream s;
          s << win_title << " : " << title_count++;

          std::string buf = s.str();
	  
	  if(titles.find(buf) == titles.end()) {
	    win_title = buf;
	    break;
	  }
	}
      }
      
      titles.insert(win_title);
      win->setTitle(win_title.c_str());
      OksXm::MenuBar::add_push_button(windowsPane, win_title.c_str(), reinterpret_cast<long>(XtParent(win->get_form_widget())), raiseCB);
    }
  }
}


void
OksDataEditorMainDialog::askUpdatedCB(Widget, XtPointer, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    done_with_dialog = ReloadFile;
  }
  else if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_CANCEL) {
    done_with_dialog = SaveFile;
  }
  else {
    done_with_dialog = CancelExit;
  }
}


void
OksDataEditorMainDialog::saveChangedCB(Widget, XtPointer client_data, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    try {
      OksFile * file_h = reinterpret_cast<OksFile *>(client_data);
      mainDlg->comment(file_h);
      mainDlg->save_data(file_h);
      mainDlg->update_data_file_row(file_h);
      mainDlg->remove_backup(file_h);
      done_with_dialog = SaveFile;
    }
    catch (oks::exception & ex) {
      report_exception("Save Modified File", ex);
      done_with_dialog = NoAnswer;
    }
  }
  else if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_CANCEL) {
    done_with_dialog = SkipFile;
  }
  else
    done_with_dialog = CancelExit;
}


enum ConfirmDialogState { CD_NO_ANSWER, CD_OK, CD_CANCEL } confirm_dialog_result;

static void ConfirmDialogCB (Widget, XtPointer client_data, XtPointer)
{
  confirm_dialog_result = (client_data ? CD_OK : CD_CANCEL);
}

bool
OksDataEditorMainDialog::confirm_dialog(const char * title, const char * text, const char ** image)
{
  Widget w = OksXm::create_dlg_with_pixmap(menuBar, title, image, text, "OK", "Cancel", 0, 0, 0);

  XtVaSetValues(w, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

  XtAddCallback(w, XmNokCallback, ConfirmDialogCB, (XtPointer)this);
  XtAddCallback(w, XmNcancelCallback, ConfirmDialogCB, 0);
  OksXm::set_close_cb(XtParent(w), ConfirmDialogCB, 0);

  XtManageChild(w);

  confirm_dialog_result = CD_NO_ANSWER;

  while(confirm_dialog_result == CD_NO_ANSWER)
    XtAppProcessEvent(XtWidgetToApplicationContext(w), XtIMAll);

  XtDestroyWidget(XtParent(w));

  return (confirm_dialog_result == CD_OK);
}

void
OksDataEditorMainDialog::cancelChangedCB(Widget, XtPointer, XtPointer)
{
  done_with_dialog = CancelExit;
}

OksDataEditorMainDialog::DialogReturn
OksDataEditorMainDialog::file_action_dialog(
  const char * text, OksFile * file_h, const char ** image,
  const char * btn_1, XtCallbackProc cb_1,
  const char * btn_2, XtCallbackProc cb_2,
  const char * btn_3, XtCallbackProc cb_3
)
{
  Widget w = OksXm::create_dlg_with_pixmap(menuBar, "Save Changes", image, text, btn_1, btn_2, btn_3, 0, 0);

  XtVaSetValues(w, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);


    // OK callback     ---> ACTION 1
    // Cancel callback ---> ACTION 2
    // Help callback   ---> CLOSE, CANCEL, DISMISS

  if(cb_1) XtAddCallback(w, XmNokCallback, cb_1, (XtPointer)file_h);
  if(cb_2) XtAddCallback(w, XmNcancelCallback, cb_2, (XtPointer)file_h);
  if(cb_3) {
    XtAddCallback(XmSelectionBoxGetChild(w, XmDIALOG_HELP_BUTTON), XmNactivateCallback, cb_3, 0);
    OksXm::set_close_cb(XtParent(w), cb_3, 0);
  }

  XtManageChild(w);

  done_with_dialog = NoAnswer;

  while(done_with_dialog == NoAnswer)
    XtAppProcessEvent(XtWidgetToApplicationContext(w), XtIMAll);

  XtDestroyWidget(XtParent(w));

  return done_with_dialog;
}



OksDataEditorMainDialog::DialogReturn
OksDataEditorMainDialog::confirm_changes(const char *action, OksFile * file_h)
{
  if(!file_h->is_updated()) return OksDataEditorMainDialog::NonStop;

  std::string s("Save data file \"");
  s += file_h->get_full_file_name();
  s += "\" before ";
  s += action;
  s += '?';

  return file_action_dialog(
    s.c_str(), file_h, exit_xpm,
    "Save", saveChangedCB,
    "Skip", saveChangedCB,
    "Cancel", cancelChangedCB
  );
}


static std::string
get_file_status(OksFile * fp)
{
  std::string status;
  std::string lock_info;

  if(fp->is_updated()) {
    status = "updated";
  }
  else if(fp->is_locked()) {
    status = "locked";
  }
  else if(fp->get_lock_string(lock_info)) {
    status = std::string("locked by ") + lock_info;
  }
  else {
    status = "";
  }

  return status;
}


void
OksDataEditorMainDialog::fill_data_file_row(OksFile * fp, String *row) const
{
  static std::string status; // is static since it's c-string is returned
  status = get_file_status(fp);

  const char * rep_text = "none";
  bool allowed;

  if(!get_user_repository_root().empty())
    {
      std::map<const OksFile *, bool>::const_iterator ia = p_am_rights_cache.find(fp);

      if(ia != p_am_rights_cache.end())
        {
          allowed = ia->second;
        }
      else
        {
          try
            {
              allowed = oks::access::is_writable(*fp);
              (const_cast<OksDataEditorMainDialog *>(this))->p_am_rights_cache[fp] = allowed;
            }
          catch (std::exception& ex)
            {
              ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
              allowed = false;
            }
        }

      rep_text = (allowed ? "remote (RW)" : "remote (R)");
    }

  row[0] = const_cast<String>(fp->get_full_file_name().c_str());
  row[1] = const_cast<String>(OksKernel::check_read_only(fp) ? "read-only" : (get_active_data() == fp ? "active" : "read-write"));
  row[2] = const_cast<String>(rep_text);
  row[3] = const_cast<String>(status.c_str());
}

void
OksDataEditorMainDialog::set_data_file_row_colors(String * row, int pos) const
{
  if(String access = row[1]) {
    XbaeMatrixSetCellColor(
      data_matrix, pos, 1,
      (!strcmp(access, "read-only")
        ? p_red_pixel
	: (!strcmp(access, "read-write") ? p_blue_pixel : p_green_pixel))
    );
  }

  if(String source = row[2]) {
    XbaeMatrixSetCellColor(
      data_matrix, pos, 2,
      (!strcmp(source, "none")
        ? p_blue_pixel
	: (strstr(source, "(RW)") ? p_green_pixel : p_red_pixel))
    );
  }

  if(row[3] && *row[3]) {
    XbaeMatrixSetCellColor(
      data_matrix, pos, 3,
      (
        !strcmp(row[3], "locked")
	  ? p_green_pixel
	  : (!strcmp(row[3], "updated") ? p_blue_pixel : p_red_pixel)
      )
    );
  }
}


void
OksDataEditorMainDialog::update_data_file_row(OksFile * pf) const
{
  int rowsNumber = XbaeMatrixNumRows(data_matrix);
  int pos;

  for(pos=0; pos<rowsNumber; pos++) {
    if(pf == (OksFile *)XbaeMatrixGetRowUserData(data_matrix, pos)) break;
  }
  
  if(pos >= rowsNumber) return;

  String row[NumOfDataFilesMatrixColumns];

  fill_data_file_row(pf, row);

  for(size_t i=0; i<NumOfDataFilesMatrixColumns; ++i)
    XbaeMatrixSetCell(data_matrix, pos, i, row[i]);
    
  set_data_file_row_colors(row, pos);
}


void
OksDataEditorMainDialog::append_data_file_row(OksFile * fp) const
{
    // set XmNfill to false if it was true
    // (this happens when add first row)
  {
    Boolean fill;
    XtVaGetValues(data_matrix, XmNfill, &fill, NULL);

    if(fill == true) {
      XtVaSetValues(data_matrix, XmNfill, false, NULL);
    }
  }

  String row[NumOfDataFilesMatrixColumns];
  fill_data_file_row(fp, row);
  int num_of_rows = XbaeMatrixNumRows(data_matrix);
  XbaeMatrixAddRows(data_matrix, num_of_rows, row, 0, 0, 1);
  XbaeMatrixSetRowUserData(data_matrix, num_of_rows, (XtPointer)fp);
  set_data_file_row_colors(row, num_of_rows);
}


void
OksDataEditorMainDialog::refresh_class_dialogs()
{
  for(std::list<OksDataEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksDataEditorDialog *oded = *i;
    if(oded->type() == OksDataEditorDialog::ClassDlg) {
      (static_cast<OksDataEditorClassDialog *>(oded))->refresh();
    }
  }
}


void
OksDataEditorMainDialog::create_find_replace_dialog(OksDataEditorReplaceDialog::ActionType a_type)
{
  OksDataEditorReplaceDialog *dlg = (
    (a_type == OksDataEditorReplaceDialog::find) ? findDialog : replaceDialog
  );

  if(dlg) {
    raise_dlg(dlg);
  }
  else {
    ((a_type == OksDataEditorReplaceDialog::find) ? findDialog : replaceDialog) = new OksDataEditorReplaceDialog(this, a_type);
    update_menu_bar();
  }
}

void
OksDataEditorMainDialog::show_versions_query_dialog()
{
  if(p_versions_query_dlg)
    raise_dlg(p_versions_query_dlg);
  else
    {
      p_versions_query_dlg = new OksDataEditorVersionsQueryDialog(this);
      update_menu_bar();
    }
}

void
OksDataEditorMainDialog::show_new_repository_versions_dlg(std::vector<OksRepositoryVersion> versions, bool add)
{
  if(p_new_repository_versions_dlg)
    {
      raise_dlg(p_new_repository_versions_dlg);
      if(add)
        p_new_repository_versions_dlg->add(versions);
      else
        p_new_repository_versions_dlg->replace(versions);
    }
  else
    {
      p_new_repository_versions_dlg = new OksDataEditorRepositoryVersionsDialog(versions, this, false);
      update_menu_bar();
    }
}

void
OksDataEditorMainDialog::show_archived_repository_versions_dlg(std::vector<OksRepositoryVersion> versions)
{
  if(p_archived_repository_versions_dlg)
    {
      raise_dlg(p_archived_repository_versions_dlg);
      p_archived_repository_versions_dlg->replace(versions);
    }
  else
    {
      p_archived_repository_versions_dlg = new OksDataEditorRepositoryVersionsDialog(versions, this, true);
      update_menu_bar();
    }
}


    //
    // The method returns true if there is active data file
    // and it has read-write access
    //

bool
OksDataEditorMainDialog::is_valid_active_data()
{
  OksFile * f = mainDlg->get_active_data();
  
  if(f) {
    return !(f->is_read_only());
  }

  return false;
}


void
OksDataEditorMainDialog::add_object_to_data_dlg(OksFile * f)
{
  if(OksDataEditorDataDialog * osesd = mainDlg->find_data_dlg(f)) {
    osesd->refresh();
  }

  update_data_file_row(f);
}


void
OksDataEditorMainDialog::remove_object_from_data_dlg(const OksObject * o, OksFile * f)
{
  if(OksDataEditorDataDialog * osesd = mainDlg->find_data_dlg(f)) {
    osesd->remove_object(o);
  }

  update_data_file_row(f);
}


void
OksDataEditorMainDialog::check_need_backup_files_cb(XtPointer call_data, XtIntervalId*)
{
  OksDataEditorMainDialog * dlg = reinterpret_cast<OksDataEditorMainDialog *>(call_data);

  for(OksFile::Map::const_iterator i = dlg->data_files().begin(); i != dlg->data_files().end(); ++i) {
    if(i->second->is_updated()) {
      try {
        dlg->backup_data(i->second, ".saved");
        ERS_DEBUG( 2 , "made backup of \'" << *(i->first) << '\'');
      }
      catch(oks::exception& ex) {
        std::ostringstream text;
        text << "failed to make a backup of \'" << *(i->first) << "\':\n" << ex.what();
        ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
      }
    }
  }

  dlg->start_check_need_backup();
}


void
OksDataEditorMainDialog::show_new_repository_versions_dlg(const std::string& version)
{
  if (p_last_repository_version.empty())
    p_last_repository_version = get_repository_version();

  std::vector<OksRepositoryVersion> versions = get_repository_versions_by_hash(true, !version.empty() ? version : p_last_repository_version);

  if (!versions.empty())
    {
      p_last_repository_version = versions[0].m_commit_hash;
      show_new_repository_versions_dlg(versions, version.empty());
    }
  else if (p_new_repository_versions_dlg)
    raise_dlg(p_new_repository_versions_dlg);
}


void
OksDataEditorMainDialog::check_list_of_files_cb(XtPointer call_data, XtIntervalId*)
{
  OksDataEditorMainDialog * dlg = (OksDataEditorMainDialog *)call_data;

  std::list<OksFile *> * mfiles = nullptr;
  std::list<OksFile *> * rfiles = nullptr;

  try
    {
      if(!dlg->get_repository_version().empty())
        {
          dlg->show_new_repository_versions_dlg();
        }
      else
        {
          dlg->create_lists_of_updated_data_files(&mfiles, &rfiles);

          ERS_DEBUG( 2 , "found " << (mfiles ? mfiles->size() : 0)
                      << " modified, " << (rfiles ? rfiles->size() : 0)
                      << " removed files"
          );

          if(rfiles) {
            while(!rfiles->empty()) {
              OksFile * f = rfiles->front();
              rfiles->pop_front();

              std::string s("File \"");
              s += f->get_full_file_name();
              s += "\" has been removed by another process.\n"
                   "Now it only exists in memory of this process.\n"
                   "To restore the file press [Save] button.\n";

              DialogReturn user_choice = dlg->file_action_dialog(
                s.c_str(), f, info_xpm,
                0, 0,
                "Save", askUpdatedCB,
                "Dismiss", cancelChangedCB
              );

              if(user_choice == SaveFile) {
                dlg->save(f);
              }
              else {
                f->update_status_of_file(true, false);  // update status of local file
              }
            }
          }

          if(mfiles) {
            while(!mfiles->empty()) {
              OksFile * f = mfiles->front();
              mfiles->pop_front();

              std::string s("File \"");
              s += f->get_full_file_name();
              s += "\" has been modified by another process.\n"
                   "To read modified file press [Reload] button";
              s += (f->is_updated() ? " (your changes will be lost!)" : "");
              s += ".\nTo overwrite modified file press [Save] button.\n";

              DialogReturn user_choice = dlg->file_action_dialog(
                s.c_str(), f, info_xpm,
                "Reload", askUpdatedCB,
                "Save", askUpdatedCB,
                "Dismiss", cancelChangedCB
              );

              if(user_choice == ReloadFile) {
                dlg->reload(f);
              }
              else if(user_choice == SaveFile) {
                dlg->save(f, (f->get_oks_format() == "extended"));
              }
              else {
                f->update_status_of_file(true, false);  // update status of local file
              }
            }
          }
        }

  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
  }

  delete rfiles;
  delete mfiles;


    // now check lock files

  int rowsNumber = XbaeMatrixNumRows(dlg->data_matrix);

  for(OksFile::Map::const_iterator i = dlg->data_files().begin(); i != dlg->data_files().end(); ++i) {
    std::string file_status = get_file_status(i->second);

    int pos;

    for(pos=0; pos<rowsNumber; pos++) {
      if(i->second == (OksFile *)XbaeMatrixGetRowUserData(dlg->data_matrix, pos)) break;
    }

    if(pos>=rowsNumber) continue;


    const char * status_cell = XbaeMatrixGetCell(dlg->data_matrix, pos, 3);

    if(status_cell && file_status != status_cell) {
      if(is_verbose()) {
        std::cout << "Update lock info for file \'" << i->second->get_full_file_name() << "\'\n";
      }

      XbaeMatrixSetCell(dlg->data_matrix, pos, 3, const_cast<char *>(file_status.c_str()));

        // set color of file status

      String row[NumOfDataFilesMatrixColumns];
      for(size_t i=0; i<NumOfDataFilesMatrixColumns; ++i) row[i] = 0;
      row[3] = const_cast<char *>(file_status.c_str());
      dlg->set_data_file_row_colors(row, pos);
    }
  }

  dlg->start_check_updated();
}

void
OksDataEditorMainDialog::start_check_updated()
{
  unsigned short check_updated_timeout(
    OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_ext_modified_files_value_name())
  );

  if(check_updated_timeout > 0) {
    p_check_updated_timer = XtAppAddTimeOut(
      XtWidgetToApplicationContext(menuBar), check_updated_timeout*1000, check_list_of_files_cb, (XtPointer)this
    );
  }
}

void
OksDataEditorMainDialog::stop_check_updated()
{
  if(p_check_updated_timer) {
    XtRemoveTimeOut(p_check_updated_timer);
    p_check_updated_timer = 0;
  }
}

void
OksDataEditorMainDialog::start_check_need_backup()
{
  unsigned short check_need_backup_timeout(
    OksGC::Parameters::def_params().get_u16_parameter(OksParameterNames::get_backup_modified_files_value_name())
  );

  if(check_need_backup_timeout > 0) {
    p_check_need_backup_timer = XtAppAddTimeOut(
      XtWidgetToApplicationContext(menuBar), check_need_backup_timeout*1000, check_need_backup_files_cb, (XtPointer)this
    );
  }
}

void
OksDataEditorMainDialog::stop_check_need_backup()
{
  if(p_check_need_backup_timer) {
    XtRemoveTimeOut(p_check_need_backup_timer);
    p_check_need_backup_timer = 0;
  }
}

std::string
OksDataEditorMainDialog::get_conf_param(const char * env_name, const char * d_path) const
{
  if(d_path == 0) d_path = "";

  if(is_verbose()) {
    std::cout << "call get_conf_param(\'" << env_name << "\', \'" << d_path << "\')\n";
  }

    // get either value of the environment variable or default path
    // put result to the string 's'

  std::string s;

  if(*d_path != 0) {
    s = d_path;
  }
  else {
    const char * env_val = (env_name && *env_name) ? getenv(env_name) : 0;

    if(env_val != 0 && *env_val != 0) {
      s = env_val;
    }
  }

  if(is_verbose()) {
    std::cout << " * parameter = \'" << s << "\'\n"
                 " * search prefix = \'" << p_init_path << "\'\n";
  }

  std::string result;

  if(!s.empty()) {
    Oks::Tokenizer t(s, ":");
    std::string token;

    while(!(token = t.next()).empty()) {
      bool found = false;
      struct stat buf;
      {
	if(lstat(token.c_str(), &buf) == 0) {
	  if(!result.empty()) result += ':';
	  result += token;

          if(is_verbose()) {
            std::cout << "  - add \'" << token << "\'\n";
          }

	  found = true;

	  continue;
	}
	else if(is_verbose()) {
          std::cout << "  - skip \'" << token << "\'\n";
	}
      }

      if(!p_init_path.empty() && token[0] != '/') {
	Oks::Tokenizer pt(p_init_path, ":");
        std::string p;

        while(!(p = pt.next()).empty()) {
          std::string token2 = p + '/' + token;
	  if(lstat(token2.c_str(), &buf) == 0) {
	    if(!result.empty()) result += ':';
	    result += token2;

            if(is_verbose()) {
              std::cout << "  - add \'" << token2 << "\'\n";
            }

	    found = true;

	    break;
	  }
	  else if(is_verbose()) {
            std::cout << "  - skip \'" << token2 << "\'\n";
	  }
	}
      }

      if(found == false) {
        std::ostringstream text;
        text << "  failed to find configuration path to parameter \'" << env_name << "\':\n"
                "  configuration path = \'" << p_init_path << "\'\n"
                "  token in parameter = \'" << token << "\'\n"
                "  check OKS_GUI_PATH, " << env_name << " variables or --init-... command line parameters";
        ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
      }
    }
  }
  else {
    std::ostringstream text;
    text << "  could not get value of parameter \'" << env_name << "\'\n"
            "  None of \'--init-dir\' command line parameter or " << env_name << ", or OKS_GUI_PATH, or TDAQ_INST_PATH environment variables is defined";
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
  }

  if(is_verbose()) {
    std::cout << " * return \'" << result << "\'\n";
  }

  return result;
}

void
OksDataEditorMainDialog::create_g_classes()
{
  if(p_g_classes_created == true) return;

  for(std::list<G_Class *>::iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
    G_Class * g_class = *i;
    OksClass * oks_class = find_class(g_class->get_name());
    if(oks_class) {
      const OksClass::FList * sub_classes = oks_class->all_sub_classes();

      g_class->set_oks_class(oks_class);

      if(g_class->p_show_all_attributes || g_class->p_show_all_relationships) {
        if(g_class->p_show_all_attributes) {
          const std::list<OksAttribute *> * class_attrs = oks_class->all_attributes();
	  if(class_attrs) {
	    for(std::list<OksAttribute *>::const_iterator j = class_attrs->begin(); j != class_attrs->end(); ++j) {
              const std::string& aname = (*j)->get_name();
              if(find(g_class->p_attributes.begin(), g_class->p_attributes.end(), aname) == g_class->p_attributes.end())
                g_class->p_shown_attributes.push_back(aname);
            }
	  }
	}

        if(g_class->p_show_all_relationships) {
          const std::list<OksRelationship *> * class_rels = oks_class->all_relationships();
	  if(class_rels) {
	    for(std::list<OksRelationship *>::const_iterator j = class_rels->begin(); j != class_rels->end(); ++j) {
              const std::string& rname = (*j)->get_name();
	      if(find(g_class->p_relationships.begin(), g_class->p_relationships.end(), rname) == g_class->p_relationships.end())
                g_class->p_shown_relationships.push_back(rname);
	    }
	  }
	}

        if(sub_classes) {
	  for(OksClass::FList::const_iterator ci = sub_classes->begin(); ci != sub_classes->end(); ++ci) {
            const OksClass * sub_class = *ci;

	    if(g_class->p_show_all_attributes) {
	      if(const std::list<OksAttribute *> * sc_attrs = sub_class->all_attributes()) {
	        for(const auto& j : *sc_attrs) {
                  const std::string& aname = j->get_name();
		  if(find(g_class->p_attributes.begin(), g_class->p_attributes.end(), aname) == g_class->p_attributes.end())
		    if(find(g_class->p_shown_attributes.begin(), g_class->p_shown_attributes.end(), aname) == g_class->p_shown_attributes.end())
                      g_class->p_shown_attributes.push_back(aname);
	        }
	      }
            }

	    if(g_class->p_show_all_relationships) {
	      if(const std::list<OksRelationship *> * sc_rels = sub_class->all_relationships()) {
	        for(const auto& j : *sc_rels) {
                  const std::string& rname = j->get_name();
                  if(find(g_class->p_relationships.begin(), g_class->p_relationships.end(), rname) == g_class->p_relationships.end())
                    if(find(g_class->p_shown_relationships.begin(), g_class->p_shown_relationships.end(), rname) == g_class->p_shown_relationships.end())
                      g_class->p_shown_relationships.push_back(rname);
                }
              }
            }
          }
        }
      }

      if(g_class->p_show_all_attributes == false) {
        for(const auto& j : g_class->p_attributes) {
          g_class->p_shown_attributes.push_back(j);
        }
      }

      if(g_class->p_show_all_relationships == false) {
        for(const auto& j : g_class->p_relationships) {
          g_class->p_shown_relationships.push_back(j);
        }
      }

      if(const size_t size = g_class->p_remove_from_used.size()) {
        OksClass * oc = oks_class;
	OksClass::FList::const_iterator sci;
	if(sub_classes) sci = sub_classes->begin();

	do {
	  if(const std::list<OksRelationship *> * rels = oc->all_relationships()) {
	    size_t * offs = new size_t [size + 1];
	    size_t ptr(oc->number_of_all_attributes());
	    size_t cnt(0);

	    for(auto j = rels->begin(); j != rels->end(); ++j, ++ptr) {
	      for(const auto& j2 : g_class->p_remove_from_used) {
	        if(j2 == (*j)->get_name()) offs[cnt++] = ptr;
	      }
	    }

	    offs[cnt] = 0xffff;	// last (can't use 0!)

	    remove_from_used[oc] = offs;
	  }
	} while(sub_classes && sci != sub_classes->end() && (oc = *(sci++)));
      }
    }
    else if(is_verbose()) {
      Oks::warning_msg("create_g_classes()") << "Can not find class \"" << g_class->get_name() << "\"\n";
    }
  }

  if(is_verbose()) {
    std::cout << "- print graphical class information ...\n";

    for(std::list<G_Class *>::iterator i2 = p_classes.begin(); i2 != p_classes.end(); ++i2) {
      G_Class * c = *i2;

      std::cout << " + class info for \'" << c->get_name() << "\'\n";

      for(const auto& j : *c->get_attributes())
        std::cout << "  - attribute \'" << j << "\'\n";

      for(const auto& j : *c->get_relationships())
        std::cout << "  - relationship \'" << j << "\'\n";
    }

    for(const auto& i : remove_from_used) {
      std::cout << "- remove from used in class \'" << i.first->get_name()
	        << "\' relationships with offset(s):";

      for(size_t * offs = i.second; *offs != 0xffff; ) std::cout << ' ' << *(offs++);

      std::cout << std::endl;
    }
  }

  if(p_root_class) {
    p_root_class->find_root_objects(p_root_relationship, p_root_objects);
    if(is_verbose()) {
      if(p_root_objects.empty()) {
        std::cout << "* there is no root object\n";
      }
      std::cout << "* there are " << p_root_objects.size() << " root object(s):\n";
      for(const auto& j : p_root_objects) {
        std::cout << " - " << j << std::endl;
      }
    }
    find_used();
  }
  else {
    if(is_verbose()) {
      std::cout << "* no root class\n";
    }
  }

  p_g_classes_created = true;
}

void
OksDataEditorMainDialog::remove_g_classes()
{
  if (p_g_classes_created == false)
    return;

  // clean graphical classes (OKS dependent)
  for (const auto& i : p_classes)
    {
      i->set_oks_class(nullptr);
      i->p_shown_attributes.clear();
      i->p_shown_relationships.clear();
    }

  // clear set of used OKS objects
  used.clear();

  p_g_classes_created = false;
}

void
OksDataEditorMainDialog::check_objects_and_files() const
{
  std::cout << "START \"CHECK INCLUSION FILE PATHS FOR LINKS BETWEEN OBJECTS\"\n";

    // build tree of include files

  std::map<OksFile *, std::set<OksFile *> > include_tree;

  for (const auto& i : data_files())
    {
      i.second->get_all_include_files(this, include_tree[i.second]);

      if (is_verbose())
        {
          std::cout << "include files of \'" << i.second->get_full_file_name() << "\' (" << i.second << ") are:\n";

          for (const auto& j : include_tree[i.second])
             std::cout << " * \"" << j->get_full_file_name() << "\"\n";
        }
    }


  for (const auto& i : classes())
    {
      for (const auto& j : *i.second->objects())
        {
          // find set of includes for given object
          auto mi = include_tree.find(j.second->get_file());
          if (mi == include_tree.end())
            {
              std::ostringstream text;
              text << "cannot find includes of \"" << j.second->get_file()->get_full_file_name() << "\" (" << j.second->get_file() << ')';
              ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
              continue;
            }

          j.second->is_consistent((*mi).second, "ERROR");
        }
    }

  std::cout << "DONE \"CHECK INCLUSION FILE PATHS FOR LINKS BETWEEN OBJECTS\"\n";
}

std::string
OksDataEditorMainDialog::iso2simple(const std::string& iso)
{
  return boost::posix_time::to_simple_string(boost::posix_time::from_iso_string(iso));
}

void
OksDataEditorMainDialog::get_all_dialogs(std::set<Widget>& result) const
{
  result.insert(app_shell());

  for (const auto& i : get_dialogs())
    result.insert(XtParent(i->get_form_widget()));

  for (const auto& i : p_windows)
    result.insert(XtParent(i->get_form_widget()));

  if (helpWindow)
    result.insert(helpWindow->get_form_widget());

  if (OksXmMessageWindow * mwin = get_msg_window())
    result.insert(mwin->get_form_widget());

  if (replaceDialog)
    result.insert(replaceDialog->get_form_widget());

  if (findDialog)
    result.insert(findDialog->get_form_widget());
}

static void
btnCB(Widget, XtPointer client_data, XtPointer)
{
  XtDestroyWidget(XtParent(reinterpret_cast<Widget>(client_data)));
}

static std::string
wrap(const char * msg, std::size_t limit = 256)
{
  std::string out(msg);

  std::size_t cur_idx = 0;

  while (cur_idx < out.size())
    {
      std::size_t idx = out.find('\n', cur_idx);

      if (idx == std::string::npos)
        idx = out.size() - 1;

      while ((idx - cur_idx) > limit)
        {
          std::size_t space_idx = out.rfind(' ', cur_idx + limit);

          if (space_idx > 0 && space_idx > cur_idx)
            {
              out[space_idx] = '\n';
              cur_idx = space_idx + 1;
            }
        }

      cur_idx = idx + 1;
    }

  return out;
}


void
OksDataEditorMainDialog::report_error(const char * title, const char * msg, Widget parent)
{
  Oks::error_msg(title) << msg << std::endl;

  std::string t(title);
  t.append(" Error");

  if (!parent)
    parent = mainDlg->get_form_widget();

  std::string s = wrap(msg);
  Widget dlg = OksXm::create_dlg_with_pixmap(parent, t.c_str(), fail_xpm, s.c_str(), "OK", 0, 0, 0, 0);

  XtVaSetValues(dlg, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

  XtAddCallback(dlg, XmNokCallback, btnCB, (XtPointer) dlg);
  OksXm::set_close_cb(XtParent(dlg), btnCB, (XtPointer) dlg);

  XtManageChild(dlg);
}

void
OksDataEditorMainDialog::report_exception(const char * title, const oks::exception& ex, Widget parent)
{
  std::string m("Caught OKS exception:\n"); m += ex.what();
  report_error(title, m.c_str(), parent);
}

void
OksDataEditorMainDialog::test_bind_classes_status() const
{
  if(!get_bind_classes_status().empty())
    {
      std::cerr << "WARNING: " << "The schema contains dangling references to non-loaded classes:\n" << get_bind_classes_status();
    }
}
