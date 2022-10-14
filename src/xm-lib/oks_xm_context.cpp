#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>

#include <stdexcept>

#include <system/User.h>
#include "system/exceptions.h"

#include <ers/ers.h>

#include <oks/defs.h>
#include <oks/kernel.h>
#include <oks/class.h>
#include <oks/attribute.h>
#include <oks/object.h>
#include <oks/xm_context.h>
#include <oks/xm_utils.h>

#include "oks_xm_context_params.h"
#include "oks_xm_context_print.h"

const char * OksGC::bs_oks_xm = "BS oks-xm";
const char * OksGC::ts_oks_xm = "TS oks-xm";


  /**
   *  The 'default_parameters' object holds default parameters.
   *  The 'initializer' is used for initialization of 'default_parameters' object.
   */

OksGC::Parameters OksGC::Parameters::default_parameters;
static int initializer = OksGC::Parameters::init_def_params();


  /**
   *  The destructor removes user-defined parameters.
   */

OksGC::Parameters::~Parameters()
{
  while(!p_graphical_string_parameters.empty()) {
    StringParameter * p = p_graphical_string_parameters.front();
    p_graphical_string_parameters.pop_front();
    delete p;
  }

  while(!p_graphical_u16_parameters.empty()) {
    UShortParameter * p = p_graphical_u16_parameters.front();
    p_graphical_u16_parameters.pop_front();
    delete p;
  }

  while(!p_generic_string_parameters.empty()) {
    StringParameter * p = p_generic_string_parameters.front();
    p_generic_string_parameters.pop_front();
    delete p;
  }

  while(!p_generic_u16_parameters.empty()) {
    UShortParameter * p = p_generic_u16_parameters.front();
    p_generic_u16_parameters.pop_front();
    delete p;
  }
}


  /**
   *  The 'init_def_params' sets default values of the graphical context.
   */

int
OksGC::Parameters::init_def_params()
{
  default_parameters.p_name                  = "oks-gui";
  default_parameters.p_font_x_margin         = 3;
  default_parameters.p_font_y_margin         = 2;
  default_parameters.p_drawing_area_x_margin = 6;
  default_parameters.p_drawing_area_y_margin = 4;
  default_parameters.p_font_name             = "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1";
  
  return 0;
}


  /**
   *  The assignment operator makes copy of graphical context parametes.
   *  It also is used by copy constructor.
   */

OksGC::Parameters&
OksGC::Parameters::operator=(const OksGC::Parameters &p)
{
  if (&p != this)
    {
      p_name = p.p_name;
      p_font_x_margin = p.p_font_x_margin;
      p_font_y_margin = p.p_font_y_margin;
      p_drawing_area_x_margin = p.p_drawing_area_x_margin;
      p_drawing_area_y_margin = p.p_drawing_area_y_margin;
      p_font_name = p.p_font_name;

      for (const auto& i : p.p_graphical_u16_parameters)
        set_graphical_parameter(i->p_name, i->p_value);

      for (const auto& i : p.p_graphical_string_parameters)
        set_graphical_parameter(i->p_name, i->p_value, i->p_range);

      for (const auto& i : p.p_generic_u16_parameters)
        set_generic_parameter(i->p_name, i->p_value);

      for (const auto& i : p.p_generic_string_parameters)
        set_generic_parameter(i->p_name, i->p_value, i->p_range);

    }

  return *this;
}


std::ostream&
operator<<(std::ostream& s, const OksGC::Parameters& p)
{
  s << "Parameters (" << (void *)&p << ") \'" << p.get_name() << "\':\n"
       "  built-in parameters:\n"
       "  - font name: \"" << p.get_font_name() << "\"\n"
       "  - font horizontal margin: " << p.get_font_x_margin() << "\n"
       "  - font vertical margin: " << p.get_font_y_margin() << "\n"
       "  - drawing area horizontal margin: " << p.get_drawing_area_x_margin() << "\n"
       "  - drawing area vertical margin: " << p.get_drawing_area_y_margin() << std::endl;

  if (!p.get_graphical_u16_parameters().empty())
    {
      s << "  user-defined u16 graphical parameters:\n";

      for (const auto& i : p.get_graphical_u16_parameters())
        s << "  - " << i->p_name << ": " << i->p_value << std::endl;
    }

  if (!p.get_graphical_string_parameters().empty())
    {
      s << "  user-defined graphical string parameters:\n";

      for (const auto& i : p.get_graphical_string_parameters())
        s << "  - " << i->p_name << ": " << i->p_value << std::endl;
    }

  if (!p.get_generic_u16_parameters().empty())
    {
      s << "  user-defined u16 generic parameters:\n";

      for (const auto& i : p.get_generic_u16_parameters())
        {
          s << "  - " << i->p_name << ": " << i->p_value << std::endl;
        }
    }

  if (!p.get_generic_string_parameters().empty())
    {
      s << "  user-defined generic string parameters:\n";

      for (const auto& i : p.get_generic_string_parameters())
        s << "  - " << i->p_name << ": " << i->p_value << std::endl;
    }

  return s;
}


