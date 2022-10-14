#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <sstream>

#include <ers/ers.h>

#include <oks/class.h>
#include <oks/attribute.h>
#include <oks/xm_popup.h>
#include <oks/xm_utils.h>
#include <oks/xm_msg_window.h>

#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <Xm/Matrix.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/CascadeB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>


#include "schema_editor_main_dlg.h"
#include "schema_editor_schema_dlg.h"
#include "schema_editor_view_dlg.h"
#include "schema_editor_class_dlg.h"
#include "schema_editor_attribute_dlg.h"
#include "schema_editor_relationship_dlg.h"
#include "schema_editor_method_dlg.h"
#include "schema_editor.h"

#include "schema_editor_about.xpm"
#include "exit.xpm"

static const size_t NumOfSchemaFilesMatrixColumns = 5;
static OksSchemaEditorMainDialog::DialogReturn done_with_dialog;

extern OksSchemaEditorMainDialog * mainDlg;

static const char * szNewFile     = "New...";
static const char * szOpen        = "Open...";
static const char * szClose       = "Close        ";
static const char * szCloseAll    = "Close All";
static const char * szSave        = "Save         ";
static const char * szForceSave   = "Force Save   ";
static const char * szSaveAs      = "Save As...   ";
static const char * szSaveAll     = "Save All";
static const char * szSetActive	  = "Set Active   ";
static const char * szUnsetActive = "Unset Active ";
static const char * szShowFile	  = "Show File    ";

static char * szMessageLog	= (char *)"Message Log";
static char * szHelp		= (char *)"Help";


// ************************** EXIT HANDLER ************************** //

static void
real_signal_handler (XtPointer data, XtIntervalId *)
{
  int num = (int) (long) data;

  std::cout << "got signal " << num << (num == SIGINT ? " (interrupt from keyboard)" : num == SIGTERM ? " (termination signal)" : "") << std::endl;
  std::cout.flush();

  if(num == SIGINT) {
    OksSchemaEditorMainDialog::exit_app("interrupt");
  }
  else {
    delete mainDlg;
    exit(0);
  }
}

extern "C" void
signal_handler(int num)
{
  if (mainDlg)
    XtAppAddTimeOut(mainDlg->app_context(), 0, real_signal_handler, (XtPointer) (long) num);
}


OksSchemaEditorDialog::OksSchemaEditorDialog(OksSchemaEditorMainDialog *p, OksClass *cl) :
  OksDialog	("", p),
  p_parent	(p),
  p_class	(cl)
{
}


void
OksSchemaEditorDialog::closeCB(Widget, XtPointer client_data, XtPointer)
{
  delete (OksSchemaEditorDialog *)client_data;
}


bool
OksSchemaEditorDialog::is_read_only() const
{
  return p_class->get_file()->is_read_only();
}


void
OksSchemaEditorDialog::make_popup(Widget w, XtPointer data, XEvent * event, bool read_only, XtCallbackProc f)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);

  if(read_only) {
    popup.addDisableItem(szUpdate);
    popup.addDisableItem("Rename");
    popup.addDisableItem(szDelete);
  }
  else {
    popup.addItem(szUpdate, idUpdate, f, data);
    popup.addItem("Rename", idRename, f, data);
    popup.addItem(szDelete, idDelete, f, data);
  }

  popup.show((XButtonPressedEvent *)event);
}


void
OksSchemaEditorMainDialog::add_class_to_schema_dlg(OksFile * f)
{
  OksSchemaEditorSchemaDialog * osesd = mainDlg->find_schema_dlg(f);
  if(osesd) osesd->refresh();
  update_schema_row(f);
}


void
OksSchemaEditorMainDialog::remove_class_from_schema_dlg(const OksClass * c, OksFile * f)
{
  OksSchemaEditorSchemaDialog *osesd = mainDlg->find_schema_dlg(f);
  if(osesd) {
    osesd->remove_class(c);
  }
  update_schema_row(f);
}


void
OksSchemaEditorMainDialog::deleteNF(OksClass *c)
{
    // delete class from schema dialog

  mainDlg->remove_class_from_schema_dlg(c, c->get_file());


    // delete class dialog and all attribute, relationship and method dialogs

  for(std::list<OksSchemaEditorDialog *>::iterator i = mainDlg->dialogs.begin(); i != mainDlg->dialogs.end();) {
    OksSchemaEditorDialog *dlg = *i;

    if(dlg->get_class() == c) {
      i = mainDlg->dialogs.erase(i);
      delete dlg;
    }
    else {
      if(dlg->type() == OksSchemaEditorDialog::ViewDlg) {
        (static_cast<OksSchemaEditorViewDialog *>(dlg))->update_class(c, true);
      }
      ++i;
    }
  }

  OksXm::List::delete_row(mainDlg->get_paned_window(idMainPanedWindow)->get_form(idClasses)->get_widget(idClasses), (char *)(c->get_name().c_str()));
}


void
OksSchemaEditorMainDialog::createNF(OksClass *c)
{
  OksXm::List::add_row(
    mainDlg->get_paned_window(idMainPanedWindow)->get_form(idClasses)->get_widget(idClasses),
    (char *)(c->get_name().c_str()),
    true
  );

  mainDlg->add_class_to_schema_dlg(c->get_file());
}


void
OksSchemaEditorMainDialog::changeNF(OksClass *c, OksClass::ChangeType type, const void *)
{
  if (OksSchemaEditorClassDialog *osecd = mainDlg->find_class_dlg(c))
    {
      switch (type)
        {
          case OksClass::ChangeSuperClassesList:
            osecd->show_superclasses();
            osecd->show_attributes();
            osecd->show_relationships();
            osecd->show_methods();
            break;

          case OksClass::ChangeAttributesList:
            osecd->show_attributes();
            break;

          case OksClass::ChangeRelationshipsList:
            osecd->show_relationships();
            break;

          case OksClass::ChangeMethodsList:
            osecd->show_methods();
            break;

          case OksClass::ChangeSubClassesList:
            osecd->show_subclasses();
            break;

          default:
            break;
        }

      if (type == OksClass::ChangeSuperClassesList)
        if (const OksClass::FList * superclasses = c->all_super_classes())
          for (const auto& i : *superclasses)
            if ((osecd = mainDlg->find_class_dlg(i)) != nullptr)
              osecd->show_subclasses();
    }

  if (type != OksClass::ChangeSubClassesList)
    mainDlg->update_schema_row(c->get_file());

  // update view dialogs if any
  for (auto& i : mainDlg->dialogs)
    if (i->type() == OksSchemaEditorDialog::ViewDlg)
      (static_cast<OksSchemaEditorViewDialog *>(i))->update_class(c, false);
}


