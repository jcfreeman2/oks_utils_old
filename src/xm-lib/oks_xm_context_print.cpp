#include <oks/xm_context.h>
#include <oks/xm_utils.h>

#include "oks_xm_context_print.h"

#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sstream>

#include <boost/date_time/posix_time/time_formatters.hpp>

#include <Xm/TextF.h>
#include <Xm/ToggleB.h>

OksGCPrint::Format OksGCPrint::formats[] = {
  {"A0", 2384, 3370},
  {"A1", 1684, 2384},
  {"A2", 1191, 1684},
  {"A3", 842, 1191},
  {"A4", 595, 842},
  {"A5", 420, 595},
  {"A6", 297, 420},
  {"A7", 210, 297},
  {"A8", 148, 210},
  {"A9", 105, 148},
  {"B0", 2920, 4127},
  {"B1", 2064, 2920},
  {"B2", 1460, 2064},
  {"B3", 1032, 1460},
  {"B4", 729, 1032},
  {"B5", 516, 729},
  {"B6", 363, 516},
  {"B7", 258, 363},
  {"B8", 181, 258},
  {"B9", 127, 181}, 
  {"B10", 91, 127},
  {"Comm #10 Envelope", 297, 684},
  {"C5 Envelope", 461, 648},
  {"DL Envelope", 312, 624},
  {"Folio", 595, 935},
  {"Executive", 522, 756},
  {"Letter", 612, 792},
  {"Legal", 612, 1008},
  {"Ledger", 1224, 792},
  {"Tabloid", 792, 1224},
  {0, 0, 0}
};


const char * OksGCPrint::title_font_names[] = {
  "Times-Roman",
  "Times-Italic",
  "Times-Bold",
  "Times-BoldItalic",
  "Helvetica",
  "Helvetica-Oblique",
  "Helvetica-Bold",
  "Helvetica-BoldOblique",
  "Courier",
  "Courier-Oblique",
  "Courier-Bold",
  "Courier-BoldOblique",
  "Symbol",
  0
};


const char * OksGCPrint::title_font_sizes[] = {
  "5", "6", "7", "8", "9", "10", "11", "12", "14", "18", "24", "36", 0
};


const char * OksGCPrint::user_defined_format   = "User Defined";
const char * OksGCPrint::as_is_format          = "As Is (no scale)";

const char * OksGCPrint::portrait_orientation  = "Portrait";
const char * OksGCPrint::landscape_orientation = "Landscape";
const char * OksGCPrint::best_fit_orientation  = "Best Fit";

const char * OksGCPrint::no_title              = "(None)";
const char * OksGCPrint::header_title          = "in header";
const char * OksGCPrint::footer_title          = "in footer";


