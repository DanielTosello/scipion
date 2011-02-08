/***************************************************************************
 * Authors:     J.M de la Rosa Trevin (jmdelarosa@cnb.csic.es)
 *
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your param) any later version.
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

#include "argsparser.h"
#include "filename.h"
#include "program.h"

//-------------------   LEXER IMPLEMENTATIONS --------------------------------

const char * ArgToken::typeString(TokenType type)
{
    switch (type)
    {
    case TOK_ID: //identifier
        return "ID";
    case TOK_OPT: // -ID or --ID
        return "OPTION";
    case TOK_INT: //integer number
        return "INT";
    case TOK_FLOAT: //float number
        return "FLOAT";
    case TOK_STR: //string value
        return "STRING";
    case TOK_MINUS: // - or --
        return "MINUS";
    case TOK_EQ: // =
        return "EQUAL";
    case TOK_COMM: // Comment: from : until end of line
        return "COMMENT";
    case TOK_LAN: // <
        return "<";
    case TOK_RAN: // >
        return ">";
    case TOK_LBRA: // [
        return "[";
    case TOK_RBRA: // ]
        return "]";
    case TOK_END: // end of input
        return "EOF";
        ///Reserved words
    case TOK_WHERE: // 'WHERE' keyword
        return "WHERE";
    case TOK_ALIAS: // 'ALIAS' keyword
        return "ALIAS";
    case TOK_SECTION:
        return "SECTION";
    case TOK_SEMI:
        return ";";
    case TOK_COMMA:
        return ",";
    case TOK_PLUS:
        return "+";
    default:
        return "UKNOWN";
    };
}

ArgLexer::ArgLexer()
{
    line = 0;
    pos = 0;
    pToken = new ArgToken();
    //Initialize reserved words dictionary
    reservedWords["WHERE"] = TOK_WHERE;
    reservedWords["ALIAS"] = TOK_ALIAS;
    reservedWords["OR"] = TOK_OR;
    reservedWords["REQUIRES"] = TOK_REQUIRES;
}

ArgLexer::~ArgLexer()
{
    delete pToken;
}

void ArgLexer::addLine(String line)
{
    input.push_back(line + " ");
}

inline void ArgLexer::nextLine()
{
    ++line;
    pos = 0;
}

void ArgLexer::setupToken(TokenType type)
{
    pToken->type = type;
    if (type != TOK_END)
    {
        pToken->line = line;
        pToken->start = pos;
        pToken->end = pos + offset - 1;
        pToken->lexeme = input[line].substr(pos, offset);

        if (type == TOK_ID)
        {
            String s = pToken->lexeme;
            std::transform(s.begin(), s.end(), s.begin(),
                           ::toupper);
            std::map<String, TokenType>::iterator it;
            it = reservedWords.find(s);
            if (it != reservedWords.end())
            {
                pToken->lexeme = it->first;
                pToken->type = it->second;
            }

        }
        else if (type == TOK_STR)
            ++offset;
        pos += offset;
        offset = 1;
    }
    else
        pToken->lexeme = "EOF";
}

void ArgLexer::checkVisibility()
{
    while (input[line][pos] == '+')
    {
        ++(pToken->visibility);
        ++pos;
    }
}

void ArgLexer::checkIndependent()
{
    if (input[line][pos]== '*')
    {
        pToken->starred = true;
        ++pos;
    }
}

bool ArgLexer::nextToken()
{
    //make it visible by default
    pToken->visibility = 0;
    pToken->starred = false;

    if (line == input.size())
    {
        setupToken(TOK_END);
        return false;
    }

    char c = input[line][pos];

    //Skip all white spaces
    while (line < input.size() && pos < input[line].length() && isspace(c
            = input[line][pos]))
    {
        ++pos;
        if (c == '\0' || c == '\n' || pos == input[line].length()) //Next line
        {
            nextLine();
        }
    }

    if (line == input.size())
        setupToken(TOK_END);
    else if (isalpha(c) || c == '_')
    {
        offset = 0;
        while (isalnum(c) || c == '_')
        {
            ++offset;
            c = input[line][pos + offset];
        }
        setupToken(TOK_ID);
    }
    else if (isalnum(c) || (c == '-' && isdigit(input[line][pos + 1])))
    {
        offset = 1;
        TokenType t = TOK_INT;
        while (isdigit(input[line][pos + offset]))
            ++offset;
        if (input[line][pos + offset] == '.')
        {
            ++offset;
            while (isdigit(input[line][pos + offset]))
                ++offset;
            t = TOK_FLOAT;
        }
        if (input[line][pos + offset] == 'e')
        {
            ++offset;
            if (input[line][pos + offset] != '+' && input[line][pos + offset]
                != '-')
            {
                std::cerr << "ERROR: expected '+' or '-' " << std::endl
                << "at line: " << line + 1 << " pos: " << offset + 1
                << std::endl;
                exit(1);
            }
            ++offset;
            while (isdigit(input[line][pos + offset]))
                ++offset;
            t = TOK_FLOAT;

        }
        setupToken(t);
    }
    else
    {
        offset = 1;
        bool empty;
        switch (c)
        {
        case '<':
            setupToken(TOK_LAN);
            break;
        case '>':
            setupToken(TOK_RAN);
            break;
        case '=':
            if (input[line][pos + offset] == '=')
            {
                pos = input[line].find_first_not_of('=', pos + 1);
                checkVisibility();
                offset = input[line].find_first_of('=', pos + 1);
                offset -= pos;
                setupToken(TOK_SECTION);
                nextLine();
            }
            else
                //simple equal sign '='
                setupToken(TOK_EQ);
            break;
        case '[':
            setupToken(TOK_LBRA);
            break;
        case ']':
            setupToken(TOK_RBRA);
            break;
        case ';':
            setupToken(TOK_SEMI);
            break;
        case ',':
            setupToken(TOK_COMMA);
            break;
        case '+':
            setupToken(TOK_PLUS);
            break;
        case '.':
            if (input[line][pos + offset] == '.' &&
                input[line][pos + offset + 1] == '.')
            {
                offset += 2;
                setupToken(TOK_ETC);
            }
            break;
        case ':':
            ++pos;

            checkVisibility();

            offset = input[line].find_first_of("\n\r", pos);
            offset -= pos;
            setupToken(TOK_COMM);
            nextLine();
            break;
        case '-':
            if (input[line][pos + offset] == '-')
                ++offset;
            empty = true;
            c = input[line][pos + offset];
            if (isalpha(c) || c == '_') //should start with letter or _
            {
                while (isalnum(c) || c == '_')
                {
                    empty = false;
                    ++offset;
                    c = input[line][pos + offset];
                }
            }
            if (empty)
            {
                std::cerr << "ERROR: Params should be of the form -ID or --ID"
                << std::endl;
                exit(1);
            }
            setupToken(TOK_OPT);
            checkVisibility();
            checkIndependent();
            break;
        case '"':
            offset = input[line].find_first_of('"', pos + 1);
            ++pos;
            offset -= pos;
            setupToken(TOK_STR);
            break;
        default:
            std::cerr << "ERROR: Unexpected character '" << c << "'"
            << std::endl << "at line: " << line + 1 << " pos: " << pos
            + 1 << std::endl;
            exit(1);
        }
    }

    //ConsolePrinter * cp = new ConsolePrinter();
    //cp->printToken(pToken);

    return true;
}

ArgToken * ArgLexer::currentToken() const
{
    return pToken;
}
TokenType ArgLexer::lookahead() const
{
    return pToken->type;
}


//-------------------   PARSER IMPLEMENTATIONS   --------------------------------
void CommentList::addComment(const String &comment, int visible)
{
    comments.push_back(comment);
    visibility.push_back(visible);
}

void CommentList::clear()
{
    comments.clear();
    visibility.clear();
}
size_t CommentList::size() const
{
    return comments.size();
}

ASTNode::ASTNode(ArgLexer * lexer, ASTNode * parent)
{
    pLexer = lexer;
    visible = 0;
    this->parent = parent;
}

TokenType ASTNode::lookahead() const
{
    return pLexer->lookahead();
}

bool ASTNode::lookahead(TokenType type) const
{
    return pLexer->lookahead() == type;
}

ArgToken * ASTNode::currentToken() const
{
    return pLexer->currentToken();
}

void ASTNode::nextToken()
{
    pLexer->nextToken();
}

bool ASTNode::consume(TokenType type)
{
    TokenType t = lookahead();
    if (t != type)
        unexpectedToken();
    //Store consumed token
    if (currentToken() != NULL)
      token = *currentToken();
    else
      REPORT_ERROR(ERR_MEM_NULLPOINTER, "current token is null");

    //Ask for new token
    nextToken();
    return true;
}

bool ASTNode::parseCommentList(CommentList &comments)
{
    comments.clear();
    //Comment List (CL)
    //CL -> comment  CL | e
    while (lookahead(TOK_COMM))
    {
        consume(TOK_COMM);
        comments.addComment(token.lexeme, token.visibility);
    }

    return true;
}

void ASTNode::error(String msg)
{
    std::cerr << ">>> ERROR: " << msg << std::endl << "    at line "
    << token.line + 1 << " column " << token.start + 1 << std::endl;
    exit(1);
}

void ASTNode::unexpectedToken(String msg)
{
    token = *currentToken();
    error((String) "Unexpected token '" + ArgToken::typeString(token.type)
          + "' " + msg);
}

ArgumentDef::ArgumentDef(ArgLexer *lexer, ASTNode * parent) :
        ASTNode(lexer, parent)
{
    isList = false;
    hasDefault = false;
}

ArgumentDef::~ArgumentDef()
{
  for (int i = 0; i < subParams.size(); ++i)
    delete subParams[i];
}

bool ArgumentDef::parse()
{
    //     A -> < ID DEF > | <...>
    //   DEF -> = VALUE | e
    // VALUE -> INT | FLOAT | STRING
    consume(TOK_LAN);
    if (lookahead(TOK_ID))
    {
        consume(TOK_ID);
        name = token.lexeme;
        if (lookahead(TOK_EQ))
        {
            consume(TOK_EQ);
            //Consume a value, that can be int, float or string
            TokenType t = lookahead();

            if (t == TOK_INT || t == TOK_FLOAT || t == TOK_STR || t == TOK_ID)
                consume(t);
            else
                unexpectedToken(" expecting INT, FLOAT, STRING or ID.");

            hasDefault = true;
            argDefault = token.lexeme;
        }
    }
    else
    {
        consume(TOK_ETC);
        name = token.lexeme;
        isList = true;
    }
    consume(TOK_RAN);
    return true;
}

bool ArgumentDef::acceptArguments(std::stringstream &errors, size_t & index, std::vector<const char *> &cmdArguments)
{
    ProgramDef * prog = (ProgramDef*) parent->parent->parent;
    if (index == cmdArguments.size())
    {
        if (hasDefault)
        {
            cmdArguments.push_back(argDefault.c_str());
        }
        else
        {
            errors << "Not enough arguments, <" << name << "> has not default. ";
            return false;
        }
    }

    if (isList)
        return true;

    if (!subParams.empty())
    {
        bool found = false;
        String optionValue = (String)cmdArguments[index];
        for (size_t i = 0; i < subParams.size(); ++i)
        {
            if (subParams[i]->name == optionValue)
            {
                found = true;
                ++index;

                subParams[i]->checkRequires(errors, prog);
                if (!errors.str().empty())
                    return false;

                for (size_t j = 0; j < subParams[i]->arguments.size(); ++j)
                    if (!subParams[i]->arguments[j]->acceptArguments(errors, index, cmdArguments))
                        return false;
                break;
            }
        }
        if (!found)
        {
            errors << optionValue << " is not a valid option for <" << name <<"> ";
            return false;
        }
    }

    //if not list increment index
    ++index;

    return true;
}

ParamDef::ParamDef(ArgLexer *lexer, ASTNode * parent) :
        ASTNode(lexer, parent)
{}

ParamDef::~ParamDef()
{
  for (int i = 0; i < arguments.size(); ++i)
    delete arguments[i];

  for (int i = 0; i < cmdArguments.size(); ++i)
    delete cmdArguments[i];
}

bool ParamDef::containsArgument(const String & argName)
{
    return findArgument(argName) == NULL;
}

ArgumentDef * ParamDef::findArgument(const String & argName)
{
    for (size_t i = 0; i < arguments.size(); ++i)
        if (argName == arguments[i]->name)
            return arguments[i];
    return NULL;
}

bool ParamDef::containsAlias(const String & alias)
{
    for (size_t i = 0; i < aliases.size(); ++i)
        if (alias == aliases[i])
            return true;
    return false;
}

bool ParamDef::parse()
{
    ProgramDef * prog = (ProgramDef*) parent->parent;
    notOptional = true;
    orBefore = false;
    independent = false;
    counter = 0;

    //Param Definition(OD)
    //OD -> OH CL
    //Param Header(OH)
    //OH -> O | [O] | OR O
    //Param(O)
    //O -> param AL
    if (lookahead(TOK_LBRA))
    {
        consume(TOK_LBRA);
        notOptional = false;
    }
    else if (lookahead(TOK_OR))
    {
        consume(TOK_OR);
        orBefore = true;
    }
    consume(TOK_OPT);
    name = token.lexeme;
    visible = token.visibility;
    independent = token.starred;

    prog->addParamName(name, this);

    //Parse argument list
    parseArgumentList();

    if (notOptional == false)
        consume(TOK_RBRA);

    //Parse comment list
    parseCommentList(comments);

    //WHERE section
    if (lookahead(TOK_WHERE))
    {
        consume(TOK_WHERE);
        while (lookahead(TOK_LAN))
        {
            consume(TOK_LAN);
            consume(TOK_ID);
            ArgumentDef * pArg = findArgument(token.lexeme);
            if (pArg == NULL)
            {
                std::cerr << "ERROR; on WHERE definition.\n Param '" << name
                << "' not contains argument '" << token.lexeme << "'"
                << std::endl;
                exit(1);
            }
            consume(TOK_RAN);

            while (lookahead(TOK_ID))
            {
                ParamDef * pOpt = new ParamDef(pLexer, this);
                pOpt->consume(TOK_ID);

                pOpt->name = pOpt->token.lexeme;
                pOpt->parseArgumentList();
                pOpt->parseCommentList(pOpt->comments);
                pOpt->parseParamList(TOK_REQUIRES, prog, pOpt->requires, false);
                pArg->subParams.push_back(pOpt);
            }

        }
    }

    //ALIAS section
    parseParamList(TOK_ALIAS, prog, aliases, true);

    //REQUIRES section
    parseParamList(TOK_REQUIRES, prog, requires, false);

    return true;
}

bool ParamDef::parseArgumentList()
{
    bool previousList = false; // to check only one list and at end of arguments
    bool previousDefault = false; // to check that default values only can be at end
    //Argument List (AL)
    //AL -> argument  AL | e
    while (lookahead(TOK_LAN))
    {
        ArgumentDef * arg = new ArgumentDef(pLexer, this);
        arg->parse();
        token = arg->token;

        if (previousList)
            error("A list <...> has found not at the end of argument list");

        if (previousDefault && !arg->hasDefault)
            error("A non default argument was found before a default one");

        previousList = arg->isList;
        previousDefault = arg->hasDefault;

        arguments.push_back(arg);
    }
    return true;
}

bool ParamDef::parseParamList(TokenType startToken, ProgramDef * prog, StringVector &paramList,
                              bool isAlias)
{
    paramList.clear();
    if (lookahead(startToken))
    {
        consume(startToken);
        consume(TOK_OPT);
        paramList.push_back(token.lexeme);

        if (isAlias)
            prog->addParamName(token.lexeme, this);
        else
            prog->addParamRequires(token.lexeme);

        while (lookahead(TOK_COMMA))
        {
            consume(TOK_COMMA);
            consume(TOK_OPT);
            paramList.push_back(token.lexeme);
            if (isAlias)
                prog->addParamName(token.lexeme, this);
            else
                prog->addParamRequires(token.lexeme);
        }
        consume(TOK_SEMI);
    }

    return true;
}

void ParamDef::checkRequires(std::stringstream & errors, ProgramDef * prog)
{
    ParamDef * param;
    for (size_t i = 0; i < requires.size(); ++i)
    {
        param = prog->findParam(requires[i]);
        if (param->counter < 1)
            errors << "Parameter " << name << " requires " << requires[i] << std::endl;
    }
}

void ParamDef::check(std::stringstream & errors)
{
    ProgramDef * prog = (ProgramDef*) parent->parent;
    if (counter > 1 )
    {
        errors << "Duplicated parameter: " << name << " (check alias)" << std::endl;
        return;
    }

    if (counter == 1)
    {
        //Check requires restrictions
        checkRequires(errors, prog);

        //Check the number of arguments
        if (arguments.empty()) //if not arguments
        {
            if (!cmdArguments.empty())
                errors << "Parameter " << name << " doesn't take any argument, "
                << cmdArguments.size() << " provided." << std::endl;
        }
        else
        {
            size_t argIndex = 0;

            for (size_t i = 0; i < arguments.size(); ++i)
                if (!arguments[i]->acceptArguments(errors, argIndex, cmdArguments))
                {
                    errors << " parameter: " << name << std::endl;
                    return;
                }
        }
    }
    else
    {
        //Fill default arguments
        for (size_t i = 0; i < arguments.size(); ++i)
            if (arguments[i]->hasDefault)
                cmdArguments.push_back(arguments[i]->argDefault.c_str());
    }
}

void SectionDef::addParamDef(ParamDef * param)
{
    ProgramDef * prog = (ProgramDef*)parent;
    if (prog->findParam(param->name) == NULL)
    {
        prog->addParamName(param->name, param);
        param->parent = this;
        params.push_back(param);
    }
}

SectionDef::SectionDef(ArgLexer * lexer, ASTNode * parent) :
        ASTNode(lexer, parent)
{}

SectionDef::~SectionDef()
{
  for (int i = 0; i < params.size(); ++i)
    delete params[i];
}

bool SectionDef::parse()
{
    if (lookahead(TOK_SECTION))
    {
        consume(TOK_SECTION);
        name = token.lexeme;
        visible = token.visibility;
        parseCommentList(comments);
    }

    //OL -> params OD ODL | SD ODL | e
    TokenType t = lookahead();

    if (!(t == TOK_OPT || t == TOK_OR || t == TOK_LBRA))
        unexpectedToken("parsing section, expecting param definition");

    while (t == TOK_OPT || t == TOK_OR || t == TOK_LBRA)
    {
        ParamDef * param = new ParamDef(pLexer, this);
        param->parse();
        params.push_back(param);
        t = lookahead();
    }

    return true;
}

ProgramDef::ProgramDef() :
        ASTNode()
{
    pLexer = new ArgLexer();
    singleOption = false;
}

ProgramDef::~ProgramDef()
{
    delete pLexer;
    for (int i = 0; i < sections.size(); ++i)
      delete sections[i];
}
/** Parse the program definition. */
bool ProgramDef::parse()
{

    // P -> ID CL OL
    //consume(TOK_ID);
    //name = token.lexeme;
    //Usage comments
    //parseCommentList(usageComments);

    //Ask for first token
    pLexer->nextToken();

    while (!lookahead(TOK_END))
    {
        SectionDef * s = new SectionDef(pLexer, this);
        s->parse();
        String name = s->name;
        sections.push_back(s);
    }
    consume(TOK_END);

    return true;
}

