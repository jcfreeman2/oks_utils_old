#ifndef __SCHEMA_EDITOR_VIEW_DLG_H
#define __SCHEMA_EDITOR_VIEW_DLG_H

#include <vector>

#include <oks/xm_context.h>

#include "schema_editor_dialog.h"

class OksRelationship;
class OksSchemaEditorViewDialog;


struct OksViewSchema {
  OksClass * view_c;
  OksClass * class_c;
  OksClass * point_c;
  OksClass * glink_c;
  OksClass * alink_c;
};


class Item {

  public:

    enum DrawType {
      Select,
      Deselect,
      Deshape,
      Normal
    };

    enum Type {
      Class,
      Relationship,
      Comment
    };

    Item() : is_selected(false) {;}
    virtual ~Item() {;}
    virtual Type get_type() const = 0;
    virtual void draw(OksSchemaEditorViewDialog *, DrawType = Item::Normal) = 0;
    virtual void shift(short, short) = 0;
    virtual bool is_inside(short, short, unsigned short, unsigned short) const = 0;
    virtual bool does_select(short, short) const = 0;
    virtual void get_rect(short &, short &, unsigned short &, unsigned short &) const = 0;
    virtual void process_select_event(XButtonEvent *) = 0;
    virtual void process_deselect_event() = 0;
    virtual void save(OksViewSchema &) const = 0;
    virtual void print(std::ostream&) const = 0;


    virtual void make_selected(OksSchemaEditorViewDialog *, bool value) {is_selected = value;}


  protected:

    bool is_selected;

};

std::ostream& operator<<(std::ostream&, const Item&);


struct Rectangle {

  Rectangle(short x1 = 0, short y1 = 0, unsigned short w1 = 0,
            unsigned short h1 = 0, void * i = 0)
            : x(x1), y(y1), w(w1), h(h1), info(i) {;}

  void set(short x1 = 0, short y1 = 0, unsigned short w1 = 0,
           unsigned short h1 = 0, void * i = 0)
	   {x = x1; y = y1; w = w1; h = h1; info = i;}

  short x;
  short y;
  unsigned short w;
  unsigned short h;
  void * info;

};


class Class : public Item {

  public:

    enum Show {
      ShowAll,
      ShowDirect,
      ShowNone
    };

    virtual Type get_type() const {return Item::Class;}
    virtual void draw(OksSchemaEditorViewDialog *, DrawType = Item::Normal);
    virtual void shift(short dx, short dy) {x += dx; y += dy;}
    virtual bool is_inside(short, short, unsigned short, unsigned short) const;
    virtual bool does_select(short, short) const;
    virtual void get_rect(short &, short &, unsigned short &, unsigned short &) const;
    virtual void process_select_event(XButtonEvent *);
    virtual void process_deselect_event();
    virtual void save(OksViewSchema &) const;
    virtual void print(std::ostream&) const;

    Class(short, short, OksClass *);

    short get_x() const {return x;}
    short get_y() const {return y;}
    void set_x(short value) {x = value;}
    void set_y(short value) {y = value;}

    unsigned short get_width() const {return w;}
    unsigned short get_height() const {return h;}

    OksClass * get_oks_class() const {return p_oks_class;}

    virtual void make_selected(OksSchemaEditorViewDialog *, bool);


  private:

    short x;
    short y;
    unsigned short w;
    unsigned short h;

    OksClass * p_oks_class;

    Show p_attributes;
    Show p_methods;

    static std::string get_attribute_text(const OksAttribute *);

    std::vector<Rectangle> p_items;
    Rectangle * p_active_rectangle;

};

class Point {

  public:

    Point(short x1 = 0, short y1 = 0) : x (x1), y (y1), p_is_selected (false) {;}

    short get_x() const {return x;}
    short get_y() const {return y;}

    short set_x(short value) {return x = value;}
    short set_y(short value) {return y = value;}

    void set_selected(bool value) {p_is_selected = value;}
    bool get_selected() const {return p_is_selected;}

  private:

    short x, y;
    bool p_is_selected;

};

class Link;

class Handle {

  public:

    enum Type {
      Vertical   = 101,
      Horizontal = 102
    };

    Handle(short x1, short y1, Link * r1, Point * t1, Point * t2 = 0) : 
      x(x1), y(y1), r(r1) {set_points(t1, t2);}

    short get_x() const {return x;}
    short get_y() const {return y;}

    short get_min_x() const {return min_x;}
    short get_max_x() const {return max_x;}

    short get_min_y() const {return min_y;}
    short get_max_y() const {return max_y;}

    void set_x(short value) {x = value;}
    void set_y(short value) {y = value;}

    Point * get_point1() const {return p1;}
    Point * get_point2() const {return p2;}

    void set_points(Point *, Point *);

    Type get_type() const {return p_type;}

  private:

    short x, y;
    short min_x, max_x, min_y, max_y;
    Type p_type;
    Point * p1;
    Point * p2;
    Link * r;
};


class Link : public Item {

  public:

    enum LinkType {
      Generalization,
      Relationship
    };