OksGCPrint::OksGCPrint(OksTopDialog * top, OksGC * gc1, const std::string& name, OksGC::ApplyFN f, XtPointer p, const OksFile::Map& files) :
  OksDialog ("", top),
  gc        (gc1),
  apply_f   (f),
  apply_f_p (p),
  p_files   (files)
{

  {
    std::string s("Print \'");
    s += name;
    s += '\'';
    setTitle(s.c_str());
  }


  add_label(idDestination, "= Print Destination =");
  attach_right(idDestination);


    // set print to file choice

  w_print_to_file = add_radio_button(idPrintToFile, "Print to");

  XtAddCallback(
    w_print_to_file,
    XmNvalueChangedCallback,
    OksGCPrint::switchOutputCB,
    (XtPointer)this
  );


  {
    std::list<const char *> fileFormatList;

    fileFormatList.push_back("PS");
    fileFormatList.push_back("MIF");
    fileFormatList.push_back("");

    add_option_menu(idFileFormat, &fileFormatList);
    set_option_menu_cb(idFileFormat, OksGCPrint::chooseFileFormatCB, (XtPointer)this);
  }


  w_file_name = create_text_field(idFileName, "file:", name.c_str(), 0);
  attach_right(idFileName);


    // set send to printer choice

  w_send_to_printer = add_radio_button(idSendToPrinter, "Send to printer");

  XtAddCallback(
    w_send_to_printer,
    XmNvalueChangedCallback,
    OksGCPrint::switchOutputCB,
    (XtPointer)this
  );

  set_value(idPrintToFile, "On");

  w_print_command = create_text_field(idPrintCommand, "(print command): ", "lpr", 6);
  attach_right(idPrintCommand);
  attach_previous(idPrintCommand);

  add_separator();

    // set "Print to file" by default
  {
    XmToggleButtonCallbackStruct tbcs;
    tbcs.set = false;
    switchOutputCB(w_send_to_printer, (XtPointer)this, (XtPointer)&tbcs);
  }


    // specify format types

  add_label(idFormatLabel, "= Page Properties =");
  attach_right(idFormatLabel);

  std::list<const char *> formatList;
  
  Format *format = formats;
  
  while(format->name)
    formatList.push_back((format++)->name);

  formatList.push_back(user_defined_format);
  formatList.push_back(as_is_format);
  formatList.push_back("Paper Format:");

  add_option_menu(idFormat, &formatList);
  set_option_menu_cb(idFormat, OksGCPrint::chooseFormatCB, (XtPointer)this);


  create_text_field(idUserDefinedWidth, "width\n(in pts.)", 0, 3);
  attach_previous(idUserDefinedWidth);
  create_text_field(idUserDefinedHeight, "height\n(in pts.)", 0, 3);

  attach_previous(idUserDefinedHeight);


    // specify paper orientation

  std::list<const char *> orientationList;
  orientationList.push_back(portrait_orientation);
  orientationList.push_back(landscape_orientation);
  orientationList.push_back(best_fit_orientation);
  orientationList.push_back("Orientation:");

  add_option_menu(idOrientation, &orientationList);

  add_label(idMarginsLabel, "Margins (in pts.):");

  create_text_field(idLeftMargin, "Left", "20", 3);
  create_text_field(idRightMargin, "Right", "20", 3);
  attach_previous(idRightMargin);
  create_text_field(idTopMargin, "Top", "12", 3);
  attach_previous(idTopMargin);
  create_text_field(idBottomMargin, "Bottom", "12", 3);
  attach_previous(idBottomMargin);


  add_separator();


  add_label(idImageParametersLabel, "= Image Parameters =");
  attach_right(idImageParametersLabel);

  add_toggle_button(idScaleToFit, "Scale to fit");
  add_toggle_button(idWhiteBackground, "White background");
  attach_previous(idWhiteBackground);
  add_toggle_button(idPrintImageBox, "Print image box");
  attach_previous(idPrintImageBox);


  w_header_text = add_text_field(idHeaderText, "Header text:");
  attach_right(idHeaderText);

  w_footer_text = add_text_field(idFooterText, "Footer text:");
  attach_right(idFooterText);

  std::list<const char *> positionList;
  positionList.push_back(no_title);
  positionList.push_back(header_title);
  positionList.push_back(footer_title);
  positionList.push_back("Print date ");
  add_option_menu(idPrintDate, &positionList);
  positionList.pop_back();

  positionList.push_back("Print database\nfiles list");
  add_option_menu(idPrintFileList, &positionList);
  attach_previous(idPrintFileList);

  create_option_menu(idHeaderFontNames, "Header Font:", title_font_names);
  create_option_menu(idHeaderFontSizes, "Size:", title_font_sizes);
  attach_previous(idHeaderFontSizes);
  set_value(idHeaderFontNames, title_font_names[0]);	// set header font "Times-Roman"
  set_value(idHeaderFontSizes, title_font_sizes[8]);	// set header size 14

  create_option_menu(idFooterFontNames, "Footer Font:", title_font_names);
  create_option_menu(idFooterFontSizes, "Size:", title_font_sizes);
  attach_previous(idFooterFontSizes);
  set_value(idFooterFontNames, title_font_names[4]);	// set footer font "Helvetica"
  set_value(idFooterFontSizes, title_font_sizes[7]);	// set footer size 12

  add_separator();


  XtAddCallback(
    add_push_button(idPrintButton, "Print"),
    XmNactivateCallback,
    OksGCPrint::buttonCB,
    (XtPointer)this
  );

  XtAddCallback(
    add_push_button(idCancelButton, "Cancel"),
    XmNactivateCallback,
    OksGCPrint::buttonCB,
    (XtPointer)this
  );

  attach_previous(idCancelButton);

  dlg_w = XtParent(get_form_widget());

  OksXm::set_close_cb(dlg_w, OksGCPrint::buttonCB, (void *)this);


  set_value(idFormat, "A4");
  chooseFormatCB(0, (XtPointer)this, 0);
  chooseFileFormatCB(0, (XtPointer)this, 0);

  show();


    // call attach here because of bug in Motif:
    //   the sizes of RowColumn containg option menu is wrong
    //   before the widget is shown

  attach_previous(idFileFormat);
  attach_previous(idFileName);


  setMinSize();
}


