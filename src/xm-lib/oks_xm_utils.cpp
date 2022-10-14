#define _OksXmBuildDll_

#include <string>
#include <vector>
#include <algorithm>

#include <fstream>
#include <sstream>

#include <boost/date_time/posix_time/time_parsers.hpp>
#include <boost/date_time/gregorian/parsers.hpp>

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/stat.h>

#include <oks/defs.h>
#include <oks/xm_utils.h>

#include <X11/cursorfont.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <Xm/SelectioB.h>
#include <Xm/Label.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>
#include <Xm/Matrix.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>

#include "../src/xm-gui/warning.xpm"


static XmStringCharSet charset = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;

XmString
OksXm::create_string(const char *s)
{
  return XmStringCreateLtoR((char *)s, charset);
}

char *
OksXm::string_to_c_str(const XmString xms)
{
  char *s;
  XmStringGetLtoR(xms, charset, &s);

  return s;
}

XtPointer
OksXm::get_user_data(Widget w)
{
  XtPointer p;
  XtVaGetValues(w, XmNuserData, &p, NULL);
  
  return p;
}


void
OksXm::set_user_data(Widget w, XtPointer p)
{
  XtVaSetValues(w, XmNuserData, p, NULL);
}

static void
act_close_cb(Widget shell, XtCallbackProc closeCB, void *arg, bool add)
{
  static Atom wmpAtom, dwAtom = 0;
  Display *display = XtDisplay(shell);

  XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);

  if (dwAtom == 0)
    {
      wmpAtom = XmInternAtom(display, (char *) "WM_PROTOCOLS", true);
      dwAtom = XmInternAtom(display, (char *) "WM_DELETE_WINDOW", true);
    }

  if (add)
    XmAddProtocolCallback(shell, wmpAtom, dwAtom, (XtCallbackProc) closeCB, (XtPointer) arg);
  else
    XmRemoveProtocolCallback(shell, wmpAtom, dwAtom, (XtCallbackProc) closeCB, (XtPointer) arg);
}


void
OksXm::set_close_cb(Widget shell, XtCallbackProc closeCB, void *arg)
{
  act_close_cb(shell, closeCB, arg, true);
}


void
OksXm::remove_close_cb(Widget shell, XtCallbackProc closeCB, void *arg)
{
  act_close_cb(shell, closeCB, arg, false);
}



Pixmap
OksXm::create_color_pixmap(Widget parent, const char **image_string)
{
  Pixmap		pixmap;

  XpmAttributes		attributes;

  XpmColorSymbol	symbols[] = {
    {(char *)"BottomShadow", 0, 0},
    {(char *)"TopShadow", 0, 0},
    {(char *)"Background", 0, 0},
    {(char *)"Foreground", 0, 0}
  };

  XtVaGetValues(
    parent,
    XmNbottomShadowColor, &symbols[0].pixel,
    XmNtopShadowColor, &symbols[1].pixel,
    XmNbackground, &symbols[2].pixel,
    XmNforeground, &symbols[3].pixel,
    NULL
  );

  attributes.colorsymbols = symbols;
  attributes.numsymbols = sizeof(symbols) / sizeof(XpmColorSymbol);
  attributes.valuemask = XpmColorSymbols;

  int status = XpmCreatePixmapFromData(
    XtDisplay(parent),
    DefaultRootWindow(XtDisplay(parent)),
    (char **)image_string,
    &pixmap,
    0,
    &attributes
  );

  if(status != XpmSuccess) {
    Oks::error_msg("OksXm::create_color_pixmap(Widget, char **)")
      << "Can't create pixmap\n"
         "Xpm library error: " << XpmGetErrorString(status) << std::endl;
	
    return 0;
  }
  else
    return pixmap;
}

