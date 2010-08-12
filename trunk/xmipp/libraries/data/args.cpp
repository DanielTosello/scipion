/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.csic.es)
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

#include "args.h"

#include <cstdio>
#include <cmath>
#include <cerrno>

// Get parameters from the command line ====================================
char *getParameter(int argc, char **argv, const char *param,
                const char *option, int _errno, const std::string &errmsg,
                int exit)
{
    int i = 0;

    while ((i < argc) && (strcmp(param, argv[i])))
        i++;
    if (i < argc - 1)
        return(argv[i+1]);
    else
        if (option == NULL)
        {
            std::string auxstr;
            if (_errno == -1)
            {
                _errno = 2104;
                auxstr = (std::string)"Argument " + param + " not found or invalid argument";
            }
            else
            {
                auxstr = errmsg;
            }
            if (exit)
                EXIT_ERROR(_errno, auxstr);
            else
                REPORT_ERROR(_errno, auxstr);
        }

    return((char *) option);
}

// Get 2 parameters ========================================================
bool getTwoDoubleParams(int argc, char **argv, const char *param,
                         double &v1, double &v2, double v1_def, double v2_def,
                         int _errno, const std::string & errmsg, int exit)
{
    bool retval;
    int i = paremeterPosition(argc, argv, param);
    if (i != -1)
    {
        if (i + 2 >= argc)
        {
            std::string msg;
            if (errmsg == "")
                msg = (std::string)"Not enough arguments after " + *param;
            else
                msg = errmsg;
            if (exit)
                EXIT_ERROR(_errno, errmsg);
            else
                REPORT_ERROR(_errno, errmsg);
        }
        v1 = textToFloat(argv[i+1]);
        v2 = textToFloat(argv[i+2]);
        retval = true;
    }
    else
    {
        v1 = v1_def;
        v2 = v2_def;
        retval = false;
    }
    return retval;
}

// Get 3 parameters ========================================================
bool getThreeDoubleParams(int argc, char **argv, const char *param,
                         double &v1, double &v2, double &v3,
                         double v1_def, double v2_def, double v3_def,
                         int _errno, const std::string & errmsg, int exit)
{
    bool retval;
    int i = paremeterPosition(argc, argv, param);
    if (i != -1)
    {
        if (i + 3 >= argc)
        {
            std::string msg;
            if (errmsg == "")
                msg = (std::string)"Not enough arguments after " + *param;
            else
                msg = errmsg;
            if (exit)
                EXIT_ERROR(_errno, errmsg);
            else
                REPORT_ERROR(_errno, errmsg);
        }
        v1 = textToFloat(argv[i+1]);
        v2 = textToFloat(argv[i+2]);
        v3 = textToFloat(argv[i+3]);
        retval = true;
    }
    else
    {
        v1 = v1_def;
        v2 = v2_def;
        v3 = v3_def;
        retval = false;
    }
    return retval;
}

// Checks if a boolean parameter was included the command line =============
bool checkParameter(int argc, char **argv, const char *param)
{
    int i = 0;

    while ((i < argc) && (strcmp(param, argv[i]) != 0))
        i++;

    if (i < argc)
        return(true);
    else
        return(false);
}

// Position of a parameter in the command line =============================
int paremeterPosition(int argc, char **argv, const char *param)
{
    int i = 0;

    while ((i < argc) && (strcmp(param, argv[i])))
        i++;

    if (i < argc - 1)
        return  i;
    else
        return -1;
}

// Number of components ====================================================
int numComponents(const std::string &str)
{
    int imax = str.length();
    int retval = 0;
    if (str[0] != '[' && str[imax-1] != ']')
        return retval;
    for (int i = 0; i < imax; i++)
        if (str[i] == ',')
            retval++;
    return retval + 1;
}