OksGCPrint::~OksGCPrint()
{
  gc->remove_print_dialog();
}


Widget
OksGCPrint::create_option_menu(short id, const char *name, const char **menu_items)
{
  std::list<const char *> theList;

  while(*menu_items)
    theList.push_back((*menu_items++));

  theList.push_back(name);

  return add_option_menu(id, &theList);
}


Widget
OksGCPrint::create_text_field(short id, const char *name, const char *value, short width)
{
  Widget w = add_text_field(id, name);

  if(width)
    XtVaSetValues(
      w,
      XmNresizeWidth, true,
      XmNcolumns, width,
      NULL
    );

  if(value)
    XmTextFieldSetString(w, const_cast<char *>(value));

  return w;
}


void
OksGCPrint::set_text_field(short id, short value) const
{
  std::ostringstream s;
  s << value;
  std::string sv = s.str();

  XmTextFieldSetString(get_widget(id), const_cast<char *>(sv.c_str()));
}


short
OksGCPrint::read_text_field(short id) const
{
  OksXm::AutoCstring buf(XmTextFieldGetString(get_widget(id)));
  std::istringstream s(buf.get());
 
  short value;
  s >> value;

  return value;
}


double
OksGCPrint::scale_factor(short width, short height) const
{
  OksGC::FileContext& fc = *gc->get_fc();

  double d_paper_width  = (double)width;
  double d_paper_height = (double)height;

  double horizontal_margin = (double)(fc.left_margin() + fc.right_margin());
  double vertical_margin   = (double)(fc.top_margin() + fc.bottom_margin() + fc.header_size() + fc.footer_size());

  double hor_ratio  = (d_paper_width - horizontal_margin) / (double)fc.image_width();
  double vert_ratio = (d_paper_height - vertical_margin)  / (double)fc.image_height();

  return (hor_ratio < vert_ratio ? hor_ratio : vert_ratio);
}


const char *
OksGCPrint::get_tmp_output_filename()
{
  static std::string file_name;

  const char *dirs[] = {
    "OKS_TEMP",
    "TEMP",
    "TMP"
  };

  const char *dir_name = 0;

  for(unsigned int i = 0; i < sizeof(dirs)/sizeof(dirs[0]); ++i)
    dir_name = getenv(dirs[i]);

  if(!dir_name) dir_name = "/tmp";
  
  file_name = dir_name;
  file_name += "/oksFC.";


    // append process id

  {
    std::ostringstream s;
 
    s << getpid();

    std::string buf = s.str();
      
    file_name += buf;
  }


    // append process 'ps' suffix

  file_name += ".ps";
  
  return file_name.c_str();
}