void addOcurrence(std::map<String, int> &map, const String &name)
{
    if (map.find(name) != map.end())
        map[name]++;
    else
        map[name] = 1;
}

void reportExclusiveErrors(std::stringstream & errors, std::vector<ParamDef*> &exclusive)
{
    if (exclusive.empty())
        return;

    std::vector<ParamDef*> exclusive2;
    for (size_t i = 0; i < exclusive.size(); ++i)
        if (exclusive[i]->counter == 1)
            exclusive2.push_back(exclusive[i]);
    if (exclusive2.size() > 1)
    {
        errors << "Parameters ";
        for (size_t i = 0; i < exclusive.size() - 1; ++i)
            errors << exclusive[i]->name << " ";
        errors << "and " << exclusive[exclusive.size()-1]->name << " are mutually exclusive (check alias)" << std::endl;
    }
    else if (exclusive2.empty() && exclusive[0]->notOptional)
    {
        errors << "You should provide parameter " << exclusive[0]->name;
        for (size_t i = 1; i < exclusive.size(); ++i)
            errors << " or " << exclusive[i]->name;
        errors << std::endl;
    }
    exclusive.clear();
}

void ProgramDef::check(std::stringstream & errors)
{
    std::vector<ParamDef*> exclusive;
    bool isAlias = false;
    SectionDef * section;
    ParamDef * param;

    for (size_t i = 0; i < sections.size(); ++i)
    {
        section = sections[i];
        for (size_t j = 0; j < section->params.size(); ++j)
        {
            param = section->params[j];
            String name = param->name;
            bool orBefore = param->orBefore;
            size_t size = exclusive.size();
            //Doesn't check for alias, for doesn't repeat error messages
            param->check(errors);
            if (!param->orBefore)
                reportExclusiveErrors(errors, exclusive);
            exclusive.push_back(param);
        }
    }
    reportExclusiveErrors(errors, exclusive);
}

