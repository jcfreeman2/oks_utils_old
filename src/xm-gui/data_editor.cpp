#include <sstream>
#include <iostream>

#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/class.h>
#include <oks/xm_msg_window.h>
#include <oks/xm_context.h>
#include <oks/xm_utils.h>

#include "data_editor.h"
#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"

#include "stop.xpm"

OksDataEditorMainDialog * mainDlg;

std::string OksParameterNames::p_icon_width_name   = "Icon width";
std::string OksParameterNames::p_ch_obj_dx_name    = "Horizontal distance between child objects";
std::string OksParameterNames::p_ch_obj_dy_name    = "Vertical distance between child objects";
std::string OksParameterNames::p_ch_max_width_name = "Wrap children objects at (max width)";
std::string OksParameterNames::p_obj_dx_name       = "Object horizontal margin";
std::string OksParameterNames::p_obj_dy_name       = "Object vertical margin";

std::string OksParameterNames::p_arrange_objs_name                = "Arrange objects";
std::string OksParameterNames::p_arrange_obj_as_row_name          = "As row";
std::string OksParameterNames::p_arrange_obj_as_column_name       = "As column";
std::string OksParameterNames::p_arrange_obj_by_classes_name      = "By classes";
std::string OksParameterNames::p_arrange_obj_wrap_any_name        = "Wrap any";
std::string OksParameterNames::p_arrange_obj_wrap_by_classes_name = "Wrap by classes";

std::string OksParameterNames::p_arrange_child_name                 = "Arrange children";
std::string OksParameterNames::p_arrange_one_child_per_line_name    = "One child per line";
std::string OksParameterNames::p_arrange_many_children_per_lin_name = "Many children per line";

std::string OksParameterNames::p_ext_modified_files_check_name      = "Externally modified files check";
std::string OksParameterNames::p_ext_modified_files_value_name      = "Externally modified files value";
std::string OksParameterNames::p_backup_modified_files_check_name   = "Backup modified files check";
std::string OksParameterNames::p_backup_modified_files_value_name   = "Backup modified files value";
std::string OksParameterNames::p_ask_comment_name                   = "Ask comment";
std::string OksParameterNames::p_author_name                        = "Author Name";

std::string OksParameterNames::p_main_window_x_pos_name             = "Main Window X Pos";
std::string OksParameterNames::p_main_window_y_pos_name             = "Main Window Y Pos";
std::string OksParameterNames::p_message_window_x_pos_name          = "Message Window X Pos";
std::string OksParameterNames::p_message_window_y_pos_name          = "Message Window Y Pos";
std::string OksParameterNames::p_restore_main_window_pos_name       = "Restore Main Window Pos";



