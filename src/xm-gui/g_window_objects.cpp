#include <oks/class.h>
#include <oks/object.h>

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "g_window.h"
#include "g_class.h"
#include "g_object.h"

std::list<OksClass *> *
G_Window::get_new_object_types(OksClass * oks_class) const
{
  std::list<OksClass *> * the_list = 0;

  const OksClass::FList * s_clist = oks_class->all_sub_classes();

  if(s_clist) {
    for(OksClass::FList::const_iterator i = s_clist->begin(); i != s_clist->end(); ++i) {
      OksClass * c = *i;

      if(c->get_is_abstract() == false) {
        if(!the_list) the_list = new std::list<OksClass *>;
	if(the_list) the_list->push_back(c);
      }
    }

    if(the_list && oks_class->get_is_abstract() == false) the_list->push_front(oks_class);
  }

  return the_list;
}


std::list<OksClass *> *
G_Window::get_new_object_types(G_Class *g_class) const
{
  return get_new_object_types(g_class->get_class());
}

G_Object *
G_Window::create_top_level_object(OksObject * o)
{
  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << "call create_top_level_object(" << o << ") for window \'" << get_title() << '\'' << std::endl;
  }

  const OksClass * c = o->GetClass();
  const G_Class * g_class = 0;

  const OksClass::FList * super_classes = c->all_super_classes();
  OksClass::FList::const_iterator i;
  if(super_classes) i = super_classes->begin();


    // search in top classes

  do {
    std::list<const G_Class *> * w_classes[2] = { &top_classes, &other_classes };

    for(int l = 0; ((l < 2) && (g_class == 0)); ++l) {
      for(std::list<const G_Class *>::const_iterator j = w_classes[l]->begin(); j != w_classes[l]->end(); ++j) {
        if(OksDataEditorMainDialog::is_verbose()) {
          std::cout << " - test graphical class \'" << (*j)->get_class()->get_name() << "\' [" << l << ']' << std::endl;
	}

        if((*j)->get_class() == c) {
          g_class = *j;
	  break;
        }
      }
    }
  } while (g_class == 0 && super_classes && i != super_classes->end() && (c = *(i++)));


    // 1. check if the object is a top-level for this window
    // 2. create new object
    // 3. insert new object

  if(OksDataEditorMainDialog::is_verbose()) {
    if(g_class) {
      std::cout << " # create graphical object " << o << " using class \'" << g_class->get_class()->get_name() << '\'' << std::endl;
    }
    else {
      std::cout << " # found no suitable graphical classes for object " << o << std::endl;
    }
  }

  if(g_class) {
    if(G_Object * obj = create_new_object(o, true)) {
      insert_new_object(obj);
      return obj;
    }
  }

  return 0;
}


void
G_Window::insert_new_object(G_Object *obj)
{
  {
    bool was_inserted = false;
      
    for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
      if((*i)->get_graphic_class() == obj->get_graphic_class()) {
        objects.insert(i, obj);
	was_inserted = true;
	break;
      }
    }


      // if the object was not inserted, append it to the end
      
    if(was_inserted == false) {
      objects.push_back(obj);
    }
  }

  objs_set.insert(obj);


    // make necessary object initialisation

  obj->set_children(&objects, &objs_set);
  obj->init_new();	// depends on object type
  init_new(obj);	// depends on object type
}


G_Object *
G_Window::create_new_object(OksObject *oks_obj, bool use_propagate)
{
  const OksClass * oks_obj_class = oks_obj->GetClass();
  G_Kid::Propagate kp = use_propagate ? G_Kid::use_propagate : G_Kid::stop_propagate;

  const OksClass::FList * super_classes = oks_obj_class->all_super_classes();
  OksClass::FList::const_iterator i;
  if(super_classes) i = super_classes->begin();

  const std::list<G_Class *> & classes = parent->get_classes();

  const OksClass * c = oks_obj_class;
  const G_Class * g_class = 0;


      // check top classes first

  do {
    for(std::list<const G_Class *>::const_iterator j = top_classes.begin(); j != top_classes.end(); ++j) {
      if((*j)->get_class() == c) {
        g_class = *j;
	break;
      }
    }
  } while (g_class == 0 && super_classes && i != super_classes->end() && (c = *(i++)));


      // otherwise check all defined classes

  if(!g_class) {
    c = oks_obj_class;
    if(super_classes) i = super_classes->begin();

    do {
      for(std::list<G_Class *>::const_iterator j = classes.begin(); j != classes.end(); ++j) {
        if((*j)->get_class() == c) {
	  g_class = *j;
	  break;
	}
      }
    } while (super_classes && i != super_classes->end() && (c = *(i++)));
  }


      // if found graphical class

  if(g_class) {
    G_Object * o = new G_Object(oks_obj, &g_context, this, g_class, 0, 0, kp);

    if(OksDataEditorMainDialog::is_verbose()) {
      std::cout << " - create new object " << oks_obj << " (use propagate: " << use_propagate
	        << ", ptr = " << (void *)o << ") using \'" << g_class->get_name() << "\' graphical class\n";
    }

    return o;
  }


      // othrewise use something

  g_class = parent->get_default_class();// *classes.begin();

  G_Object * o = new G_Object(oks_obj, &g_context, this, g_class, 0, 0, kp);

  if(OksDataEditorMainDialog::is_verbose()) {
    Oks::warning_msg("G_Window::create_new_object()")
      << " Could not find graphical class for " << oks_obj << "\n"
         " Create new object " << oks_obj << " using \'" << g_class->get_name() << "\'\n";
  }

  return o;
}


G_Object *
G_Window::create_new_object(G_Class *c, const char *obj_id, const char *obj_type)
{
  OksClass * oks_class = parent->find_class(obj_type ? obj_type : c->get_class()->get_name().c_str());

  if(!oks_class) {
    std::ostringstream text;
    text << "there is no class \'" << (obj_type ? obj_type : "(null)") << "\' defined by the schema";
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
    return 0;
  }
  else
  {
    try {
      return create_new_object(new OksObject(oks_class, obj_id), true);
    }
    catch(oks::exception& ex)
    {
      ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
      return 0;
    }
  }
}


bool
G_Object::is_one_child_per_row() const
{
  return (p_window->child_arrange_type == G_Window::OnePerLine);
}