void
OksSchemaEditorMainDialog::exit_app(const char * message)
{
  if (mainDlg)
    {
      for (const auto& i : mainDlg->schema_files())
        if (mainDlg->confirm_changed(message, i.second) == OksSchemaEditorMainDialog::CancelExit)
          return;

      delete mainDlg;
    }

  exit(0);
}


void
OksSchemaEditorMainDialog::classDefaultCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksXm::AutoCstring name(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));
  OksSchemaEditorMainDialog * dlg = (OksSchemaEditorMainDialog *)client_data;
  if(OksClass * cl = dlg->find_class(name.get())) dlg->create_class_dlg(cl);
}


void
OksSchemaEditorMainDialog::openCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmFileSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksSchemaEditorMainDialog * dlg = (OksSchemaEditorMainDialog *)client_data;
    OksXm::AutoCstring name(OksXm::string_to_c_str(((XmFileSelectionBoxCallbackStruct *)call_data)->value));

    try {
      mainDlg->load_schema(name.get());
      dlg->showSchemaList();
      dlg->showClassList();
      dlg->refresh();

      OksXm::Matrix::select_row((Widget)OksXm::get_user_data(w), name.get());

      mainDlg->test_bind_classes_status();
    }
    catch (oks::exception & ex) {
      std::cerr << "ERROR: " << ex.what() << std::endl;
    }
  }

  XtUnmanageChild(w);
  XtDestroyWidget(w);
}


void
OksSchemaEditorMainDialog::schemaCB(Widget w, XtPointer client_data, XtPointer) 
{
  long user_choice = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  OksSchemaEditorMainDialog * dlg = (OksSchemaEditorMainDialog *)client_data;
  Widget matrix = dlg->p_schema_matrix;
  OksFile * file_h = (OksFile *)(OksXm::Matrix::get_selected_row_user_data(matrix));

  switch (user_choice) {
    case idAcMainSchemaNew: {
      std::string s = OksXm::ask_file(
        dlg->get_form_widget(),
	"Create new OKS schema file",
	"New OKS schema file name",
        (
            !dlg->get_user_repository_root().empty() ? dlg->get_user_repository_root().c_str() :
            !dlg->get_repository_dirs().empty() ? dlg->get_repository_dirs().front().c_str() :
            nullptr
        ),
	"*.schema.xml"
      );

      if(s.length() && OksXm::ask_overwrite_file(dlg->get_form_widget(), s.c_str())) {
        try {
          dlg->new_schema(s);
          dlg->showSchemaList();
          OksXm::Matrix::select_row(matrix, s.c_str());
        }
        catch (oks::exception & ex) {
          std::cerr << "ERROR: " << ex.what() << std::endl;
	}
      }

      break;
    }

    case idAcMainSchemaOpen:
      XtVaSetValues(
        OksXm::create_file_selection_box(
          dlg->get_form_widget(), "Open OKS Schema", "Selection of file with OKS schema",
	  0, "*.schema.xml", true, openCB, (XtPointer)client_data, 0, 0
        ),
	XmNuserData, (XtPointer)matrix,
	NULL
      );
      break;

    case idAcMainSchemaClose: {
      if(file_h && dlg->confirm_changed("Close", file_h) != OksSchemaEditorMainDialog::CancelExit) {
        OksSchemaEditorSchemaDialog * osesd = dlg->find_schema_dlg(file_h);
        if(osesd) delete osesd;
        dlg->close_schema(file_h);
        dlg->showSchemaList();
        dlg->showClassList();
      }
      break;
    }

    case idAcMainSchemaCloseAll:
      for(OksFile::Map::const_iterator i = dlg->schema_files().begin(); i != dlg->schema_files().end(); ++i) {
        if(dlg->confirm_changed("Close", i->second) == OksSchemaEditorMainDialog::CancelExit) return;
      }

      dlg->close_all_schema();
      dlg->showSchemaList();
      dlg->showClassList();

      break;

    case idAcMainSchemaSave:
    case idAcMainSchemaForceSave:
      try {
        dlg->save_schema(file_h, (user_choice == idAcMainSchemaForceSave));
        dlg->remove_backup(file_h);
        dlg->update_schema_row(file_h);
      }
      catch (oks::exception & ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
      }

      break;

    case idAcMainSchemaSaveAll:
      try {
        dlg->save_all_schema();
	for(std::set<OksFile *>::iterator i = dlg->p_backup_files.begin(); i != dlg->p_backup_files.end();) {
	  dlg->remove_backup(*(i++));  // note, the current OksFile will be removed from the set
	}
      }
      catch (oks::exception & ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
      }
      break;

    case idAcMainSchemaSaveAs: 
      if(file_h) {
        std::string s = OksXm::ask_name(
          dlg->get_form_widget(),
	  "Save schema as",
	  "Save OKS Schema As",
	  file_h->get_full_file_name().c_str()
        );

        if(s.length() && OksXm::ask_overwrite_file(dlg->get_form_widget(), s.c_str())) {
          std::string old_name(file_h->get_full_file_name());
          try {
            dlg->save_as_schema(s, file_h);
            dlg->showSchemaList();
            OksXm::Matrix::select_row(matrix, s.c_str());


              // change name of datafile in Data Dialog (if exists)

            OksSchemaEditorSchemaDialog *osesd = dlg->find_schema_dlg(file_h);
            if(osesd) {
              osesd->change_schema_file();
	      dlg->update_menu_bar();
            }


              // change name of datafile in all Class Dialogs related to the datafile
              // change read-only state in all Dialogs related to the datafile

            for(std::list<OksSchemaEditorDialog *>::iterator i = dlg->dialogs.begin(); i != dlg->dialogs.end();++i) {
              OksSchemaEditorDialog * osed = *i;
	      OksClass *c = osed->get_class();

	      if(c && file_h == c->get_file()) {
	        osed->change_schema_file();
	      }
	      else if(osed->type() == OksSchemaEditorDialog::SchemaDlg) {
	        OksSchemaEditorSchemaDialog * schema_dlg = static_cast<OksSchemaEditorSchemaDialog *>(osed);
	        if(schema_dlg->get_file() == file_h) {
	          osed->change_schema_file();
	          dlg->update_menu_bar();
	        }
	      }
            }
	  }
          catch (oks::exception & ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
          }
        }
      }

      break;

    case idAcMainSchemaSetActive:
    case idAcMainSchemaUnsetActive:
      try {
        dlg->set_active_schema(user_choice == idAcMainSchemaSetActive ? file_h : 0);
        dlg->showSchemaList();
      }
      catch(oks::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
      }
      break;

    case idAcMainSchemaShowClasses:
      dlg->create_schema_dlg(file_h);
      break;
  }
}