static void
usage(std::ostream& s)
{
  s <<
    "Usage: oks_data_editor [X toolkit command line options] [--no-message-window] [--no-message-window-auto-scroll]\n"
    "                       [-V | --oks-verbose]\n"
    "                       [-v | --verbose]\n"
    "                       [-I | --init-dirs init-directories]\n"
    "                       [-D | --init-data-files init-data-files]\n"
    "                       [-P | --pixmap-dirs pixmap-directories]\n"
    "                       [-B | --bitmap-dirs bitmap-directories]\n"
    "                       [-W | --icon_width width]\n"
    "                       [-H | --ch-obj-dx width]\n"
    "                       [-R | --ch-obj-dy width]\n"
    "                       [-A | --ch-obj-max-width width]\n"
    "                       [-O | --font-dx width]\n"
    "                       [-E | --font-dy width]\n"
    "                       [-Z | --obj-dx width]\n"
    "                       [-T | --obj-dy width]\n"
    "                       [-N | --da-x-margin width]\n"
    "                       [-C | --da-y-margin width]\n"
    "                       [schema-file(s)] [data-file(s)]\n"
    "\n"
    "Options/Arguments:\n"
    "  --no-message-window                               do not create graphical window for messages and errors\n"
    "  --no-message-window-auto-scroll                   do not auto scroll message window unless there are new warnings or errors\n"
    "                                                    (to fix performance problem of Motif 2.3)\n"
    "  -v | --verbose                                    display editor messages\n"
    "  -a | --allow-duplicated-objects-via-inheritance   do not stop if there are duplicated object via inheritance hierarchy\n"
    "  -u | --allow-duplicated-classes                   do not stop if there are duplicated classes\n"
    "  -I | --init-dirs init-directories                 colon-separated list of directories containing initialization files\n"
    "  -D | --init-data-files data-files                 colon-separated list of initialization data files (absolute or relative to init-dirs)\n"
    "  -P | --pixmap-dirs pixmap-dirs                    colon-separated list of pixmap directories (absolute or relative to init-dirs)\n"
    "  -B | --bitmap-dirs bitmap-dirs                    colon-separated list of bitmap directories (absolute or relative to init-dirs)\n"
    "  -W | --icon_width width                           graphical window: width of the icons\n"
    "  -H | --ch-obj-dx width                            graphical window: children objects horizontal spacing\n"
    "  -R | --ch-obj-dy width                            graphical window: children objects vertical spacing\n"
    "  -A | --ch-obj-max-width width                     graphical window: maximum width of child object (wrap relationship branch)\n"
    "  -O | --font-dx width                              graphical window: text horizontal spacing\n"
    "  -E | --font-dy width                              graphical window: text vertical spacing\n"
    "  -Z | --obj-dx width                               graphical window: graphical objects horizontal spacing\n"
    "  -T | --obj-dy width                               graphical window: graphical objects vertical spacing\n"
    "  -N | --da-x-margin width                          graphical window: drawing area top/bottom margins\n"
    "  -C | --da-y-margin width                          graphical window: drawing area left/right margins\n"
    "\n"
    "Description:\n"
    "  The program is OKS data graphical editor.\n"
    "\n"
    "Related environment variables:\n"
    "  OKS_GUI_NO_MSG_WINDOW               do not show message window\n"
    "  OKS_GUI_MSG_WINDOW_NO_AUTO_SCROLL   do not auto scroll message window if there are no warnings or errors\n"
    "  OKS_GUI_HELP_URL                    specifies base URL for online help (e.g. 'file://local/share/online-help')\n"
    "  OKS_GUI_PATH                        same as \'--init-dirs\' command line parameter\n"
    "  OKS_GUI_INIT_DATA                   same as \'--init-data-files\' command line parameter\n"
    "  OKS_GUI_XPM_DIRS                    same as \'--pixmap-dirs\' command line parameter\n"
    "  OKS_GUI_XBM_DIRS                    same as \'--bitmap-dirs\' command line parameter\n"
    "  TDAQ_INST_PATH                      default place for configuration files for online sw release (obsolete way, use OKS_GUI_PATH)\n"
    "                                      OKS_GUI_PATH = \'${TDAQ_INST_PATH}/share/data\'\n\n";
}

static void
no_param(const char * s)
{
  std::cerr << "ERROR: no parameter for " << s << " provided\n\n";
  usage(std::cerr);
}

