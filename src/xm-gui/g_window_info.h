#ifndef __GUI_EDITOR_WINDOW_INFO_H
#define __GUI_EDITOR_WINDOW_INFO_H

#include <string>
#include <list>

class G_Class;


class G_WindowInfo {
  friend class OksDataEditorMainDialog;

  public:

    class Item {

      public: 

        enum Type {
          ClassType,
	  SeparatorType
        };

	virtual Type	type() const = 0;
    };


    class Class : public Item {

      public:

        enum TopLevelShow {
	  ShowUsedMenu,
	  ShowWithoutUsedMenu,
	  DoNotShow
	};

        enum ShownWithChildren {
	  RootObj,
	  AnyObj,
	  NoneObj
	};

	virtual Type		type() const {return ClassType;}

        Class			(const G_Class * gc, const std::string& s, TopLevelShow t, bool b1, bool b2, ShownWithChildren c, const std::string& s2) :
			  	   p_g_class(gc), p_top_level_name(s), p_top_level_show(t),
			  	   p_create_tl_objs(b1), p_show_in_new(b2),
				   p_shown_with_children(c), p_root_relationship_name(s2) {;}

        virtual ~Class		() {;}

	const G_Class *		g_class() const {return p_g_class;}
	const std::string&	top_level_name() const {return p_top_level_name;}
	TopLevelShow		top_level_show() const {return p_top_level_show;}
	bool			create_top_level_objs() const {return p_create_tl_objs;}
	bool			show_in_new() const {return p_show_in_new;}
	ShownWithChildren	shown_with_children() const {return p_shown_with_children;}
	const std::string&	root_relationship_name() const {return p_root_relationship_name;}


      private:
      
        const G_Class *		p_g_class;
        const std::string	p_top_level_name;
        const TopLevelShow	p_top_level_show;
        const bool		p_create_tl_objs;
        const bool		p_show_in_new;
        const ShownWithChildren	p_shown_with_children;
        const std::string	p_root_relationship_name;
    };


    class Separator : public Item {

      public:

        enum BreakAt {
	  TopLevel,
	  NewObject,
	  BothPlaces
	};

	virtual Type type() const {return SeparatorType;}

	Separator (BreakAt bat) : p_break_at(bat) {;}
        virtual ~Separator () {;}

        BreakAt break_at() const {return p_break_at;}


      private:
      
        const BreakAt p_break_at;
    };

    const std::string& title() const {return p_title;}
    const std::list<Item *>& items() const {return p_items;}


  private:

    G_WindowInfo (const std::string& s) : p_title(s) {;}

    std::string p_title;
    std::list<Item *> p_items;
};

#endif
