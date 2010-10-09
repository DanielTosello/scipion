%{
#include "../metadata.h"
#include "../metadata_extension.h"
//#include "../metadata_sql.h"
#include "../metadata_label.h"
%}
%ignore MetaData::write(std::ostream &);
%ignore ImgSize(MetaData const &,int &,int &,int &,int &);
%ignore ImgSize(MetaData const &,int &,int &,int &);
%ignore ImgSize(MetaData const &,int &,int &);
%ignore ImgSize(MetaData const &,int &);
//%ignore ImgSize(FileName const &,int &,int &,int &,int &);
%ignore ImgSize(FileName const &,int &,int &,int &);
%ignore ImgSize(FileName const &,int &,int &);
%ignore ImgSize(FileName const &,int &);

//%rename(MetaDataWrite) MetaData::write(FileName const &);


%include ../metadata.h
%include ../metadata_extension.h
%include ../metadata_sql.h
%include ../metadata_label.h
namespace std {
   %template(vectorm) vector<MDLabel>;
};

%template(setValueBool)      setValueSwig<bool>; 
%template(setValueInt)       setValueSwig<int>; 
%template(setValueDouble)    setValueSwig<double>; 
%template(setValueString)    setValueSwig<std::string>;

%template(getValueBool)      getValueSwig<bool>; 
%template(getValueInt)       getValueSwig<int>; 
%template(getValueDouble)    getValueSwig<double>; 
%template(getValueString)    getValueSwig<std::string>;

%template(MDValueEqualDouble) MDValueEqualSwig<double>; 
%template(MDValueRangeDouble) MDValueRangeSwig<double>; 
%template(MDValueAboveDouble) MDValueAboveSwig<double>; 
%template(MDValueBelowDouble) MDValueBelowSwig<double>;

%template(setValueColBool)      setValueColSwig<bool>; 
%template(setValueColInt)       setValueColSwig<int>; 
%template(setValueColDouble)    setValueColSwig<double>; 
%template(setValueColString)    setValueColSwig<std::string>;