const char * fallback_resources[][2] = {
  { "*XmFrame*Background",                           "rgb:f6/f6/f6" },
  { "*XmList.background",                            "rgb:f6/f6/f6" },
  { "*XmTextField.background",                       "rgb:f6/f6/f6" },
  { "*XmText.background",                            "rgb:f6/f6/f6" },
  { "*XmComboBox*background",                        "rgb:f6/f6/f6" },
  { "*XbaeMatrix*cellBackgrounds",                   "rgb:f6/f6/f6" },
  { "*File.Save All Updated.accelerator",            "Ctrl<Key>S"   },
  { "*File.Save All Updated.acceleratorText",        "Ctrl+S"       },
  { "*File.Release User Repository.accelerator",     "Ctrl<Key>D"   },
  { "*File.Release User Repository.acceleratorText", "Ctrl+D"       },
  { "*File.Update User Repository.accelerator",      "Ctrl<Key>U"   },
  { "*File.Update User Repository.acceleratorText",  "Ctrl+U"       },
  { "*File.Commit User Repository.accelerator",      "Ctrl<Key>C"   },
  { "*File.Commit User Repository.acceleratorText",  "Ctrl+C"       },
  { "*File.Exit.accelerator",                        "Ctrl<Key>Q"   },
  { "*File.Exit.acceleratorText",                    "Ctrl+Q"       },
  { "*File.Check consistency.accelerator",           "Ctrl<Key>Y"   },
  { "*File.Check consistency.acceleratorText",       "Ctrl+Y"       },
  { "*Edit.Find.accelerator",                        "Ctrl<Key>F"   },
  { "*Edit.Find.acceleratorText",                    "Ctrl+F"       },
  { "*Edit.Replace.accelerator",                     "Ctrl<Key>R"   },
  { "*Edit.Replace.acceleratorText",                 "Ctrl+R"       },
  { "*Edit.Refresh.accelerator",                     "Ctrl<Key>E"   },
  { "*Edit.Refresh.acceleratorText",                 "Ctrl+E"       },
  { "*Windows.Help.accelerator",                     "Ctrl<Key>H"   },
  { "*Windows.Help.acceleratorText",                 "Ctrl+H"       },
  { "*Windows.Message Log.accelerator",              "Ctrl<Key>M"   },
  { "*Windows.Message Log.acceleratorText",          "Ctrl+M"       },
  { 0,                                               0              }
};