ParamDef * ProgramDef::findParam(const String &name)
{
    if (paramsMap.find(name) != paramsMap.end())
        return paramsMap[name];
    return NULL;
}

void ProgramDef::addParamName(const String &name, ParamDef * param)
{
    if (paramsMap.find(name) != paramsMap.end())
        error((String) "The param '" + name + "' is repeated.");
    else
        paramsMap[name] = param;
}

void ProgramDef::addParamRequires(const String &name)
{
    pendingRequires.push_back(name);
}

void ProgramDef::clear()
{

    SectionDef * section;
    ParamDef * param;

    for (size_t i = 0; i < sections.size(); ++i)
    {
        section = sections[i];
        for (size_t j = 0; j < section->params.size(); ++j)
        {
            param = section->params[j];
            param->counter = 0;
            param->cmdArguments.clear();
        }
    }
}

void ProgramDef::read(int argc, char ** argv, bool reportErrors)
{
    clear();
    std::stringstream errors;
    //Set the name with the first argument
    name = argv[0];
    singleOption = false;
    //We assume that all options start with -
    if (argc > 1 && argv[1][0] != '-')
        REPORT_ERROR(ERR_ARG_INCORRECT, "Parameters should start with a -");

    ParamDef * param = NULL;

    //Read command line params and arguments
    for (int i = 1; i < argc; ++i)
    {


        double _d = atof (argv[i]);
        if (argv[i][0] == '-' && _d == 0)
        {
            param = findParam(argv[i]);
            if (param == NULL)
                errors << "Unrecognized parameter: " << argv[i] << std::endl;
            else
            {
                ++(param->counter);
                if (param->independent)
                    singleOption = true;
            }
        }
        else if (param != NULL)
            param->cmdArguments.push_back(argv[i]);
    }

    if (!singleOption)
        check(errors);

    //Report errors found
    if (reportErrors && errors.str().length() > 0)
    {
        //Unrecognized parameters
        //for (size_t i = 0; i < unrecognized.size(); ++i)
        //    std::cerr << "Unrecognized parameter: " << unrecognized[i] << std::endl;
        REPORT_ERROR(ERR_ARG_BADCMDLINE, errors.str().c_str());
    }
}