void
OksSchemaEditorMainDialog::schemaAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksSchemaEditorMainDialog * dlg = (OksSchemaEditorMainDialog *)client_data;
  OksFile * pf = (OksFile *)(OksXm::Matrix::get_selected_row_user_data(w));
  const char * schemaName = OksXm::Matrix::get_selected_value(w);

  OksPopupMenu popup(w);

  popup.addItem(szNewFile, idAcMainSchemaNew, schemaCB, client_data);
  popup.addItem(szOpen, idAcMainSchemaOpen, schemaCB, client_data);
  popup.add_separator();

  if(schemaName) {
    popup.addItem(szClose, schemaName, idAcMainSchemaClose, schemaCB, client_data);
    popup.addItem(szCloseAll, idAcMainSchemaCloseAll, schemaCB, client_data);
    popup.add_separator();
    popup.addItem(szSave, schemaName, idAcMainSchemaSave, schemaCB, client_data);
    popup.addItem(szForceSave, schemaName, idAcMainSchemaForceSave, schemaCB, client_data);
    popup.addItem(szSaveAs, schemaName, idAcMainSchemaSaveAs, schemaCB, client_data);
    popup.addItem(szSaveAll, idAcMainSchemaSaveAll, schemaCB, client_data);
    popup.add_separator();
    Widget btn;
    if(pf == dlg->get_active_schema()) {
      btn = popup.addItem(szUnsetActive, schemaName, idAcMainSchemaUnsetActive, schemaCB, client_data);
    }
    else {
      btn = popup.addItem(szSetActive, schemaName, idAcMainSchemaSetActive, schemaCB, client_data);
    }
    popup.addItem(szShowFile, schemaName, idAcMainSchemaShowClasses, schemaCB, client_data);

    if(!pf || OksKernel::check_read_only(pf) == true) XtSetSensitive(btn, false);
  }
  else {
    popup.addDisableItem(szClose);
    popup.addItem(szCloseAll, idAcMainSchemaCloseAll, schemaCB, client_data);
    popup.add_separator();
    popup.addDisableItem(szSave);
    popup.addDisableItem(szSaveAs);
    popup.addItem(szSaveAll, idAcMainSchemaSaveAll, schemaCB, client_data);
    popup.add_separator();
    popup.addDisableItem(szSetActive);
    popup.addDisableItem(szShowFile);
  }

  popup.show((XButtonPressedEvent *)event);
}


OksSchemaEditorClassDialog *
OksSchemaEditorMainDialog::create_new_class_dlg()
{
  std::string s = OksXm::ask_name(
    get_form_widget(),
    "Input name of new class",
    "New OKS Class"
  );

  if (s.length())
    {
      try
        {
          return create_class_dlg(new OksClass(s, this));
        }
      catch (const std::exception & ex)
        {
          Oks::error_msg("Create new class") << ex.what() << std::endl;
        }
    }

  return 0;
}



void
OksSchemaEditorMainDialog::classCB(Widget w, XtPointer client_data, XtPointer) 
{
  long user_data = (long)OksXm::get_user_data(w);
  Widget list = XtParent(XtParent(XtParent(w)));
  OksPopupMenu::destroy(w);

  OksSchemaEditorMainDialog * dlg = (OksSchemaEditorMainDialog *)client_data;

  switch (user_data) {
    case idAcMainClassNew:
      dlg->create_new_class_dlg();
      break;
	
    case idAcMainClassShow:
    case idAcMainClassDelete: {
      OksXm::AutoCstring className(OksXm::List::get_selected_value(list));
      if(OksClass * cl = dlg->find_class(className.get())) {
        if(user_data == idAcMainClassShow) {
          dlg->create_class_dlg(cl);
        }
        else {
          try {
            OksClass::destroy(cl);
          }
          catch(oks::exception& ex) {
            Oks::error_msg("OksSchemaEditorMainDialog::classCB()") << ex.what() << std::endl;
          }
        }
      }
      break;
    }
  }
}


void
OksSchemaEditorMainDialog::classAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if(((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);

  if(is_valid_active_schema())
    popup.addItem(szNew, idAcMainClassNew, classCB, client_data);
  else
    popup.addDisableItem(szNew);

  if(OksXm::List::get_selected_pos(w)) {
    popup.addItem(szShow, idAcMainClassShow, classCB, client_data);

    OksXm::AutoCstring class_name(OksXm::List::get_selected_value(w));
    const OksClass * c = mainDlg->find_class(class_name.get());

    if(c->get_file()->is_read_only())
      popup.addDisableItem(szDelete);
    else
      popup.addItem(szDelete, idAcMainClassDelete, classCB, client_data);
  }
  else {
    popup.addDisableItem(szShow);
    popup.addDisableItem(szDelete);
  }

  popup.add_separator();


  if(OksXm::List::size(w) > 1)
    popup.addItem(szSort, 0, OksXm::List::sortCB, (XtPointer)w);
  else
    popup.addDisableItem(szSort);

  popup.show((XButtonPressedEvent *)event);
}


void
OksSchemaEditorMainDialog::labelCB(Widget w, XtPointer, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct *cbs = (XbaeMatrixLabelActivateCallbackStruct *)cb;

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, OksXm::Matrix::SortByString);
}

void
OksSchemaEditorMainDialog::schemaDefaultCB(Widget w, XtPointer client_data, XtPointer)
{
  OksFile * file_h = (OksFile *)(OksXm::Matrix::get_selected_row_user_data(w));
  ((OksSchemaEditorMainDialog *)client_data)->create_schema_dlg(file_h);
}