int main (int argc, char **argv)
{
    // set default parameters

  OksGC::Parameters& dp = OksGC::Parameters::def_params();
  dp.set_name("oks-data-editor");

  dp.set_graphical_parameter(OksParameterNames::get_icon_width_name(),   32);
  dp.set_graphical_parameter(OksParameterNames::get_ch_obj_dx_name(),    3);
  dp.set_graphical_parameter(OksParameterNames::get_ch_obj_dy_name(),    2);
  dp.set_graphical_parameter(OksParameterNames::get_ch_max_width_name(), 768);
  dp.set_graphical_parameter(OksParameterNames::get_obj_dx_name(),       3);
  dp.set_graphical_parameter(OksParameterNames::get_obj_dy_name(),       4);

  dp.set_graphical_parameter(
    OksParameterNames::get_arrange_objects_name(),
    OksParameterNames::get_arrange_obj_by_classes_name(),
    (
      OksParameterNames::get_arrange_obj_as_row_name() + ',' +
      OksParameterNames::get_arrange_obj_as_column_name() + ',' +
      OksParameterNames::get_arrange_obj_by_classes_name() + ',' +
      OksParameterNames::get_arrange_obj_wrap_any_name() + ',' +
      OksParameterNames::get_arrange_obj_wrap_by_classes_name()
    )
  );

  dp.set_graphical_parameter(
    OksParameterNames::get_arrange_children_name(),
    OksParameterNames::get_arrange_one_child_per_line_name(),
    (
      OksParameterNames::get_arrange_one_child_per_line_name() + ',' +
      OksParameterNames::get_arrange_many_children_per_lin_name()
    )
  );

  dp.set_generic_parameter(OksParameterNames::get_ext_modified_files_check_name(), static_cast<unsigned short>(true));
  dp.set_generic_parameter(OksParameterNames::get_ext_modified_files_value_name(), 60);
  dp.set_generic_parameter(OksParameterNames::get_backup_modified_files_check_name(), static_cast<unsigned short>(true));
  dp.set_generic_parameter(OksParameterNames::get_backup_modified_files_value_name(), 60);
  dp.set_generic_parameter(OksParameterNames::get_ask_comment_name(), static_cast<unsigned short>(true));
  dp.set_generic_parameter(OksParameterNames::get_author_name(), "Unknown");

  dp.set_generic_parameter(OksParameterNames::get_main_window_x_pos(), static_cast<unsigned short>(10));
  dp.set_generic_parameter(OksParameterNames::get_main_window_y_pos(), static_cast<unsigned short>(10));
  dp.set_generic_parameter(OksParameterNames::get_message_window_x_pos(), static_cast<unsigned short>(310));
  dp.set_generic_parameter(OksParameterNames::get_message_window_y_pos(), static_cast<unsigned short>(10));
  dp.set_generic_parameter(OksParameterNames::get_restore_main_window_pos(), static_cast<unsigned short>(true));


  std::ostringstream rc_read_error_text;

  try {
    dp.read();
  }
  catch(oks::FailedLoadGuiRcFile& ex) {
    rc_read_error_text << "WARNING: failed to read GUI rc file:\n" << ex.what();
  }


  bool no_msg_dlg = (getenv("OKS_GUI_NO_MSG_WINDOW") != 0);
  bool msg_dlg_auto_scroll = (getenv("OKS_GUI_MSG_WINDOW_NO_AUTO_SCROLL") == 0);

    // check is user is asking for help or does not need message window

  for(int count = 0; count < argc; count++) {
    if(!strcmp(argv[count], "-h") || !strcmp(argv[count], "--help")) {
      usage(std::cout);
      return 0;
    }
    else if(!strcmp(argv[count], "--no-message-window")) {
      no_msg_dlg = true;
    }
    else if(!strcmp(argv[count], "--no-message-window-auto-scroll")) {
      msg_dlg_auto_scroll = false;
    }
  }

    // create main window
    // note: argc and argv are passed to extract X11
    // toolkit specific parameters only

  mainDlg = new OksDataEditorMainDialog(&argc, argv, fallback_resources, no_msg_dlg, msg_dlg_auto_scroll);

  mainDlg->set_test_duplicated_objects_via_inheritance_mode(true);


    // pass command line parameters and perform necessary actions

  {
    std::list<const char *> db_files;

    const char * init_dirs = 0;    // -I | --init-dirs : directories containing initialization files
    const char * init_data = 0;    // -D | --init-data-files  : colon-separated list of initialisation data file
    const char * pxm_dirs = 0;     // -P | --pixmap-dirs  : colon-separated list of pixmap directories
    const char * bmp_dirs = 0;     // -B | --bitmap-dirs  : colon-separated list of bitmap directories
    bool verbose = false;          // -v | --verbose : display editor messages

    for(int i = 1; i < argc; i++) {
      const char * cp = argv[i];

      if(!strcmp(cp, "--no-message-window") || !strcmp(cp, "--no-message-window-auto-scroll")) continue;

      else if(!strcmp(cp, "-v") || !strcmp(cp, "--verbose"))                                  { verbose = true; }
      else if(!strcmp(cp, "-a") || !strcmp(cp, "--allow-duplicated-objects-via-inheritance")) { mainDlg->set_test_duplicated_objects_via_inheritance_mode(false); }
      else if(!strcmp(cp, "-u") || !strcmp(cp, "--allow-duplicated-classes"))                 { mainDlg->set_allow_duplicated_classes_mode(true); }

      else if(!strcmp(cp, "-I") || !strcmp(cp, "--init-dirs"))        if(++i == argc) { no_param(cp); } else { init_dirs = argv[i]; }
      else if(!strcmp(cp, "-D") || !strcmp(cp, "--init-data-files"))  if(++i == argc) { no_param(cp); } else { init_data = argv[i]; }
      else if(!strcmp(cp, "-P") || !strcmp(cp, "--pixmap-dirs"))      if(++i == argc) { no_param(cp); } else { pxm_dirs = argv[i]; }
      else if(!strcmp(cp, "-B") || !strcmp(cp, "--bitmap-dirs"))      if(++i == argc) { no_param(cp); } else { bmp_dirs = argv[i]; }
      else if(!strcmp(cp, "-W") || !strcmp(cp, "--icon_width"))       if(++i == argc) { no_param(cp); } else { dp.set_graphical_parameter(OksParameterNames::get_icon_width_name(),   atoi(argv[i])); }
      else if(!strcmp(cp, "-H") || !strcmp(cp, "--ch-obj-dx"))        if(++i == argc) { no_param(cp); } else { dp.set_graphical_parameter(OksParameterNames::get_ch_obj_dx_name(),    atoi(argv[i])); }
      else if(!strcmp(cp, "-R") || !strcmp(cp, "--ch-obj-dy"))        if(++i == argc) { no_param(cp); } else { dp.set_graphical_parameter(OksParameterNames::get_ch_obj_dy_name(),    atoi(argv[i])); }
      else if(!strcmp(cp, "-A") || !strcmp(cp, "--ch-obj-max-width")) if(++i == argc) { no_param(cp); } else { dp.set_graphical_parameter(OksParameterNames::get_ch_max_width_name(), atoi(argv[i])); }
      else if(!strcmp(cp, "-Z") || !strcmp(cp, "--obj-dx"))           if(++i == argc) { no_param(cp); } else { dp.set_graphical_parameter(OksParameterNames::get_obj_dx_name(),       atoi(argv[i])); }
      else if(!strcmp(cp, "-T") || !strcmp(cp, "--obj-dy"))           if(++i == argc) { no_param(cp); } else { dp.set_graphical_parameter(OksParameterNames::get_obj_dy_name(),       atoi(argv[i])); }
      else if(!strcmp(cp, "-O") || !strcmp(cp, "--font-dx"))          if(++i == argc) { no_param(cp); } else { dp.set_font_x_margin(atoi(argv[i])); }
      else if(!strcmp(cp, "-E") || !strcmp(cp, "--font-dy"))          if(++i == argc) { no_param(cp); } else { dp.set_font_y_margin(atoi(argv[i])); }
      else if(!strcmp(cp, "-N") || !strcmp(cp, "--da-x-margin"))      if(++i == argc) { no_param(cp); } else { dp.set_drawing_area_x_margin(atoi(argv[i])); }
      else if(!strcmp(cp, "-C") || !strcmp(cp, "--da-y-margin"))      if(++i == argc) { no_param(cp); } else { dp.set_drawing_area_y_margin(atoi(argv[i])); }

      else {
        db_files.push_back(cp);
      }   
    }


      // in verbose mode report possible problems coming from read of GUI RC file

    if(verbose) {
      if(!rc_read_error_text.str().empty()) std::cerr << rc_read_error_text.str() << std::endl;
    }


      // init graphical windows

    mainDlg->init(verbose, init_dirs, init_data, pxm_dirs, bmp_dirs);


      // load data files

    if(!db_files.empty()) {
      bool foundErrors = false;

      while(!db_files.empty()) {
        const char * db_file = db_files.front();
	db_files.pop_front();

        try {
          mainDlg->load_file(db_file, false);
        }
        catch (oks::exception & ex) {
          std::cerr << "ERROR: cannot load file \"" << db_file << "\":\n" << ex.what() << std::endl << std::endl;
	  foundErrors = true;
	}
      }

        // link OKS objects (by performance reason they were not linked during reading of files)

      try {
        mainDlg->bind_objects();
      }
      catch (oks::exception & ex) {
        foundErrors = true;
        OksDataEditorMainDialog::report_exception("Bind Objects", ex);
      }

      mainDlg->test_bind_classes_status();

      if(foundErrors) {
        if(!mainDlg->data_files().empty()) {
          Widget warn_dlg = OksXm::create_dlg_with_pixmap(
            mainDlg->get_form_widget(), "Warning", stop_xpm,
            "Files have been loaded with errors.\nCheck file's contents before saving, you can lose some data!",
            "Ok", 0, 0, 0, 0
          );
          XtManageChild(warn_dlg);
        }
      }
    }
  }

  mainDlg->refresh_schemes();
  mainDlg->refresh_data();
  mainDlg->refresh_classes();

  XtRealizeWidget(mainDlg->app_shell());

  mainDlg->setMinSize();

  // FIXME: implement for git!!!
//  mainDlg->test_out_of_date_files();

  XtAppMainLoop(mainDlg->app_context());

  return 0;
}