// Get float vector ========================================================
Matrix1D<double> getVectorParameter(int argc, char **argv, const char *param,
                                  int dim, int _errno,
                                  const std::string & errmsg,
                                  int exit)
{
    Matrix1D<double> aux;
    bool count_dimensionality = (dim == -1);

    // Find and form vector
    int pos = paremeterPosition(argc, argv, param);
    if (pos == -1 || pos + 1 == argc)
        if (count_dimensionality)
            return aux;
        else
        {
            std::string auxstr;
            if (_errno == -1)
            {
                _errno = 2104;
                auxstr = (std::string)"Argument " + param + " not found or invalid argument";
            }
            else
            {
                auxstr=errmsg;
            }
            if (exit)
                EXIT_ERROR(_errno, auxstr);
            else
                REPORT_ERROR(_errno, auxstr);
        }
    pos++;
    if (*(argv[pos]) != '[')
    {
        try
        {
            double d = textToFloat(argv[pos]);
            aux.resize(1);
            aux(0) = d;
            return aux;
        }
        catch (Xmipp_error XE)
        {
            std::string auxstr;
            if (_errno == -1)
            {
                _errno = 2104;
                auxstr = (std::string)"Argument " + param + " not found or invalid argument";
            }
            else
            {
                auxstr=errmsg;
            }
            if (exit)
                EXIT_ERROR(_errno, auxstr);
            else
                REPORT_ERROR(_errno, auxstr);
        }
    }

    std::string vector;
    bool finished = false;
    while (!finished)
    {
        vector += argv[pos];
        if (vector[vector.length()-1] == ']')
            finished = true;
        if (++pos == argc && !finished)
        {
            std::string auxstr;
            if (_errno == -1)
            {
                _errno = 2104;
                auxstr = (std::string)"Argument " + param + " not found or invalid argument";
            }
            else
            {
                auxstr=errmsg;
            }
            if (exit)
                EXIT_ERROR(_errno, auxstr);
            else
                REPORT_ERROR(_errno, auxstr);
        }
    }

    // Remove brackets
    vector = vector.substr(1, vector.length() - 2);

    // Count dimensionality
    int start_copy = 0, end_copy;
    if (count_dimensionality)
    {
        dim = 0;
        start_copy = 0;
        do
        {
            end_copy = vector.find(',', start_copy);
            if (end_copy == -1)
                break;
            start_copy = end_copy + 1;
            dim++;
        }
        while (1);
        dim++;
    }

    // Read diferent vector elements
    int i = 0;
    start_copy = 0;
    aux.resize(dim);
    while (i < dim - 1)
    {
        // Find colon
        end_copy = vector.find(',', start_copy);
        // Store number
        aux(i) = textToFloat(vector.substr(start_copy, end_copy));

        // Prepare for next iteration
        i++;
        start_copy = end_copy + 1;
    }

    // Copy last element
    aux(i) = textToFloat(vector.substr(start_copy, vector.length()));

    return aux;
}

// Specific command line ===================================================
void specificCommandLine(const std::string &prog_name, int argc, char **argv,
                           int &argcp, char ***argvp)
{
    int i = 1;
    // Look for the program name
    while (i < argc)
        if (prog_name == argv[i])
            break;
        else
            i++;
    // If not found, exit
    if (i == argc)
    {
        argcp = 0;
        return;
    }

    // Check that following token is '('
    i++;
    if (i == argc || strcmp(argv[i], "(") != 0)
    {
        argcp = 0;
        return;
    }

    // The specific command line starts here
    *argvp = &argv[i];
    argcp = 1;

    // Look for ')'
    i++;
    if (i == argc)
    {
        argcp = 0;
        return;
    }
    while (i < argc)
    {
        if (strcmp(argv[i], ")") != 0)
        {
            i++;
            argcp++;
        }
        else
            break;
    }
    // If ')' not found exit
    if (i == argc)
    {
        argcp = 0;
        return;
    }
}

// Generate command line ===================================================
#define INSIDE_WORD  1
#define OUTSIDE_WORD 2
void generateCommandLine(const std::string &command_line, int &argcp,
                           char ** &argvp, char* &copy)
{
    int L = command_line.length();

    // Some initialization
    if (L == 0)
    {
        argcp = 0;
        return;
    }
    if (command_line[0] == '\n')
    {
        argcp = 0;
        return;
    }

    // Check that argvp and copy are empty
    if (argvp != NULL)
        delete argvp;
    if (copy != NULL)
        delete copy;

    // Copy command line
    copy = new char[L+1];
    int i = 0;
    while (i < L && command_line[i] != '\n')
    {
        copy[i] = command_line[i];
        i++;
    }
    L = i;
    copy[L] = '\0';

    // Now count how many different words are there
    int words;
    int state;
    if (copy[0] == ' ')
    {
        state = OUTSIDE_WORD;
        words = 0;
    }
    else
    {
        state = INSIDE_WORD;
        words = 1;
    }
    i = 1;
    while (i < L)
    {
        if (state == OUTSIDE_WORD && copy[i] != ' ')
        {
            state = INSIDE_WORD;
            words++;
        }
        if (state == INSIDE_WORD && copy[i] == ' ')
            state = OUTSIDE_WORD;
        i++;
    }

    // Resize argv and cut words
    argvp = new char *[words+1];
    argvp[0] = new char[6];
    strcpy(argvp[0], "autom");
    if (copy[0] == ' ')
    {
        state = OUTSIDE_WORD;
        argcp = 1;
    }
    else
    {
        state = INSIDE_WORD;
        argvp[1] = &(copy[0]);
        argcp = 2;
    }
    i = 1;
    while (i < L)
    {
        if (state == OUTSIDE_WORD && copy[i] != ' ')
        {
            state = INSIDE_WORD;
            argvp[argcp] = &(copy[i]);
            argcp++;
        }
        if (state == INSIDE_WORD && copy[i] == ' ')
        {
            state = OUTSIDE_WORD;
            copy[i] = '\0';
        }
        i++;
    }
}

