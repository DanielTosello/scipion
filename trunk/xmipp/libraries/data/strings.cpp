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

#include "strings.h"

std::string removeChar( const std::string& str, char character )
{
    std::string temp;

    for( unsigned int i = 0 ; i < str.length( ) ; i++ )
    {
        if ( str[ i ] != character )
            temp += str[ i ];
    }

    return temp;
}

std::string unescape( const std::string& str )
{
    std::string temp;

    for( unsigned int i = 0 ; i < str.length( ) ; i++ )
    {
        char current_char = str[ i ];

        if( current_char != '\n' && current_char != '\t' &&
            current_char != '\v' && current_char != '\b' &&
            current_char != '\r' && current_char != '\f' &&
            current_char != '\a' )
        {
            temp += str[ i ];
        }
    }

    return temp;
}

std::string simplify( const std::string& str )
{
    std::string temp;

    // First, unescape string
    std::string straux = unescape( str );

    // Remove spaces from the beginning
    int pos = straux.find_first_not_of( ' ' );
    straux.erase( 0, pos );

    // Trim the rest of spaces
    for( unsigned int i = 0 ; i < straux.length( ) ; )
    {
        temp += straux[ i ];

        if ( straux[ i ] == ' ' )
        {
            while( straux[ i ] == ' ' )
            {
                i++;
            }
        }
        else
        {
            i++;
        }
    }

    // Remove space left at the end of the string
    // if needed
    if( temp[ temp.size( ) - 1 ] == ' ' )
    {
        temp.resize( temp.size() - 1 );
    }

    return temp;
}

/** Trim all spaces from the begining and the end */
void trim(std::string& str)
{
    std::string::size_type pos = str.find_last_not_of(' ');

    if (pos != std::string::npos)
    {
        str.erase(pos + 1);
        pos = str.find_first_not_of(' ');
        if (pos != std::string::npos)
            str.erase(0, pos);
    }
    else
        str.clear();
}

void Tokenize(const std::string& str,
              std::vector<std::string>& tokens,
              const std::string& delimiters)
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}