void
OksSchemaEditorMainDialog::configureAC (Widget shell, XtPointer /*client_data*/, XEvent *event, Boolean * /*unused*/)
{
  XConfigureEvent *cevent = (XConfigureEvent *) event;
  if(cevent->type == ConfigureNotify) {
    OksDialog::resetCascadePos(shell, cevent->x, cevent->y);
  }
}

OksSchemaEditorMainDialog::OksSchemaEditorMainDialog(int *app_argc, char **app_argv,
                                                     const char *applClassName,
						     const char * fallback_resources[][2],
						     bool no_msg_dlg, bool msg_dlg_auto_scroll, bool no_verbose_out) :
  OksTopDialog    ("Oks Schema Editor", app_argc, app_argv, applClassName, fallback_resources),
  OksKernel       (),
  p_menu_bar      (add_menubar()),
  p_no_msg_window (getenv("OKS_SCHEMA_EDITOR_NO_MSG_WINDOW") != 0 ? true : no_msg_dlg),
  p_verbouse_out  (getenv("OKS_SCHEMA_EDITOR_VERBOSE_OUT") != 0 ? true : no_verbose_out),
  p_mt_click_time (XtGetMultiClickTime(display())),
  p_check_need_backup_timeout (60),
  p_check_need_backup_timer   (0)
{
  set_allow_duplicated_classes_mode(false);

  create_menu_bar(p_menu_bar);
  XtManageChild(p_menu_bar);

  Widget w;
  Widget panedWidget = add_paned_window(idMainPanedWindow);
  OksPanedWindow * panedWindow = get_paned_window(idMainPanedWindow);
  attach_right(idMainPanedWindow);

  OksForm *f = new OksForm(panedWidget);


    //
    // FIXME: Igor Soloviev
    //
    // Create temporal widget to avoid bug of XmMatrix:
    //   if the matrix will be first widget which may have input focus,
    //   the XmNenterCellCallback will be called each time the editor main 
    //   window becames active that will cause select of first matrix row
    //

  Widget tmpw = XmCreateTextField(f->get_form_widget(), (char *)"field", 0, 0);
  XtManageChild(tmpw);


  p_schema_matrix = f->add_matrix(idSchema, (void *)szSchema);
  f->attach_right(idSchema);
  f->attach_bottom(idSchema);
  panedWindow->add_form(f, idSchema);


    // set data files matrix specific parameters

  {
    char const * labels[] = {"File", "Access", "Status", "State", ""};
    unsigned char label_alinments[NumOfSchemaFilesMatrixColumns];
    short widths[NumOfSchemaFilesMatrixColumns];
  
    for(size_t i=0; i<NumOfSchemaFilesMatrixColumns; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }
  
    XtVaSetValues(
      p_schema_matrix,
      XmNcolumns, NumOfSchemaFilesMatrixColumns,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      NULL
    );

    XtAddCallback(p_schema_matrix, XmNdefaultActionCallback, schemaDefaultCB, (XtPointer)this);
    XtAddCallback(p_schema_matrix, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
    XtAddCallback(p_schema_matrix, XmNlabelActivateCallback, labelCB, 0);
    XtAddEventHandler(p_schema_matrix, ButtonPressMask, False, schemaAC, (XtPointer)this);
  }


  f = new OksForm(panedWidget);
  w = f->add_list(idClasses, szClasses);
  f->attach_right(idClasses);
  f->attach_bottom(idClasses);
  panedWindow->add_form(f, idClasses);
  XtAddCallback (w, XmNdefaultActionCallback, classDefaultCB, (XtPointer)this);
  XtAddEventHandler(w, ButtonPressMask, False, classAC, (XtPointer)this);


  OksXm::set_close_cb(p_app_shell, menuCB, (void *)idMenuFileExit);

  attach_bottom(idMainPanedWindow);

  refresh();
  show();

    // adjust right column width

  XtAddCallback(p_schema_matrix, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);

  subscribe_create_class(createNF);
  subscribe_delete_class(deleteNF);
  subscribe_change_class(changeNF);

  if(p_no_msg_window == false) { 
    p_msg_window = new OksXmMessageWindow("Oks Schema Editor Message Log", this, msg_dlg_auto_scroll);
  }

  setIcon(OksXm::create_color_pixmap(form, schema_editor_about_xpm));

    // register handlers for user's signals

  signal(SIGINT,signal_handler);
  signal(SIGTERM,signal_handler);

  start_check_need_backup();

  XtUnmapWidget(tmpw);

  XtAddEventHandler (p_app_shell, StructureNotifyMask, False, configureAC, NULL);
}


OksSchemaEditorMainDialog::~OksSchemaEditorMainDialog()
{
  subscribe_create_class(0);
  subscribe_delete_class(0);
  subscribe_change_class(0);

  while(!dialogs.empty()) {
    OksSchemaEditorDialog * dlg = dialogs.front();
    dialogs.pop_front();
    delete dlg;
  }

  XtUnmanageChild(form);

  close_all_schema();
}


void
OksSchemaEditorMainDialog::refresh()
{
  showClassList();
  showSchemaList();
}


void
OksSchemaEditorMainDialog::showClassList()
{
  OksForm * f = get_paned_window(idMainPanedWindow)->get_form(idClasses);
  Widget w = f->get_widget(idClasses);
  OksXm::AutoCstring item(OksXm::List::get_selected_value(w));
  
  XmListDeleteAllItems(w);
  
  for(OksClass::Map::const_iterator i = classes().begin(); i != classes().end(); ++i)
    OksXm::List::add_row(w, (char *)(i->first));

  if(item.get()) {
    OksXm::List::select_row(w, item.get());
  }
}


void
OksSchemaEditorMainDialog::showSchemaList()
{
  const char * selection = OksXm::Matrix::get_selected_value(p_schema_matrix);

  std::string s; /* keeps copy of selected item: it will be destroyed */
  if(selection) s = selection;

  XbaeMatrixDeleteRows(p_schema_matrix, 0, XbaeMatrixNumRows(p_schema_matrix));
  XtVaSetValues(p_schema_matrix, XmNfill, true, NULL);

  size_t count = 0;
  for(OksFile::Map::const_iterator i = schema_files().begin(); i != schema_files().end(); ++i) {
    append_schema_row(i->second);
    XbaeMatrixSetRowUserData(p_schema_matrix, count++, (XtPointer)i->second);
  }

	//
	// Do not allow matrix capture all screen's width
	// if filenames are too long (set 75% limit)
	//

  static Dimension max_width = (WidthOfScreen(XtScreen(p_schema_matrix)) / 4) * 3;

  OksXm::Matrix::adjust_column_widths(p_schema_matrix, max_width);
  OksXm::Matrix::set_visible_height(p_schema_matrix);
  OksXm::Matrix::set_widths(p_schema_matrix, 2);
  OksXm::Matrix::set_cell_background(
    p_schema_matrix,
    get_paned_window(idMainPanedWindow)->get_form(idClasses)->get_widget(idClasses)
  );

  if(!s.empty()) {
    OksXm::Matrix::select_row(p_schema_matrix, s.c_str());
  }
}


OksSchemaEditorSchemaDialog *
OksSchemaEditorMainDialog::find_schema_dlg(const OksFile * file_h)
{
  for(std::list<OksSchemaEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksSchemaEditorDialog *osed = *i;
    if(osed->type() == OksSchemaEditorDialog::SchemaDlg) {
      OksSchemaEditorSchemaDialog *dlg = static_cast<OksSchemaEditorSchemaDialog *>(osed);
      if(dlg->get_file() == file_h) return dlg;
    }
  }

  return 0;
}

OksSchemaEditorClassDialog *
OksSchemaEditorMainDialog::find_class_dlg(const OksClass *c)
{
  for(std::list<OksSchemaEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksSchemaEditorDialog *osed = *i;
    if(osed->type() == OksSchemaEditorDialog::ClassDlg) {
      OksSchemaEditorClassDialog *dlg = static_cast<OksSchemaEditorClassDialog *>(osed);
      if(dlg->get_class() == c) return dlg;
    }
  }
  	
  return 0;
}

OksSchemaEditorViewDialog *
OksSchemaEditorMainDialog::find_view_dlg(const std::string& s)
{
  for(std::list<OksSchemaEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksSchemaEditorDialog *osed = *i;
    if(osed->type() == OksSchemaEditorDialog::ViewDlg) {
      OksSchemaEditorViewDialog *dlg = static_cast<OksSchemaEditorViewDialog *>(osed);
      if(dlg->get_name() == s) return dlg;
    }
  }

  return 0;
}

OksSchemaEditorAttributeDialog *
OksSchemaEditorMainDialog::find_attribute_dlg(const OksAttribute *a, const OksClass *c)
{
  for(std::list<OksSchemaEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksSchemaEditorDialog *osed = *i;
    if(osed->type() == OksSchemaEditorDialog::AttributeDlg) {
      OksSchemaEditorAttributeDialog *dlg = static_cast<OksSchemaEditorAttributeDialog *>(osed);
      if(dlg->get_attribute() == a && dlg->get_class() == c) return dlg;
    }
  }

  return 0;
}

OksSchemaEditorRelationshipDialog *
OksSchemaEditorMainDialog::find_relationship_dlg(const OksRelationship *r, const OksClass *c)
{
  for(std::list<OksSchemaEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksSchemaEditorDialog *osed = *i;
    if(osed->type() == OksSchemaEditorDialog::RelationshipDlg) {
      OksSchemaEditorRelationshipDialog *dlg = static_cast<OksSchemaEditorRelationshipDialog *>(osed);
      if(dlg->get_relationship() == r && dlg->get_class() == c) return dlg;
    }
  }

  return 0;
}

OksSchemaEditorMethodDialog *
OksSchemaEditorMainDialog::find_method_dlg(const OksMethod *m, const OksClass *c)
{
  for(std::list<OksSchemaEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksSchemaEditorDialog *osed = *i;
    if(osed->type() == OksSchemaEditorDialog::MethodDlg) {
      OksSchemaEditorMethodDialog *dlg = static_cast<OksSchemaEditorMethodDialog *>(osed);
      if(dlg->get_method() == m && dlg->get_class() == c) return dlg;
    }
  }

  return 0;
}

OksSchemaEditorMethodImplementationDialog *
OksSchemaEditorMainDialog::find_method_implementation_dlg(const OksMethodImplementation * m)
{
  for(std::list<OksSchemaEditorDialog *>::iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    OksSchemaEditorDialog *osed = *i;
    if(osed->type() == OksSchemaEditorDialog::MethodImplementationDlg) {
      OksSchemaEditorMethodImplementationDialog *dlg = static_cast<OksSchemaEditorMethodImplementationDialog *>(osed);
      if(dlg->get_method_implementation() == m) return dlg;
    }
  }

  return 0;
}

void
OksSchemaEditorMainDialog::remove_dlg(OksSchemaEditorDialog *dlg)
{
  dialogs.remove(dlg);
  update_menu_bar();
}

void
OksSchemaEditorMainDialog::raise_dlg(OksSchemaEditorDialog * dlg) const
{
  XMapRaised(display(), XtWindow(XtParent(dlg->get_form_widget())));
}


void
OksSchemaEditorMainDialog::create_schema_dlg(OksFile * f)
{
  OksSchemaEditorSchemaDialog * osesd = find_schema_dlg(f);

  if(osesd) 
    raise_dlg(osesd);
  else {
    dialogs.push_back(new OksSchemaEditorSchemaDialog(f, this));
    update_menu_bar();
  }
}


OksSchemaEditorViewDialog *
OksSchemaEditorMainDialog::create_view_dlg(const std::string & s)
{
  OksSchemaEditorViewDialog * osevd = s.empty() ? 0 : find_view_dlg(s);

  if(osevd) 
    raise_dlg(osevd);
  else {
    osevd = (
      s.empty() ? new OksSchemaEditorViewDialog(this) : new OksSchemaEditorViewDialog(s, this)
    );
    dialogs.push_back(osevd);
    update_menu_bar();
  }

  return osevd;
}

OksSchemaEditorClassDialog *
OksSchemaEditorMainDialog::create_class_dlg(OksClass *c)
{
  OksSchemaEditorClassDialog * osesd = find_class_dlg(c);

  if(osesd) 
    raise_dlg(osesd);
  else {
    dialogs.push_back( (osesd = new OksSchemaEditorClassDialog(c, this)) );
    update_menu_bar();
  }

  return osesd;
}


OksSchemaEditorAttributeDialog *
OksSchemaEditorMainDialog::create_attribute_dlg(OksAttribute *a, OksClass *c)
{  
  OksSchemaEditorAttributeDialog *osead = find_attribute_dlg(a, c);

  if(osead) 
    raise_dlg(osead);
  else {
    dialogs.push_back(osead = new OksSchemaEditorAttributeDialog(a, c, this));
    update_menu_bar();
  }
  
  return osead;
}

OksSchemaEditorRelationshipDialog *
OksSchemaEditorMainDialog::create_relationship_dlg(OksRelationship *r, OksClass *c)
{  
  OksSchemaEditorRelationshipDialog *oserd = find_relationship_dlg(r, c);

  if(oserd) 
    raise_dlg(oserd);
  else {
    dialogs.push_back(oserd = new OksSchemaEditorRelationshipDialog(r, c, this));
    update_menu_bar();
  }
  
  return oserd;
}

OksSchemaEditorMethodDialog *
OksSchemaEditorMainDialog::create_method_dlg(OksMethod *m, OksClass *c)
{  
  OksSchemaEditorMethodDialog * osemd = find_method_dlg(m, c);

  if(osemd) 
    raise_dlg(osemd);
  else {
    dialogs.push_back(osemd = new OksSchemaEditorMethodDialog(m, c, this));
    update_menu_bar();
  }

  return osemd;
}

OksSchemaEditorMethodImplementationDialog *
OksSchemaEditorMainDialog::create_method_implementation_dlg(OksMethodImplementation *i, OksMethod *m, OksClass *c)
{  
  OksSchemaEditorMethodImplementationDialog * osemid = find_method_implementation_dlg(i);

  if(osemid) 
    raise_dlg(osemid);
  else {
    dialogs.push_back(osemid = new OksSchemaEditorMethodImplementationDialog(i, m, c, this));
    update_menu_bar();
  }

  return osemid;
}


void
OksSchemaEditorMainDialog::menuCB (Widget w, XtPointer client_data, XtPointer call_data) 
{
  switch ((long)client_data) {
    case idMenuFileExit:
      mainDlg->update();
      exit_app("Exit");
      break;

    case idAcMainViewNew:
      mainDlg->create_view_dlg("");
      break;

    case idAcMainViewOpen: {
      std::string s = OksXm::ask_file(
        mainDlg->get_menu_bar(),
	"Input name of the file to open view",
	"Selection of file with view",
	0, "*.view.xml"
      );

      if(s.length()) {
        mainDlg->create_view_dlg(s);
      }
      break; }

    case idAcMainViewMakeSchema: {
      std::string s = OksXm::ask_file(
        mainDlg->get_menu_bar(),
	"Input name of schema file",
	"Selection of (new) file for schema",
	0, "*.schema.xml"
      );

      if(s.length()) {
        OksSchemaEditorViewDialog::save_schema(s);
      }
      break; }

    case idMenuOptionCheckNeedBackupMode:
      if(((XmToggleButtonCallbackStruct *)call_data)->set) {
	mainDlg->start_check_need_backup();
	XtSetSensitive(mainDlg->backupPeriodBtn, true);
      }
      else {
	mainDlg->stop_check_need_backup();
	XtSetSensitive(mainDlg->backupPeriodBtn, false);
      }

      break;

    case idMenuOptionCheckNeedBackupPeriod: {
      mainDlg->stop_check_need_backup();

      std::ostringstream ss;
      ss << mainDlg->p_check_need_backup_timeout;

      std::string buf = ss.str();
      std::string s = OksXm::ask_name(
        mainDlg->get_form_widget(),
        "Backup modified files period",
        "Input period (from 5 to 3600 seconds):",
        buf.c_str()
      );

      if(s.length()) {
        mainDlg->p_check_need_backup_timeout = atoi(s.c_str());

        if(mainDlg->p_check_need_backup_timeout < 5) {
          std::cerr << "WARNING: period " << mainDlg->p_check_need_backup_timeout << " is too small.\n";
          mainDlg->p_check_need_backup_timeout = 5;
          std::cerr << "Set the period equal " << mainDlg->p_check_need_backup_timeout << " seconds.\n";
        }
        else if(mainDlg->p_check_need_backup_timeout > 3600) {
          std::cerr << "WARNING: period " << mainDlg->p_check_need_backup_timeout << " is too big.\n";
          mainDlg->p_check_need_backup_timeout = 3600;
          std::cerr << "Set the period equal " << mainDlg->p_check_need_backup_timeout << " seconds.\n";
	}
      }

      mainDlg->start_check_need_backup();

      break; }

    case idMenuWindowMessages:
      mainDlg->get_msg_window()->show_window();
      break;

    case idMenuHelpHelp:
       break;

    case idMenuHelpAbout: {
      static Widget about_dlg = 0;

      if(!about_dlg) {
        std::string s("OKS Schema Editor ");
        s += OksKernel::GetVersion();
        s += "\n<Igor.Soloviev@cern.ch>";
        about_dlg = OksXm::create_dlg_with_pixmap(w, "About", schema_editor_about_xpm, s.c_str(), "Ok", 0, 0, 0, 0);
      }

      XtManageChild(about_dlg);
      break;
    }
  }
}


void
OksSchemaEditorMainDialog::create_menu_bar(Widget menu_bar)
{
  Widget menu_pane = 0;	


    // File menu

  menu_pane = XmCreatePulldownMenu (menu_bar, (char *)"File", 0, 0);

  OksXm::MenuBar::add_push_button(menu_pane, "Exit", idMenuFileExit, menuCB),

  OksXm::MenuBar::add_cascade_button(menu_bar, menu_pane, "File", 'F');


    // View menu

  menu_pane = XmCreatePulldownMenu (menu_bar, (char *)"View", 0, 0);

  OksXm::MenuBar::add_push_button(menu_pane, "New",  idAcMainViewNew, menuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Open", idAcMainViewOpen, menuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "Make View Schema File", idAcMainViewMakeSchema, menuCB);

  OksXm::MenuBar::add_cascade_button(menu_bar, menu_pane, "View", 'V');


    // Options menu

  menu_pane = XmCreatePulldownMenu (menu_bar, (char *)"Options", 0, 0);

  OksXm::MenuBar::add_toggle_button(menu_pane, "Backup modified files", idMenuOptionCheckNeedBackupMode, true, menuCB);
  backupPeriodBtn = OksXm::MenuBar::add_push_button(menu_pane, "Set backup period", idMenuOptionCheckNeedBackupPeriod, menuCB);

  OksXm::MenuBar::add_cascade_button(menu_bar, menu_pane, "Options", 'O');


    // Window menu

  p_windows_pane = XmCreatePulldownMenu (menu_bar, (char *)"Windows", 0, 0);

  Widget msg_btn = OksXm::MenuBar::add_push_button(p_windows_pane, szMessageLog, idMenuWindowMessages, menuCB);

  if(p_no_msg_window == true) XtSetSensitive(msg_btn, false);

  OksXm::MenuBar::add_cascade_button(menu_bar, p_windows_pane, "Windows", 'W');


    // Help menu

  menu_pane = XmCreatePulldownMenu (menu_bar, (char *)"Help", 0, 0);

  OksXm::MenuBar::add_push_button(menu_pane, szHelp, idMenuHelpHelp, menuCB);
  OksXm::MenuBar::add_push_button(menu_pane, "About...", idMenuHelpAbout, menuCB);

  XtVaSetValues(menu_bar,
    XmNmenuHelpWidget, OksXm::MenuBar::add_cascade_button(menu_bar, menu_pane, "Help", 'H'),
    NULL
  );
}

void
OksSchemaEditorMainDialog::raiseCB(Widget w, XtPointer client_data, XtPointer)
{
  XMapRaised(XtDisplay(w), XtWindow((Widget)client_data));
}

void
OksSchemaEditorMainDialog::update_menu_bar()
{
  WidgetList	items;
  int		nItems,
  		n;
  Widget	w;

  XtVaGetValues(p_windows_pane, XmNchildren, &items, XmNnumChildren, &nItems, NULL);

  for(n=0; n < nItems; n++) {
    XtUnmanageChild(items[n]);
    XtDestroyWidget(items[n]);
  }

  w = OksXm::MenuBar::add_push_button(p_windows_pane, szMessageLog, idMenuWindowMessages, menuCB);

  if(p_no_msg_window == true) XtSetSensitive(w, false);


  if(!dialogs.empty()) {
    OksSchemaEditorDialog::Type types[] = {
      OksSchemaEditorDialog::SchemaDlg,
      OksSchemaEditorDialog::ViewDlg,
      OksSchemaEditorDialog::ClassDlg,
      OksSchemaEditorDialog::AttributeDlg,
      OksSchemaEditorDialog::RelationshipDlg,
      OksSchemaEditorDialog::MethodDlg,
      OksSchemaEditorDialog::MethodImplementationDlg
    };
    
    for(size_t j = 0; j < sizeof(types)/sizeof(OksSchemaEditorDialog::Type); ++j) {
      bool first = true;
      
      for(std::list<OksSchemaEditorDialog *>::const_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
        if((*i)->type() == types[j]) {
	  if(first == true) {
	    XtManageChild(XmCreateSeparator(p_windows_pane, (char *)"", 0, 0));
	    first = false;
	  }

          char * title;
          Widget shell = XtParent((*i)->get_form_widget());

          XtVaGetValues(shell, XmNtitle, &title, NULL);

          Widget w = XmCreatePushButton(p_windows_pane, title, 0, 0);
          XtAddCallback(w, XmNactivateCallback, raiseCB, (XtPointer)shell);
          XtManageChild(w);
        }
      }
    }
  }
}


void
OksSchemaEditorMainDialog::saveCB(Widget, XtPointer client_data, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    try {
      OksFile * file_h = reinterpret_cast<OksFile *>(client_data);
      mainDlg->save_schema(file_h);
      mainDlg->update_schema_row(file_h);
      mainDlg->remove_backup(file_h);
      done_with_dialog = OksSchemaEditorMainDialog::Save;
    }
    catch (oks::exception & ex) {
      std::cerr << "ERROR: " << ex.what() << std::endl;
    }
  }
  else if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_CANCEL) {
    done_with_dialog = OksSchemaEditorMainDialog::Skip;
  }
  else {
    done_with_dialog = OksSchemaEditorMainDialog::CancelExit;
  }
}