/*
==================
First example
==================
python
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
#create metadata object
outFile=XmippData.FileName('kk')
mD=XmippData.MetaData();
#create metadatacontainer (one line of metadata)
id=mD.addObject();
valor=10.3
#fill an object data
ss=XmippData.stringP()
dd=XmippData.doubleP()
dd2=XmippData.doubleP()
ss.assign( "image0001.xmp")
dd.assign(10.3)
dd2.assign(22.3)
XmippData.setValueString( mD, XmippData.MDL_IMAGE, ss)
XmippData.setValueDouble( mD, XmippData.MDL_ANGLEPSI, dd)
XmippData.setValueDouble( mD, XmippData.MDL_ANGLEROT, dd2)


mD2=XmippData.MetaData();

mD2.read(outFile)
ss=XmippData.stringP()
dd=XmippData.doubleP()
getValueString( mD2,XmippData.MDL_IMAGE, ss)
ss.value()
getValueDouble( mD2, XmippData.MDL_ANGLEPSI, dd)
dd.value()
ii=XmippData.intP()
ii.assign(1)
XmippData.setValueInt( mD2, XmippData.MDL_ENABLED, ii)
ii.assign(0)
XmippData.getValueInt( mD2,XmippData.MDL_ENABLED, ii)
print ii.value()


==================
ANOTHER EXAMPLE (xmipp_selfile_create)
==================

#!/usr/bin/env python
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
outFile=XmippData.FileName()
if len(sys.argv) == 1:
   print 'Usage: selfile_create "pattern"  metadataFile'
   sys.exit()
elif len(sys.argv) == 2:
   outFile='/dev/stdout'
elif len(sys.argv) == 3:
   outFile=sys.argv[2]
else:
   print 'Usage   : xmipp_selfile_create "pattern"  metadataFile'
   print 'Example1: xmipp_selfile_create "Images/*xmp"    all_images.sel'
   print 'Example2: xmipp_selfile_create "Images/*xmp"  > all_images.sel'
   sys.exit()


mD=XmippData.MetaData()

files = glob.glob(sys.argv[1])
files.sort()
for file in files:
    mD.addObject();
    mD.setValue( XmippData.MDL_IMAGE, file)
    mD.setValue( XmippData.MDL_ENABLED, 1)
mD.write(outFile)

================================
#Do not use de asigment operator rather use the copy constructor
mD2=XmippData.MetaData(mD);
mD2.write("two")
mD2.setValue( XmippData.MDL_ANGLEROT, 111.2)
mD2.write("three")

=================
import XmippData
outFile=XmippData.FileName('kk')
mD=XmippData.MetaData();
id=mD.addObject();
mD.setValue( XmippData.MDL_ANGLEPSI, 10.3)
b=XmippData.vectord()
b.push_back(12);
b.push_back(45);
mD.setValue(XmippData.MDL_NMA,b)
mD.setValue(XmippData.MDL_ANGLEROT, 11.0)
mD.write(outFile)

import XmippData
inFile=XmippData.FileName('kk')
mD=XmippData.MetaData();
mD.read(inFile)
b=XmippData.vectord();
mD.getValue(XmippData.MDL_NMA,b)
b[0]
b[1]
dd=XmippData.doubleP()
mD.getValue(XmippData.MDL_ANGLEROT,dd) 
print dd.value()

================

import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
outFile=XmippData.FileName("yy.doc")
dbFile=XmippData.FileName("kkk.db")
mD=XmippData.MetaData();
mD.read(outFile)
vv=XmippData.vectorm()
vv.append(XmippData.MDL_IMAGE)
vv.append(XmippData.MDL_ANGLEPSI)

print "here"
mD.toDataBase( dbFile, "prueba" );

mD.toDataBase( dbFile, "prueba",vv );

====================
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
mD=XmippData.MetaData();
dbFile=XmippData.FileName("kkk.db")
outFile=XmippData.FileName("fromdatabase.doc")
vv=XmippData.vectorm()
vv.append(XmippData.MDL_IMAGE)
vv.append(XmippData.MDL_ANGLEPSI)

mD1=XmippData.MetaData();
mD1.fromDataBase( dbFile, "prueba" );
mD1.write("fromdatabase.doc")
mD1.fromDataBase( dbFile, "prueba",vv );

mD.fromDataBase( dbFile, "prueba" );
====================

 XmippData.isBool(XmippData.MDL_ENABLED)
 
 ====================
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
inputMetaDataFile  = XmippData.FileName('kk.ctfparam')
mD = XmippData.MetaData(inputMetaDataFile)
voltage=XmippData.doubleP()
defocusu=XmippData.doubleP()
mD.getValue(XmippData.MDL_DEFOCUSU,defocusu)
mD.getValue(XmippData.MDL_VOLTAGE,voltage)
print defocusu.value(),voltage.value()
mD.setValue(XmippData.MDL_DEFOCUSU,3333.)
mD.write('ww')
==================================
#combinewithfiles
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
inputMetaDataFile   = XmippData.FileName('t.doc')
mD = XmippData.MetaData(inputMetaDataFile)


mD.combineWithFiles(XmippData.MDL_CTFMODEL)
mD.write('ww')

==============
test operator plus
================
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
yy   = XmippData.FileName('yy.doc')
aa   = XmippData.FileName('aa.doc')
mDyy = XmippData.MetaData(yy)
mDaa = XmippData.MetaData(aa)
mDaa.union_(mDyy)
mDaa.write("sum.doc")
mDyy.write("sum2.doc")

==============
test operator minus
================
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
aa   = XmippData.FileName('aa.doc')
bb   = XmippData.FileName('bb.doc')
mDaa = XmippData.MetaData(aa)
mDbb = XmippData.MetaData(bb)
mDresult = XmippData.MetaData()
mDresult.intersection(mDaa,mDbb,XmippData.MDL_IMAGE)
mDresult.write("minus.doc")
==============
test operator substraction
================
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
aa   = XmippData.FileName('aa.doc')
bb   = XmippData.FileName('bb.doc')
mDaa = XmippData.MetaData(aa)
mDbb = XmippData.MetaData(bb)
mDresult = XmippData.MetaData()
mDresult.substraction(mDaa,mDbb,XmippData.MDL_IMAGE)
mDresult.write("substraction.doc")
==============
test operator findObjects in range
================
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData
aa   = XmippData.FileName('aa.doc')
mDaa = XmippData.MetaData(aa)
mDResult = XmippData.MetaData()
mDResult.findObjectsInRange(mDaa,XmippData.MDL_ANGLEROT,3.,100.)
mDResult.write("findObjectsInRange.doc")
==============
test row format
================
import os,glob,sys

scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path

import XmippData
aa   = XmippData.FileName('aa.doc')
MDo = XmippData.MetaData()
MDo.setColumnFormat(False)
MDo.addObject();
MDo.setValue(XmippData.MDL_LL, -88888.);
MDo.setValue(XmippData.MDL_PMAX, 666.67898);
MDo.setValue(XmippData.MDL_SIGMANOISE, 2233.890);
MDo.setValue(XmippData.MDL_SIGMAOFFSET, 22345.8888);
MDo.setValue(XmippData.MDL_SUMWEIGHT, 0.555);

MDo.write("rowformat.doc")

=========================
import os,glob,sys
scriptdir=os.path.split(os.path.dirname(os.popen('which xmipp_protocols','r').read()))[0]+'/lib'
sys.path.append(scriptdir) # add default search path
import XmippData

inMetaDataFile   = XmippData.FileName('rand.doc')
outMetaDataFile  = XmippData.FileName('kk.doc')
MD1=XmippData.MetaData(inMetaDataFile)
MD2=XmippData.MetaData()

MD2.randomize(MD1)

MD2.write(outMetaDataFile)

*/
