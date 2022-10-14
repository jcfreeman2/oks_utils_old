#include <ers/ers.h>

#include <stdlib.h>

#include <fstream>

#include <oks/class.h>
#include <oks/object.h>
#include <oks/kernel.h>

#include "g_class.h"
#include "g_window_info.h"
#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"

static OksClass *
find_class_and_report(const char * name, const OksKernel& kernel)
{
  OksClass * oks_class = kernel.find_class(name);

  if(!oks_class) {
    std::ostringstream text;
    text << "cannot find class \'" << name << '\'';
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    return 0;
  }

  if(oks_class->number_of_objects() == 0) {
    std::ostringstream text;
    text << "there are no instances of \'" << name << '\'';
    ers::warning(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    return 0;
  }
  
  return oks_class;
}


static OksData *
read_attribute(const OksObject * o, const char * name)
{
  try {
    return o->GetAttributeValue(name);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
    return 0;
  }
}

static OksData *
read_relationship(const OksObject * o, const char * name)
{
  try {
    return o->GetRelationshipValue(name);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
    return 0;
  }
}


static std::string
attr_to_file_name(const OksObject *o, const char * attr_name, const char * base_dirs)
{
  std::string s;

  OksData * d = read_attribute(o, attr_name);

  if(d) {
    s = *d->data.STRING;

    if(s.empty()) return s;
 
       // test if attribute points file with relative name

    std::ifstream f(s.c_str());
    if(!f.good() && base_dirs && *base_dirs != 0) {
      Oks::Tokenizer t(base_dirs, ":");
      std::string token;

        // try with base_dir token

      while(!(token = t.next()).empty()) {
        token += '/';
	token += s;

        std::ifstream f2(token.c_str());
        if(f2.good()) {
          return token;
	}
      }
    }
  }

  return s;
}


G_Class::G_Bitmap::G_Bitmap(const OksObject *o, const char * attr_name, const char * base_dirs)
{
  std::string s = attr_to_file_name(o, attr_name, base_dirs);

  if(s.empty()) {
    std::ostringstream text;
    text << "cannot create \'" << attr_name << "\' for \'" << o->GetId() << '\'';
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    return;
  }
  else {
    if(OksDataEditorMainDialog::is_verbose()) {
      std::cout << "  - read bitmap file \'" << s << "\'\n";
    }
  }

  int dummu1, dummu2;

  int status = XReadBitmapFileData(
    const_cast<char *>(s.c_str()),
    &w, &h, &bmp,
    &dummu1, &dummu2
  );

  if(status != BitmapSuccess) {
    std::ostringstream text;
    text << "cannot create bitmap from file \"" << s << "\"\nXReadBitmapFileData() failed with status " << status << ": "
         << (
              (status == BitmapOpenFailed)  ? "Can not open file" :
              (status == BitmapFileInvalid) ? "File contains invalid bitmap data" :
              (status == BitmapNoMemory)    ? "Not enough memory" :
                                              "Unknown error"
            );
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    return;
  }
}


static char **
get_pixmap_image(const OksObject * o, const char * attr_name, const char * base_dirs)
{
  std::string s = attr_to_file_name(o, attr_name, base_dirs);

  if(s.empty()) {
    if(OksDataEditorMainDialog::is_verbose()) {
      std::cout << "  - value \'" << attr_name << "\' for \'" << o->GetId() << "\' is not defined\n";
    }

    return 0;
  }

  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << "  - read pixmap file \'" << s << "\'\n";
  }

  char ** data_return = 0;
  int status = XpmReadFileToData(const_cast<char *>(s.c_str()), &data_return);
  
  if(status != XpmSuccess) {
    std::ostringstream text;
    text << "cannot create pixmap from file \"" << s << "\"\nXpmReadFileToData() failed with status " << status << ": " << XpmGetErrorString(status);
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    return 0;
  }
  
  return data_return;
}


G_Class *
OksDataEditorMainDialog::get_class(const OksObject * oks_object) const
{
  for(std::list<G_Class *>::const_iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
    if((*i)->p_obj == oks_object) return *i;
  }

  return 0;
}