OksGC::Parameters::StringParameter *
OksGC::Parameters::find_string_parameter(const std::string & s) const
{
  for (const auto& i : p_graphical_string_parameters)
    if (i->p_name == s)
      return i;

  for (const auto& i : p_generic_string_parameters)
    if (i->p_name == s)
      return i;

  return nullptr;
}


OksGC::Parameters::UShortParameter *
OksGC::Parameters::find_u16_parameter(const std::string & s) const
{
  for (const auto& i : p_graphical_u16_parameters)
    if (i->p_name == s)
      return i;

  for (const auto& i : p_generic_u16_parameters)
    if (i->p_name == s)
      return i;

  return nullptr;
}


void
OksGC::Parameters::set_graphical_parameter(const std::string & name, unsigned short value)
{
  if (UShortParameter * p = find_u16_parameter(name))
    {
      p->p_value = value;
    }
  else
    {
      p_graphical_u16_parameters.push_back(new UShortParameter(name, value));
    }

  if (default_parameters.find_u16_parameter(name) == nullptr)
    {
      default_parameters.p_graphical_u16_parameters.push_back(new UShortParameter(name, value));
    }
}

void
OksGC::Parameters::set_graphical_parameter(const std::string & name, const std::string & value)
{
  if (StringParameter * p = find_string_parameter(name))
    {
      p->p_value = value;
    }
  else
    {
      p_graphical_string_parameters.push_back(new StringParameter(name, value));
    }

  if (default_parameters.find_string_parameter(name) == nullptr)
    {
      default_parameters.p_graphical_string_parameters.push_back(new StringParameter(name, value));
    }
}

void
OksGC::Parameters::set_graphical_parameter(const std::string & name, const std::string & value, const std::string & range)
{
  if(StringParameter * p = find_string_parameter(name)) {
    p->p_value = value;
    p->p_range = range;
  }
  else {
    p_graphical_string_parameters.push_back(new StringParameter(name, value, range));
  }


  if(default_parameters.find_string_parameter(name) == nullptr) {
     default_parameters.p_graphical_string_parameters.push_back(new StringParameter(name, value, range));
  }
}

void
OksGC::Parameters::set_generic_parameter(const std::string & name, unsigned short value)
{
  if (UShortParameter * p = find_u16_parameter(name))
    {
      p->p_value = value;
    }
  else
    {
      p_generic_u16_parameters.push_back(new UShortParameter(name, value));
    }

  if (default_parameters.find_u16_parameter(name) == nullptr)
    {
      default_parameters.p_generic_u16_parameters.push_back(new UShortParameter(name, value));
    }
}

void
OksGC::Parameters::set_generic_parameter(const std::string & name, const std::string & value)
{
  if (StringParameter * p = find_string_parameter(name))
    {
      p->p_value = value;
    }
  else
    {
      p_generic_string_parameters.push_back(new StringParameter(name, value));
    }

  if (default_parameters.find_string_parameter(name) == nullptr)
    {
      default_parameters.p_generic_string_parameters.push_back(new StringParameter(name, value));
    }
}

void
OksGC::Parameters::set_generic_parameter(const std::string & name, const std::string & value, const std::string & range)
{
  if (StringParameter * p = find_string_parameter(name))
    {
      p->p_value = value;
      p->p_range = range;
    }
  else
    {
      p_generic_string_parameters.push_back(new StringParameter(name, value, range));
    }

  if (default_parameters.find_string_parameter(name) == nullptr)
    {
      default_parameters.p_generic_string_parameters.push_back(new StringParameter(name, value, range));
    }
}

unsigned short
OksGC::Parameters::get_u16_parameter(const std::string & s) const
{
  if (UShortParameter * p = find_u16_parameter(s))
    {
      return p->p_value;
    }
  else
    {
      return 0;
    }
}

const std::string *
OksGC::Parameters::get_string_parameter(const std::string & s) const
{
  if (StringParameter * p = find_string_parameter(s))
    {
      return &p->p_value;
    }
  else
    {
      return nullptr;
    }
}