void
OksSchemaEditorMainDialog::cancelCB(Widget, XtPointer, XtPointer)
{
  done_with_dialog = OksSchemaEditorMainDialog::CancelExit;
}


OksSchemaEditorMainDialog::DialogReturn
OksSchemaEditorMainDialog::confirm_changed(const char *action, OksFile * file_h)
{
  if(!file_h->is_updated()) return NonStop;

  std::string s("Save schema file \"");
  s += file_h->get_full_file_name();
  s += "\" before ";
  s += action;
  s += '?';

  Widget w = OksXm::create_dlg_with_pixmap(p_menu_bar, "Save Changes", exit_xpm, s.c_str(), "Save", "Skip", "Cancel", 0, 0);

  XtVaSetValues(w, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

  XtAddCallback(w, XmNokCallback, saveCB, (XtPointer)file_h);	/* SAVE */
  XtAddCallback(w, XmNcancelCallback, saveCB, (XtPointer)file_h);	/* SKIP */
  XtAddCallback(XmSelectionBoxGetChild(w, XmDIALOG_HELP_BUTTON),
			XmNactivateCallback, cancelCB, 0);	/* CANCEL */

  OksXm::set_close_cb(XtParent(w), cancelCB, 0);

  XtManageChild(w);

  done_with_dialog = NoAnswer;

  while(done_with_dialog == NoAnswer)
    XtAppProcessEvent(XtWidgetToApplicationContext(w), XtIMAll);

  XtDestroyWidget(XtParent(w));

  return done_with_dialog;
}


void
OksSchemaEditorMainDialog::update()
{
  for(std::list<OksSchemaEditorDialog *>::const_iterator i = dialogs.begin(); i != dialogs.end(); ++i) {
    (*i)->update();
  }
}

void
OksSchemaEditorMainDialog::create_select_class_dlg(const char * title, const std::list<OksClass *> * exclude, XtCallbackProc f, XtPointer c_data)
{
  Widget dialog, listw, shell;
  Widget kid[6];
  Arg args[10];
  int n;
  XmString listLabelString = OksXm::create_string("Classes:");
  XmString textLabelString = OksXm::create_string("double click to select class");
  
  n = 0;
  XtSetArg (args[n], XmNallowShellResize, True);  n++;
  shell = XtCreatePopupShell (title, topLevelShellWidgetClass, get_form_widget(), args, n);
  
  n = 0;
  XtSetArg (args[n], XmNdialogType, XmDIALOG_SELECTION);  n++;
  XtSetArg (args[n], XmNlistLabelString, listLabelString);  n++;
  XtSetArg (args[n], XmNselectionLabelString, textLabelString);  n++;
  dialog = XmCreateSelectionBox (shell, const_cast<char *>(title), args, n);
  XtVaSetValues(XtParent(dialog), XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

  n = 0;
  kid[n++] = XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT);
  kid[n++] = XmSelectionBoxGetChild(dialog, XmDIALOG_SEPARATOR);
  kid[n++] = XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON);
  kid[n++] = XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON);
  kid[n++] = XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON);
  kid[n++] = XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
  XtUnmanageChildren(kid, n);

  listw = XmSelectionBoxGetChild(dialog, XmDIALOG_LIST);

  for (const auto& i : classes())
    {
      const char * name = i.first;

      if (exclude)
        for (const auto& j : *exclude)
          {
            if (i.second == j)
              {
                name = nullptr;
                break;
              }
          }

      if (name)
        OksXm::List::add_row(listw, name);
    }

  XmStringFree(listLabelString);
  XmStringFree(textLabelString);

  XtAddCallback(listw, XmNdefaultActionCallback, f, c_data);
  XtManageChild(dialog);
  XtRealizeWidget(XtParent(dialog));  
  XtPopup(XtParent(dialog), XtGrabNone);
}

