#include <sstream>

#include <oks/attribute.h>
#include <oks/object.h>
#include <oks/xm_utils.h>
#include <oks/xm_context.h>

#include "oks_xm_context_params.h"

#include <Xm/TextF.h>


OksGCParameters::OksGCParameters(OksTopDialog * top, OksGC * gc1, const std::string& name, OksGC::ApplyFN f, XtPointer p) :
  OksDialog ("", top),
  gc        (gc1),
  apply_f   (f),
  apply_f_p (p)
{

  {
    std::string s("Set Parameters \'");
    s += name;
    s += '\'';
    setTitle(s.c_str());
  }

  add_label(idLabel1, "General DrawingArea properties:");
  create_text_field(idDrawingAreaHorizontalMargin, "Area horizontal margin");
  create_text_field(idDrawingAreaVerticalMargin, "Area vertical margin");

  add_separator();

  add_label(idLabel2, "Font properties:");
  create_text_field(idFontName, "Font name");
  create_text_field(idFontDx, "Horizontal margin");
  create_text_field(idFontDy, "Vertical margin");

  add_separator();
  
  if(!gc->get_params().get_graphical_string_parameters().empty() || !gc->get_params().get_graphical_u16_parameters().empty()) {
    add_label(idLabel3, "More properties:");
    
    {
      int count = 0;
      for(std::list<OksGC::Parameters::UShortParameter *>::const_iterator i = gc->get_params().get_graphical_u16_parameters().begin(); i != gc->get_params().get_graphical_u16_parameters().end(); ++i) {
        create_text_field(idMoreUShorts + (count++), (*i)->p_name.c_str());
      }
    }

    {
      int count = 0;
      for(std::list<OksGC::Parameters::StringParameter *>::const_iterator i = gc->get_params().get_graphical_string_parameters().begin(); i != gc->get_params().get_graphical_string_parameters().end(); ++i) {
        short w_id = idMoreStrings + (count++);
        if((*i)->p_range.empty()) {
          create_text_field(w_id, (*i)->p_name.c_str());
        }
        else {
          create_option_menu(w_id, (*i)->p_name.c_str(), (*i)->p_range.c_str());
        }
      }
    }

    add_separator();
  }

  create_push_button(idGetDefaultValueButton, "Get Default Values", OksGCParameters::buttonCB, true);
  create_push_button(idSetDefaultValueButton, "Set Default Values", OksGCParameters::buttonCB, false);

  create_push_button(idApplyButton, "Apply", OksGCParameters::buttonCB, true);
  create_push_button(idOkButton, "OK", OksGCParameters::buttonCB, false);
  create_push_button(idCancelButton, "Cancel", OksGCParameters::buttonCB, false);

  dlg_w = XtParent(get_form_widget());

  XtVaSetValues(dlg_w, XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL, NULL);

  OksXm::set_close_cb(dlg_w, OksGCParameters::buttonCB, (void *)this);

  show_parameters();
  show();

  setMinSize();
}


OksGCParameters::~OksGCParameters() {
  gc->remove_parameters_dialog();
}


void
OksGCParameters::create_text_field(short id, const char *name)
{
  Widget w = add_text_field(id, name);

  if(id == idFontName || (id >= idMoreStrings && id < idOkButton))
    attach_right(id);
  else
    XtVaSetValues(w, XmNresizeWidth, true, XmNcolumns, 3, NULL);
}

void
OksGCParameters::create_option_menu(short id, const char *name, const char *range)
{
  OksAttribute a("dummy",
    OksAttribute::string_type,
    true, /* attribute is 'multi-value' !!! */
    "", range, "", true
  );

  OksData d;
  d.SetValues(range, &a);

  std::list<const char *> l;

  for(OksData::List::const_iterator i = d.data.LIST->begin(); i != d.data.LIST->end(); ++i) {
    p_to_be_removed.push_back(*(*i)->data.STRING);
    l.push_back(p_to_be_removed.back().c_str());
  }

  l.push_back(name);

  add_option_menu(id, &l);
}


void
OksGCParameters::create_push_button(short id, const char * name, XtCallbackProc f, int is_first_in_row)
{
  XtAddCallback(
    add_push_button(id, name),
    XmNactivateCallback,
    f,
    (XtPointer)this
  );

  if(!is_first_in_row)
    attach_previous(id);
}


void
OksGCParameters::show_parameter(short value, short id) const
{
  std::ostringstream s;
  s << value;

  std::string buf = s.str();
  XmTextFieldSetString(get_widget(id), const_cast<char *>(buf.c_str()));
}