    virtual Type get_type() const {return Item::Relationship;}
    virtual void draw(OksSchemaEditorViewDialog *, DrawType = Item::Normal);
    virtual void shift(short, short);
    virtual bool is_inside(short, short, unsigned short, unsigned short) const;
    virtual bool does_select(short, short) const;
    virtual void get_rect(short &, short &, unsigned short &, unsigned short &) const;
    virtual void process_select_event(XButtonEvent *);
    virtual void process_deselect_event();
    virtual void save(OksViewSchema &) const;
    virtual void print(std::ostream&) const;


    Link(const class Class *, const class Class *, OksRelationship * = 0);
    virtual ~Link() {;}

    OksRelationship * get_oks_relationship() const {return p_oks_relationship;}

    const class Class * get_class_from() const {return p_from;}
    const class Class * get_class_to() const {return p_to;}
    Point& get_point_from() {return *(p_segments.front());}
    Point& get_point_to() {return *(p_segments.back());}
    const std::list<Point *> & get_points() const {return p_segments;}
    const std::list<Handle *> & get_handles() const {return p_handles;}

    short get_x(const Point *) const;
    short get_y(const Point *) const;

    void set_multiplicity_text();
    const char * get_multiplicity_text() const {return p_multiplicity_text;}

    Handle * get_handle(short, short) const;
    void move_handle(short, short);

    bool does_reshape(short, short);
    bool get_reshaped() const {return p_is_reshaped;}
    void set_reshaped(bool value) {p_is_reshaped = value;}
    const Point * get_reshape_point() const {return p_active_point;}
    const Point * get_reshape_point_from() const {return &p_point_from;}
    const Point * get_reshape_point_to() const {return &p_point_to;}


  private:

    const LinkType p_link_type;
    OksRelationship * p_oks_relationship;
    const class Class * p_from;
    const class Class * p_to;
    const char * p_multiplicity_text;

    std::list<Point *> p_segments;
    std::list<Handle *> p_handles;

    Handle * p_active_handle;

    Point p_name;
    Point p_multiplicity;

    short rx1, rx2, ry1, ry2;  // drawing rectangle
    void adjust_rect(short, short, short, short);

    Point p_point_from;
    Point p_point_to;
    
    Point * p_active_point;
    
    bool p_is_reshaped;

};


class OksSchemaEditorViewDialog : public OksSchemaEditorDialog
{

  public:

    struct Schema {
      OksClass * class_c;
      OksClass * point_c;
      OksClass * glink_c;
      OksClass * alink_c;
    };

    OksSchemaEditorViewDialog(OksSchemaEditorMainDialog *);
    OksSchemaEditorViewDialog(const std::string&, OksSchemaEditorMainDialog *);
    ~OksSchemaEditorViewDialog();

    Type type() const {return OksSchemaEditorDialog::ViewDlg;}
    virtual void update() {};
    virtual void change_schema_file() {};
    void update_title() const;
    void update_class(const OksClass *, bool);

    const std::string& get_name() const {return p_name;}

    void draw(XExposeEvent *);

    Item * get_item(short, short);
    Widget get_remove_class_button() const {return get_widget(idPRemoveClass);}

    void read_data();
    int save(const std::string &) const;
    static int save_schema(const std::string &);

    static void refresh(XtPointer p) {((OksSchemaEditorViewDialog *)p)->draw(0);}

    OksGC& get_gc() {return gc;}

    unsigned short get_aggregation_sign_size() const {return p_aggregation_sign_size;}
    unsigned short get_generalization_sign_size() const {return  p_generalization_sign_size;}


  private:

    std::string p_name;
    std::string p_file;
    Widget p_drawing_area;
    long p_tool;
    std::list<Widget> p_tool_widgets;
    std::list<Item *> p_items;
    std::list<OksXm::TipsInfo *> p_label_tips;
    Item * p_selected_item;
    short p_selected_item_x, p_selected_item_y;

    bool p_looking4place;
    std::string p_add_class_name;

    unsigned short p_aggregation_sign_size;
    unsigned short p_generalization_sign_size;

    OksGC gc;

    void init();

    void add_tool_button(const char *, const char **, long, bool = false);
    void add_menu_button(const char *, const char **, long, Dimension = 0xFFFF, bool = false);
    void set_tool_state(Widget) const;

    static void define_schema(OksViewSchema &, OksKernel *);

    static void drawCB(Widget, XtPointer , XtPointer);
    static void buttonAC(Widget, XtPointer , XEvent *, Boolean *);
    static void inputCB(Widget, XtPointer , XtPointer);
    static void motionAC(Widget, XtPointer , XEvent *, Boolean *);

    static void toolCB(Widget, XtPointer , XtPointer);
    static void menuCB(Widget, XtPointer , XtPointer);
    static void putCB(Widget, XtPointer , XtPointer);

    void input(XButtonEvent *);
    void move(XMotionEvent *);

    void deselect();
    void deshape();

    void add(Class *);
    void remove_selected_class();

    enum {

      idDrawingForm = 100,

      idPSelect,
      idPMove,
      idPLink1,
      idPLink2,
      idPGLink,
      idPNewClass,
      idPAddClass,
      idPAddNote,
      idPRemoveClass,

      idPSave,
      idPSaveAs,
      idPRevertToSaved,
      idPRename,
      idPParameters,
      idPPrint

    };

};

#endif
