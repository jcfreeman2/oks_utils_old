#include <string.h>
#include <stdlib.h>

#include <iostream>

#include <ers/ers.h>

#include <oks/xm_msg_window.h>
#include <oks/xm_context.h>

#include "schema_editor.h"
#include "schema_editor_main_dlg.h"

const char *szClasses			= "Classes:";
const char *szSchema			= "Schema:";
const char *szClass			= "Class:";
const char *szIsAbstract		= "Is Abstract:";
const char *szDescription		= "Description:";
const char *szSuperClasses		= "Super Classes:";
const char *szSubClasses		= "Sub Classes:";
const char *szAttributes		= "Attributes:";
const char *szRelationships		= "Relationships:";
const char *szMethods			= "Methods:";
const char *szType			= "Type:";
const char *szCardinality		= "Cardinality:";
const char *szNonNullValue		= "Non-null Value:";
const char *szInitialValue		= "Initial Value:";
const char *szRange			= "Range:";
const char *szFormat			= "Format:";
const char *szName			= "Name:";
const char *szDefinedInClass		= "Defined in class:";
const char *szCardinalityConstraint	= "Cardinality Constraint:";
const char *szReferenceType		= "Reference type:";
const char *szBrowse			= "Browse";
const char *szClassType			= "Class Type:";
const char *szBody			= "Body:";
const char *szConditions		= "Conditions:";
const char *szActions			= "Actions:";

const char *szYes			= "Yes";
const char *szNo			= "No";
const char *szSingleValue		= "Single Value";
const char *szMultiValue		= "Multi Value";
const char *szCanBeNull			= "Can be null";
const char *szNonNull			= "Non-null";

const char *szCreate			= "Create";
const char *szUpdate			= "Update";
const char *szModify			= "Modify";
const char *szDelete			= "Delete";

const char *szNew			= "New";
const char *szShow			= "Show";
const char *szSort			= "Sort";

OksSchemaEditorMainDialog		*mainDlg;
static const char			*appClassName	= "OksEditor";

const char * fallback_resources[][2] = {
  { "*XmFrame*Background",                     "rgb:f6/f6/f6" },
  { "*XmList.background",                      "rgb:f6/f6/f6" },
  { "*XmTextField.background",                 "rgb:f6/f6/f6" },
  { "*XbaeMatrix*cellBackgrounds",             "rgb:f6/f6/f6" },
  { "*File.Exit.accelerator",                  "Ctrl<Key>Q"   },
  { "*File.Exit.acceleratorText",              "Ctrl+Q"       },
  { "*Windows.Message Log.accelerator",        "Ctrl<Key>M"   },
  { "*Windows.Message Log.acceleratorText",    "Ctrl+M"       },
  { 0,                                         0              }
};

static void
usage(std::ostream& s)
{
  s << "Usage: oks_schema_editor [X-toolkit-options] [-h] [-v] [--no-message-window] [--no-message-window-auto-scroll]\n"
  "                              [schema-file* [view-file*]]\n"
       "\n"
       "  Options:\n"
       "    -h | --help                      print this text\n"
       "    -v | --verbose                   verbose output\n"
       "    --no-message-window              do not create graphiical window for messages and errors\n"
       "    --no-message-window-auto-scroll  do not auto scroll message window unless there are new warning or errors\n"
       "                                     (to fix performance problem of Motif 2.3)\n"
       "    X-toolkit-options                X11 and Motif options, e.g. -display host:0\n"
       "    schema-file*                     one or several oks schema files\n"
       "    view-file*                       one or several oks view files\n"
       "\n"
       "  Description:\n"
       "    GUI tool to graphically design OKS schema.\n"
       "\n"
       "  Related environment variables:\n"
       "    OKS_GUI_NO_MSG_WINDOW               do not show message window\n"
       "    OKS_GUI_MSG_WINDOW_NO_AUTO_SCROLL   do not auto scroll message window if there are no warnings or errors\n";
}

int main (int argc, char **argv)
{
  int count = 0;
  bool no_msg_dlg = (getenv("OKS_GUI_NO_MSG_WINDOW") != 0);
  bool msg_dlg_auto_scroll = (getenv("OKS_GUI_MSG_WINDOW_NO_AUTO_SCROLL") == 0);
  bool verbose_out = false;

  while(++count < argc) {
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
    else if(!strcmp(argv[count], "-v") || !strcmp(argv[count], "--verbose")) {
      verbose_out = true;
    }
  }


    //
    // set default window parameters
    //

  OksGC::Parameters& dp = OksGC::Parameters::def_params();
  dp.set_name("oks-schema-editor");

  dp.set_graphical_parameter("Aggregation sign size", 18);
  dp.set_graphical_parameter("Generalization sign size", 16);

  try {
    dp.read();
    ERS_DEBUG(1, "The default graphic parameters are:\n" << dp);
  }
  catch(oks::FailedLoadGuiRcFile& ex) {
    if(verbose_out) {
      std::cerr << "WARNING: failed to read GUI rc file:\n" << ex.what() << std::endl;
    }
  }


  if(!(mainDlg = new OksSchemaEditorMainDialog(&argc, argv, appClassName, fallback_resources, no_msg_dlg, msg_dlg_auto_scroll, verbose_out))) {
    Oks::error_msg(argv[0]) << ": out of memory, exiting...\n";
    return (-2);
  }

  if(argc > 1) {
    bool foundErrors = false;

    for(count = 1; count < argc; count++) {
      if(
        !strcmp(argv[count], "-h") || !strcmp(argv[count], "--help") ||
	!strcmp(argv[count], "--no-message-window") ||
	!strcmp(argv[count], "--no-message-window-auto-scroll") ||
	!strcmp(argv[count], "-v") || !strcmp(argv[count], "--verbose")
      ) continue;

      std::string full_file_name;

      try {
        full_file_name = mainDlg->get_file_path(argv[count], 0);
      }
      catch(std::exception& ex) {
        std::cerr << "ERROR: cannot find file \"" << argv[count] << "\":\n" << ex.what() << std::endl;
        foundErrors = true;
        full_file_name.erase(0);
      }

      if(!full_file_name.empty()) {
        OksFile * f = 0;

        try {
          f = mainDlg->create_file_info(argv[count], full_file_name);
        }
        catch (oks::exception & ex) {
          std::cerr << "ERROR:\n" << ex.what() << std::endl;
          foundErrors = true;
        }
	
	if(f) {
	  if(f->get_oks_format() == "schema") {
            try {
              mainDlg->load_schema(argv[count]);
            }
            catch (oks::exception & ex) {
              std::cerr << "ERROR:\n" << ex.what() << std::endl;
              foundErrors = true;
            }
	  }
	  else {
	    mainDlg->create_view_dlg(argv[count]);
	  }
	  delete f;
	}
      }
    }

    mainDlg->test_bind_classes_status();

    if(foundErrors) {
      usage(std::cerr);
    }
  }

  mainDlg->showSchemaList();
  mainDlg->showClassList();
  mainDlg->refresh();

  XtRealizeWidget(mainDlg->app_shell());

  mainDlg->setMinSize();

  XtAppMainLoop(mainDlg->app_context());

  return 0;
}