void
OksSchemaEditorMainDialog::destroy_select_class_dlg(Widget w)
{
  Widget top = XtParent(XtParent(XtParent(w)));
  XtUnmanageChild(top);
  XtDestroyWidget(top);
}


void
OksSchemaEditorMainDialog::fill_schema_row(OksFile * fp, String *row) const
{
  row[0] = const_cast<String>(fp->get_full_file_name().c_str());
  row[1] = const_cast<String>(OksKernel::check_read_only(fp) ? "read-only" : "read-write");
  row[2] = const_cast<String>(fp->is_updated() ? "updated" : (fp->is_locked() ? "locked" : ""));
  row[3] = const_cast<String>((get_active_schema() == fp && fp->is_read_only() == false) ? "active" : "");
  row[4] = const_cast<String>("");
}


void
OksSchemaEditorMainDialog::update_schema_row(OksFile * pf) const
{
  int rowsNumber = XbaeMatrixNumRows(p_schema_matrix);
  int pos;

  for(pos=0; pos<rowsNumber; pos++) {
    const char *first_cell = XbaeMatrixGetCell(p_schema_matrix, pos, 0);
    if(first_cell && !strcmp(first_cell, pf->get_full_file_name().c_str())) break;
  }

  if(pos>=rowsNumber) return;

  String row[NumOfSchemaFilesMatrixColumns];

  fill_schema_row(pf, row);

  for(size_t i=0; i<NumOfSchemaFilesMatrixColumns; ++i)
    XbaeMatrixSetCell(p_schema_matrix, pos, i, row[i]);
}


