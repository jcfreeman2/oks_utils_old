/**
 *  \file oks_diff_data.cpp
 *
 *  This file is part of the OKS package.
 *  Author: <Igor.Soloviev@cern.ch>
 *
 *  This file contains the implementation of the OKS application to show equal objects.
 */


#include <iostream>
#include <sstream>

#include <set>
#include <map>

#include "boost/program_options.hpp"

#include "oks/kernel.hpp"
#include "oks/class.hpp"
#include "oks/object.hpp"
#include "oks/attribute.hpp"
#include "oks/relationship.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SortObjById {
  bool operator() (const OksObject * o1, const OksObject * o2) const {
    return o1->GetId() < o2->GetId();
  }
};

typedef std::set<const OksObject *, SortObjById> ObjsSet;

////////////////////////////////////////////////////////////////////////////////////////////////////


static void process_class(const OksClass *c, bool verbose)
{
  bool printed_class(false);

  if(verbose) {
    std::cout << "* processing class \'" << c->get_name() << "\'" << std::endl;
    printed_class = true;
  }

  const OksObject::Map * objects = c->objects();
  if(!objects || objects->empty()) {
    if(verbose) {
      std::cout << "  no objects" << std::endl;
    }
    return;
  }

  std::multimap<std::string, const OksObject *> vals;

  const std::list<OksAttribute *> * alist = c->all_attributes();
  const std::list<OksRelationship *> * rlist = c->all_relationships();

  OksDataInfo di(0, (const OksAttribute *)(0));

  for(OksObject::Map::const_iterator i = objects->begin(); i != objects->end(); ++i) {
    const OksObject * o(i->second);
    std::ostringstream s;
    OksData * d(o->GetAttributeValue(&di));

    if(alist) {
      for(std::list<OksAttribute *>::const_iterator j = alist->begin(); j != alist->end(); ++j, ++d) {
        s << (*j)->get_name() << *d << '\n';
      }
    }

    if(rlist) {
      for(std::list<OksRelationship *>::const_iterator j = rlist->begin(); j != rlist->end(); ++j, ++d) {
        s << (*j)->get_name() << *d << '\n';
      }
    }

    s << std::ends;

    vals.insert(std::make_pair(s.str(), o));
  }
  
  std::string last;
  const OksObject * prev(0);
  
  for(std::multimap<std::string, const OksObject *>::iterator i = vals.begin(); i != vals.end(); ) {
    if(last == i->first) {
      ObjsSet equal_objs;
      equal_objs.insert(prev);
      equal_objs.insert(i->second);
      ++i;
      for(; i != vals.end(); ++i) {
        if(last == i->first) {
          equal_objs.insert(i->second);
        }
        else {
          if(!printed_class) {
            std::cout << "* found equal objects in class \'" << c->get_name() << "\'" << std::endl;
            printed_class = true;
          }

          std::cout << "  # the following " << equal_objs.size() << " objects are equal:\n";
          for(ObjsSet::const_iterator j = equal_objs.begin(); j != equal_objs.end(); ++j) {
            std::cout << "   - " << (*j) << " from \'" << (*j)->get_file()->get_full_file_name() << "\'\n";
          }

          last = i->first;
          prev = i->second;
          ++i;
          break;
        }
      }
    }
    else {
      last = i->first;
      prev = i->second;
      ++i;
    }
  }
}



namespace po = boost::program_options;

int
main(int argc, char **argv)
{
  std::vector<std::string> files;
  std::string class_name;
  bool verbose(false);

  try {
    po::options_description desc("This program is loading OKS data files and searching for equal object.\nUsage: oks_report_equal_objects [options] [-f] file+  ");

    desc.add_options()
      ("class,c" , po::value<std::string>(&class_name)            , "If defined, only compare objects of this class")
      ("files,f" , po::value<std::vector<std::string> >(&files)   , "Names of input OKS files")
      ("verbose,v"                                                , "Run in verbose mode")
      ("help,h"                                                   , "Print help message")
    ;

    po::positional_options_description p;
    p.add("files", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);

    if(vm.count("help"))    { std::cout << desc << std::endl; return EXIT_SUCCESS; }
    if(vm.count("verbose")) { verbose = true; }

    if(files.empty())       { throw std::runtime_error("Missing application name" ); }
  }
  catch(std::exception& ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  try {
    OksKernel kernel;
    kernel.set_test_duplicated_objects_via_inheritance_mode(true);
    kernel.set_silence_mode(!verbose);

    for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i) {
      kernel.load_file(*i);
    }

    if(!class_name.empty()) {
      if(OksClass * c = kernel.find_class(class_name)) {
        process_class(c, verbose);
        return 0;
      }
      else {
        std::cerr << "ERROR: cannot find class \'" << class_name << '\'' << std::endl;
        return EXIT_FAILURE;
      }
    }

    for(OksClass::Map::const_iterator i = kernel.classes().begin(); i != kernel.classes().end(); ++i) {
      process_class(i->second, verbose);
    }
  }
  catch(oks::exception& ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}
