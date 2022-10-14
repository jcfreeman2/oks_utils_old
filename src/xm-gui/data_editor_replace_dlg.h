#ifndef __DATA_EDITOR_REPLACE_DLG_H
#define __DATA_EDITOR_REPLACE_DLG_H 

#include <oks/object.h>
#include <oks/xm_dialog.h>
#include <oks/xm_utils.h>

class OksDataEditorMainDialog;
class OksObject;
class OksClass;
class OksAttribute;
class OksRelationship;

namespace ReplaceDialog {
  enum FileStatus {
    writable_file = 0,
    read_only_file,
    locked_file,
    bad_file
  };

  FileStatus get_file_status(const OksObject * obj, std::string& status);
}

struct OksDataEditorReplaceResult {
  const OksObject * m_object;
  const OksAttribute * m_attr;
  const OksRelationship * m_rel;
  std::string m_value;
  std::string m_new_value;
  std::string m_text;
  const ReplaceDialog::FileStatus m_status;

    // create find attribute result 

  OksDataEditorReplaceResult(const OksObject * o, const OksAttribute * a, const OksData& d) :
    m_object     (o),
    m_attr       (a),
    m_rel        (0),
    m_value      (d.str()),
    m_status     (ReplaceDialog::writable_file)
  { ; }

    // create find relationship result 

  OksDataEditorReplaceResult(const OksObject * o, const OksRelationship * r, const OksData& d) :
    m_object     (o),
    m_attr       (0),
    m_rel        (r),
    m_value      (d.str(o->GetClass()->get_kernel())),
    m_status     (ReplaceDialog::writable_file)
  { ; }

     // create replace attribute result (no exception)

  OksDataEditorReplaceResult(const OksObject * o, const OksAttribute * a, const std::string& from, const std::string& to) :
    m_object     (o),
    m_attr       (a),
    m_rel        (0),
    m_value      (from),
    m_new_value  (to),
    m_status     (ReplaceDialog::writable_file)
  { ; }
  
     // create replace attribute result (with status text)

  OksDataEditorReplaceResult(const OksObject * o, const OksAttribute * a, const std::string& from, const std::string& to, const std::string& text, ReplaceDialog::FileStatus status) :
    m_object     (o),
    m_attr       (a),
    m_rel        (0),
    m_value      (from),
    m_new_value  (to),
    m_text       (text),
    m_status     (status)
  { ; }

     // create replace attribute result (with exception)

  OksDataEditorReplaceResult(const OksObject * o, const OksAttribute * a, const std::string& from, const std::string& to, oks::exception& ex) :
    m_object     (o),
    m_attr       (a),
    m_rel        (0),
    m_value      (from),
    m_new_value  (to),
    m_text       (ex.what()),
    m_status     (ReplaceDialog::bad_file)
  { ; }

     // create replace relationship result (no exception)

  OksDataEditorReplaceResult(const OksObject * o, const OksRelationship * r, const std::string& from, const std::string& to) :
    m_object     (o),
    m_attr       (0),
    m_rel        (r),
    m_value      (from),
    m_new_value  (to),
    m_status     (ReplaceDialog::writable_file)
  { ; }

     // create replace relationship result (with status text)

  OksDataEditorReplaceResult(const OksObject * o, const OksRelationship * r, const std::string& from, const std::string& to, const std::string& text, ReplaceDialog::FileStatus status) :
    m_object     (o),
    m_attr       (0),
    m_rel        (r),
    m_value      (from),
    m_new_value  (to),
    m_text       (text),
    m_status     (status)
  { ; }

     // create replace relationship result (with exception)

  OksDataEditorReplaceResult(const OksObject * o, const OksRelationship * r, const std::string& from, const std::string& to, oks::exception& ex) :
    m_object     (o),
    m_attr       (0),
    m_rel        (r),
    m_value      (from),
    m_new_value  (to),
    m_text       (ex.what()),
    m_status     (ReplaceDialog::bad_file)
  { ; }


};

class OksDataEditorReplaceDialog : public OksDialog
{
  friend class OksDataEditorMainDialog;
  friend class OksDataEditorSearchResultDialog;

  public:
    enum ItemType {
      simple,
      complex_string,
      enumeration
    };

    enum ActionType {
      replace,
      find
    };
  
    enum CompareType {
      ignore_case,
      use_case
    };

    OksDataEditorReplaceDialog(OksDataEditorMainDialog *, ActionType);
    ~OksDataEditorReplaceDialog();

    int do_replacement(OksData &, OksData &);

   private:
     const ActionType m_action_type;
     OksDataEditorMainDialog * parent;

     bool m_find_only;
     bool m_attr_type_on;
     ItemType item_type;
     bool match_whole_string;
     bool m_is_reg_exp;
     CompareType case_compare;
     const OksClass * m_selected_class;
     bool m_search_in_subclasses;
     std::string m_search_query;
     const OksAttribute * m_selected_attribute;
     const OksRelationship * m_selected_relationship;

     Widget m_query_text_field;

     OksXm::TipsInfo * m_from_rel_tips;
     OksXm::TipsInfo * m_to_rel_tips;

     static int			compare(const char *, const char *, CompareType);
     static const char *	index(const char *, const char *, CompareType);

     static void		closeCB(Widget, XtPointer, XtPointer);
     static void		replaceCB(Widget, XtPointer, XtPointer);
     static void		helpCB(Widget, XtPointer, XtPointer);
     static void		switchTypeCB(Widget, XtPointer, XtPointer);
     static void		chooseAttrTypeCB(Widget, XtPointer, XtPointer);
     static void		changeOnRegExpCB(Widget, XtPointer, XtPointer);
     static void		chooseSearchInClassCB(Widget, XtPointer, XtPointer);
     static void		chooseSearchClassExtraCB(Widget, XtPointer, XtPointer);
     static void		chooseSearchInAttrOrRelCB(Widget, XtPointer, XtPointer);
     static void		pasteCB(Widget, XtPointer, XtPointer);

     static void                relValueAC(Widget, XtPointer, XEvent *, Boolean *);
     static void                selectAC(Widget, XtPointer, XEvent *, Boolean *);


     std::string		do_partial_replacement(const OksData &, const OksData &, const OksString&) const;
     bool			check_class_type(const OksRelationship *, const OksData *) const;
     const OksAttribute *       find_enum_attribute(const char * enumerator);


     void update_search_in_attrs_and_rels();

	//
	// Controls
	//

     enum {
       idTitle = 111,
       idAttributeValueType,
       idRelationshipValueType,
       idFrom,
       idTo,
       idAttributeProperties,
       idAttributeTypes,
       idAttributeWholeString,
       idAttributeCaseString,
       idAttributeRegExpString,
       idSearchInClass,
       idSearchInClassExtra,
       idSearchInClassWithQuery,
       idSearchInDerivedClasses,
       idSearchInAttrOrRel,
       idReplaceBtn,
       idFindBtn,
       idHelpBtn,
       idCloseBtn
     };

     static const char * s_extra_search_none;
     static const char * s_extra_search_and_subclasses;
     static const char * s_extra_search_with_query;

};


#endif
