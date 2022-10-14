/**
 *  \file oks_dump.cpp
 *
 *  This file is part of the OKS package.
 *  https://gitlab.cern.ch/atlas-tdaq-software/oks
 *
 *  Report classes without descriptions.
 */

#include <stdlib.h>

#include "oks/attribute.hpp"
#include "oks/relationship.hpp"
#include "oks/class.hpp"
#include "oks/kernel.hpp"
#include "oks/file.hpp"
#include "oks/exceptions.hpp"


static void
printUsage(std::ostream& s)
{
  s << "Usage: oks_report_bad_classes [--help] database-file [database-file(s)]\n"
       "\n"
       "Options:\n"
       "    -h | --help     print this text\n"
       "\n"
       "Description:\n"
       "    Report classes, attributes and relationships without description.\n"
       "\n";
}

static void
report_class(const OksClass * c, bool no_description)
{
  std::cout << " * class \"" << c->get_name() << '\"';

  if (no_description)
    std::cout << " has no description";

  std::cout << " (from \"" << c->get_file()->get_short_file_name() << "\")\n";
}

int
main(int argc, char **argv)
{
  OksKernel kernel;
  kernel.set_silence_mode(true);

  try
    {
      for (int i = 1; i < argc; i++)
        {
          const char *cp = argv[i];

          if (!strcmp(cp, "-h") || !strcmp(cp, "--help"))
            {
              printUsage(std::cout);
              return EXIT_SUCCESS;
            }
          else
            {
              if (kernel.load_file(cp) == 0)
                {
                  Oks::error_msg("oks_dump") << "\tCan not load file \"" << cp << "\", exiting...\n";
                  return EXIT_FAILURE;
                }
            }
        }

      if (kernel.schema_files().empty())
        {
          Oks::error_msg("oks_dump") << "\tAt least one oks file have to be provided, exiting...\n";
          return EXIT_FAILURE;
        }
    }
  catch (oks::exception &ex)
    {
      std::cerr << "Caught oks exception:\n" << ex << std::endl;
      return EXIT_FAILURE;
    }
  catch (std::exception &e)
    {
      std::cerr << "Caught standard C++ exception: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  catch (...)
    {
      std::cerr << "Caught unknown exception" << std::endl;
      return EXIT_FAILURE;
    }

  std::cout << "Processed schema files:\n";
  for (const auto &i : kernel.schema_files())
    std::cout << i.second->get_short_file_name() << std::endl;

  for (const auto i : kernel.classes())
    {
      bool printed(false);

      if (i.second->get_description().empty())
        {
          report_class(i.second, true);
          printed = true;
        }

      if (const std::list<OksAttribute*> *attrs = i.second->direct_attributes())
        for (const auto &j : *attrs)

          if (j->get_description().empty())
            {
              if (!printed)
                {
                  report_class(i.second, false);
                  printed = true;
                }
              std::cout << "   - attribute \"" << j->get_name() << "\" has no description\n";
            }

      if (const std::list<OksRelationship*> *rels = i.second->direct_relationships())
        for (const auto &j : *rels)
          if (j->get_description().empty())
            {
              if (!printed)
                {
                  report_class(i.second, false);
                  printed = true;
                }
              std::cout << "   - relationship \"" << j->get_name() << "\" has no description\n";
            }
    }

  return EXIT_SUCCESS;
}