void
OksSchemaEditorMainDialog::append_schema_row(OksFile * fp) const
{
    // set XmNfill to false if it was true
    // (this happens when add first row)
  {
    Boolean fill;
    XtVaGetValues(p_schema_matrix, XmNfill, &fill, NULL);

    if(fill == true) {
      XtVaSetValues(p_schema_matrix, XmNfill, false, NULL);
    }
  }

  String row[NumOfSchemaFilesMatrixColumns];
  fill_schema_row(fp, row);
  XbaeMatrixAddRows(p_schema_matrix, XbaeMatrixNumRows(p_schema_matrix), row, 0, 0, 1);
}


    //
    // The method returns true if there is active schema file
    // and it has read-write access
    //

bool
OksSchemaEditorMainDialog::is_valid_active_schema()
{
  OksFile * f = mainDlg->get_active_schema();
  
  if(f) {
    return !(f->is_read_only());
  }

  return false;
}

void
OksSchemaEditorMainDialog::create_backup(OksFile * pf)
{
  try {
    backup_schema(pf, ".saved");
    p_backup_files.insert(pf);
  }
  catch (oks::exception & ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
  }
}

void
OksSchemaEditorMainDialog::remove_backup(OksFile * pf)
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
    std::cerr <<
      "ERROR [remove_backup()]:\n"
      "  failed to unlink backup file \"" << name << "\":\n"
      "  unlink() failed with code " << code << ", reason = \'" << strerror(errno) << "\'\n";
  }
  else {
    p_backup_files.erase(pf);
  }
}