// Generate command line from file =========================================
bool generateCommandLine(FILE *fh, const char *param, int &argcp,
                           char ** &argvp, char* &copy)
{
    long actual_pos = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    char    line[201];
    char   *retval;
    bool found = false;

    // Read lines
    while (fgets(line, 200, fh) != NULL && !found)
    {
        if (line[0] == 0)
            continue;
        if (line[0] == '#')
            continue;
        if (line[0] == ';')
            continue;
        if (line[0] == '\n')
            continue;

        int i = 0;
        while (line[i] != 0 && line[i] != '=')
            i++;
        if (line[i] == '=')
        {
            line[i] = 0;
            if (strcmp(line, param) == 0)
            {
                retval = line + i + 1;
                found = true;
                break;
            }
        }
    }
    fseek(fh, actual_pos, SEEK_SET);
    if (!found)
        return false;

    std::string artificial_line;
    artificial_line = (std::string)"-" + param + " " + retval;
    generateCommandLine(artificial_line, argcp, argvp, copy);
    return true;
}

// Get "parameter" from file ===============================================
std::string getParameter(FILE *fh, const char *param, int skip, const char *option,
                 int _errno, const std::string &errmsg, int exit)
{
    long actual_pos = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    char    line[201];
    std::string  retval;
    bool found = false;
    int skipped = 0;

    // Read lines
    while (fgets(line, 200, fh) != NULL && !found)
    {
        if (line[0] == 0)
            continue;
        if (line[0] == '#')
            continue;
        if (line[0] == ';')
            continue;
        if (line[0] == '\n')
            continue;

        int i = 0;
        char *line_wo_spaces = line;
        while (*line_wo_spaces == ' ' || *line_wo_spaces == '\t')
            line_wo_spaces++;
        while (line_wo_spaces[i] != 0 && line_wo_spaces[i] != '=')
            i++;
        if (line_wo_spaces[i] == '=')
        {
            line_wo_spaces[i] = 0;
            if (strcmp(line_wo_spaces, param) == 0)
            {
                if (skipped == skip)
                {
                    retval = line_wo_spaces + i + 1;
                    found = true;
                    break;
                }
                else
                    skipped++;
            }
        }
    }
    fseek(fh, actual_pos, SEEK_SET);
    if (!found)
        if (option == NULL)
        {
            std::string auxstr;
            if (_errno == -1)
            {
                _errno = 2104;
                auxstr = (std::string)"Argument " + param + " not found or invalid argument";
            }
            else
            {
                auxstr=errmsg;
            }
            if (exit)
                EXIT_ERROR(_errno, auxstr);
            else
                REPORT_ERROR(_errno, auxstr);
        }
        else
            return option;
    else
        return removeSpaces(retval);
    return "";
}

// Check "parameter" from file =============================================
bool checkParameter(FILE *fh, const char *param)
{
    long actual_pos = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    char    line[201];
    bool found = false;
    std::string retval;

    // Read lines
    while (fgets(line, 200, fh) != NULL)
    {
        if (line[0] == 0)
            continue;
        if (line[0] == '#')
            continue;
        if (line[0] == '\n')
            continue;

        int i = 0;
        while (line[i] != 0 && line[i] != '=')
            i++;
        if (line[i] == '=')
        {
            line[i] = 0;
            if (strcmp(line, param) == 0)
            {
                retval = line + i + 1;
                found = true;
                break;
            }
        }
    }
    fseek(fh, actual_pos, SEEK_SET);
    return found && retval != "No" && retval != "NO" && retval != "no";
}

// Get vector param from file ==============================================
Matrix1D<double> getVectorParameter(FILE *fh, const char *param,
                                  int dim, int _errno, 
                                  const std::string &errmsg, int exit)
{
    int    argcp;
    char **argvp = NULL;
    char  *copy = NULL;
    Matrix1D<double> retval;
    if (!generateCommandLine(fh, param, argcp, argvp, copy))
        if (dim == -1)
            return retval;
        else if (exit)
            EXIT_ERROR(_errno, errmsg);
        else
            REPORT_ERROR(_errno, errmsg);
    else
    {
        retval = getVectorParameter(argcp, argvp, ((std::string)"-" + param).c_str(), dim,
                                  _errno, errmsg, exit);
        delete copy;
        return retval;
    }
    return retval;
}
