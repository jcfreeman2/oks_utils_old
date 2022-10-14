#include "g_object.h"
#include "g_window.h"

#include <Xm/ScrollBar.h>


  // timeout (in milli-seconds) between two autoscrolling calls
const unsigned short auto_scroll_timeout = 100;

  // minimal increment (in pixels) for autoscrolling
const unsigned short auto_scroll_min_increment = 4;

  // maximum increment (in parts of scroll window) for autoscrolling
const unsigned short auto_scroll_max_increment_part = 10;



void
G_Window::start_auto_scrolling(G_Window * window)
{
  destroy_auto_scrolling_timer();
  
  if(auto_scrolling_window == window) {
    create_auto_scrolling_timer(window);
  }
}


void
G_Window::stop_auto_scrolling()
{
  destroy_auto_scrolling_timer();
}


    //
    // that method performs auto-scrolling
    //

void
G_Window::make_auto_scrolling(Widget w, Dimension pos)
{
  if(w) {
    int value, slider_size, increment, page_increment, max;

    XtVaGetValues(w, XmNmaximum, &max, NULL);
    XmScrollBarGetValues(w, &value, &slider_size, &increment, &page_increment);

    Dimension max_pos = max - slider_size;
    Dimension old_value = value;
    
    short d_pos = (
      (pos < value) ? (pos - value) :
      (pos > (value + slider_size)) ? (pos - value - slider_size) :
      0
    );

    increment = (d_pos < 0) ? -d_pos : d_pos;

    Dimension auto_scroll_max_increment = slider_size / auto_scroll_max_increment_part;
    if(increment > auto_scroll_max_increment) increment = auto_scroll_max_increment;
    if(increment < auto_scroll_min_increment) increment = auto_scroll_min_increment;

    if(d_pos < 0 && value > 0) value -= (increment > value ? value : increment);
    if(d_pos > 0 && value < max_pos) value += ((increment + value) > max_pos ? (max_pos - value) : increment);

    if(value != old_value)
      XmScrollBarSetValues(w, value, slider_size, increment, page_increment, true);
  }
}


void
G_Window::autoScrollingCB(XtPointer call_data, XtIntervalId*)
{
  G_Window * window = (G_Window *)call_data;


    // find position of cursor

  int win_x, win_y;
  
  {
    Window wnd, root_window;
    int root_x, root_y;
    unsigned int mask;

    XQueryPointer(window->g_context.get_display(), window->g_context.get_xt_window(),
       &root_window, &wnd, &root_x, &root_y, &win_x, &win_y, &mask);
  }


    // get scrollbar widgets

  Widget v_sb_w = 0;
  Widget h_sb_w = 0;

  XtVaGetValues(
    XtParent(XtParent(window->get_form(idDrawingForm)->get_form_widget())),
    XmNverticalScrollBar, &v_sb_w,
    XmNhorizontalScrollBar, &h_sb_w,
    NULL
  );


    // perform autoscrolling by both axises

  make_auto_scrolling(v_sb_w, win_y);
  make_auto_scrolling(h_sb_w, win_x);


    // reset autoscrolling timer

  create_auto_scrolling_timer(window);
}


void
G_Window::create_auto_scrolling_timer(G_Window * window)
{
  auto_scrolling_timer = XtAppAddTimeOut(
    XtWidgetToApplicationContext(window->d_area),
    auto_scroll_timeout,
    G_Window::autoScrollingCB,
    (XtPointer)window
  );
}


void
G_Window::destroy_auto_scrolling_timer()
{
  if(auto_scrolling_timer) {
    XtRemoveTimeOut(auto_scrolling_timer);
    auto_scrolling_timer = 0;
  }
}

void
G_Object::stop_auto_scrolling()
{
  G_Window::stop_auto_scrolling();
}