SectionDef * ProgramDef::addSection(String sectionName, int visibility)
{
    SectionDef * section = new SectionDef(NULL, this);
    section->name = sectionName;
    section->visible = visibility;
    sections.push_back(section);
    return section;
}

ParamDef* ProgramDef::findAndFillParam(const String &param)
{
    ParamDef * paramDef = findParam(param);
    if (paramDef == NULL)
        REPORT_ERROR(ERR_ARG_INCORRECT, ((String)"Doesn't exists param: " + param));
    ///Param was provided, not need to fill it
    //if (paramDef->counter == 1)
    //    return paramDef;
    std::stringstream errors;
    size_t argIndex = 0;
    for (size_t i = 0; i < paramDef->arguments.size(); ++i)
        if (!paramDef->arguments[i]->acceptArguments(errors, argIndex, paramDef->cmdArguments))
        {
            errors << " parameter: " << paramDef->name << std::endl;
            REPORT_ERROR(ERR_ARG_INCORRECT, errors.str());
        }
    return paramDef;
}

const char * ProgramDef::getParam(const char * paramName, int argNumber)
{
    ParamDef * param = findAndFillParam(paramName);

    return param->cmdArguments.at(argNumber);
}

const char * ProgramDef::getParam(const char * paramName, const char * subParam, int argNumber)
{
    ParamDef * param = findAndFillParam(paramName);

    size_t i = 0;
    for (i = 0; i < param->cmdArguments.size(); ++i)
        if (strcmp(param->cmdArguments[i], subParam) == 0)
            break;

    if (i == param->cmdArguments.size())
        REPORT_ERROR(ERR_ARG_INCORRECT, ((String)"Sub-param " + subParam + " was not supplied in command line."));

    return param->cmdArguments.at(i + 1 + argNumber);
}

