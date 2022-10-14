#include "data_editor_exceptions.h"

#include <X11/xpm.h>
#include <Xm/Xm.h>

#include <memory>

#include <oks/class.h>

#include "g_class.h"


Widget		G_Class::parent_w;
XpmColorSymbol	G_Class::xpm_color_symbols[4];

const std::string&
G_Class::get_name() const
{
  if(oks_class) return oks_class->get_name();
  else return p_name;
}

void
G_Class::init(Widget w)
{
  parent_w = w;

  xpm_color_symbols[0].name = (char *)"BottomShadow";
  xpm_color_symbols[1].name = (char *)"TopShadow";
  xpm_color_symbols[2].name = (char *)"Background";
  xpm_color_symbols[3].name = (char *)"Foreground";

  xpm_color_symbols[0].value = 0;
  xpm_color_symbols[1].value = 0;
  xpm_color_symbols[2].value = 0;
  xpm_color_symbols[3].value = 0;

  XtVaGetValues(w,
    XmNbottomShadowColor, &xpm_color_symbols[0].pixel,
    XmNtopShadowColor,    &xpm_color_symbols[1].pixel,
    XmNbackground,        &xpm_color_symbols[2].pixel,
    XmNforeground,        &xpm_color_symbols[3].pixel,
    NULL
  );
}

Pixmap
G_Class::create_pixmap(const char **image_string, unsigned int& pw, unsigned int& ph)
{
  Pixmap pixmap = 0;

  if(image_string) {
    XpmAttributes xpm_attributes;

    xpm_attributes.colorsymbols = xpm_color_symbols;
    xpm_attributes.numsymbols = sizeof(xpm_color_symbols) / sizeof(XpmColorSymbol);
    xpm_attributes.valuemask = XpmColorSymbols;

    int status = XpmCreatePixmapFromData(
      XtDisplay(parent_w),
      DefaultRootWindow(XtDisplay(parent_w)),
      (char **)image_string,
      &pixmap,
      0,
      &xpm_attributes
    );

    if(status != XpmSuccess) {
      std::ostringstream text;
      text << "cannot create pixmap: " << XpmGetErrorString(status);
      ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    }
    else {
      ph = xpm_attributes.height;
      pw = xpm_attributes.width;
    }
  }
  else
    pw = ph = 0;

  return pixmap;
}

G_Class::G_Class(OksClass *oc, const char **normal_image, const char **used_image,
                 unsigned char *icon_bits, unsigned int icon_w, unsigned int icon_h,
		 unsigned char *mask_bits, unsigned int mask_w, unsigned int mask_h) :

  oks_class	     (oc),
  p_obj              (0),
  p_normal_image     (normal_image),
  p_used_image       (used_image),
  normal_pixmap	     (normal_image),
  used_pixmap        (used_image),
  icon_bitmap	     (icon_bits, icon_w, icon_h),
  icon_mask_bitmap   (mask_bits, mask_w, mask_h)
{
}

G_Class::G_Class(const OksObject *obj, const std::string & name, const char **normal_image, const char **used_image,
                 G_Bitmap ib, G_Bitmap imb) :

  p_name	     (name),
  oks_class	     (0),
  p_obj              (obj),
  p_normal_image     (normal_image),
  p_used_image       (used_image),
  normal_pixmap	     (normal_image),
  used_pixmap        (used_image),
  icon_bitmap	     (ib),
  icon_mask_bitmap   (imb)
{
}

G_Class::G_Class(const G_Class &c) :
  oks_class	     (c.oks_class),
  p_obj              (c.p_obj),
  p_normal_image     (c.p_normal_image),
  p_used_image       (c.p_used_image),
  normal_pixmap	     (c.normal_pixmap),
  used_pixmap	     (c.used_pixmap),
  icon_bitmap	     (c.icon_bitmap),
  icon_mask_bitmap   (c.icon_mask_bitmap)
{
}

void
G_Class::show_attribute(const std::string & attr_name)
{
  if(p_show_all_attributes == false)
    p_attributes.push_back(attr_name);
}

void
G_Class::show_relationship(const std::string & rel_name)
{
  if(p_show_all_relationships == false)
    p_relationships.push_back(rel_name);
}

void
G_Class::hide_attribute(const std::string & attr_name)
{
  if(p_show_all_attributes == true)
    p_attributes.push_back(attr_name);
}

void
G_Class::hide_relationship(const std::string & rel_name)
{
  if(p_show_all_relationships == true)
    p_relationships.push_back(rel_name);
}

const std::string *
G_Class::get_reverse_relationship(const std::string& direct_name) const
{
  if(!p_dual_relationships.empty()) {
    for(std::list<DualRelationship *>::const_iterator i = p_dual_relationships.begin();i != p_dual_relationships.end();++i) {
      const DualRelationship * dr = *i;
      if(dr->direct() == direct_name) return &dr->reverse();
    }
  }

  return 0;
}


const std::list<std::string> *
G_Class::get_env_init_attr(const std::string& attr_name) const
{
  for(std::list<EnvInitAttribute *>::const_iterator i = p_attributes_init_by_env.begin(); i != p_attributes_init_by_env.end(); ++i) {
    const EnvInitAttribute * a = *i;
    if(a->name() == attr_name) return &a->variables();
  }

  return 0;
}


void
G_Class::find_root_objects(const std::string & name, std::set<OksObject *>& root_objects) const
{
  ERS_DEBUG(3, "ENTER find_root_objects(rel-name: \'" << name << "\') of class \'" << oks_class->get_name() << '\'');

  root_objects.clear();

  std::set<OksObject *, std::less<OksObject *> > table;

    // find all instances of objects described by graphical class

  {
    std::unique_ptr< std::list<OksObject *> > objs (oks_class->create_list_of_all_objects());


      // return, if there are no root objects

    if(objs.get() == 0) return;

    else if(objs->empty()) {
      return;
    }

      // return the object if there is only one

    else if(objs->size() == 1) {
      root_objects.insert(*(objs->begin()));
      return;
    }

      // create table of objects

    std::list<OksObject *>::const_iterator i;

    for(i = objs->begin(); i != objs->end(); ++i) {
      table.insert(*i);
    }


      // remove from table any referenced object

    for(i = objs->begin(); i != objs->end(); ++i) {
      OksObject * o = *i;

      if(name.empty()) {
        OksObject::FSet refs;
        o->references(refs, 1000);
        for(OksObject::FSet::const_iterator j = refs.begin(); j != refs.end(); ++j) {
          ERS_DEBUG(3, " - " << *j);
          table.erase(const_cast<OksObject *>(*j));
        }
      }
      else {
        try {
          OksData * d(o->GetAttributeValue(name));
          if(OksData::List * olist = d->data.LIST) {
            for(OksData::List::const_iterator j = olist->begin(); j != olist->end(); ++j) {
              ERS_DEBUG(3, " - " << (*j)->data.OBJECT);
              table.erase((*j)->data.OBJECT);
            }
          }
        }
        catch(oks::exception & ex) {
          ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
        }
      }
    }
  }

  if(table.empty()) {
    std::ostringstream text;
    text << "cannot find a root object for class \'" << get_name() << "\' (via relationship \'" << name << "\')";
    ers::warning(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
  }
  else {
    for(std::set<OksObject *, std::less<OksObject *> >::iterator i =  table.begin(); i != table.end(); ++i) {
      ERS_DEBUG(3, " + " << *i);
      root_objects.insert(*i);
    }
  }
}
