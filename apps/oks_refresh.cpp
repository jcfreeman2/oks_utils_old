/**
 *  \file oks_validate_repository.cpp
 *
 *  This file is part of the OKS package.
 *  Author: <Igor.Soloviev@cern.ch>
 */

#include <stdlib.h>

#include "boost/program_options.hpp"

#include "oks/kernel.hpp"

int
main(int argc, char **argv)
{
  boost::program_options::options_description desc("This program opens and saves oks files to refresh their format");

  std::vector<std::string> include;
  std::vector<std::string> files;
  bool show_hidden_attributes = false;

  try
    {
      desc.add_options()
        ("files,f", boost::program_options::value<std::vector<std::string> >(&files)->multitoken()->required(), "oks files")
        ("include,i", boost::program_options::value<std::vector<std::string> >(&include)->multitoken(), "preload oks files (to fix missing includes)")
        ("show-all-attributes,x", "show hidden attributes with values equal to defaults")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      boost::program_options::notify(vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_SUCCESS;
        }

      if (vm.count("show-all-attributes"))
        show_hidden_attributes = true;
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  for (const auto &x : files)
    try
      {
        std::cout << "processing file " << x  << std::endl;
        OksKernel kernel(false, false, false, false);

        for(const auto& i : include)
          kernel.load_file(i);

        auto f = kernel.load_file(x);

        if (f->get_oks_format() == "schema")
          kernel.save_schema(f);
        else
          kernel.save_data(f, true, nullptr, show_hidden_attributes);
      }
    catch (oks::exception &ex)
      {
        oks::log_timestamp(oks::Error) << "Caught oks exception:\n" << ex << std::endl;
        return EXIT_FAILURE;
      }

  return EXIT_SUCCESS;
}