//-------------------   PRINTER IMPLEMENTATIONS   --------------------------------
//--------- CONSOLE PRINTER -----------------------
ConsolePrinter::ConsolePrinter(std::ostream & out)
{
    this->pOut = &out;
}

void ConsolePrinter::printProgram(const ProgramDef &program, int v)
{
    //print program name and usage
    *pOut << "PROGRAM" << std::endl << "   " << program.name << std::endl;
    if (program.usageComments.size() > 0)
    {
        *pOut << "USAGE" << std::endl;
        for (size_t i = 0; i < program.usageComments.size(); ++i)
            *pOut << "   " << program.usageComments.comments[i] << std::endl;
    }
    //print see also
    if (!program.seeAlso.empty())
    {
      *pOut << "SEE ALSO" << std::endl;
      *pOut << "   " << program.seeAlso << std::endl;
    }

    //print sections and params
    if (program.sections.size() > 0)
    {
        *pOut << "OPTIONS" << std::endl;
        for (size_t i = 0; i < program.sections.size(); ++i)
            printSection(*program.sections[i], v);
    }
    //print examples
    if (program.examples.size() > 0)
    {
        *pOut << "EXAMPLES" << std::endl;
        for (size_t i = 0; i < program.examples.size(); ++i)
        {
            if (program.examples.visibility[i])
              *pOut << "   ";
            *pOut << "   " << program.examples.comments[i] << std::endl;
        }

    }
}