////////////////////////////////////////////////////////////////////////////////

  /**
   *  The 'create_parameters_dialog' dialog is used to graphically set
   *  graphical context parameters.
   *
   *  The function 'f' with parameter 'p' is called when user
   *  is pressing "OK" or "Apply" button.
   */

void
OksGC::create_parameters_dialog(OksTopDialog * top, const std::string & name, ApplyFN f, XtPointer p)
{
  if(parameters_dlg) {
    XMapRaised(get_display(), XtWindow(parameters_dlg));
  }
  else {
    OksGCParameters * dlg = new OksGCParameters(top, this, name, f, p);
    parameters_dlg = dlg->get_dialog();
  }
}


  /**
   *  The 'create_print_dialog' dialog is used to graphically set
   *  parameters and produce PostScript file.
   *
   *  The function 'f' with parameter 'p' is called when user
   *  is pressing "Print" button.
   */

void
OksGC::create_print_dialog(OksTopDialog * top, const std::string & name, ApplyFN f, XtPointer p, const OksFile::Map & files)
{
  if(print_dlg) {
    XMapRaised(get_display(), XtWindow(print_dlg));
  }
  else {
    OksGCPrint * dlg = new OksGCPrint(top, this, name, f, p, files);
    print_dlg = dlg->get_dialog();
  }
}

////////////////////////////////////////////////////////////////////////////////

  /**
   *  The 'reset' method is used to set-up new font.
   */

void
OksGC::reset()
{
  XFontStruct *new_font = XLoadQueryFont(display, params.get_font_name().c_str());
  
  if(new_font) {
    font = new_font;

    XGCValues values;
    values.font = font->fid;
 
    XChangeGC(display, gc, GCFont, &values);
    XChangeGC(display, gc2, GCFont, &values);

    font_height = font->ascent + font->descent + 1;
  }
  else {
    Oks::error_msg("OKS XM LIBRARY")
      << "Can't load font \'" << params.get_font_name() << "\'\n";
  }
}

void
OksGC::init(Widget w1)
{
  XGCValues values;

  w = w1;
  display = XtDisplay(w);		
  drawable = XtWindow(w);


    // Create gc

  XtVaGetValues(XtParent(w), XmNforeground, &values.foreground, NULL);

  gc  = XCreateGC(display, drawable, GCForeground, &values);
  gc2 = XCreateGC(display, drawable, GCForeground, &values);
  
  p_gc2_color = "";
  p_gc2_line_width = 1;
  p_gc2_fill = false;


    // Set font parameters for gc, gc2

  reset();


    // Create top_shadow and bottom_shadow graphical contexts

  values.line_style = LineSolid;
  XtVaGetValues(XtParent(w), XmNtopShadowColor, &values.foreground, NULL);
  ts_gc = XCreateGC( display, drawable, GCLineStyle | GCForeground, &values);

  values.line_style = LineSolid;
  XtVaGetValues(XtParent(w), XmNbottomShadowColor, &values.foreground, NULL);
  bs_gc = XCreateGC(display, drawable, GCLineStyle | GCForeground, &values);

  shadow_lines_width = 1;


    // Create stipled_gc

  stipled_gc = XCreateGC(display, drawable, GCForeground, &values);
  XSetLineAttributes(display, stipled_gc, 1, LineOnOffDash, CapButt, JoinMiter);


    // Create selection_gc

  selection_gc = XCreateGC(display, drawable, GCForeground, &values);
  XSetLineAttributes(display, stipled_gc, 1, LineOnOffDash, CapRound, JoinRound);
  XSetFunction(display, selection_gc, GXinvert);
}

OksGC::~OksGC()
{
  if(parameters_dlg)
    XtDestroyWidget(parameters_dlg);
  
  if(print_dlg)
    XtDestroyWidget(print_dlg);

  XFreeGC(display, gc);
  XFreeGC(display, gc2);
  XFreeGC(display, ts_gc);
  XFreeGC(display, bs_gc);
  XFreeGC(display, stipled_gc);
}

XColor
OksGC::get_color(Pixel pixel) const
{
  XrmValue from, to;

  from.size = sizeof(Pixel);
  from.addr = (char *)&pixel;
  to.addr = 0;
  XtConvert(w, XmRPixel, &from, XmRColor, &to);

  return *((XColor *)to.addr);
}

Pixel
OksGC::get_color(const char *colorstr) const
{
  return OksXm::string_to_pixel(w, colorstr);
}

int
OksGC::get_string_width(const char * s) const
{
  return XTextWidth(font, s, strlen(s));
}