void
OksGCPrint::buttonCB(Widget w, XtPointer client_data, XtPointer)
{
  OksGCPrint *dlg = (OksGCPrint *)client_data;

  if(w == dlg->get_widget(idPrintButton)) {
    if(XmToggleButtonGetState(dlg->w_print_to_file)) {
      if(OksXm::ask_overwrite_file(dlg->get_form_widget(), XmTextFieldGetString(dlg->w_file_name)) == false) return;
    }

    OksGC::FileContext& fc = *dlg->gc->create_fc();

    fc.white_background() = ((XmToggleButtonGetState(dlg->get_widget(idWhiteBackground))) ? 1 : 0);
    fc.print_image_box()  = ((XmToggleButtonGetState(dlg->get_widget(idPrintImageBox))) ? 1 : 0);


      // set up header title

    {
      OksXm::AutoCstring header_text(XmTextFieldGetString(dlg->w_header_text));
      if(header_text.is_non_empty()) fc.header().push_back(new std::string(header_text.get()));
    }


      // set up footer title

    {
      OksXm::AutoCstring footer_text(XmTextFieldGetString(dlg->w_footer_text));
      if(footer_text.is_non_empty()) fc.footer().push_back(new std::string(footer_text.get()));
    }


      // set up date

    {
      const char *print_date_choice = dlg->get_value(idPrintDate);
 
      if(strcmp(print_date_choice, no_title)) {
	std::string *s = new std::string("Date: ");


          // get current time

        {
          boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
          *s += boost::posix_time::to_simple_string(now);
        }

	(
	  (!strcmp(print_date_choice, header_title))
	    ? fc.header()
	    : fc.footer()
	).push_back(s);
      }
    }

      // set up database files

    {
      const char *print_db_files_choice = dlg->get_value(idPrintFileList);
    
      if(strcmp(print_db_files_choice, no_title)) {
        std::list<std::string *>& files_list = (
	  !strcmp(print_db_files_choice, header_title)
	    ? fc.header()
	    : fc.footer()
        );

        files_list.push_back(new std::string("Database files:"));

        const OksFile::Map & dfs = dlg->p_files;

	for(OksFile::Map::const_iterator i = dfs.begin(); i != dfs.end(); ++i) {
	    std::string * s = new std::string("- \'");
	    *s += *(i->first);
	    *s += '\'';
	    files_list.push_back(s);
	}
      }
    }

    if(!fc.header().empty()) {
      fc.header_font_name() = dlg->get_value(idHeaderFontNames);
      
      const char * header_size_buf = dlg->get_value(idHeaderFontSizes);
      unsigned int value;
      std::istringstream s(header_size_buf);
      s >> value;
      fc.header_font_size() = value;
    }

    if(!fc.footer().empty()) {
      fc.footer_font_name() = dlg->get_value(idFooterFontNames);

      const char *footer_size_buf = dlg->get_value(idFooterFontSizes);
      unsigned int value;
      std::istringstream s(footer_size_buf);
      s >> value;
      fc.footer_font_size() = value;
    }


      //
      // detect file name:
      //  - read from text field if print to file
      //  - generate if send to printer
      //

    char * ps_file_name = (
      (XmToggleButtonGetState(dlg->w_print_to_file))
        ? XmTextFieldGetString(dlg->w_file_name)
	: const_cast<char *>(get_tmp_output_filename())
    );


      // detect file type (PostScript of MakerInterchangeFormat)

    OksGC::FileContext::CType ct = OksGC::FileContext::PostScript;
    
    if(XmToggleButtonGetState(dlg->w_print_to_file)) {
      const char * value = dlg->get_value(idFileFormat);
      if(!strcmp(value, "MIF")) ct = OksGC::FileContext::MakerInterchangeFormat;
    }

    fc.open(ps_file_name, *dlg->gc, ct);

    if(!fc.good()) return;

    std::cout << "Printing to file \'" << ps_file_name << "\'\n";

    if(XmToggleButtonGetState(dlg->w_print_to_file)) XtFree(ps_file_name);


      // draw the image

    (*dlg->apply_f)(dlg->apply_f_p);


      // set up image sizes

    dlg->get_image_size(fc.image_width(), fc.image_height());


      // set up image margins

    fc.left_margin()   = dlg->read_text_field(idLeftMargin);
    fc.right_margin()  = dlg->read_text_field(idRightMargin);
    fc.top_margin()    = dlg->read_text_field(idTopMargin);
    fc.bottom_margin() = dlg->read_text_field(idBottomMargin);    


      // set up paper sizes

    const char *format = 0;

    {
      unsigned short width, height;
      format = dlg->get_format(width, height);

      const char *orientation = dlg->get_value(idOrientation);

      if(strcmp(format, as_is_format) && !strcmp(orientation, best_fit_orientation)) {
        double portrait_factor = dlg->scale_factor(width, height);
        double landscape_factor = dlg->scale_factor(height, width);

	orientation = (portrait_factor > landscape_factor ? portrait_orientation : landscape_orientation);
      }

      if(strcmp(format, as_is_format) && !strcmp(orientation, landscape_orientation)) {
        fc.paper_width() = height;
        fc.paper_height() = width;
	fc.orientation() = OksGC::FileContext::Landscape;
      }
      else {
        fc.paper_width() = width;
        fc.paper_height() = height;
        fc.orientation() = OksGC::FileContext::Portrait;
      }
    }


      // set up scale (if "Fit to page" has choosen)

    if(strcmp(format, as_is_format) && XmToggleButtonGetState(dlg->get_widget(idScaleToFit)))
      fc.scale_factor() = dlg->scale_factor(fc.paper_width(), fc.paper_height());


      // print PS and close PC context

    fc.print_and_close();
    dlg->gc->destroy_fc();

    if(!XmToggleButtonGetState(dlg->w_print_to_file)) {
      OksXm::AutoCstring cmd(XmTextFieldGetString(dlg->w_print_command));

      std::string command_string("cat ");
      command_string += ps_file_name;
      command_string += " | ";
      command_string += cmd.get();

      std::cout << "Execute command \'" << command_string << "\'...\n";

      if(system(command_string.c_str())) {
        std::cerr << "ERROR [OksGC Print]:\ncommand failed\n";
      }
      else {
        if(unlink(ps_file_name)) {
	  std::cerr << "ERROR [OksGC Print]:\ncannot remove file \'" << ps_file_name << "\'\n";
	}
      }
    }
  }

    // close window

  delete dlg;
}


