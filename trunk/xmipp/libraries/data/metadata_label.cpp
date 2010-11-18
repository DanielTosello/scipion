/***************************************************************************
 *
 * Authors:    J.M. De la Rosa Trevin (jmdelarosa@cnb.csic.es)
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
#include "metadata_label.h"

//This is needed for static memory allocation
std::map<MDLabel, MDLabelData> MDL::data;
std::map<std::string, MDLabel> MDL::names;
MDLabelStaticInit MDL::initialization; //Just for initialization

void MDL::addLabel(const MDLabel label, const MDLabelType type, const String &name, const String &name2, const String &name3)
{
    data[label] = MDLabelData(type, name);
    names[name] = label;
    if (name2 != "")
        names[name2] = label;
    if (name3 != "")
        names[name3] = label;
}//close function addLabel

MDLabel  MDL::str2Label(const String &labelName)
{
    if (names.find(labelName) == names.end())
        return MDL_UNDEFINED;
    return names[labelName];
}//close function str2Label

String  MDL::label2Str(const MDLabel label)
{
    if (data.find(label) == data.end())
        return "";
    return data[label].str;
}//close function label2Str

String MDL::label2SqlColumn(const MDLabel label)
{
    std::stringstream ss;
    ss << MDL::label2Str(label) << " ";
    switch (MDL::labelType(label))
    {
    case LABEL_BOOL: //bools are int in sqlite3
    case LABEL_INT:
        ss << "INTEGER";
        break;
    case LABEL_DOUBLE:
        ss << "REAL";
        break;
    case LABEL_STRING:
        ss << "TEXT";
        break;
    case LABEL_VECTOR:
        ss << "TEXT"; //FIXME: This is provisional
        break;
    }
    return ss.str();
}

bool MDL::isInt(const MDLabel label)
{
    return (data[label].type == LABEL_INT);
}
bool MDL::isBool(const MDLabel label)
{
    return (data[label].type == LABEL_BOOL);
}
bool MDL::isString(const MDLabel label)
{
    return (data[label].type == LABEL_STRING);
}
bool MDL::isDouble(const MDLabel label)
{
    return (data[label].type == LABEL_DOUBLE);
}
bool MDL::isVector(const MDLabel label)
{
    return (data[label].type == LABEL_VECTOR);
}

bool MDL::isValidLabel(const MDLabel label)
{
    return (label > MDL_UNDEFINED && label < MDL_LAST_LABEL);
}

bool MDL::isValidLabel(const String &labelName)
{
    return isValidLabel(str2Label(labelName));
}

MDLabelType MDL::labelType(const MDLabel label)
{
    return data[label].type;
}

MDLabelType MDL::labelType(const String &labelName)
{
    return data[str2Label(labelName)].type;
}


bool vectorContainsLabel(const std::vector<MDLabel>& labelsVector, const MDLabel label)
{
    std::vector<MDLabel>::const_iterator location;
    location = std::find(labelsVector.begin(), labelsVector.end(), label);

    return (location != labelsVector.end());
}

void MDObject::copy(const MDObject &obj)
{
    label = obj.label;
    type = obj.type;
    if (type == LABEL_STRING)
    {
      delete data.stringValue;
      data.stringValue = new String(*(obj.data.stringValue));
    }
    else if (type == LABEL_VECTOR)
    {
      delete data.vectorValue;
        data.vectorValue = new std::vector<double>(*(obj.data.vectorValue));
    }
    else
      data = obj.data;
}

MDObject::MDObject(const MDObject & obj)
{
  data.doubleValue = 0;
    copy(obj);
}
MDObject & MDObject::operator = (const MDObject &obj)
{
  data.doubleValue = 0;
  copy(obj);
    return *this;
}

//----------- Implementation of MDValue -----------------
inline void MDObject::labelTypeCheck(MDLabelType checkingType) const
{
    if (this->type != checkingType)
    {
        std::stringstream ss;
        ss << "Mismatch Label (" << MDL::label2Str(label) << ") and value type(";
        switch (checkingType)
        {
        case LABEL_INT:
            ss << "int";
            break;
        case LABEL_LONG:
            ss << "long int";
            break;
        case LABEL_STRING:
            ss << "string";
            break;
        case LABEL_BOOL:
            ss << "bool";
            break;
        case LABEL_VECTOR:
            ss << "vector";
            break;
        default:
            ss << "weird: " << checkingType;
        }
        ss << ")";
        REPORT_ERROR(ERR_MD_BADLABEL, ss.str());
    }
}

//Just a simple constructor with the label
//dont do any type checking as have not value yet
MDObject::MDObject(MDLabel label)
{
    this->label = label;
    if (label != MDL_UNDEFINED)
    {
        type = MDL::labelType(label);
        if (type == LABEL_STRING)
            data.stringValue = new String;
        else if (type == LABEL_VECTOR)
            data.vectorValue = new std::vector<double>;
    }
    else
    	type = LABEL_NOTYPE;
}
///Constructors for each Label supported type
///these constructor will do the labels type checking
MDObject::MDObject(MDLabel label, const int &intValue)
{
    this->label = label;
    this->type = MDL::labelType(label);
    labelTypeCheck(LABEL_INT);
    this->data.intValue = intValue;
}
MDObject::MDObject(MDLabel label, const double &doubleValue)
{
    this->label = label;
    this->type = MDL::labelType(label);
    labelTypeCheck(LABEL_DOUBLE);
    this->data.doubleValue = doubleValue;
}
MDObject::MDObject(MDLabel label, const bool &boolValue)
{
    this->label = label;
    this->type = MDL::labelType(label);
    labelTypeCheck(LABEL_BOOL);
    this->data.boolValue = boolValue;
}
MDObject::MDObject(MDLabel label, const String &stringValue)
{
    this->label = label;
    this->type = MDL::labelType(label);
    labelTypeCheck(LABEL_STRING);
    this->data.stringValue = new String(stringValue);
}
MDObject::MDObject(MDLabel label, const std::vector<double> &vectorValue)
{
    this->label = label;
    this->type = MDL::labelType(label);
    labelTypeCheck(LABEL_VECTOR);
    this->data.vectorValue = new std::vector<double>(vectorValue);
}
MDObject::MDObject(MDLabel label, const long int longintValue)
{
    this->label = label;
    this->type = MDL::labelType(label);
    labelTypeCheck(LABEL_LONG);
    this->data.longintValue = longintValue;

}
MDObject::MDObject(MDLabel label, const float &floatValue)
{
    std::cerr << "Do not use setValue with floats, use double"<< std::endl;
    std::cerr << "Floats are banned from metadata class"<< std::endl;
    exit(1);
}
MDObject::MDObject(MDLabel label, const char* &charValue)
{
    std::cerr << "Do not use setValue with char, use string"<< std::endl;
    std::cerr << "chars are banned from metadata class"<< std::endl;
    exit(1);
}

MDObject::~MDObject()
{
    if (type == LABEL_STRING)
        delete data.stringValue;
    else if (type == LABEL_VECTOR)
        delete data.vectorValue;
}

//These getValue also do a compilation type checking
//when expanding templates functions and only
//will allow the supported types
//TODO: think if the type check if needed here
void MDObject::getValue(int &iv) const
{
    labelTypeCheck(LABEL_INT);
    iv = this->data.intValue;
}
void MDObject::getValue(double &dv) const
{
    labelTypeCheck(LABEL_DOUBLE);
    dv = this->data.doubleValue;
}
void MDObject::getValue(bool &bv) const
{
    labelTypeCheck(LABEL_BOOL);
    bv = this->data.boolValue;
}
void MDObject::getValue(String &sv) const
{
    labelTypeCheck(LABEL_STRING);
    sv = *(this->data.stringValue);
}
void  MDObject::getValue(std::vector<double> &vv) const
{
    labelTypeCheck(LABEL_VECTOR);
    vv = *(this->data.vectorValue);
}
void MDObject::getValue(long int &lv) const
{
    labelTypeCheck(LABEL_INT);
    lv = this->data.longintValue;
}
void MDObject::getValue(float &floatvalue) const
{
    std::cerr << "Do not use setValue with floats, use double"<< std::endl;
    std::cerr << "Floats are banned from metadata class"<< std::endl;
    exit(1);
}
void MDObject::getValue(char*  &charvalue) const
{
    std::cerr << "Do not use setValue with char, use string"<< std::endl;
    std::cerr << "chars are banned from metadata class"<< std::endl;
    exit(1);
}

void MDObject::setValue(const int &iv)
{
    labelTypeCheck(LABEL_INT);
    this->data.intValue = iv;
}
void MDObject::setValue(const double &dv)
{
    labelTypeCheck(LABEL_DOUBLE);
    this->data.doubleValue = dv;
}

void MDObject::setValue(const bool &bv)
{
    labelTypeCheck(LABEL_BOOL);
    this->data.boolValue = bv;
}

void MDObject::setValue(const String &sv)
{
    labelTypeCheck(LABEL_STRING);
    //if (this->data.stringValue == NULL)
    //  this->data.stringValue = new String(sv);
    //else
    *(this->data.stringValue) = sv;
}
void  MDObject::setValue(const std::vector<double> &vv)
{
    labelTypeCheck(LABEL_VECTOR);
    //if (this->data.vectorValue == NULL)
    //  this->data.vectorValue = new std::vector<double>(vv);
    //else
    *(this->data.vectorValue) = vv;
}
void MDObject::setValue(const long int &lv)
{
    labelTypeCheck(LABEL_INT);
    this->data.longintValue = lv;
}
void MDObject::setValue(const float &floatvalue)
{
    std::cerr << "Do not use setValue with floats, use double"<< std::endl;
    std::cerr << "Floats are banned from metadata class"<< std::endl;
    exit(1);
}
void MDObject::setValue(const char*  &charvalue)
{
    std::cerr << "Do not use setValue with char, use string"<< std::endl;
    std::cerr << "chars are banned from metadata class"<< std::endl;
    exit(1);
}
#define DOUBLE2STREAM(d) \
    if (withFormat) {\
            (os) << std::setw(12); \
            (os) << (((d) != 0. && ABS(d) < 0.001) ? std::scientific : std::fixed);\
        } os << d;

#define INT2STREAM(i) \
    if (withFormat) os << std::setw(10); \
    os << i;

void MDObject::toStream(std::ostream &os, bool withFormat, bool isSql) const
{
    String c = (isSql) ? "'" : "";

    if (label == MDL_UNDEFINED) //if undefine label, store as a literal string
        os << data.stringValue;
    else
        switch (MDL::labelType(label))
        {
        case LABEL_BOOL: //bools are int in sqlite3
            os << data.boolValue;
            break;
        case LABEL_INT:
            INT2STREAM(data.intValue);
            break;
        case LABEL_LONG:
            INT2STREAM(data.longintValue);
            break;
        case LABEL_DOUBLE:
            DOUBLE2STREAM(data.doubleValue);
            break;
        case LABEL_STRING:
            os << c << *(data.stringValue) << c;
            break;
        case LABEL_VECTOR:
            os << "[ ";
            int size = data.vectorValue->size();
            for (int i = 0; i < size; i++)
            {
                DOUBLE2STREAM(data.vectorValue->at(i));
                os << " ";
            }
            os << "]";
            break;
        }//close switch
}//close function toStream

String MDObject::toString(bool withFormat, bool isSql) const
{
    std::stringstream ss;
    toStream(ss, withFormat, isSql);
    return ss.str();
}

//bool MDValue::fromStream(std::istream &is)
std::ostream& operator<< (std::ostream& os, MDObject &value)
{
    value.toStream(os);
    return os;
}

//bool MDValue::fromStream(std::istream &is)
std::istream& operator>> (std::istream& is, MDObject &value)
{
    value.fromStream(is);
    return is;
}

bool MDObject::fromStream(std::istream &is)
{
    if (label == MDL_UNDEFINED) //if undefine label, store as a literal string
    {
    	String s;
    	is >> s;
    }
    	else
    {
        // int,bool and long are read as double for compatibility with old doc files
        double d;
        switch (type)
        {
        case LABEL_BOOL: //bools are int in sqlite3
            is >> d;
            data.boolValue = (bool) ((int)d);
            break;
        case LABEL_INT:
            is >> d;
            data.intValue = (int) d;
            break;
        case LABEL_LONG:
            is >> d;
            data.longintValue = (long int) d;
            break;
        case LABEL_DOUBLE:
            is >> data.doubleValue;
            break;
        case LABEL_STRING:
            //if (data.stringValue == NULL)
            //    data.stringValue = new std::string;
            is >> *(data.stringValue);
            break;
        case LABEL_VECTOR:
            is.ignore(256, '[');
            //if (data.vectorValue == NULL)
            //  data.vectorValue = new std::vector<double>;
            data.vectorValue->clear();
            while (is >> d) //This will stop at ending "]"
                data.vectorValue->push_back(d);
            is.clear(); //this is for clear the fail state after found ']'
            is.ignore(256, ']'); //ignore the ending ']'
            break;
        }
    }
    return is.good();
}

bool MDObject::fromString(const String&str)
{
    std::stringstream ss(str);
    fromStream(ss);
}

bool MDObject::fromChar(const char * szChar)
{
    std::stringstream ss(szChar);
    fromStream(ss);
}
//MDObject & MDRow::operator [](MDLabel label)
//{
//    for (iterator it = begin(); it != end(); ++it)
//        if ((*it)->label == label)
//            return *(*it);
//    MDObject * pObj = new MDObject(label);
//    push_back(pObj);
//
//    return *pObj;
//}

bool MDRow::containsLabel(MDLabel label) const
{
    for (const_iterator it = begin(); it != end(); ++it)
        if ((*it)->label == label)
            return true;
    return false;
}

MDObject * MDRow::getObject(MDLabel label)
{
    for (const_iterator it = begin(); it != end(); ++it)
        if ((*it)->label == label)
            return *it;
    return NULL;
}
MDRow::~MDRow()
{
    for (iterator it = begin(); it != end(); ++it)
        delete *it;
}

MDRow::MDRow(const MDRow & row)
{
    copy(row);
}

MDRow::MDRow():std::vector<MDObject*>()
{}

MDRow& MDRow::operator = (const MDRow &row)
{
    copy(row);
    return *this;
}

void MDRow::copy(const MDRow &row)
{
    resize(row.size());
    int pos = 0;
    for (iterator it = begin(); it != end(); ++it)
    {
        if (*it == NULL)
            *it = new MDObject(*(row[pos]));
        else
            *(*it) = *(row[pos]);
        ++pos;
    }
}

std::ostream& operator << (std::ostream &out, const MDRow &row)
{
    for (MDRow::const_iterator it = row.begin(); it != row.end(); ++it)
    {
        (*it)->toStream(out);
        out << " ";
    }
    return out;
}
