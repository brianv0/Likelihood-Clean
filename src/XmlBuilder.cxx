/**
 * @file XmlBuilder.cxx
 * @brief Concrete implementation that is shareable by subclasses.
 * @author J. Chiang
 *
 * $Header$
 */

#include "xml/XmlParser.h"

#include "optimizers/Dom.h"

#include "Likelihood/XmlBuilder.h"

namespace Likelihood {

XmlBuilder::XmlBuilder() {
   m_parser = new xml::XmlParser();
   m_doc = optimizers::Dom::createDocument();
}

XmlBuilder::~XmlBuilder() {
   delete m_doc;
   delete m_parser;
}

} //namespace Likelihood