OksObject*
findOksObject(char *objName)
{
  char *id = objName;

  if(id[0] == '[') {
    size_t len = strlen(id) - 1;
    if(id[len] == ']') {
      id[len] = '\0';
      id++;
    }
  }

  if((objName = strchr(id, '@')) == 0) return 0;

  *objName = '\0';
  objName++;
  
  OksClass *c = mainDlg->find_class(objName);

  if(c == 0) {
    std::ostringstream text;
    text << "cannot find class \"" << objName << '\'';
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    return 0;
  }

  OksObject *o = c->get_object(id);

  if(o == 0) {
    std::ostringstream text;
    text << "cannot find object \"" << objName << '@' << id << '\"';
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
  }

  return o;
}

OksClass*
findOksClass(const char *className)
{
  OksClass *c = mainDlg->find_class(className);

  if(c == 0) {
    std::ostringstream text;
    text << "cannot find class \"" << className << '\'';
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
  }

  return c;
}

const char * bool2str(bool b) { return (b ? "yes" : "no"); }

std::string make_description(const OksAttribute * a)
{
  OksXm::TipsText text;

  text.add("Attribute", a->get_name());
  text.add("Type", a->get_type());
  if(!a->get_range().empty()) text.add("Range", a->get_range());
  text.add("Multi-value", bool2str(a->get_is_multi_values()));
  if(!a->get_init_value().empty()) text.add("Init value",a->get_init_value());
  if(a->get_data_type() != OksData::bool_type && a->get_data_type() != OksData::enum_type) text.add("Can be NULL", bool2str(!a->get_is_no_null()));
  if(!a->get_description().empty()) text.add("Description", a->get_description());

  return text.text();
}