Widget
OksXm::RowColumn::create_pixmap_and_text(Widget parent, const char **image_string, const char *message)
{
  Arg   al[10];
  int   ac;

  // detect max dimensions of the message
  Dimension screen_h = HeightOfScreen(XtScreen(parent)) - 132; // take into account elements of window
  Dimension screen_w = WidthOfScreen(XtScreen(parent)) - 132;

  ac = 0;
  XmString string = OksXm::create_string(message);
  XtSetArg(al[ac], XmNlabelString, string); ac++;
  Widget l = XmCreateLabel(parent, (char *)"text_label", al, ac);
  Dimension width, height;

  XtVaGetValues(l, XmNwidth, &width, XmNheight, &height, NULL);
  XtDestroyWidget(l);

  width += 100; // add pixmap width, margins, etc.
  height += 20; // add margins, etc.

  // add scrollbars if the message is too large
  if (height > screen_h || width > screen_w)
    {
      if (height > screen_h) height = screen_h;
      if (width > screen_w) width = screen_w;

      ac = 0;
      XtSetArg(al[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
      XtSetArg(al[ac], XmNwidth, width);  ac++;
      XtSetArg(al[ac], XmNheight, height);  ac++;

      Widget scrolledWindow = XmCreateScrolledWindow(parent, (char*) "scrolled window", al, ac);
      XtManageChild(scrolledWindow);
      parent = scrolledWindow;
    }

  ac = 0;
  XtSetArg(al[ac], XmNorientation, XmHORIZONTAL); ac++;

  Widget row = XmCreateRowColumn(parent, (char *)"workarea", al, ac);
  XtManageChild(row);
  
  Pixmap pixmap = OksXm::create_color_pixmap(parent, image_string);

  if(pixmap) {
    ac = 0;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, pixmap); ac++;
    XtManageChild(XmCreateLabel(row, (char *)"pixmap_label", al, ac));	
  }


  ac = 0;
  XtSetArg(al[ac], XmNlabelString, string); ac++;
  XtManageChild(XmCreateLabel(row, (char *)"text_label", al, ac));
  XmStringFree(string);

  return row;
}


Widget
OksXm::create_dlg_with_pixmap (Widget parent, const char *name, const char **image_string, const char *message, const char *b1, const char *b2, const char *b3, ArgList arglist, int argcount)
{
  Widget	warning_dialog;
  Widget	work_area;
  Widget	button;
  Widget	kid[6];
  Arg		al[10];
  XmString	string;

  warning_dialog = XmCreatePromptDialog(parent, (char *)name, arglist, argcount);
  XtVaSetValues(XtParent(warning_dialog), XmNtitle, name, NULL);
 
  work_area = XmCreateRowColumn(warning_dialog, (char *)"workarea", al, 0);
  XtManageChild(work_area);
  
  OksXm::set_user_data(warning_dialog, (XtPointer)work_area);
  
  OksXm::RowColumn::create_pixmap_and_text(work_area, image_string, message);

  int i = 0;
  kid[i++] = XmSelectionBoxGetChild (warning_dialog, XmDIALOG_TEXT);
  kid[i++] = XmSelectionBoxGetChild (warning_dialog, XmDIALOG_SELECTION_LABEL);
  
  button = XmSelectionBoxGetChild(warning_dialog, XmDIALOG_OK_BUTTON);
  if(b1) {
    string = OksXm::create_string(b1);
    XtVaSetValues(button, XmNlabelString, string, NULL);
    XmStringFree(string);
  }
  else
	kid[i++] = button;

  button = XmSelectionBoxGetChild(warning_dialog, XmDIALOG_CANCEL_BUTTON);
  if(b2) {
    string = OksXm::create_string(b2);
    XtVaSetValues(button, XmNlabelString, string, NULL);
    XmStringFree(string);
  }
  else
	kid[i++] = button;

  button = XmSelectionBoxGetChild(warning_dialog, XmDIALOG_HELP_BUTTON);
  if(b3) {
    string = OksXm::create_string(b3);
    XtVaSetValues(button, XmNlabelString, string, NULL);
    XmStringFree(string);
  }
  else
    kid[i++] = button;

  XtUnmanageChildren (kid, i);

  return(warning_dialog);
}


char*
OksXm::List::get_selected_value(Widget w)
{
  char *value = 0;
  int number_of_items = OksXm::List::size(w);

  if(number_of_items) {
    int *position_list = 0, position_count = 0;

    if(XmListGetSelectedPos(w, &position_list, &position_count)) {
      if(position_list && position_count) {
        if(*position_list > number_of_items)
	  Oks::warning_msg("oks-xm-library")
	    << "XmListGetSelectedPos() returned bad selected item position ("
	    << *position_list << ") but list contains " << number_of_items << " item(s) only.\n";
	else {
          XmString *strings;
          XtVaGetValues(w, XmNitems, &strings, NULL);
          value = OksXm::string_to_c_str(strings[*position_list - 1]);
          XtFree((char *)position_list);
	}
      }
    }
  }

  return value;
}


char *
OksXm::List::get_value(Widget w, int pos)
{
  if(pos <= OksXm::List::size(w)) {
    XmString *strings;
    XtVaGetValues(w, XmNitems, &strings, NULL);
    return OksXm::string_to_c_str(strings[pos-1]);
  }
  else
    return 0;
}


int
OksXm::List::get_selected_pos(Widget w)
{
  int position = 0;
  int number_of_items = OksXm::List::size(w);
  
  if(number_of_items) {
    int *position_list = 0, position_count = 0;

    if(XmListGetSelectedPos(w, &position_list, &position_count)) {
      if(position_list && position_count) {
        if(*position_list > number_of_items)
	  Oks::warning_msg("oks-xm-library")
	    << "XmListGetSelectedPos() returned bad selected item position ("
	    << *position_list << ") but list contains " << number_of_items << " item(s) only.\n";
	else {
          position = *position_list;
          XtFree((char *)position_list);  
        }
      }
    }
  }
  
  return position;
}


int
OksXm::List::size(Widget w)
{
  int itemCount;
  XtVaGetValues(w, XmNitemCount, &itemCount, NULL);

  return itemCount;
}


void
OksXm::List::select_row(Widget w, const char *value)
{
  XmString string = OksXm::create_string(value);
  if(XmListItemExists(w, string)) XmListSelectItem(w, string, true);
  XmStringFree(string);
}


void
OksXm::List::delete_row(Widget w, const char *value)
{
  XmString string = OksXm::create_string(value);
  if(XmListItemExists(w, string)) XmListDeleteItem(w, string);
  XmStringFree(string);
}


void
OksXm::List::add_row(Widget w, const char *value, Boolean testExistance, const char * render_tag)
{
  XmString string = (
    render_tag
      ? XmStringGenerate ((XtPointer) value, 0, XmCHARSET_TEXT, const_cast<char *>(render_tag))
      : OksXm::create_string(value)
  );
  
  if(!testExistance || !XmListItemExists(w, string)) XmListAddItem(w, string, 0);
  XmStringFree(string);
}


void
OksXm::List::move_item(Widget w, int pos, bool up)
{
  XmString *strings;
  XtVaGetValues(w, XmNitems, &strings, NULL);

  XmString xm_string = XmStringCopy(strings[pos - 1]);
  XmListDeletePos(w, pos);
  XmListAddItem(
    w,
    xm_string,
    (up == true ? pos - 1 : pos + 1)
  );

  XmListSelectItem(w, xm_string, true);

  XmStringFree(xm_string);
}


void
OksXm::List::replace(Widget w, const char * old_item, const char * new_item)
{
  XmString xm_old_obj_name = OksXm::create_string(old_item);

  int pos = XmListItemPos(w, xm_old_obj_name);

  if(pos) {
    XmString xm_new_obj_name = OksXm::create_string(new_item);
    XmListReplaceItemsPos(w, &xm_new_obj_name, 1, pos);
    XmStringFree(xm_new_obj_name);
  }

  XmStringFree(xm_old_obj_name);
}

void
OksXm::Label::set(Widget w, const char *value)
{
  XmString string = OksXm::create_string(value);
  XtVaSetValues(w, XmNlabelString, string, NULL);
  XmStringFree(string);
}


void
OksXm::TextField::set_not_editable(Widget w)
{
  XtVaSetValues(
    w,
    XmNcursorPositionVisible, false,
    XmNeditable, false,
    XmNverifyBell, false,
    NULL
  );
}


void
OksXm::TextField::set_editable(Widget w)
{
  XtVaSetValues(
    w,
    XmNcursorPositionVisible, true,
    XmNeditable, true,
    XmNverifyBell, true,
    NULL
  );
}


void
OksXm::TextField::set_width(Widget w, short i)
{
  if(i == 0) {
    OksXm::AutoCstring buf(XmTextFieldGetString(w));
    i = strlen(buf.get());
  }

  XtVaSetValues(w, XmNresizeWidth, true, XmNcolumns, i, NULL);
}

	//
	// The structure XmStringTableItem describes an item of
	// XmList widget and is used to sort that list
	//

struct XmStringTableItem {
  XmString xm_string;
  OksXm::AutoCstring value;

  XmStringTableItem(XmString xms) : xm_string(XmStringCopy(xms)), value(OksXm::string_to_c_str(xms)) { ; }
  ~XmStringTableItem() { XmStringFree(xm_string); }
  
  static int is_less(XmStringTableItem * i1, XmStringTableItem * i2) {
    return (strcmp(i1->value.get(), i2->value.get()) < 0 ? true : false);
  }
};


void
OksXm::List::sortCB(Widget, XtPointer w, XtPointer)
{
  size_t n, i;
  XmStringTable	table;

  XtVaGetValues((Widget)w, XmNitemCount, &n, XmNitems, &table, NULL);
  if(n < 2) return;
  
  std::vector<XmStringTableItem *> vec;
  vec.reserve(n);

  for(i=0;i<n;i++) vec.push_back(new XmStringTableItem(table[i]));

  std::sort(vec.begin(), vec.end(), XmStringTableItem::is_less);
  
  XmListDeleteAllItems((Widget)w);

  for(i=0;i<n;i++) {
    XmListAddItem((Widget)w, vec[i]->xm_string, 0);
    delete vec[i];
  }
}


	//
	// The structure XbaeMatrixCell describes an item of
	// XbaeMatrix widget and is used to sort that matrix
	//

struct XbaeMatrixCell {
  const String			*items;
  const Pixel			*pixels;
  static int			lenght;
  static int			position;
  static OksXm::Matrix::SortType sort_type;
  XtPointer			row_user_data;

  XbaeMatrixCell(const String * c,const Pixel *p, XtPointer d) : items (c), pixels (p), row_user_data (d) {
  }

  ~XbaeMatrixCell() {
  }

  static int is_less(XbaeMatrixCell * i1, XbaeMatrixCell * i2) {
    const char * str1 = i1->items[position];
    const char * str2 = i2->items[position];

    if(*str1 == '(') str1++;
    if(*str2 == '(') str2++;

    if(sort_type == OksXm::Matrix::SortByString) {
      return (strcmp(str1, str2) < 0 ? true : false);
    }
    else if(sort_type == OksXm::Matrix::SortByDate) {
      boost::gregorian::date dt1(boost::gregorian::from_string(str1));
      boost::gregorian::date dt2(boost::gregorian::from_string(str2));

      return (dt1 < dt2);
    }
    else if(sort_type == OksXm::Matrix::SortByTime) {
      boost::posix_time::ptime tm1(boost::posix_time::time_from_string(str1)); 
      boost::posix_time::ptime tm2(boost::posix_time::time_from_string(str2));

      return (tm1 < tm2);
    }
    else {
      if(sort_type == OksXm::Matrix::SortByInteger) {
        int64_t i1(strtoll(str1,0,0));
        int64_t i2(strtoll(str2,0,0));

        return (i1 < i2);
      }
      else if(sort_type == OksXm::Matrix::SortByUnsigned) {
        uint64_t u1(strtoull(str1,0,0));
        uint64_t u2(strtoull(str2,0,0));

        return (u1 < u2);
      }
      else {
        double d1(strtod(str1,0));
        double d2(strtod(str2,0));

        return (d1 < d2);
      }
    }
  }
};


int			XbaeMatrixCell::lenght;
int			XbaeMatrixCell::position;
OksXm::Matrix::SortType	XbaeMatrixCell::sort_type;


void
OksXm::Matrix::sort_rows(Widget matrix, int pos, OksXm::Matrix::SortType st)
{
  int w, h, i;
  const String **rows;
  const Pixel **pixels = 0;

  XtVaGetValues(matrix, XmNcolumns, &w, XmNrows, &h, XmNcells, &rows, XmNcolors, &pixels, NULL);

  XbaeMatrixSetColumnUserData(matrix, 0, reinterpret_cast<XtPointer>(st));
  if(w>1) XbaeMatrixSetColumnUserData(matrix, 1, reinterpret_cast<XtPointer>(pos));

  if(h < 2) return;

  std::vector<XbaeMatrixCell *> vec;
  vec.reserve(h);

  XbaeMatrixCell::lenght = w;
  XbaeMatrixCell::position = pos;
  XbaeMatrixCell::sort_type = st;

  for(i=0;i<h;i++) {
    vec.push_back(new XbaeMatrixCell(rows[i], (pixels ? pixels[i] : 0), XbaeMatrixGetRowUserData(matrix, i)));
  }

  std::stable_sort(vec.begin(), vec.end(), XbaeMatrixCell::is_less);

  for(i=0;i<h;i++) {
    rows[i] = vec[i]->items;
    if(pixels) { pixels[i] = vec[i]->pixels; }
  }

  XtVaSetValues(matrix, XmNcells, rows, NULL);

  if(pixels) {
    XtVaSetValues(matrix, XmNcells, rows, XmNcolors, pixels, NULL);
  }

  for(i=0;i<h;i++)
    XbaeMatrixSetRowUserData(matrix, i, vec[i]->row_user_data);

  for(i=0;i<h;i++)
    delete vec[i];
  
  XbaeMatrixRefresh(matrix);
}

bool
OksXm::Matrix::resort_rows(Widget matrix)
{
  if(XtPointer p = XbaeMatrixGetColumnUserData(matrix, 0)) {
    int w;
    XtVaGetValues(matrix, XmNcolumns, &w, NULL);
    if(w>1) {
      w = static_cast<int>(reinterpret_cast<long>(XbaeMatrixGetColumnUserData(matrix, 1)));
    }
    else {
      w = 0;
    }

    ERS_DEBUG(3, "sort column " << w  << " as " << p << " of matrix " << matrix );
    OksXm::Matrix::sort_rows(matrix, w, static_cast<OksXm::Matrix::SortType>(reinterpret_cast<long>(p)));
    return true;
  }
  else {
    ERS_DEBUG(3, "cannot resort unsorted matrix " << matrix );
    return false;
  }
}

void
OksXm::Matrix::set_cell_background(Widget matrix, Widget sample)
{
  int h, w, i, j;
  Pixel	pixel;
  XtVaGetValues(sample, XmNbackground, &pixel, NULL);
  XtVaGetValues(matrix, XmNcolumns, &w, XmNrows, &h, NULL);

  for(j=0; j<h; j++)
    for(i=0; i<w; i++)
      XbaeMatrixSetCellBackground(matrix, j, i, pixel);
}

size_t
OksXm::Matrix::get_row_by_value(Widget matrix, const char *value)
{
  int rowsNumber = XbaeMatrixNumRows(matrix);

  for(int j=0; j<rowsNumber; j++) {
    const char *first_cell = XbaeMatrixGetCell(matrix, j, 0);
		
    if(first_cell && !strcmp(first_cell, value)) return j;
  }
  
  return (size_t)-1;
}

const char*
OksXm::Matrix::get_selected_value(Widget matrix)
{
  int row = -1, column = -1;

  XbaeMatrixFirstSelectedCell(matrix, &row, &column);

  return (
    (row != -1 && column != -1)
      ? XbaeMatrixGetCell(matrix, row, column) : 0
  );
}

XtPointer
OksXm::Matrix::get_selected_row_user_data(Widget matrix)
{
  int row = -1, column = -1;

  XbaeMatrixFirstSelectedCell(matrix, &row, &column);

  return (
    (row != -1 && column != -1)
      ? XbaeMatrixGetRowUserData(matrix, row) : 0
  );
}


void
OksXm::Matrix::delete_row_by_user_data(Widget matrix, XtPointer value)
{
  int rowsNumber = XbaeMatrixNumRows(matrix);

  for(int j=0; j<rowsNumber; j++) {
    if(value == XbaeMatrixGetRowUserData(matrix, j)) {
      XbaeMatrixDeleteRows(matrix, j, 1);
      break;
    }
  }
}

void
OksXm::Matrix::select_row(Widget matrix, const char *value)
{
  int rowsNumber = XbaeMatrixNumRows(matrix);

  for(int j=0; j<rowsNumber; j++) {
    const char *first_cell = XbaeMatrixGetCell(matrix, j, 0);
    if(first_cell && !strcmp(first_cell, value)) {
      XbaeMatrixDeselectAll(matrix);
      XbaeMatrixSelectRow(matrix, j);
      XbaeMatrixMakeCellVisible(matrix, j, 0);
      break;
    }
  }
}

void
OksXm::Matrix::set_visible_height(Widget matrix, size_t vh)
{
  size_t rowsNumber = XbaeMatrixNumRows(matrix);

  XtVaSetValues(
    matrix,
    XmNvisibleRows,
    (
      (rowsNumber <= 1) ? 1 :
      (rowsNumber <= vh) ? rowsNumber :
      vh
    ),
    NULL
  );
}

XFontStruct *
OksXm::get_font_from_list(XmFontList font_list)
{
  XmFontContext context;

  if (!XmFontListInitFontContext(&context, font_list)) return 0;

  XmFontListEntry font_list_entry = XmFontListNextEntry(context);
  if (!font_list_entry) return 0;

  XmFontType type;
  XFontStruct *font = (XFontStruct*)XmFontListEntryGetFont(font_list_entry, &type);

  XmFontListFreeFontContext(context);

  return font;
}


    //
    // A matrix cell has the following structure relevant to it's width:
    //
    //   *ST*HT*MW*TEXT*MW*HT*ST, where
    //   1) ST   - XmNcellShadowThickness    (1 by default)
    //   2) HT   - XmNcellHighlightThickness (2 by default)
    //   3) MW   - XmNcellMarginWidth        (3 by default)
    //   4) TEXT - text                      (width depeneds on text and used font)
    //

static Dimension
get_matrix_cell_delta(Widget w, const char * what = XmNcellMarginWidth)
{
  Dimension cell_margin_width;
  Dimension cell_shadow_thickness;
  Dimension cell_highlight_thickness;

  XtVaGetValues(w,
    what, &cell_margin_width,
    XmNcellShadowThickness, &cell_shadow_thickness,
    XmNcellHighlightThickness, &cell_highlight_thickness,
    NULL
  );

  return 2 * (cell_margin_width + cell_shadow_thickness + cell_highlight_thickness);
}

static Dimension
get_matrix_scrollbar_width(Widget w, const char * which, const char * what)
{
  Widget scb;
  XtVaGetValues(w, which, &scb, NULL);

  if(XtIsManaged(scb)) {
    Dimension width, border_width;  // scrollbar properties
    Dimension space;                // matrix property

    XtVaGetValues(scb, what, &width, XmNborderWidth, &border_width, NULL);
    XtVaGetValues(w, XmNspace, &space, NULL);

    return (width + 2 * border_width + space);
  }

  return 0;
}

	//
	// max_cell_width is maximum width of cell in pixels
	// default value is 300 pixels
	//

void
OksXm::Matrix::adjust_column_widths(Widget matrix, int max_cell_width)
{
  int h, w, i, j;
  const String	**rows;
  const String	*labels;

  XmFontList cell_font_list, label_font_list;
  XtVaGetValues(matrix, XmNlabelFont, &label_font_list, XmNfontList, &cell_font_list, NULL);
  XFontStruct * cell_font = OksXm::get_font_from_list(cell_font_list);
  XFontStruct * label_font = OksXm::get_font_from_list(label_font_list);

  int max_cell_letter_width = (cell_font->max_bounds.width + cell_font->min_bounds.width) / 2;
  
  if(max_cell_letter_width <= 0) {
    Oks::warning_msg("oks-xm-library") << "Can't detect font parameters, use default values\n";
    max_cell_letter_width = 10;
  }

  XtVaGetValues(matrix, XmNcolumns, &w, XmNrows, &h, XmNcells, &rows, XmNcolumnLabels, &labels, NULL);

  short * widths = new short[w];
  String * new_labels = new String[w];


    // process cell items first

  for(i=0; i<w; i++) {
    widths[i] = 1;

    for(j=0; j<h; j++) {
      const String * row = rows[j];
      const char * cell_value = row[i];
      int cell_width = XTextWidth(cell_font, cell_value, strlen(cell_value));

      if(widths[i] < cell_width) widths[i] = cell_width;
    }

    if(widths[i] > max_cell_width) widths[i] = max_cell_width;
  }


  const char * delims = " \t\n,";

    // process labels and modify if possible

  for(i=0; i<w; i++) {

      // processing label

    const std::string label(labels[i]);

      // calculate minimal possible width

    if(XTextWidth(label_font, label.c_str(), label.length()) > widths[i]) {
      Oks::Tokenizer t(label, delims);
      std::string token;

      while(!(token = t.next()).empty()) {
	int word_width = XTextWidth(label_font, token.c_str(), token.length());
	if(widths[i] < word_width) widths[i] = word_width;
      }
    }
    else {
      new_labels[i] = new char[label.length() + 1];
      strcpy(new_labels[i], label.c_str());
      continue;
    }


      // put as many words as possible on single line, start new line otherwise

    std::string new_label, line;

    Oks::Tokenizer t(label,delims);
    std::string token;

    while(!(token = t.next()).empty()) {
      std::string test(line);
      if(test.length()) test += " ";
      test += token;
	
      if(XTextWidth(label_font, test.c_str(), test.length()) <= widths[i]) line = test;
      else {
        if(new_label.length()) new_label += "\n";
        new_label += line;
        line = token;
      }
    }

    if(new_label.length()) new_label += "\n";
    new_label += line;

    new_labels[i] = new char[new_label.length() + 1];
    strcpy(new_labels[i], new_label.c_str());
  }

  for(i=0; i<w; i++)
    widths[i] = widths[i]/max_cell_letter_width + 1;

  XtVaSetValues(matrix, XmNcolumnWidths, widths, XmNcolumnLabels, new_labels, NULL);

  for(i=0; i<w; i++)
    delete [] new_labels[i];

  delete [] new_labels;
  delete [] widths;
}


void
OksXm::Matrix::set_widths(Widget mw, int visibleColumns)
{
  int w;
  short *widths;
  Dimension d = get_matrix_scrollbar_width(mw, XmNverticalScrollBar, XmNwidth);

  XmFontList cell_font_list;
  Dimension shadow_thickness;

  XtVaGetValues(mw,
    XmNfontList, &cell_font_list,
    XmNshadowThickness, &shadow_thickness,
    XmNcolumns, &w,
    XmNcolumnWidths, &widths,
    NULL
  );

  XFontStruct * cell_font = OksXm::get_font_from_list(cell_font_list);
  Dimension max_cell_letter_width = (cell_font->max_bounds.width + cell_font->min_bounds.width)/2;

  Dimension cell_delta = get_matrix_cell_delta(mw, XmNcellMarginWidth);

  if(visibleColumns > w) {
    visibleColumns = w;
  }

  for(int i=0; i<visibleColumns; i++) {
    d += widths[i] * max_cell_letter_width + cell_delta;
  }

  d += shadow_thickness;

  XtVaSetValues(mw, XmNwidth, d, NULL);
}

void
OksXm::Matrix::cellCB(Widget w, XtPointer data, XtPointer cb)
{
  XbaeMatrixEnterCellCallbackStruct *cbs = (XbaeMatrixEnterCellCallbackStruct *)cb;
  cbs->doit = False;
  cbs->map = False;

  if(cbs->event == 0) return; // skip non-button events
//std::cout << "reason = " << cbs->reason << ", event.button: " << (cbs->event ? cbs->event->xbutton.button : 0) << std::endl;

  if(XbaeMatrixIsCellSelected(w, cbs->row, 0) == false) {
    XbaeMatrixDeselectAll(w);

    if(!data) {
      XbaeMatrixSelectRow(w, cbs->row);
    }
  }
}

void
OksXm::Matrix::resizeCB(Widget w, XtPointer client_data, XtPointer)
{
  Dimension parent_width = 0;
  XtVaGetValues(XtParent(w), XmNwidth, &parent_width, NULL);

  int columns_num;
  short * widths;
  XmFontList cell_font_list;
  int left_offset, right_offset;
  Dimension shadow_thickness, border_width;

  XtVaGetValues(w,
    XmNshadowThickness, &shadow_thickness,
    XmNborderWidth, &border_width,
    XmNcolumns, &columns_num,
    XmNcolumnWidths, &widths,
    XmNfontList, &cell_font_list, 
    XmNleftOffset, &left_offset,
    XmNrightOffset, &right_offset,
    NULL
  );

  

  if(border_width == 0) border_width = 1; // FIXME: XbaeMatrix bug?

  Dimension cell_delta = get_matrix_cell_delta(w, XmNcellMarginWidth);
  Dimension table_delta = left_offset + right_offset + 2 * (shadow_thickness + border_width);


    // calculate letter width

  XFontStruct * cell_font = OksXm::get_font_from_list(cell_font_list);
  int max_cell_letter_width = (cell_font->max_bounds.width + cell_font->min_bounds.width) / 2;

  if(max_cell_letter_width <= 0) {
    Oks::warning_msg("oks-xm-library") << "Can't detect font parameters, use default values\n";
    max_cell_letter_width = 10;
  }


  short width = get_matrix_scrollbar_width(w, XmNverticalScrollBar, XmNwidth);
  for(int i=0; i<columns_num - 1; i++) {
    width += (widths[i] * max_cell_letter_width + cell_delta);
  }

  short last_column_width = (parent_width - width - table_delta - cell_delta) / max_cell_letter_width;

  ERS_DEBUG(8, "last-column-width: " << last_column_width << ", error: " << ((parent_width - width - table_delta - cell_delta) - (last_column_width * max_cell_letter_width)));

  if(last_column_width < 0) last_column_width = 0;


    // calculate last column width if it is non-empty

  {
    const String *labels;

    XtVaGetValues(w, XmNcolumnLabels, &labels, NULL);

    if(labels) {
      String label = labels[columns_num - 1];

      if(label && *label != 0) {
        int real_width = 0;


          // get matrix attributes

        const String **rows = nullptr;
        int h;
        XmFontList label_font_list;

        XtVaGetValues(w, XmNlabelFont, &label_font_list, XmNcells, &rows, XmNrows, &h, NULL);


          // process label

        XFontStruct * label_font = OksXm::get_font_from_list(label_font_list);

        Oks::Tokenizer t(label, "\n");
        std::string token;

        while(!(token = t.next()).empty()) {
	  int word_width = XTextWidth(label_font, token.c_str(), token.length());
	  if(real_width < word_width) real_width = word_width;
        }


          // process cells of last column

        if(rows)
          for(int i=0; i<h; i++) {
            const char * cell_value = rows[i][columns_num - 1];
            int cell_width = XTextWidth(cell_font, cell_value, strlen(cell_value));
            if(real_width < cell_width) real_width = cell_width;
          }


          // reset last column width if necessary

        if(real_width > (parent_width - width - table_delta - cell_delta)) {
          last_column_width = real_width / max_cell_letter_width;
        }
      }
    }
  }

  if(client_data) {
    OksXm::Matrix::ResizeInfo * info = (OksXm::Matrix::ResizeInfo *)client_data;
    if((size_t)last_column_width <= info->column_width) {
      last_column_width = widths[columns_num - 1];
    }
  }

  if(last_column_width != widths[columns_num - 1]) {
    ERS_DEBUG(8, "set new width for last column = " << last_column_width);

    short * widths2 = new short[columns_num];

    for(int i=0; i<columns_num - 1; i++) {
      widths2[i] = widths[i];
    }

    widths2[columns_num - 1] = last_column_width;
    XtVaSetValues(w, XmNcolumnWidths, widths2, NULL);

    delete [] widths2;
  }

  if(client_data) {

      // remove edit cell if it is managed
      // NOTE: move this above, if it will be used not only in message log
    {
      Widget edit_cell;
      XtVaGetValues(w, XmNtextField, &edit_cell, NULL);
      if(XtIsManaged(edit_cell)) {
        XbaeMatrixCancelEdit(w, True);
      }
    }

      // detect if we need to insert new rows

    Dimension h;
    XtVaGetValues(w, XmNheight, &h, NULL);

    Dimension sbar_h = get_matrix_scrollbar_width(w, XmNhorizontalScrollBar, XmNheight);

    int row_height = XbaeMatrixGetRowHeight(w, -1);
    

      // calculate minimal required number of rows to fill height of widget

    table_delta = 2 * (shadow_thickness + border_width) + sbar_h;

    Dimension full_row_height = row_height + get_matrix_cell_delta(w, XmNcellMarginHeight);
    
    int required_num_of_rows = (h - table_delta) / full_row_height;
    int real_num_of_rows = XbaeMatrixNumRows(w);
    
    if(required_num_of_rows > real_num_of_rows) {
      XbaeMatrixAddRows(w, real_num_of_rows, 0, 0, 0, required_num_of_rows - real_num_of_rows);
    }
    else {
      OksXm::Matrix::ResizeInfo * info = (OksXm::Matrix::ResizeInfo *)client_data;
      if(info->num_of_rows > (size_t)required_num_of_rows) {
        required_num_of_rows = info->num_of_rows;
      }

      if(required_num_of_rows < real_num_of_rows) {
        XbaeMatrixDeleteRows(w, required_num_of_rows, real_num_of_rows - required_num_of_rows);
      }
    }
  }
}


	//
	// The utilities to simplify menu creation
	//

Widget
OksXm::MenuBar::add_push_button(Widget parent, const char *name, long id, XtCallbackProc cb)
{
  Widget button = XmCreatePushButton (parent, const_cast<char *>(name), 0, 0);
  XtManageChild (button);
  XtAddCallback (button, XmNactivateCallback, cb, (XtPointer)id);
  
  return button;
}

Widget
OksXm::MenuBar::add_toggle_button(Widget parent, const char *name, long id, int on, XtCallbackProc cb)
{
  Widget button = XmCreateToggleButton (parent, const_cast<char *>(name), 0, 0);
  XtManageChild (button);
  XtAddCallback (button, XmNvalueChangedCallback, cb, (XtPointer)id);

  if(on) XtVaSetValues(button, XmNset, true, NULL);

  return button;
}

Widget
OksXm::MenuBar::add_cascade_button(Widget parent, Widget sub_menu_id, const char *name, char mnemonic)
{
  Arg args[2];

  XtSetArg (args[0], XmNsubMenuId, sub_menu_id);
  XtSetArg(args[1], XmNmnemonic, mnemonic);

  Widget cascade = XmCreateCascadeButton (parent, const_cast<char *>(name), args, 2);
  XtManageChild (cascade);
  
  return cascade;
}

void
OksXm::MenuBar::add_separator(Widget parent)
{
  XtManageChild(XmCreateSeparator(parent, (char *)"", 0, 0));
}


	//
	// Create prompt dialog and get string value
	//

static std::string ask_str;
static bool ask_dlg_is_shown;

static void
askCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if((long)client_data == 0 && ((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksXm::AutoCstring item_name(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));
    ask_str = item_name.get();
  }

  if((long)client_data == 1) {
    XmFileSelectionBoxCallbackStruct * fsb = (XmFileSelectionBoxCallbackStruct *)call_data;
    if(fsb->reason == XmCR_OK) {
      OksXm::AutoCstring item_name(OksXm::string_to_c_str(fsb->value));
      ask_str = item_name.get();
    }
  }

  ask_dlg_is_shown = false;
}


std::string
OksXm::ask_name(Widget parent, const char * item_name, const char * item_title, const char * item_value, XtCallbackProc hf, XtPointer hf_data)
{
  std::string s(item_name);
  s += ':';

  Arg args[10];

  XmString string = OksXm::create_string(s.c_str());
  Widget dialog;

  XtSetArg(args[0], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
  XtSetArg(args[1], XmNselectionLabelString, string);

  dialog = XmCreatePromptDialog(parent, (char *)"prompt dialog", args, 2);
  XtVaSetValues(XtParent(dialog), XmNtitle, item_title, NULL);

  if(item_value)
    XmTextSetString(XmSelectionBoxGetChild(dialog, XmDIALOG_VALUE_TEXT), const_cast<char *>(item_value));

  XtAddCallback(dialog, XmNokCallback, askCB, 0);
  XtAddCallback(dialog, XmNcancelCallback, askCB, 0);

  if(hf) XtAddCallback(dialog, XmNhelpCallback, hf, hf_data);

  XtManageChild(dialog);
  XmStringFree(string);

  ask_str = "";
  ask_dlg_is_shown = true;

  while(ask_dlg_is_shown == true)
    XtAppProcessEvent(XtWidgetToApplicationContext(dialog), XtIMAll);

  XtDestroyWidget(XtParent(dialog));

  return ask_str;
}

static enum TextDlgResponce
{
  ask_text_dlg_no_answer = 0,
  ask_text_dlg_ok = 1,
  ask_text_dlg_cancel = 2,
  ask_text_dlg_parent_delete = 3
} done_with_ask_text_dlg;

static void
ask_text_CB(Widget, XtPointer d, XtPointer)
{
  done_with_ask_text_dlg = static_cast<TextDlgResponce>(reinterpret_cast<long>(d));
}

std::string
OksXm::ask_text(Widget parent, const char * item_title, const char * item_label, const char * item_value)
{
  short num_of_rows(1);
  short num_of_columns(1);

  if(!item_value) item_value = "";

  short len(0);
  const char * p = item_value;

  while(*p != 0)
    {
      if(*p++ == '\n')
        {
          num_of_rows++;
          if(len > num_of_columns) num_of_columns = len;
          len = 0;
        }
      else
        {
          len++;
        }
    }

  if(len > num_of_columns) num_of_columns = len;

  if(num_of_rows < 2) num_of_rows = 2;
  if(num_of_rows > 20) num_of_rows = 20;

  if(num_of_columns < 32) num_of_columns = 32;
  if(num_of_columns > 128) num_of_columns = 128;

  Arg args[20];

  XtSetArg(args[0], XmNautoUnmanage, false);
  XtSetArg(args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
  XtSetArg(args[2], XmNtitle, item_title);
  XtSetArg(args[3], XmNiconName, item_title);
  XtSetArg(args[4], XmNallowShellResize, true);

  Widget dialog = XtCreateWidget((char *)"ask_text", xmDialogShellWidgetClass, parent, args, 5);
  Widget form = XmCreateForm(dialog, (char *)"form", args, 2);

  // Create Label
  int n = 0;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);               n++;
  XtSetArg (args[n], XmNtopOffset, 10);                              n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);              n++;
  XtSetArg (args[n], XmNleftOffset, 10);                             n++;

  Widget label = XmCreateLabel(form, (char *)item_label, args, n);
  XtManageChild(label);

  n = 0;
  XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);            n++;
  XtSetArg (args[n], XmNbottomOffset, 10);                           n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);              n++;
  XtSetArg (args[n], XmNleftOffset, 10);                             n++;
  XtSetArg (args[n], XmNdefaultButtonShadowThickness, 1);            n++;
  XtSetArg (args[n], XmNshowAsDefault, 2);                           n++;
  XtSetArg (args[n], XmNmarginWidth, 4);                             n++;
  XtSetArg (args[n], XmNmarginHeight, 4);                            n++;

  Widget ok_button = XmCreatePushButton(form, (char *)"OK", args, n);
  XtManageChild (ok_button);

  n = 0;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET);    n++;
  XtSetArg (args[n], XmNtopWidget, ok_button);                       n++;
  XtSetArg (args[n], XmNbottomOffset, 10);                           n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET);            n++;
  XtSetArg (args[n], XmNleftWidget, ok_button);                      n++;
  XtSetArg (args[n], XmNleftOffset, 10);                             n++;
  XtSetArg (args[n], XmNdefaultButtonShadowThickness, 1);            n++;
  XtSetArg (args[n], XmNmarginWidth, 4);                             n++;
  XtSetArg (args[n], XmNmarginHeight, 4);                            n++;

  Widget cancel_button = XmCreatePushButton(form, (char *)"Cancel", args, n);
  XtManageChild (cancel_button);

  {
    Dimension width;
    XtVaGetValues(cancel_button, XmNwidth, &width, NULL);
    XtVaSetValues(ok_button, XmNwidth, width, NULL);
  }

  n = 0;
  XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);          n++;
  XtSetArg (args[n], XmNbottomWidget, ok_button);                    n++;
  XtSetArg (args[n], XmNbottomOffset, 10);                           n++;
  XtSetArg (args[n], XmNleftOffset, 1);                              n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);              n++;
  XtSetArg (args[n], XmNrightOffset, 1);                             n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);             n++;

  Widget separator = XmCreateSeparator (form, (char *)"separator", args, n);
  XtManageChild (separator);

  // Create ScrolledText
  n = 0;
  XtSetArg (args[n], XmNrows, num_of_rows);                          n++;
  XtSetArg (args[n], XmNcolumns, num_of_columns);                    n++;
  XtSetArg (args[n], XmNeditable, True);                             n++;
  XtSetArg (args[n], XmNeditMode, XmMULTI_LINE_EDIT);                n++;
  XtSetArg (args[n], XmNcursorPositionVisible, True);                n++;
  XtSetArg (args[n], XmNtopWidget, label);                           n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);             n++;
  XtSetArg (args[n], XmNtopOffset, 4);                               n++;
  XtSetArg (args[n], XmNbottomWidget, separator);                    n++;
  XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);          n++;
  XtSetArg (args[n], XmNbottomOffset, 10);                           n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);              n++;
  XtSetArg (args[n], XmNleftOffset, 10);                             n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);             n++;
  XtSetArg (args[n], XmNrightOffset, 10);                            n++;

  Widget text_w = XmCreateScrolledText (form, (char *)"text", args, n);
  XtManageChild (text_w);

  XmTextSetString (text_w, (char *)item_value);

  XtAddCallback (ok_button, XmNactivateCallback, ask_text_CB, reinterpret_cast<XtPointer>(ask_text_dlg_ok));
  XtAddCallback (cancel_button, XmNactivateCallback, ask_text_CB, reinterpret_cast<XtPointer>(ask_text_dlg_cancel));
  OksXm::set_close_cb(dialog, ask_text_CB, reinterpret_cast<XtPointer>(ask_text_dlg_cancel));
  OksXm::set_close_cb(XtParent(parent), ask_text_CB, reinterpret_cast<XtPointer>(ask_text_dlg_parent_delete));

  XtManageChild(form);
  XtManageChild(dialog);

  XtRealizeWidget(dialog);
  XtPopup(dialog, XtGrabNone);

  done_with_ask_text_dlg = ask_text_dlg_no_answer;

  while(done_with_ask_text_dlg == ask_text_dlg_no_answer) {
    XtAppProcessEvent(XtWidgetToApplicationContext(form), XtIMAll);
  }

  std::string new_value;

  if (done_with_ask_text_dlg != ask_text_dlg_parent_delete)
    {
      OksXm::remove_close_cb(XtParent(parent), ask_text_CB, reinterpret_cast<XtPointer>(ask_text_dlg_parent_delete));
    }

  if (done_with_ask_text_dlg == ask_text_dlg_ok)
    {
      OksXm::AutoCstring v(XmTextGetString(text_w));
      new_value = v.get();
    }
  else
    {
      new_value = item_value;
    }

  XtDestroyWidget(dialog);

  return new_value;
}


