#ifndef _CONFDB_GUI_G_DND_
#define _CONFDB_GUI_G_DND_

#include <X11/Intrinsic.h>

  //
  // Forward declarations of used classes
  //

#include <iostream>

class G_Object;


  //
  // This struct is used to pass data from Drag side to Drop side
  //
  // IMPORTANT NOTE:
  //   drag and drop sites can be in differnt applications => no
  //   pointers to oks or graphical objects are allowed
  //

struct OksObjectDragAndDropInfo {
  unsigned long info_length;
  Widget        w;

  const char * get_object_id() const;
  const char * get_class_name() const;

  static const char * atom_oks_object_name;
  static const char * atom_oks_object_info_name;

  static OksObjectDragAndDropInfo * create(const char *, const char *, Widget);
};

std::ostream& operator<<(std::ostream&, const OksObjectDragAndDropInfo&);


  //
  // These classes are used to transfer the user response
  // from drag & drop popup menu 
  //

class DragAndDropTransferResult {
public:
  enum Type {
    move,
    link
  };

  DragAndDropTransferResult           (const char *s) : rel_name (s) {;}
  virtual ~DragAndDropTransferResult  () {;}
  virtual Type get_type               () const = 0;

  const char * get_rel_name() const {return rel_name;}

private:
  const char *rel_name;
};


class DragAndDropMoveTransferResult : public DragAndDropTransferResult {
public:
  DragAndDropMoveTransferResult          (const char *, G_Object *);
  virtual ~DragAndDropMoveTransferResult () {;}
  virtual Type get_type                  () const {return move;}

  G_Object * get_parent() const {return parent;}

private:
  G_Object *parent;
};


class DragAndDropLinkTransferResult : public DragAndDropTransferResult {
public:
  DragAndDropLinkTransferResult          (const char *);
  virtual ~DragAndDropLinkTransferResult () {;}
  virtual Type get_type                  () const {return link;}
};


inline
DragAndDropMoveTransferResult::DragAndDropMoveTransferResult(const char *s, G_Object *p) :
  DragAndDropTransferResult (s),
  parent		    (p)
{;}

inline
DragAndDropLinkTransferResult::DragAndDropLinkTransferResult(const char *s) :
  DragAndDropTransferResult (s)
{;}


  // inline methods to access object_id and class_name

inline const char *
OksObjectDragAndDropInfo::get_object_id() const
{
  return ((char *)this) + sizeof(OksObjectDragAndDropInfo);
}

inline const char *
OksObjectDragAndDropInfo::get_class_name() const
{
  char *object_id = const_cast<char *>(get_object_id());

  while(*(object_id++) != 0) { ; }

  return object_id;
}

#endif