int
OksDataEditorMainDialog::init_window_info(const OksObject * oks_obj, const OksKernel& gk)
{
  if(is_verbose()) {
    std::cout << " # process window \'" << oks_obj->GetId() << "\'\n";
  }

  const OksClass * window_class_oks_class = gk.find_class("Window Class");
  const OksClass * separator_oks_class = gk.find_class("Window Separator");

  OksData * d = read_attribute(oks_obj, "Title");
  
  G_WindowInfo * w_info = new G_WindowInfo(*d->data.STRING);
  
  p_win_infos.push_back(w_info);
  
  if((d = read_relationship(oks_obj, "Items")) != 0) {
    OksData::List * ilist = d->data.LIST;
    if(ilist && !ilist->empty()) {
      for(OksData::List::iterator j = ilist->begin(); j != ilist->end(); ++j) {
	const OksObject *o = ((*j)->type == OksData::object_type ? (*j)->data.OBJECT : 0);

	if(o) {
	  if(o->GetClass() == window_class_oks_class) {

              // find graphical class first

	    G_Class * g_class = 0;

	    if((d = read_relationship(o, "Graphical Class")) != 0) {
	      if(d->type == OksData::object_type && d->data.OBJECT) {
		g_class = get_class(d->data.OBJECT);
	      }
	    }

	    if(!g_class) return 10;

            const std::string& s = *(read_attribute(o, "Show on top level"))->data.STRING;
            const std::string& s2 = *(read_attribute(o, "Shown with children"))->data.STRING;

	    std::string tname(*(read_attribute(o, "Top Level Name"))->data.STRING);


               // replace "\n" by real <CR> symbol
            {
              const char * delim("\\n");

	      std::string::size_type idx = tname.find(delim);

              while(idx != std::string::npos) {
	        tname.erase(idx, 1);
	        tname[idx] = '\n';
	        idx = tname.find(delim);
	      }
	    }

	    w_info->p_items.push_back(
	      new G_WindowInfo::Class(
	        g_class,
		tname,
		(
		  (s == "with-used-menu") ? G_WindowInfo::Class::ShowUsedMenu :
		  (s == "without-used-menu") ? G_WindowInfo::Class::ShowWithoutUsedMenu :
		  G_WindowInfo::Class::DoNotShow
		),
		(read_attribute(o, "Use to create top-level objects"))->data.BOOL,
		(read_attribute(o, "Show in new"))->data.BOOL,
		(
		  (s2 == "none") ? G_WindowInfo::Class::NoneObj :
		  (s2 == "any") ? G_WindowInfo::Class::AnyObj :
		  G_WindowInfo::Class::RootObj
		),
		*(read_attribute(o, "Root relationship name"))->data.STRING
	      )
	    );
	  }
	  else if(o->GetClass() == separator_oks_class) {
	    const std::string& s = *(read_attribute(o, "Break At"))->data.STRING;
	    
	    w_info->p_items.push_back(
	      new G_WindowInfo::Separator(
	        (s == "Top-level") ? G_WindowInfo::Separator::TopLevel :
	        (s == "New-object") ? G_WindowInfo::Separator::NewObject :
		G_WindowInfo::Separator::BothPlaces
	      )
	    );
	  }
	  else {
            std::ostringstream text;
            text << "window \'" << w_info->title() << "\' contains object " << o << ";\nonly \'Window Class\' and \'Window Separator\' classes are supported";
            ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
	  }
	}
      }
    }
  }
  else
    return 4;

  return 0;
}