Dimension
OksGC::get_font_height() const
{
  return font->ascent + font->descent + 1;
}

GC
OksGC::get_gc(const char * color, unsigned short line_width, bool fill)
{
  if(color) {
    if(color == bs_oks_xm || color == ts_oks_xm) {
      if(shadow_lines_width != line_width) {
        XGCValues values;
        values.line_width = shadow_lines_width = line_width;
        XChangeGC(display, ts_gc, GCLineWidth, &values);
        XChangeGC(display, bs_gc, GCLineWidth, &values);
      }

      return (
        color == bs_oks_xm ? bs_gc : ts_gc
      );
    }
  }

  if(color != 0 || line_width != 1 || fill != false) {
    unsigned long value_mask = 0;
    XGCValues values;

    if(color == 0) {
      if(p_gc2_color != 0) {
        XCopyGC(display, gc, GCForeground, gc2);
        p_gc2_color = 0;
      }
    }
    else {
      values.foreground = values.background = get_color(color);
      p_gc2_color = color;
      value_mask = GCForeground | GCBackground;
    }

    if(p_gc2_line_width != line_width) {
      values.line_width = p_gc2_line_width = line_width;
      value_mask |= GCLineWidth;
    }

    if(fill != p_gc2_fill) {
      values.fill_style = FillSolid;
      value_mask |= GCFillStyle;
    }

    if(value_mask != 0) {
      XChangeGC(display, gc2, value_mask, &values);
    }

    return gc2;
  }

  return gc;
}

  // rectangle functions

void
OksGC::clean_rectangle(Dimension x, Dimension y, Dimension width, Dimension height)
{
  if(!fc)
    XClearArea(display, drawable, x, y, width, height, False);
}

void
OksGC::draw_rectangle(Dimension x, Dimension y, Dimension width, Dimension height, const char * color, unsigned short line_width, bool fill)
{
  if(fc) {
    draw_line(x, y, x + width, y, color, line_width);
    draw_line(x + width, y, x + width, y + height, color, line_width);
    draw_line(x, y, x, y + height, color, line_width);
    draw_line(x, y + height, x + width, y + height, color, line_width);
  }
  else {
    if(fill)
      XFillRectangle(display, drawable, get_gc(color, line_width, fill), x, y, width, height);
    else
      XDrawRectangle(display, drawable, get_gc(color, line_width, fill), x, y, width, height);
  }
}

void
OksGC::draw_stippled_rectangle(Dimension x, Dimension y, Dimension width, Dimension height, const char * color)
{
  if(fc) {
    draw_rectangle(x, y, width, height);
  }
  else {

      // fill drawing segments

    XSegment segments[4];

    segments[0].x1 = segments[2].x1 = segments[3].x1 = segments[3].x2 = x;
    segments[0].x2 = segments[1].x1 = segments[1].x2 = segments[2].x2 = x + width;
    segments[0].y1 = segments[0].y2 = segments[1].y1 = segments[3].y1 = y;
    segments[1].y2 = segments[3].y2 = segments[2].y1 = segments[2].y2 = y + height;


      // set color

    if(color) {
      XGCValues values;
      values.foreground = get_color(color);
      XChangeGC(display, stipled_gc, GCForeground, &values);
    }

      // draw segments

    XDrawSegments(display, drawable, stipled_gc, segments, 4);
  }
}

void
OksGC::draw_shadowed_rectangle(Dimension x, Dimension y, Dimension width, Dimension height, ShadowType shadow_type, unsigned short line_width)
{
  if (shadow_lines_width != line_width)
    {
      XGCValues values;
      values.line_width = shadow_lines_width = line_width;
      XChangeGC(display, ts_gc, GCLineWidth, &values);
      XChangeGC(display, bs_gc, GCLineWidth, &values);
    }

  const char * c1, *c2;

  switch (shadow_type)
    {
      case shadow_in:
        c1 = bs_oks_xm;
        c2 = ts_oks_xm;
        break;

      case shadow_out:
        c1 = ts_oks_xm;
        c2 = bs_oks_xm;
        break;

      default:
        c1 = c2 = nullptr;
        break;
    }

  draw_line(x, y, x + width, y, c1, line_width);
  draw_line(x, y, x, y + height, c1, line_width);
  draw_line(x, y + height, x + width, y + height, c2, line_width);
  draw_line(x + width, y, x + width, y + height, c2, line_width);
}


////////////////////////////////////////////////////////////////////////////////