void ConsolePrinter::printSection(const SectionDef &section, int v)
{
    if (section.visible <= v)
    {
        *pOut << std::endl;
        if (section.name.length() > 0)
            *pOut << section.name << std::endl;
        for (size_t i = 0; i < section.params.size(); ++i)
            printParam(*section.params[i], v);
    }
}

void ConsolePrinter::printRequiresList(StringVector requires)
{
    if (!requires.empty())
    {
        *pOut << " ( requires ";
        for (size_t i = 0; i < requires.size(); ++i)
            *pOut << requires[i] << " ";
        *pOut << ")";
    }
}

void ConsolePrinter::printParam(const ParamDef &param, int v)
{
    if (param.visible <= v)
    {
        if (param.orBefore)
            *pOut << "   OR" << std::endl;

        *pOut << "   ";
        if (!param.notOptional)
            *pOut << "[";
        *pOut << param.name;
        //print alias
        for (size_t i = 0; i < param.aliases.size(); ++i)
            *pOut << ", " << param.aliases[i];
        //print arguments
        for (size_t i = 0; i < param.arguments.size(); ++i)
        {
            *pOut << " ";
            printArgument(*param.arguments[i], v);
        }
        if (!param.notOptional)
            *pOut << "]";

        printRequiresList(param.requires);
        *pOut << std::endl;
        printCommentList(param.comments, v);

        for (size_t i = 0; i < param.arguments.size(); ++i)
        {
            ArgumentDef &arg = *param.arguments[i];
            if (!arg.subParams.empty())
            {
                *pOut << "      where <" << arg.name << "> can be:" << std::endl;
                for (size_t j = 0; j < arg.subParams.size(); ++j)
                {
                    *pOut << "        " << arg.subParams[j]->name;
                    for (size_t k = 0; k < arg.subParams[j]->arguments.size(); ++k)
                    {
                        *pOut << " ";
                        printArgument(*(arg.subParams[j]->arguments[k]));
                    }
                    printRequiresList(arg.subParams[j]->requires);
                    *pOut << std::endl;
                    printCommentList(arg.subParams[j]->comments);

                }
            }
        }

    }
}

void ConsolePrinter::printArgument(const ArgumentDef & argument, int v)
{
    *pOut << "<" << argument.name;
    if (argument.hasDefault)
        *pOut << "=" << argument.argDefault;
    *pOut << ">";
}

void ConsolePrinter::printCommentList(const CommentList &comments, int v)
{
    for (size_t i = 0; i < comments.size(); ++i)
        if (comments.visibility[i] <= v)
            *pOut << "          " << comments.comments[i] << std::endl;
}