void
OksSchemaEditorMainDialog::start_check_need_backup()
{
  if(p_check_need_backup_timeout > 0) {
    p_check_need_backup_timer = XtAppAddTimeOut(
      XtWidgetToApplicationContext(p_menu_bar), p_check_need_backup_timeout*1000, check_need_backup_files_cb, (XtPointer)this
    );
  }
}

void
OksSchemaEditorMainDialog::stop_check_need_backup()
{
  if(p_check_need_backup_timer) {
    XtRemoveTimeOut(p_check_need_backup_timer);
    p_check_need_backup_timer = 0;
  }
}

void
OksSchemaEditorMainDialog::check_need_backup_files_cb(XtPointer call_data, XtIntervalId*)
{
  OksSchemaEditorMainDialog * dlg = reinterpret_cast<OksSchemaEditorMainDialog *>(call_data);

  for(OksFile::Map::const_iterator i = dlg->schema_files().begin(); i != dlg->schema_files().end(); ++i) {
    if(i->second->is_updated()) {
      try {
        dlg->backup_schema(i->second, ".saved");
        ERS_DEBUG( 2 , "made backup of \'" << *(i->first) << '\'');
      }
      catch(oks::exception& ex) {
        Oks::error_msg("OksSchemaEditorMainDialog::check_need_backup_files_cb")
          << "  Failed to make a backup of \'" << *(i->first) << "\':\n"
	  << ex.what() << std::endl;
      }
    }
  }

  dlg->start_check_need_backup();
}

void
OksSchemaEditorMainDialog::test_bind_classes_status() const
{
  if(!get_bind_classes_status().empty())
    {
      std::cerr << "WARNING: " << "The schema contains dangling references to non-loaded classes:\n" << get_bind_classes_status();
    }
}