void
OksGC::draw_line(Dimension x1, Dimension y1, Dimension x2, Dimension y2, const char * color, unsigned short line_width)
{
  if(fc) {

      // find list of lines having this color and line width

    OksGC::FileContext::Lines * lc = 0;
    OksGC::FileContext::Lines::LColor lcolor = (
      color == bs_oks_xm ? OksGC::FileContext::Lines::BSColor :
      color == ts_oks_xm ? OksGC::FileContext::Lines::TSColor :
                           OksGC::FileContext::Lines::DFColor
    );

    for(std::list<OksGC::FileContext::Lines *>::iterator i = fc->lines().begin(); i != fc->lines().end(); ++i) {
      if(
        (lcolor == (*i)->color) &&
	(line_width == (*i)->width)
      ) {
        lc = *i;
	break;
      }
    }

    if(lc == 0) {
      lc = new OksGC::FileContext::Lines;
      lc->color = lcolor;
      lc->width = line_width;
      fc->lines().push_back(lc);
    }

    lc->lines.push_back(new OksGC::FileContext::Line(x1, y1, x2, y2));
  }
  else
    XDrawLine(display, drawable, get_gc(color, line_width), x1, y1, x2, y2);
}

void
OksGC::draw_stippled_line(Dimension x1, Dimension y1, Dimension x2, Dimension y2, const char * color)
{
  if(fc) {
    draw_line(x1, y1, x2, y2);
  }
  else {
    if(color) {
      XGCValues values;
      values.foreground = get_color(color);
      XChangeGC(display, stipled_gc, GCForeground, &values);
    }

    XDrawLine(display, drawable, stipled_gc, x1, y1, x2, y2);
  }
}

void
OksGC::draw_shadowed_line(Dimension x1, Dimension y1, Dimension x2, Dimension y2, ShadowType shadow_type, unsigned short width)
{
  const char * c1, * c2;

  switch (shadow_type) {
    case shadow_in:
      c1 = bs_oks_xm;
      c2 = ts_oks_xm;
      break;

    case shadow_out:
      c1 = ts_oks_xm;
      c2 = bs_oks_xm;
      break;

    default:
      c1 = c2 = nullptr;
  }

  unsigned short dx = ((y1 == y2) ? 0 : width);
  unsigned short dy = ((x1 == x2) ? 0 : width);

  draw_line(x1, y1, x2, y2, c1, width);
  draw_line(x1 + dx, y1 + dy, x2 + dx, y2 + dy, c2, width);
}


void
OksGC::draw_selection_line(Dimension x1, Dimension y1, Dimension x2, Dimension y2, bool hide)
{
  if(!fc) {
    static Dimension last_x1 = 0, last_x2 = 0, last_y1 = 0, last_y2 = 0;

    if(last_x1 != 0 || last_x2 != 0 || last_y1 != 0 || last_y2 != 0) {
      XSetForeground(display, selection_gc, WhitePixelOfScreen(XtScreen(w)));
      XDrawLine(display, drawable, selection_gc, last_x1, last_y1, last_x2, last_y2);
    }

    if(hide == false) {
      XSetForeground(display, selection_gc, BlackPixelOfScreen(XtScreen(w)));
      XDrawLine(display, drawable, selection_gc, (last_x1 = x1), (last_y1 = y1), (last_x2 = x2), (last_y2 = y2));
    }
    else {
      last_x1 = last_x2 = last_y1 = last_y2 = 0;
    }
  }
}


////////////////////////////////////////////////////////////////////////////////


void
OksGC::draw_string(Dimension x, Dimension y, const char *s, const char * color)
{
  y += font->ascent;

  if(fc)
    fc->texts().push_back(new OksGC::FileContext::Text(x, y, s));
  else
    XDrawString(display, drawable, get_gc(color), x, y, s, strlen(s));
}


std::string
OksGC::wrap_text(const std::string& text, Dimension& width, Dimension& height) const
{
  const char * delims(" \t\n,");

  height = font_height + 2 * get_params().get_font_y_margin();


    // calculate minimal possible width

  Dimension w1 = get_string_width(text.c_str());

  if(w1 > width) {
    Oks::Tokenizer t(text, delims);
    std::string token;

    while(!(token = t.next()).empty()) {
      int word_width = get_string_width(token.c_str());
      if(width < word_width) width = word_width;
    }
  }
  else {
    width = w1;
    return text;
  }


    // put as many words as possible on single line start new line otherwise

  Dimension actual_width = 0;
  std::string new_label, line;

  Oks::Tokenizer t(text, delims);
  std::string token;

  while(!(token = t.next()).empty()) {
    std::string test(line);
    if(test.length()) test += " ";
    test += token;

      // set actual width to first token

    if(actual_width == 0)
      actual_width = get_string_width(test.c_str());

    if(get_string_width(test.c_str()) <= width) line = test;
    else {
      if(new_label.length()) new_label += '\n';
      new_label += line;
      line = token;
      height += font_height + get_params().get_font_y_margin();
    }
    
    Dimension lw = get_string_width(line.c_str());
    if(lw > actual_width) actual_width = lw;
  }

	
  if(line.length()) {
    if(new_label.length()) new_label += '\n';
    new_label += line;
  }

    // set actual width (do not forget about font dx!)

  width = actual_width + 2 * get_params().get_font_y_margin();

  return new_label;
}