short
OksGCParameters::read_parameter(short id) const
{
  OksXm::AutoCstring buf(XmTextFieldGetString(get_widget(id)));
  std::istringstream s(buf.get());

  short value;
  s >> value;

  return value;
}


void
OksGCParameters::show_parameters()
{
  XmTextFieldSetString(get_widget(idFontName), const_cast<char *>(gc->get_params().get_font_name().c_str()));
  show_parameter(gc->get_params().get_font_x_margin(), idFontDx);
  show_parameter(gc->get_params().get_font_y_margin(), idFontDy);
  show_parameter(gc->get_params().get_drawing_area_x_margin(), idDrawingAreaHorizontalMargin);
  show_parameter(gc->get_params().get_drawing_area_y_margin(), idDrawingAreaVerticalMargin);

  {
    int count = 0;
    for(std::list<OksGC::Parameters::UShortParameter *>::const_iterator i = gc->get_params().get_graphical_u16_parameters().begin(); i != gc->get_params().get_graphical_u16_parameters().end(); ++i) {
      show_parameter((*i)->p_value, idMoreUShorts + (count++));
    }
  }

  {
    int count = 0;
    for(std::list<OksGC::Parameters::StringParameter *>::const_iterator i = gc->get_params().get_graphical_string_parameters().begin(); i != gc->get_params().get_graphical_string_parameters().end(); ++i) {
      set_value(idMoreStrings + (count++), (*i)->p_value.c_str());
    }
  }
}


void
OksGCParameters::read_parameters()
{
  OksXm::AutoCstring buf(XmTextFieldGetString(get_widget(idFontName)));

  gc->get_params().set_font_name(buf.get());
  gc->get_params().set_font_x_margin(read_parameter(idFontDx));
  gc->get_params().set_font_y_margin(read_parameter(idFontDy));
  gc->get_params().set_drawing_area_x_margin(read_parameter(idDrawingAreaHorizontalMargin));
  gc->get_params().set_drawing_area_y_margin(read_parameter(idDrawingAreaVerticalMargin));

  {
    int count = 0;
    for(std::list<OksGC::Parameters::UShortParameter *>::const_iterator i = gc->get_params().get_graphical_u16_parameters().begin(); i != gc->get_params().get_graphical_u16_parameters().end(); ++i) {
      gc->get_params().set_graphical_parameter((*i)->p_name, read_parameter(idMoreUShorts + (count++)));
    }
  }

  {
    int count = 0;
    for(std::list<OksGC::Parameters::StringParameter *>::const_iterator i = gc->get_params().get_graphical_string_parameters().begin(); i != gc->get_params().get_graphical_string_parameters().end(); ++i) {
      short w_id = idMoreStrings + (count++);
      if((*i)->p_range.empty()) {
        OksXm::AutoCstring buf2(XmTextFieldGetString(get_widget(idMoreStrings + (count++))));
        gc->get_params().set_graphical_parameter((*i)->p_name, buf2.get());
      }
      else {
        gc->get_params().set_graphical_parameter((*i)->p_name, get_value(w_id));
      }
    }
  }

  gc->reset();
}


void
OksGCParameters::buttonCB(Widget w, XtPointer client_data, XtPointer)
{
  OksGCParameters * dlg = (OksGCParameters *)client_data;
  OksGC::Parameters& def_params = OksGC::Parameters::def_params();
  OksGC::Parameters& win_params = dlg->gc->get_params();


    // reset default parameters

  if(w == dlg->get_widget(idGetDefaultValueButton)) {
    win_params = def_params;
    dlg->show_parameters();
    return;
  }

    // make current parameters default

  else if(w == dlg->get_widget(idSetDefaultValueButton)) {
    dlg->read_parameters();
    def_params = win_params;

    try {
      def_params.save();
    }
    catch(oks::FailedSaveGuiRcFile & ex) {
      std::cerr << "ERROR [OKS GUI]: " << ex << std::endl;
    }

    return;
  }
 
    // apply current settings

  else if(w == dlg->get_widget(idApplyButton)) {
    dlg->read_parameters();
    (*dlg->apply_f)(dlg->apply_f_p);
    return;
  }

    // apply current settings and close window (no 'return' statement!)

  else if(w == dlg->get_widget(idOkButton)) {
    dlg->read_parameters();
    (*dlg->apply_f)(dlg->apply_f_p);
  }


    // close window

  delete (OksGCParameters *)client_data;
}