void
OksGCPrint::switchOutputCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  OksGCPrint *dlg = (OksGCPrint *)client_data;

  bool is_set = ((XmToggleButtonCallbackStruct *)call_data)->set;
  bool res = ((w == dlg->w_print_to_file && is_set) || (w == dlg->w_send_to_printer && !is_set));
  
  XmToggleButtonSetState((w == dlg->w_print_to_file ? dlg->w_send_to_printer : dlg->w_print_to_file), !is_set, false);
  dlg->enable_text_field(dlg->w_file_name, res);
  dlg->enable_text_field(dlg->w_print_command, !res);
}


void
OksGCPrint::chooseFormatCB(Widget, XtPointer client_data, XtPointer)
{
  OksGCPrint * dlg = (OksGCPrint *)client_data;

  unsigned short width, height;

  dlg->get_format(width, height);

  dlg->set_text_field(idUserDefinedWidth, width);
  dlg->set_text_field(idUserDefinedHeight, height);
}

void
OksGCPrint::chooseFileFormatCB(Widget, XtPointer client_data, XtPointer)
{
  OksGCPrint * dlg = (OksGCPrint *)client_data;

  const char * value = dlg->get_value(idFileFormat);
  OksXm::AutoCstring file_name(XmTextFieldGetString(dlg->w_file_name));

  std::string s(file_name.get());
  std::string::size_type idx = s.find_last_of('.');

  if(idx != std::string::npos) {
    s.erase(idx);
  }

  s += ((value == 0 || *value == 0 || !strcmp(value, "PS")) ? ".ps" : ".mif");
  XmTextFieldSetString(dlg->w_file_name, const_cast<char *>(s.c_str()));
}

void
OksGCPrint::enable_text_field(Widget w, int is_enabled) const
{
#if !defined(__linux__) || (XmVersion > 2000)
  if(is_enabled)
    OksXm::TextField::set_editable(w);
  else
    OksXm::TextField::set_not_editable(w);
#else
  XtSetSensitive(w, is_enabled);
#endif
  
}


void
OksGCPrint::enable_format_size_boxes(int is_enabled) const
{
  enable_text_field(get_widget(idUserDefinedWidth), is_enabled);
  enable_text_field(get_widget(idUserDefinedHeight), is_enabled);
}


void
OksGCPrint::use_as_is_settings(int is_as_is) const
{
  XtSetSensitive(get_widget(idOrientation), is_as_is ? false : true);
  XtSetSensitive(get_widget(idScaleToFit), is_as_is ? false : true);
  
  if(is_as_is) {
    XmToggleButtonSetState(get_widget(idScaleToFit), false, false);
    (const_cast<OksGCPrint *>(this))->set_value(idOrientation, portrait_orientation);
  }
}


const char *
OksGCPrint::get_format(unsigned short &width, unsigned short &height) const
{
  width = height = 0;


  const char *value = get_value(idFormat);

  Format *format = formats;


    // choose one predefined format

  while(format->name) {
    if(!strcmp(value, format->name)) {
      width  = format->width;
      height = format->height;

      enable_format_size_boxes(false);
      use_as_is_settings(false);

      return value;
    }

    format++;
  }


    // use user defined format

  if(!strcmp(value, user_defined_format)) {
    width  = read_text_field(idUserDefinedWidth);
    height = read_text_field(idUserDefinedHeight);

    enable_format_size_boxes(true);
    use_as_is_settings(false);
  }


    // use size of image (as is)

  else if(!strcmp(value, as_is_format)) {
    get_image_size(width, height);

    width += read_text_field(idLeftMargin) + read_text_field(idRightMargin);
    height += read_text_field(idTopMargin) + read_text_field(idBottomMargin);

    enable_format_size_boxes(false);
    use_as_is_settings(true);
  }

  return value;
}


void
OksGCPrint::get_image_size(unsigned short &width, unsigned short &height) const
{
  XtVaGetValues(
    gc->get_widget(),
    XtNwidth, &width, XtNheight, &height,
    NULL
  );
}