void
OksGC::draw_text(const std::string& text, Dimension x, Dimension y, Dimension width, const char * color)
{
  y += 1 + get_params().get_font_y_margin();

  Oks::Tokenizer t(text, "\n");
  std::string token;

  while(!(token = t.next()).empty()) {
    draw_string(x + (width - get_string_width(token.c_str()))/2, y, token.c_str(), color);
    y += font_height + get_params().get_font_y_margin();
  }
}


////////////////////////////////////////////////////////////////////////////////


void
OksGC::define_cursor(unsigned int shape) const
{
  Cursor cursor = XCreateFontCursor(get_display(), shape);
  XDefineCursor(get_display(), XtWindow(XtParent(XtParent(w))), cursor);
}

void
OksGC::undefine_cursor() const
{
  XUndefineCursor(get_display(), XtWindow(XtParent(XtParent(w))));
}


////////////////////////////////////////////////////////////////////////////////


void
OksGC::draw_pixmap(Pixmap pixmap, Dimension pw, Dimension ph, Dimension x, Dimension y)
{
  if(fc) {
    OksGC::FileContext::Image * image = new OksGC::FileContext::Image(
      x, y, pw, ph,
      ((fc->get_ctype() == OksGC::FileContext::PostScript)
        ? OksGC::FileContext::Image::PS_FType
	: OksGC::FileContext::Image::XWD_FType
      ),
      (void *)pixmap
    );

    fc->images().push_back(image);

    XpmAttributes attr;
    attr.valuemask = 0;
    XpmImage xpm_image;
    XpmCreateXpmImageFromPixmap(display, pixmap, 0, &xpm_image, &attr);

    image->set_image(
      xpm_image,
      (
        fc->white_background()
          ? OksGC::FileContext::Image::find_x_color(xpm_image, fc->white_background_color())
	  : xpm_image.ncolors + 1
      )
    );

    XpmFreeXpmImage(&xpm_image);
  }
  else
    XCopyArea(display, pixmap, drawable, gc, 0, 0, pw, ph, x, y);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

  // schema name
const char * gui_dp_schema_file_name   = "oks/gui/default-parameters.schema.xml";

  // names of classes in the schema
const char * oks_str_param_class_name  = "String Parameter";
const char * oks_u16_param_class_name  = "Unsigned Short Parameter";

  // names of "Value" attribute
const char * oks_value_attr_name       = "Value";

  // names of "Value" attribute
const char * oks_type_attr_name        = "Type";

  // identity of pre-defined objects
const char * font_name_object_id       = "Font Name";
const char * font_x_margin_object_id   = "Font X Margin";
const char * font_y_margin_object_id   = "Font Y Margin";
const char * da_x_margin_object_id     = "Drawing Area X Margin";
const char * da_y_margin_object_id     = "Drawing Area Y Margin";


namespace oks
{

  std::string
  FailedLoadGuiRcFile::fill(const std::string& name, const std::string& reason)
  {
    std::string s("Failed to load OKS GUI resource file ");
    if(!name.empty()) s += '\'' + name + "\' ";
    return (s + "because:\n" + reason);
  }

  std::string
  FailedSaveGuiRcFile::fill(const std::string& name, const std::string& reason)
  {
    std::string s("Failed to save OKS GUI resource file ");
    if(!name.empty()) s += '\'' + name + "\' ";
    return (s + "because:\n" + reason);
  }

}

static std::string
get_home_dir()
{
  if (const char *p = getenv("HOME"))
    {
      ERS_DEBUG(2, "home dir (process environment HOME) = \'" << p << '\'');
      return p;
    }

  try
    {
      static System::User s_user(getuid());
      return s_user.home();
    }
  catch (const System::SystemCallIssue& ex)
    {
      std::ostringstream text;
      text << "get_home_dir() failed:\n\tcaused by: " << ex;
      throw std::runtime_error(text.str().c_str());
    }
}

static std::string
get_gui_rc_file(const std::string& name)
{
  std::string file(get_home_dir());
  if(file[file.size()-1] != '/') file += '/';
  return (file + '.' + name + "-rc.xml");
}

static void
add_tdaq_path(OksKernel& k)
{
  if(const char * tdaq_path = getenv("TDAQ_INST_PATH")) {
    std::string s(tdaq_path);
    s += "/share/data";
    k.insert_repository_dir(s, false);
  }
  else {
    throw std::runtime_error("TDAQ_INST_PATH is not set");
  }
}


void
OksGC::Parameters::save() const
{
  std::string file;

  try {
    file = get_gui_rc_file(get_name());

    OksKernel k(false, false, false, false);
    k.set_silence_mode(true);

    add_tdaq_path(k);

    k.load_schema(gui_dp_schema_file_name);

    OksFile * h = k.new_data(file);
    h->add_include_file(gui_dp_schema_file_name);

    OksClass * str_param_class = k.find_class(oks_str_param_class_name);
    if(!str_param_class) {
      std::string text = std::string("Cannot find class \'") + oks_str_param_class_name + "\' in \'" + gui_dp_schema_file_name + '\'';
      throw std::runtime_error(text.c_str());
    }

    OksClass * u16_param_class = k.find_class(oks_u16_param_class_name);
    if(!u16_param_class) {
      std::string text = std::string("Cannot find class \'") + oks_u16_param_class_name + "\' in \'" + gui_dp_schema_file_name + '\'';
      throw std::runtime_error(text.c_str());
    }

    OksData graphical_d;
    graphical_d.Set("Graphical");

    OksAttribute * oks_type_attr= str_param_class->find_attribute(oks_type_attr_name);
    const char __Generic[] = "Generic";
    OksData generic_d;
    generic_d.SetE(__Generic, sizeof(__Generic)-1, oks_type_attr);

    {
      OksObject * o = new OksObject(str_param_class, font_name_object_id);
      OksData d(get_font_name());
      o->SetAttributeValue(oks_value_attr_name, &d);
      o->SetAttributeValue(oks_type_attr_name, &graphical_d);
    }

    {
      struct StrU16Pair {
        const char * name;
        unsigned short value;
      } d_u16_params[] = {
        {font_x_margin_object_id, get_font_x_margin()},
        {font_y_margin_object_id, get_font_y_margin()},
        {da_x_margin_object_id, get_drawing_area_x_margin()},
        {da_y_margin_object_id, get_drawing_area_y_margin()}
      };

      for(int i=0; i<4; ++i) {
        OksObject * o = new OksObject(u16_param_class, d_u16_params[i].name);
        OksData d(d_u16_params[i].value);
        o->SetAttributeValue(oks_value_attr_name, &d);
        o->SetAttributeValue(oks_type_attr_name, &graphical_d);
      }
    }

    {
      for(std::list<OksGC::Parameters::UShortParameter *>::const_iterator i = get_graphical_u16_parameters().begin(); i != get_graphical_u16_parameters().end(); ++i) {
        OksObject * o = new OksObject(u16_param_class, (*i)->p_name.c_str());
        OksData d2((*i)->p_value);
        o->SetAttributeValue(oks_value_attr_name, &d2);
	o->SetAttributeValue(oks_type_attr_name, &graphical_d);
      }
    }

    {
      for(std::list<OksGC::Parameters::StringParameter *>::const_iterator i = get_graphical_string_parameters().begin(); i != get_graphical_string_parameters().end(); ++i) {
        OksObject * o = new OksObject(str_param_class, (*i)->p_name.c_str());
        OksData d2((*i)->p_value);
        o->SetAttributeValue(oks_value_attr_name, &d2);
	o->SetAttributeValue(oks_type_attr_name, &graphical_d);
      }
    }

    {
      for(std::list<OksGC::Parameters::UShortParameter *>::const_iterator i = get_generic_u16_parameters().begin(); i != get_generic_u16_parameters().end(); ++i) {
        OksObject * o = new OksObject(u16_param_class, (*i)->p_name.c_str());
        OksData d2((*i)->p_value);
        o->SetAttributeValue(oks_value_attr_name, &d2);
	o->SetAttributeValue(oks_type_attr_name, &generic_d);
      }
    }

    {
      for(std::list<OksGC::Parameters::StringParameter *>::const_iterator i = get_generic_string_parameters().begin(); i != get_generic_string_parameters().end(); ++i) {
        OksObject * o = new OksObject(str_param_class, (*i)->p_name.c_str());
        OksData d2((*i)->p_value);
        o->SetAttributeValue(oks_value_attr_name, &d2);
	o->SetAttributeValue(oks_type_attr_name, &generic_d);
      }
    }

    k.save_data(h);

  }
  catch (oks::exception & e) {
    throw oks::FailedSaveGuiRcFile(file, e);
  }
  catch (std::exception & e) {
    throw oks::FailedSaveGuiRcFile(file, e.what());
  }
}


void
OksGC::Parameters::read()
{
  std::string file;

  try {
    file = get_gui_rc_file(get_name());

    OksKernel k(false, false, false, false);
    k.set_silence_mode(true);

    add_tdaq_path(k);

    k.load_data(file);

    if(OksClass * str_param_class = k.find_class(oks_str_param_class_name)) {
      if(const OksObject::Map * objects = str_param_class->objects()) {
        for(OksObject::Map::const_iterator i = objects->begin(); i != objects->end(); ++i) {
	  OksData * d(i->second->GetAttributeValue(oks_value_attr_name));
	  if(d) {
	    ERS_DEBUG(2, "read " << *d << " (string) of " << i->second);
	    if(*(i->first) == font_name_object_id) {
	      set_font_name(*d->data.STRING);
	    }
	    else {
	      OksData * d2(i->second->GetAttributeValue(oks_type_attr_name));
	      if(d2) {
	        if(*d2->data.ENUMERATION == "Graphical") {
	          set_graphical_parameter(*(i->first), *d->data.STRING);
		}
		else if(*d2->data.ENUMERATION == "Generic") {
	          set_generic_parameter(*(i->first), *d->data.STRING);
		}
		else {
	          std::string text = std::string("Read unsupported type \'") + *d2->data.ENUMERATION + "\' of \'" + *(i->first) + '@' + oks_str_param_class_name + '\'';
                  throw std::runtime_error(text.c_str());
		}
              }
	    }
	  }
	  else {
	    std::string text = std::string("Failed to read \'") + oks_value_attr_name + "\' of \'" + *(i->first) + '@' + oks_str_param_class_name + '\'';
            throw std::runtime_error(text.c_str());
	  }
	}
      }
    }
    else {
      std::string text = std::string("Cannot find class \'") + oks_str_param_class_name + "\' in \'" + gui_dp_schema_file_name + '\'';
      throw std::runtime_error(text.c_str());
    }

    if(OksClass * u16_param_class = k.find_class(oks_u16_param_class_name)) {
      if(const OksObject::Map * objects = u16_param_class->objects()) {
        for(OksObject::Map::const_iterator i = objects->begin(); i != objects->end(); ++i) {
	  OksData * d(i->second->GetAttributeValue(oks_value_attr_name));
	  if(d) {
	    ERS_DEBUG(2, "read " << *d << " (u16) of " << i->second);
	    if(*(i->first) == font_x_margin_object_id) {
	      set_font_x_margin(d->data.U16_INT);
	    }
	    else if(*(i->first) == font_y_margin_object_id) {
	      set_font_y_margin(d->data.U16_INT);
	    }
	    else if(*(i->first) == da_x_margin_object_id) {
	      set_drawing_area_x_margin(d->data.U16_INT);
	    }
	    else if(*(i->first) == da_y_margin_object_id) {
	      set_drawing_area_y_margin(d->data.U16_INT);
	    }
	    else {
	      OksData * d2(i->second->GetAttributeValue(oks_type_attr_name));
	      if(d2) {
	        if(*d2->data.ENUMERATION == "Graphical") {
	          set_graphical_parameter(*(i->first), d->data.U16_INT);
		}
		else if(*d2->data.ENUMERATION == "Generic") {
	          set_generic_parameter(*(i->first), d->data.U16_INT);
		}
		else {
	          std::string text = std::string("Read unsupported type \'") + *d2->data.ENUMERATION + "\' of \'" + *(i->first) + '@' + oks_u16_param_class_name + '\'';
                  throw std::runtime_error(text.c_str());
		}
              }
	    }
	  }
	  else {
	    std::string text = std::string("Failed to read \'") + oks_value_attr_name + "\' of \'" + *(i->first) + '@' + oks_u16_param_class_name + '\'';
            throw std::runtime_error(text.c_str());
	  }
	}
      }
    }
    else {
      std::string text = std::string("Cannot find class \'") + oks_u16_param_class_name + "\' in \'" + gui_dp_schema_file_name + '\'';
      throw std::runtime_error(text.c_str());
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedLoadGuiRcFile(file, e);
  }
  catch (std::exception & e) {
    throw oks::FailedLoadGuiRcFile(file, e.what());
  }
}
