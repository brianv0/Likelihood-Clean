/**
 * @file Util.h
 * @brief Provide basic utililty functions to Likelihood applications.
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_Util_h
#define Likelihood_Util_h

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "facilities/Util.h"

namespace Likelihood {

/**
 * @class Util
 * @brief Static functions used by Likelihood applications.
 *
 * @author J. Chiang
 *
 * $Header$
 */

class Util {

public:

   static bool fileExists(const std::string & filename) {
      std::ifstream file(filename.c_str());
      return file.is_open();
   }

   static void file_ok(std::string filename) {
      facilities::Util::expandEnvVar(&filename);
      if (fileExists(filename)) {
         return;
      } else {
         throw std::invalid_argument("File not found: " + filename);
      }
   }

   static void readLines(std::string inputFile, 
                         std::vector<std::string> &lines,
                         const std::string &skip = "#") {
      facilities::Util::expandEnvVar(&inputFile);
      std::ifstream file(inputFile.c_str());
      lines.clear();
      std::string line;
      while (std::getline(file, line, '\n')) {
         if (line != "" && line != " "             //skip (most) blank lines 
             && line.find_first_of(skip) != 0) {   //and commented lines
            lines.push_back(line);
         }
      }
   }

   static void resolve_fits_files(std::string filename, 
                                  std::vector<std::string> &files) {
      facilities::Util::expandEnvVar(&filename);
      files.clear();
// Read the first line of the file and see if the first 6 characters
// are "SIMPLE".  If so, then we assume it's a FITS file.
      std::ifstream file(filename.c_str());
      std::string firstLine;
      std::getline(file, firstLine, '\n');
      if (firstLine.find("SIMPLE") == 0) {
// This is a FITS file. Return that as the sole element in the files
// vector.
         files.push_back(filename);
         return;
      } else {
// filename contains a list of fits files.
         readLines(filename, files);
         return;
      }
   }

   static bool isXmlFile(std::string filename) {
      std::vector<std::string> tokens;
      facilities::Util::stringTokenize(filename, ".", tokens);
      if (*(tokens.end()-1) == "xml") {
         return true;
      }
      return false;
   }
};

} // namespace Likelihood

#endif // Likelihood_Util_h