std::string make_description(const OksRelationship * r)
{
  OksXm::TipsText text;

  text.add("Realtionship", r->get_name());
  text.add("Class type", r->get_type());

  OksRelationship::CardinalityConstraint lcc(r->get_low_cardinality_constraint());
  OksRelationship::CardinalityConstraint hcc(r->get_high_cardinality_constraint());

  const char * card = (
    (lcc == OksRelationship::Zero) && (hcc == OksRelationship::One)  ? "0..1" :
    (lcc == OksRelationship::Zero) && (hcc == OksRelationship::Many) ? "0..N" :
    (lcc == OksRelationship::One)  && (hcc == OksRelationship::One)  ? "1" : "1..N"
  );

  text.add("Cardinality", card);
  
  const char * ref_type = (
    (r->get_is_composite() == false) ? "weak (non-composite)" :
    (r->get_is_exclusive())  && r->get_is_dependent()  ? "exclusive dependent" :
    (r->get_is_exclusive())  && !r->get_is_dependent() ? "exclusive independent" :
    (!r->get_is_exclusive()) && r->get_is_dependent()  ? "shared dependent" : "shared independent"
  );
  
  text.add("Reference type", ref_type);
  
  if(!r->get_description().empty()) text.add("Description", r->get_description());

  return text.text();
}

char *
strip_brackets(const OksAttribute * a, char * s)
{
  if(a->get_is_multi_values() && (strlen(s)) > 2 && (*s == '(') && (s[strlen(s) - 1] == ')')) {
    s++;
    s[strlen(s) - 1] = '\0';
  }

  return s;
}