G_Class *
OksDataEditorMainDialog::init_g_class(const OksObject * oks_obj, const char * pixmap_base_dirs, const char * bitmap_base_dirs)
{
  if(is_verbose()) {
    std::cout << " + process class \'" << oks_obj->GetId() << "\'\n";
  }

  OksData * d = read_attribute(oks_obj, "Name");

  const std::string & class_name = *d->data.STRING;

  G_Class * g_class = new G_Class(
    oks_obj,
    class_name,
    (const char **)get_pixmap_image(oks_obj, "Generic Pixmap File", pixmap_base_dirs),
    (const char **)get_pixmap_image(oks_obj, "Used Pixmap File", pixmap_base_dirs),
    G_Class::G_Bitmap(oks_obj, "Icon Bitmap File", bitmap_base_dirs),
    G_Class::G_Bitmap(oks_obj, "Icon Mask Bitmap File", bitmap_base_dirs)
  );

  p_classes.push_back(g_class);


  if((d = read_attribute(oks_obj, "Show All Attributes")) != 0) {
    if(d->data.BOOL) g_class->show_all_attributes();
    else g_class->hide_all_attributes();
  }
  else
    return 0;

  if((d = read_attribute(oks_obj, "Show All Relationships")) != 0) {
    if(d->data.BOOL) g_class->show_all_relationships();
    else g_class->hide_all_relationships();
  }
  else
    return 0;

  if((d = read_attribute(oks_obj, "Attributes")) != 0) {
    OksData::List * alist = d->data.LIST;
    if(alist && !alist->empty()) {
      for(OksData::List::iterator j = alist->begin(); j != alist->end(); ++j) {
        const std::string & attr_name = *((*j)->data.STRING);
        if(g_class->p_show_all_attributes) g_class->hide_attribute(attr_name);
        else g_class->show_attribute(attr_name);
      }
    }
  }
  else
     return 0;

  if((d = read_attribute(oks_obj, "Relationships")) != 0) {
    OksData::List * alist = d->data.LIST;
    if(alist && !alist->empty()) {
      for(OksData::List::iterator j = alist->begin(); j != alist->end(); ++j) {
	const std::string & rel_name = *((*j)->data.STRING);
	if(g_class->p_show_all_relationships) g_class->hide_relationship(rel_name);
	else g_class->show_relationship(rel_name);
      }
    }
  }
  else
    return 0;

  if((d = read_attribute(oks_obj, "Remove From Used")) != 0) {
    OksData::List * alist = d->data.LIST;
    if(alist && !alist->empty()) {
      for(OksData::List::iterator j = alist->begin(); j != alist->end(); ++j) {
	g_class->p_remove_from_used.push_back(*((*j)->data.STRING));
      }
    }
  }
  else
    return 0;

  if((d = read_attribute(oks_obj, "Icon Title")) != 0)
    g_class->p_icon_title = *d->data.STRING;
  else
    return 0;

  if((d = read_relationship(oks_obj, "Initialize Attributes")) != 0) {
    OksData::List * rlist = d->data.LIST;
    if(rlist && !rlist->empty()) {
      for(OksData::List::iterator j = rlist->begin(); j != rlist->end(); ++j) {
	const OksObject *o = ((*j)->type == OksData::object_type ? (*j)->data.OBJECT : 0);

	if(o) {
	  G_Class::EnvInitAttribute * env_attr = new G_Class::EnvInitAttribute(*(read_attribute(o, "Attribute Name"))->data.STRING);

          OksData::List * vlist = read_attribute(o, "Environment Variables")->data.LIST;

	  if(vlist && !vlist->empty()) {
	    g_class->p_attributes_init_by_env.push_back(env_attr);

	    for(OksData::List::iterator v = vlist->begin(); v != vlist->end(); ++v) {
	      env_attr->add(*((*v)->data.STRING));
	    }
	  }
	  else {
	    delete env_attr;
	  }
	}
      }
    }
  }
  else
    return 0;


  if((d = read_relationship(oks_obj, "Dual Relationships")) != 0) {
    OksData::List * rlist = d->data.LIST;
    if(rlist && !rlist->empty()) {
      for(OksData::List::iterator j = rlist->begin(); j != rlist->end(); ++j) {
	const OksObject *o = ((*j)->type == OksData::object_type ? (*j)->data.OBJECT : 0);

	if(o) {
	  g_class->p_dual_relationships.push_back(
	    new G_Class::DualRelationship(
	      *(read_attribute(o, "Direct"))->data.STRING,
	      *(read_attribute(o, "Reverse"))->data.STRING
	    )
	  );
	}
      }
    }
  }
  else
    return 0;
  
  return g_class;
}


