#ifndef _CONFDB_GUI_G_CLASS_
#define _CONFDB_GUI_G_CLASS_

#include <X11/Intrinsic.h>
#include <X11/xpm.h>

class OksClass;

#include <string>
#include <list>


class G_Class {
  friend class G_Object;
  friend class OksDataEditorMainDialog;

public:
  static Pixmap create_pixmap(const char **, unsigned int&, unsigned int&);

  class G_Map {
    public:

      G_Map              () : w (0), h (0) {;}
      G_Map              (unsigned int w1, unsigned int h1) : w(w1), h(h1) {;}

      unsigned int       get_width() const {return w;}
      unsigned int       get_height() const {return h;}


    protected:

      unsigned int       w;
      unsigned int       h;
  };

  class G_Bitmap : public G_Map {
    public:

      G_Bitmap           (const OksObject *, const char *, const char *);
      G_Bitmap           (unsigned char *s, unsigned int w1, unsigned int h1) : G_Map(w1, h1), bmp(s) {;}

      unsigned char *    get() const {return bmp;}


    private:

      unsigned char *    bmp;
  };

  class G_Pixmap : public G_Map {
    public:

      G_Pixmap           (const char **s) {pxm = G_Class::create_pixmap(s, w, h);}

      Pixmap	         get() const {return pxm;}


    private:

      Pixmap             pxm;
  };

  class EnvInitAttribute {
    public:

      EnvInitAttribute (const std::string& s) : p_name(s) {;}

      void add(const std::string& s) {p_env_vars.push_back(s);}

      const std::string & name() const {return p_name;}
      const std::list<std::string> & variables() const {return p_env_vars;}


    private:

      std::string p_name;
      std::list<std::string> p_env_vars;
  };

  class DualRelationship {
    public:

      DualRelationship (const std::string& s1, const std::string& s2) : p_direct(s1), p_reverse(s2) {;}

      const std::string& direct() const {return p_direct;}
      const std::string& reverse() const {return p_reverse;}


    private:

      std::string p_direct;
      std::string p_reverse;
  };

  G_Class		   (OksClass *, const char **, const char **, unsigned char *, unsigned int, unsigned int, unsigned char *, unsigned int, unsigned int);
  G_Class		   (const OksObject *, const std::string &, const char **, const char **, G_Bitmap, G_Bitmap);
  G_Class		   (const G_Class &);

  bool			   operator==(const G_Class& c) const
				{return c.oks_class == oks_class;}

  static void		   init(Widget);

  const std::string&	   get_name() const;
  OksClass *		   get_class() const {return oks_class;}

  Pixmap		   get_pixmap() const {return normal_pixmap.get();}
  unsigned int		   get_pixmap_width() const {return normal_pixmap.get_width();}
  unsigned int		   get_pixmap_height() const {return normal_pixmap.get_height();}

  Pixmap		   get_used_pixmap() const {return used_pixmap.get();}
  unsigned int		   get_used_pixmap_width() const {return used_pixmap.get_width();}
  unsigned int		   get_used_pixmap_height() const {return used_pixmap.get_height();}

  unsigned char	*          get_icon_bits() const {return icon_bitmap.get();}
  unsigned int             get_icon_width() const {return icon_bitmap.get_width();}
  unsigned int             get_icon_height() const {return icon_bitmap.get_height();}

  unsigned char	*          get_icon_mask_bits() const {return icon_mask_bitmap.get();}
  unsigned int             get_icon_mask_width() const {return icon_mask_bitmap.get_width();}
  unsigned int             get_icon_mask_height() const {return icon_mask_bitmap.get_height();}

  const std::list<std::string> * get_attributes() const {return &p_shown_attributes;}
  const std::list<std::string> * get_relationships() const {return &p_shown_relationships;}

  const std::string *	   get_reverse_relationship(const std::string&) const;

  const std::list<std::string> * get_remove_from_used() const {return &p_remove_from_used;}

  const std::list<std::string> * get_env_init_attr(const std::string&) const;

  const std::string&	   get_icon_title() const {return p_icon_title;}

  void		           find_root_objects(const std::string &, std::set<OksObject *>&) const;


private:
  std::string		   p_name;
  OksClass *		   oks_class;
  const OksObject *        p_obj; // ref. to object from which the class was created

  bool			   p_show_all_attributes;
  std::list<std::string>   p_attributes;
  std::list<std::string>   p_shown_attributes;

  bool			   p_show_all_relationships;
  std::list<std::string>   p_relationships;
  std::list<std::string>   p_shown_relationships;

  std::list<std::string>   p_remove_from_used;

  std::list<DualRelationship *> p_dual_relationships;
  std::list<EnvInitAttribute *> p_attributes_init_by_env;

  std::string		   p_icon_title;

  const char **		   p_normal_image;
  const char **		   p_used_image;
  const char **		   p_icon_image;
  const char **		   p_icon_mask_image;

  G_Pixmap		   normal_pixmap;
  G_Pixmap		   used_pixmap;

  G_Bitmap		   icon_bitmap;
  G_Bitmap		   icon_mask_bitmap;

  static Widget		   parent_w;			// pointer to the top-level form
  static XpmColorSymbol	   xpm_color_symbols[4];	// the xmp colours defined once

  void			   show_all_attributes() {p_show_all_attributes = true;}
  void			   hide_all_attributes() {p_show_all_attributes = false;}
  void			   show_attribute(const std::string &);
  void			   hide_attribute(const std::string &);

  void			   show_all_relationships() {p_show_all_relationships = true;}
  void			   hide_all_relationships() {p_show_all_relationships = false;}
  void			   show_relationship(const std::string &);
  void			   hide_relationship(const std::string &);

  void			   set_oks_class(OksClass * oc) {oks_class = oc;}
};

#endif