Widget
OksXm::create_file_selection_box(Widget w, const char * title, const char * text, const char * directory, const char * mask, bool return_file, XtCallbackProc cf, XtPointer cf_data, XtCallbackProc hf, XtPointer hf_data)
{
  Arg args[6];
  int j = 0;
  Widget dialog;

  XmString xms_dir = 0;
  XmString xms_mask = 0;
  XmString xms_text = 0;
  unsigned char file_type = return_file ? XmFILE_REGULAR : XmFILE_DIRECTORY;

  XtSetArg(args[j], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL); j++;
  XtSetArg(args[j], XmNfileTypeMask, file_type); j++;
  XtSetArg(args[j], XmNpathMode, XmPATH_MODE_RELATIVE); j++;

  if(directory && *directory) {
    xms_dir = OksXm::create_string(directory);
    XtSetArg(args[j], XmNdirectory, xms_dir); j++;
  }

  if(mask && *mask) {
    xms_mask = OksXm::create_string(mask);
    XtSetArg(args[j], XmNdirMask, xms_mask); j++;
  }

  if(text && *text) {
    xms_text = OksXm::create_string(text);
    XtSetArg(args[j], XmNselectionLabelString, xms_text); j++;
  }

  dialog = XmCreateFileSelectionDialog(w, (char *)"file selection", args, j);

  if(title) {
    XtVaSetValues(XtParent(dialog), XmNtitle, title, NULL);
  }

  if(cf) {
    XtAddCallback (dialog, XmNokCallback, cf, cf_data);
    XtAddCallback (dialog, XmNcancelCallback, cf, cf_data);
  }

  if(hf) {
    XtAddCallback(dialog, XmNhelpCallback, hf, hf_data);
  }
  else {
    XtUnmanageChild(XmFileSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
  }

  if(!return_file) {
    XtUnmanageChild(XmFileSelectionBoxGetChild(dialog, XmDIALOG_LIST_LABEL));
    XtUnmanageChild(XtParent(XmFileSelectionBoxGetChild(dialog, XmDIALOG_LIST)));
    OksXm::TextField::set_not_editable(XmFileSelectionBoxGetChild(dialog, XmDIALOG_TEXT));
  }

  XtManageChild(dialog);

  if(xms_dir) XmStringFree(xms_dir);
  if(xms_mask) XmStringFree(xms_mask);
  if(xms_text) XmStringFree(xms_text);

  return dialog;
}


std::string
OksXm::ask_file(Widget w, const char * title, const char * text, const char * directory, const char * mask, bool return_file, XtCallbackProc hf, XtPointer hf_data)
{
  Widget dialog = create_file_selection_box(
    w, title, text, directory, mask, return_file, askCB, (XtPointer)1, hf, hf_data
  );

  ask_str = "";

  if(dialog) {
    ask_dlg_is_shown = true;

    while(ask_dlg_is_shown == true)
      XtAppProcessEvent(XtWidgetToApplicationContext(dialog), XtIMAll);

    XtDestroyWidget(XtParent(dialog));
  }

  return ask_str;
}

static bool info_dlg_is_shown;

static void
btnCB(Widget, XtPointer, XtPointer)
{
  info_dlg_is_shown = false;
}

void
OksXm::show_info(Widget parent, const char * title, const char ** image, const char * msg, const char * button)
{
  Widget dlg = OksXm::create_dlg_with_pixmap(
    parent, title, image, msg, button, 0, 0, 0, 0
  );

  XtVaSetValues(dlg, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

  XtAddCallback(dlg, XmNokCallback, btnCB, 0);
  OksXm::set_close_cb(XtParent(dlg), btnCB , 0);

  XtManageChild(dlg);

  info_dlg_is_shown = true;
  while(info_dlg_is_shown == true)
    XtAppProcessEvent(XtWidgetToApplicationContext(dlg), XtIMAll);

  XtDestroyWidget(XtParent(dlg));
}

///////////////////////////////////////////////////////////////////

static Widget tips_w = 0;
XtIntervalId timer = 0;


void 
OksXm::make_tips(Widget parent, const char * text)
{
  if(tips_w) {
    XtDestroyWidget(tips_w);
    tips_w = 0;
  }

  Arg temp_args[10];
  int i;

  int x = 0;
  int y = 0;
  
  {
    Dimension width, height;
    
    XtVaGetValues(parent, XmNwidth, &width, XmNheight, &height, NULL);
  
    Window wnd, root_window;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;

    XQueryPointer(XtDisplay(parent), XtWindow(parent), &root_window, &wnd,
          &root_x, &root_y, &win_x, &win_y, &mask);

    x = root_x - win_x + width / 2;
    y = root_y - win_y + height;
  }

  Pixel tips_pixel;
  XtVaGetValues(parent, XmNtopShadowColor, &tips_pixel, NULL);

  i = 0;
  XtSetArg(temp_args[i], XmNallowShellResize, True); i++;
  XtSetArg(temp_args[i], XmNborderWidth, 1); i++;
  XtSetArg(temp_args[i], XmNx, x); i++;
  XtSetArg(temp_args[i], XmNy, y); i++;
  XtSetArg(temp_args[i], XmNbackground, tips_pixel); i++;

  tips_w = XtCreatePopupShell("Notifier", overrideShellWidgetClass, parent, temp_args, i);

  i = 0;
  XtSetArg(temp_args[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
  XtSetArg(temp_args[i], XmNbackground, tips_pixel); i++; 
  XtCreateManagedWidget((char *)text,
    xmLabelWidgetClass,
    tips_w,
    temp_args, i);

  XtPopdown(tips_w);
  XtPopup(tips_w, XtGrabNone);
}


static void
show_label_timer_cb(XtPointer call_data, XtIntervalId*)
{
  timer = 0;

  OksXm::TipsInfo * ti = (OksXm::TipsInfo *)call_data;

  XmString xms;
  XtVaGetValues(ti->m_widget, XmNlabelString, &xms, NULL);
  OksXm::AutoCstring s(OksXm::string_to_c_str(xms));
  OksXm::make_tips(ti->m_widget, s.get());
}

static void
show_text_timer_cb(XtPointer call_data, XtIntervalId*)
{
  timer = 0;

  OksXm::TipsInfo * ti = (OksXm::TipsInfo *)call_data;

  OksXm::make_tips(ti->m_widget, ti->m_text);
}

static void
enterAC(Widget w, XtPointer data, XEvent *, Boolean *)
{
  if(timer) {
    XtRemoveTimeOut(timer);
  }

  OksXm::TipsInfo * i = (OksXm::TipsInfo *)(data);
  i->m_widget = w;

  timer = XtAppAddTimeOut(
    XtWidgetToApplicationContext(w), 1000, i->m_proc, data
  );
}

static void
leaveAC(Widget, XtPointer, XEvent *, Boolean *)
{
  if(tips_w) {
    XtDestroyWidget(tips_w);
    tips_w = 0;
  }

  if(timer) {
    XtRemoveTimeOut(timer);
    timer = 0;
  }
}

void
OksXm::show_tips(Widget w, TipsInfo * params)
{
  XtAddEventHandler(w, EnterWindowMask, False, enterAC, (XtPointer)params);
  XtAddEventHandler(w, LeaveWindowMask, False, leaveAC, 0);
}

OksXm::TipsInfo *
OksXm::show_label_as_tips(Widget w)
{
  OksXm::TipsInfo * params = new OksXm::TipsInfo(show_label_timer_cb, 0);
  show_tips(w, params);
  return params;
}

OksXm::TipsInfo *
OksXm::show_text_as_tips(Widget w, const char * text)
{
  char * s = new char[strlen(text) + 1];
  strcpy(s, text);
  OksXm::TipsInfo * params = new OksXm::TipsInfo(show_text_timer_cb, s);
  show_tips(w, params);
  return params;
}

Pixel
OksXm::string_to_pixel(Widget widget, const char *name)
{
  XrmValue from_value, to_value; /* For resource conversion */

  from_value.addr = const_cast<char *>(name);
  from_value.size = strlen( name ) + 1;
  if(from_value.size < sizeof(String)) from_value.size = sizeof(String);
  to_value.addr = 0;
  XtConvertAndStore (widget, XmRString, &from_value, XmRPixel, &to_value);

  if (to_value.addr) {
    return (*((Pixel*) to_value.addr)) ;
  }

  return XmUNSPECIFIED_PIXEL ;
}

  // max number of parents potentially having scrollbars to be checked,
  // when child receives scroll request from the mouse wheel

const unsigned int max_parents_num(3);


  // fill array of parents for given widget

static void get_parents(Widget w, Widget * parents)
{
  parents[0] = XtParent(w);
  for(unsigned int i = 1; i < max_parents_num; ++i) {
    if(Widget w2 = parents[i-1]) parents[i] = XtParent(w2);
    else parents[i] = 0;
  }
}

static void scroll_action(Widget w, XEvent *event, const char * action, const char * direction)
{
  Widget parents[max_parents_num]; get_parents(w, parents);
  String al[1]; al[0] = const_cast<char *>(direction);

  for(unsigned int i = 0; i < max_parents_num; ++i) {
    if(parents[i]) {
      if(Widget scrollBar = XtNameToWidget (parents[i], "VertScrollBar")) {
        XtCallActionProc(scrollBar, action, event, al, 1) ;
        return;
      }
      else {
        ERS_DEBUG(5, "skip non-scrollable widget[" << i << "]=" << (void *)parents[i]);
      }
    }
    else {
      ERS_DEBUG(5, "skip empty widget[" << i << ']');
    }
  }
}

static void pageUpAP(Widget w, XEvent *event, String *, Cardinal *)
{
  ERS_DEBUG(5, "call pageUpAP(" << (void *)w << ')');
  scroll_action(w, event, "PageUpOrLeft", "Up" );
}

static void pageDownAP(Widget w, XEvent *event, String *, Cardinal *)
{
  ERS_DEBUG(5, "call pageDownAP(" << (void *)w << ')');
  scroll_action(w, event, "PageDownOrRight", "Down" );
}

static void scrollUpAP(Widget w, XEvent *event, String *, Cardinal *)
{
  ERS_DEBUG(5, "call scrollUpAP(" << (void *)w << ')');
  scroll_action(w, event, "IncrementUpOrLeft", "Up" );
}

static void scrollDownAP(Widget w, XEvent *event, String *, Cardinal *)
{
  ERS_DEBUG(5, "call scrollDownAP(" << (void *)w << ')');
  scroll_action(w, event, "IncrementDownOrRight", "Down" );
}


/*
** Global installation of mouse wheel actions for scrolled windows.
*/
void OksXm::init_mouse_wheel_actions(XtAppContext context)
{   
  static XtActionsRec Actions[] = {
    {const_cast<char *>("scrolled-window-scroll-up"),   scrollUpAP},
    {const_cast<char *>("scrolled-window-page-up"),     pageUpAP},
    {const_cast<char *>("scrolled-window-scroll-down"), scrollDownAP},
    {const_cast<char *>("scrolled-window-page-down"),   pageDownAP} 
  };

  XtAppAddActions(context, Actions, XtNumber(Actions));
}

/*
** Add mouse wheel support to a specific widget, which must be the scrollable
** widget of a ScrolledWindow.
*/
void OksXm::add_mouse_wheel_action(Widget w)
{
  static XtTranslations trans_table = 0;
  static const char scrollTranslations[] =
    "Shift<Btn4Down>,<Btn4Up>: scrolled-window-scroll-up(1)\n"
    "Shift<Btn5Down>,<Btn5Up>: scrolled-window-scroll-down(1)\n"
    "Ctrl<Btn4Down>,<Btn4Up>:  scrolled-window-page-up()\n"
    "Ctrl<Btn5Down>,<Btn5Up>:  scrolled-window-page-down()\n"
    "<Btn4Down>,<Btn4Up>:      scrolled-window-scroll-up(3)\n"
    "<Btn5Down>,<Btn5Up>:      scrolled-window-scroll-down(3)\n";

  if(trans_table == 0) {
    trans_table = XtParseTranslationTable(scrollTranslations);
  }

  Widget parents[max_parents_num]; get_parents(w, parents);

  for(unsigned int i = 0; i < max_parents_num; ++i) {
    if(parents[i] && XmIsScrolledWindow(parents[i])) {
      ERS_DEBUG(5, "parent[" << i << "]: call XtOverrideTranslations(" << (void *)w << ')');
      XtOverrideTranslations(w, trans_table);
      return;
    }
    else {
      ERS_DEBUG(5, "skip widget[" << i << "] = " << (void *)parents[i]);
    }
  }
}

OksXm::ShowWatchCursor::ShowWatchCursor(Display * display, std::set<Widget>& widgets) : m_display(display), m_widgets(widgets)
{
  static Cursor watch_cursor = (Cursor) 0 ;

  if(watch_cursor == (Cursor) 0) {
    watch_cursor = XCreateFontCursor(m_display, XC_watch) ;
  }

  for(std::set<Widget>::const_iterator i = m_widgets.begin(); i != m_widgets.end(); ++i) {
    XDefineCursor(m_display, XtWindow(*i), watch_cursor) ;
  }

  XFlush(m_display);
}

OksXm::ShowWatchCursor::~ShowWatchCursor()
{
  for(std::set<Widget>::const_iterator i = m_widgets.begin(); i != m_widgets.end(); ++i) {
    if(XtIsWidget(*i)) XUndefineCursor (m_display, XtWindow(*i));
  }

  XFlush (m_display);
}

unsigned long OksXm::TipsText::m_max_width(0);

OksXm::TipsText::TipsText(char sep) : m_separator(sep)
{
  if(!m_max_width) {
    if(const char * env = getenv("OKS_GUI_TIPS_MAX_WIDTH")) {
      m_max_width = strtol(env, 0, 0);
    }

    if(!m_max_width) m_max_width = 80;

    ERS_DEBUG( 2 , "set m_max_width = " << m_max_width );
  }
}

std::string::size_type find_next_sep(const std::string& s, std::string::size_type cur, const char * sep = " \n\t")
{
  std::string::size_type len(s.size());

  for(std::string::size_type i = cur; i < len; ++i) {
    char c(s[i]);
    for(const char * p = sep; *p; p++) {
      if(c == *p) return i;
    }
  }

  return (len-1);
}

std::string OksXm::TipsText::text()
{
   // detect max width of title

  std::string::size_type max_title_width(0);

  for(std::list<Pair>::const_iterator i = m_data.begin(); i != m_data.end(); ++i) {
    std::string::size_type len((*i).title.size());
    if(len > max_title_width) max_title_width = len;
  }


    // if necessary, make tips wider (if title is too wide)

  std::string::size_type max_message_width(m_max_width);

  if(max_title_width*2 > max_message_width) {
    max_message_width = max_title_width*2;
    ERS_DEBUG( 2 , "set max_message_width = " << max_message_width );
  }


    // calculate title offset and make offset string

  const std::string::size_type max_value_width(max_message_width - max_title_width - 3);
  const std::string offset_str(max_title_width + 3, ' ');


    // prepare output string

  std::ostringstream s;
  bool first_line(true);

  for(std::list<Pair>::const_iterator i = m_data.begin(); i != m_data.end(); ++i) {
    if(first_line) { first_line = false; }
    else           { s << '\n';          }

    s.setf(std::ios::left, std::ios::adjustfield);
    s.width(max_title_width);
    s << (*i).title << ' ' << m_separator << ' ';

    const std::string v((*i).value);
    const std::string::size_type len(v.size());
    const std::string::size_type last_non_sep = v.find_last_not_of(" \n\t");

    std::string::size_type cur_pos(0);

    for(std::string::size_type j = 0; j < len; ++j) {
      char c(v[j]);

      if(c == '\n') {
          // only start new line, if there non-separator symbols after current postion
        if(last_non_sep == std::string::npos || last_non_sep > j) {
          s << c << offset_str;
          cur_pos = 0;
        }
      }
      else {
        cur_pos++;

        if(c == ' ' || c == '\t') {
          std::string::size_type next = find_next_sep(v, j);
          if((next - j + cur_pos) > max_value_width) {
            s << '\n' << offset_str;
            cur_pos = 0;
            continue;
          }
        }

        s << c;
      }
    }
  }

  s << std::ends;

  return s.str();
}


enum OverwriteAnswer { ConfirmOverwrite, CancelOverwrite, NoOverwriteAnswer } done_with_overwrite_dialog;

static void
cancelOverwriteCB(Widget, XtPointer, XtPointer)
{
  done_with_overwrite_dialog = CancelOverwrite;
}

static void
confirmOverwriteCB(Widget, XtPointer, XtPointer)
{
  done_with_overwrite_dialog = ConfirmOverwrite;
}

bool
OksXm::ask_overwrite_file(Widget parent, const char * file)
{
  struct stat buf;
  if(stat(file, &buf) != 0) {
    return true;
  }

  std::ostringstream text;
  text << "File \"" << file << "\" already exists.\nDo you want to overwrite it?";

  Widget w = OksXm::create_dlg_with_pixmap(parent, "Overwrite File", warning_xpm, text.str().c_str(), "Overwrite", "Cancel", 0, 0, 0);

  XtVaSetValues(w, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

  XtAddCallback(w, XmNokCallback, confirmOverwriteCB, 0);
  XtAddCallback(w, XmNcancelCallback, cancelOverwriteCB, 0);
  OksXm::set_close_cb(XtParent(w), cancelOverwriteCB, 0);

  XtManageChild(w);

  done_with_overwrite_dialog = NoOverwriteAnswer;

  while(done_with_overwrite_dialog == NoOverwriteAnswer)
    XtAppProcessEvent(XtWidgetToApplicationContext(w), XtIMAll);

  XtDestroyWidget(XtParent(w));

  return (done_with_overwrite_dialog == ConfirmOverwrite);
}

void
OksXm::destroy_modal_dialogs()
{
  ask_dlg_is_shown = false;
  info_dlg_is_shown = false;
  done_with_overwrite_dialog = CancelOverwrite;
  done_with_ask_text_dlg = ask_text_dlg_cancel;
}
