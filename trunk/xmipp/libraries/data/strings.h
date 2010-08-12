/***************************************************************************
 * 
 * Authors:     J.R. Bilbao-Castro (jrbcast@ace.ual.es)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

#ifndef XMIPP_STRINGS_H
#define XMIPP_STRINGS_H

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

/// @defgroup StringUtilities String utilities
/// @ingroup DataLibrary
//@{
/** Removes all occurrences of 'character' from the string no matter
where they are */
std::string removeChar( const std::string& str, char character );

/** Removes escaped symbols ESC+n, t, v, b, r, f, and a */
std::string unescape( const std::string& str );

/** Removes white spaces from the beginning and the end of the string
as well as escaped characters
and simplifies the rest of groups of white spaces of the string to
a single white space */
std::string simplify( const std::string& str );

/** Tokenize a string
 */
void Tokenize(const std::string& str,
              std::vector<std::string>& tokens,
              const std::string& delimiters = " ");

/** Remove trailing spaces */
void trim(std::string& str);
//@}
#endif