void ConsolePrinter::printToken(ArgToken * token)
{

    std::cerr << "token: '" << token->lexeme
    << "' type: " << ArgToken::typeString(token->type)
    << " line: " << token->line + 1
    << " pos: " << token->start + 1 << std::endl;
}

//-------------------   TK PRINTER IMPLEMENTATIONS   --------------------------------

TkPrinter::TkPrinter()
{
    FileName dir = xmippBaseDir();
    dir.append("/applications/scripts/program_gui/program_gui.py");
    output = popen(dir.c_str(), "w");
}

TkPrinter::~TkPrinter()
{
    pclose(output);
}

void TkPrinter::printProgram(const ProgramDef &program, int v)
{
    //    *pOut << "PROGRAM" << std::endl << "   " << program.name << std::endl;
    fprintf(output, "XMIPP %d.%d - %s\n", XMIPP_MAJOR, XMIPP_MINOR, program.name.c_str());
    //Send number of usage lines
    fprintf(output, "%d\n", program.usageComments.size());

    if (program.usageComments.size() > 0)
    {
        for (size_t i = 0; i < program.usageComments.size(); ++i)
            fprintf(output, "%s\n", program.usageComments.comments[i].c_str());
    }

    for (size_t i = 0; i < program.sections.size(); ++i)
        printSection(*program.sections[i], v);
}

void TkPrinter::printSection(const SectionDef &section, int v)
{
    if (section.visible <= v)
    {
        //Just ignore in the GUI this section
        if (section.name == " Common options ")
            return;

        //if (section.name.length() > 0)
        fprintf(output, "section = self.addSection('%s');\n", section.name.c_str());
        bool first_group = true;
        for (size_t i = 0; i < section.params.size(); ++i)
        {
            if (section.params[i]->visible <= v)
            {
                if (!section.params[i]->orBefore)
                {
                    const char * single = (i < section.params.size()-1 && section.params[i+1]->orBefore) ? "False" : "True";

                    if (!first_group)
                        fprintf(output, "section.addGroup(group);\n");
                    else
                        first_group = false;
                    fprintf(output, "group = ParamsGroup(section, %s);\n", single);

                }
                printParam(*section.params[i], v);
            }
        }
        //close last open group
        if (!first_group)
            fprintf(output, "section.addGroup(group);\n");
    }
}

void TkPrinter::printParam(const ParamDef &param, int v)
{
    if (param.visible <= v)
    {
        //Independent params are some kind of special ones
        if (param.independent)
            return;

        int n_args = param.arguments.size();

        fprintf(output, "param = ParamWidget(group, \"%s\");\n", param.name.c_str());
        if (param.notOptional)
            fprintf(output, "param.notOptional = True; \n");
        for (size_t i = 0; i < param.arguments.size(); ++i)
        {
            printArgument(*param.arguments[i], v);
        }
        //Add comments to the help
        for (size_t i = 0; i < param.comments.size(); ++i)
            //if (param.comments.visibility[i] <= v)
            fprintf(output, "param.addCommentLine('''%s''');\n", param.comments.comments[i].c_str());
        //End with options of the param
        fprintf(output, "param.endWithOptions();\n");

    }
}

void TkPrinter::printArgument(const ArgumentDef & argument, int v)
{
    static String paramStr = "param";
    fprintf(output, "%s.addOption(\"%s\", \"%s\", %d);\n",
            paramStr.c_str(), argument.name.c_str(), argument.argDefault.c_str(), argument.subParams.size());
    if (argument.subParams.size() > 0)
    {

        for (size_t j = 0; j < argument.subParams.size(); ++j)
        {
            fprintf(output, "subparam = param.addSubParam(\"%s\");\n",
                    argument.subParams[j]->name.c_str());
            for (size_t i = 0; i < argument.subParams[j]->comments.size(); ++i)
                fprintf(output, "subparam.addCommentLine('''%s''');\n", argument.subParams[j]->comments.comments[i].c_str());
            paramStr = "subparam";
            for (size_t k = 0; k < argument.subParams[j]->arguments.size(); ++k)
            {
                printArgument(*(argument.subParams[j]->arguments[k]));
            }
            paramStr = "param";
        }
    }
}

//--------- WIKI  PRINTER -----------------------
WikiPrinter::WikiPrinter(std::ostream & out)
{
    this->pOut = &out;
}