int
OksDataEditorMainDialog::init_g_classes()
{
  if(is_verbose()) {
    std::cout << "Enter OksDataEditorMainDialog::init_g_classes()\n"
	         " - data=\'" << p_init_data_files << "\'\n";
  }

  if(p_init_data_files.empty()) {
    if(is_verbose()) {
      ers::error(OksDataEditor::Problem(ERS_HERE, "no init-data-file parameter defined"));
    }

    return 2;
  }

  OksKernel gk(false, false, false, false);

  {
    Oks::Tokenizer t(p_init_path, ":");
    std::string token;

    while(!(token = t.next()).empty()) {
      gk.insert_repository_dir(token, false);
    }
  }

  gk.set_silence_mode(!is_verbose());


    // load colon-separated list of data files

  try {
    Oks::Tokenizer t(p_init_data_files, ":");
    std::string token;

    while(!(token = t.next()).empty()) {
      gk.load_data(token);
    }
  }
  catch (oks::exception & ex) {
    ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
    return 2;
  }


    // initialize main window

  OksObject * root_g_class_oks_obj = 0;

  {
    if(OksClass * oks_class = find_class_and_report("Main", gk)) {
      const OksObject::Map * oks_objs = oks_class->objects();
      
      if(oks_objs->empty()) {
        ers::error(OksDataEditor::Problem(ERS_HERE, "cannot find any instance of \'Main\' class"));
	return 7;
      }
      else if(oks_objs->size() != 1) {
        ers::error(OksDataEditor::Problem(ERS_HERE, "there are too many instances of \'Main\' class"));
	return 8;
      }


      const OksObject * oks_obj = (*oks_objs->begin()).second;

      p_show_used = (
        (*(read_attribute(oks_obj, "Used objects"))->data.STRING) == "Show-as-used" ? true : false
      );
      
      p_root_relationship = *(read_attribute(oks_obj, "Root relationship"))->data.STRING;

      OksData *d = read_relationship(oks_obj, "Root class");
      
      if(d && d->type == OksData::object_type && d->data.OBJECT) {
        root_g_class_oks_obj = d->data.OBJECT;
        ERS_DEBUG(1, "Set root graphical class object: " << root_g_class_oks_obj);
      }
      else {
        std::ostringstream text;
        text << "object " << oks_obj << " needs \'Root class\' relationship to be set";
        ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
	return 8;
      }
    }
    else
      return 6;
  }


    // initialize graphical classes

  {
    if(is_verbose()) {
      std::cout << "* process graphical classes ...\n";
    }
    
    OksClass * oks_class = find_class_and_report("Class", gk);

    if(oks_class) {
      const OksObject::Map * oks_objs = oks_class->objects();

      for(OksObject::Map::const_iterator i = oks_objs->begin(); i != oks_objs->end(); ++i) {
        G_Class * gc = init_g_class((*i).second, p_pixmaps_dirs.c_str(), p_bitmaps_dirs.c_str());
        if(gc == 0) return 4;
	if(root_g_class_oks_obj == (*i).second) {
          p_root_class = gc;
          ERS_DEBUG(1, "Set root graphical class: " << p_root_class->get_name());
        }
      }
    }
    else
      return 5;
  }


    // initialize windows information

  {
    if(is_verbose()) {
      std::cout << "* process windows information ...\n";
    }

    OksClass * oks_class = find_class_and_report("Window", gk);

    if(oks_class) {
      const OksObject::Map * oks_objs = oks_class->objects();

      for(OksObject::Map::const_iterator i = oks_objs->begin(); i != oks_objs->end(); ++i) {
        int status = init_window_info((*i).second, gk);
        if(status != 0) return status;
      }
    }
    else
      return 6;
  }

  return 0;
}