void WikiPrinter::printProgram(const ProgramDef &program, int v)
{
    //print program name and usage
    *pOut << "---+ !!" << program.name << " (v" << XMIPP_MAJOR <<"." << XMIPP_MINOR << ")" << std::endl;
    *pOut << "%TOC%" << std::endl;
    //print usage
    if (program.usageComments.size() > 0)
    {
        *pOut << "---++ Usage" << std::endl;
        for (size_t i = 0; i < program.usageComments.size(); ++i)
        	switch (program.usageComments.visibility[i])
        	{
        	case 1:
            	*pOut << "   <pre>" << program.usageComments.comments[i] << "</pre>\n";
            	break;
        	default:
            	*pOut << "   " << program.usageComments.comments[i] << std::endl;
        	}
    }
    if (!program.seeAlso.empty())
    {
      *pOut << std::endl << "*See also* %BR%" << std::endl;
      StringVector links;
      splitString(program.seeAlso, ",", links);
      for (int i = 0; i < links.size(); ++i)
        *pOut << "[[" << links[i] << "_v" << XMIPP_MAJOR << "][" << links[i] <<"]]  ";
      *pOut << "%BR%" << std::endl;
    }
    //print sections and params
    if (program.sections.size() > 0)
    {
        *pOut << std::endl << "*Parameters*" << std::endl;
        for (size_t i = 0; i < program.sections.size(); ++i)
            printSection(*program.sections[i], v);
    }
    //print examples
    if (program.examples.size() > 0)
    {
        *pOut << "---++ Examples" << std::endl;
        bool verbatim = false;
        for (size_t i = 0; i < program.examples.size(); ++i)
        {
          if (program.examples.visibility[i])
          {
              if (!verbatim)
              {
                  *pOut << "<pre>" << std::endl;
                  verbatim = true;
              }
          }
          else
          {
            if (verbatim)
            {
              *pOut << "</pre>" << std::endl;
              verbatim = false;
            }
          }
            *pOut << "   " << program.examples.comments[i] << std::endl;
        }
        if (verbatim)
          *pOut << "</pre>" << std::endl;
    }
    //print user comments
    *pOut << "---++ User's comments" << std::endl;
    *pOut << "%COMMENT{type=\"tableappend\"}%" << std::endl;
}

void WikiPrinter::printSection(const SectionDef &section, int v)
{
    if (section.name != " Common options "
         && section.visible <= v)
    {
        *pOut << std::endl;
        String name = section.name;
        trim(name);
        if (name.length() > 0)
            *pOut << "_" << name << "_" << std::endl;
        for (size_t i = 0; i < section.params.size(); ++i)
            printParam(*section.params[i], v);
    }
}

void WikiPrinter::printRequiresList(StringVector requires)
{
    if (!requires.empty())
    {
        *pOut << " ( requires ";
        for (size_t i = 0; i < requires.size(); ++i)
            *pOut << requires[i] << " ";
        *pOut << ")";
    }
}

void WikiPrinter::printParam(const ParamDef &param, int v)
{
    //* =%BLUE%-i [selfile] %ENDCOLOR%= This file contains all the images that are to build the 3D reconstruction
    //* =%GREEN% -o [output file root name] %ENDCOLOR%= If you don't supply this parameter, the same as the input selection one is taken without extension. If you give, for instance, =-o art0001= the following files are created:
    if (param.visible <= v)
    {
        *pOut << "   $";

        if (param.orBefore)
            *pOut << " or";

        String color = param.notOptional ? "BLUE" : "GREEN";

        *pOut << " =%" << color << "%" << param.name;
        //print alias
        for (size_t i = 0; i < param.aliases.size(); ++i)
            *pOut << ", " << param.aliases[i];
        //print arguments
        for (size_t i = 0; i < param.arguments.size(); ++i)
        {
            *pOut << " ";
            printArgument(*param.arguments[i], v);
        }
        *pOut << " %ENDCOLOR%=";
        printRequiresList(param.requires);
        *pOut <<": " ;
        printCommentList(param.comments, v);

        if (param.comments.size() == 0)
            *pOut << "%BR%" << std::endl;

        for (size_t i = 0; i < param.arguments.size(); ++i)
        {
            ArgumentDef &arg = *param.arguments[i];
            if (!arg.subParams.empty())
            {
                *pOut << "      where &lt;" << arg.name << "&gt; can be:" << std::endl;
                for (size_t j = 0; j < arg.subParams.size(); ++j)
                {
                    *pOut << "      * %MAROON% " << arg.subParams[j]->name;
                    for (size_t k = 0; k < arg.subParams[j]->arguments.size(); ++k)
                    {
                        *pOut << " ";
                        printArgument(*(arg.subParams[j]->arguments[k]));
                    }
                    *pOut << " %ENDCOLOR%" << std::endl;
                    printRequiresList(arg.subParams[j]->requires);
                    //                    *pOut << std::endl;
                    printCommentList(arg.subParams[j]->comments);

                }
            }
        }

    }
}

void WikiPrinter::printArgument(const ArgumentDef & argument, int v)
{
    *pOut << "&lt;" << argument.name;
    if (argument.hasDefault)
        *pOut << "=" << argument.argDefault;
    *pOut << "&gt;";
}

void WikiPrinter::printCommentList(const CommentList &comments, int v)
{
    for (size_t i = 0; i < comments.size(); ++i)
        if (comments.visibility[i] <= v)
            *pOut << "          " << comments.comments[i] << "%BR%" << std::endl;
}

void WikiPrinter::printToken(ArgToken * token)
{

    std::cerr << "token: '" << token->lexeme
    << "' type: " << ArgToken::typeString(token->type)
    << " line: " << token->line + 1
    << " pos: " << token->start + 1 << std::endl;
}
