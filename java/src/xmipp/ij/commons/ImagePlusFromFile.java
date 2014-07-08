/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package xmipp.ij.commons;

import ij.ImagePlus;
import java.io.File;
import xmipp.jni.Filename;
import xmipp.jni.ImageGeneric;

/**
 *
 * @author airen
 */
public class ImagePlusFromFile extends ImagePlusReader{
    
        
        private String fileName;
        private long modified;
        
        public ImagePlusFromFile(String fileName)
        {
            if (fileName == null || fileName.equals(""))
                throw new IllegalArgumentException("empty file");
            this.fileName = fileName;
            this.modified = new File(fileName).lastModified();
            
        }
        
        public ImagePlusFromFile(String fileName, ImagePlus imp, ImageGeneric ig)
        {
            this(fileName);
            this.imp = imp;
            this.ig = ig;
        }
        
       
        
        @Override
    	public ImagePlus loadImagePlus()
	{
		imp = null;
		try
		{
			if (ig == null || hasChanged())
                        {
                            ig = new ImageGeneric(fileName);//to read again file
                            if(index == -1)
                                imp = XmippImageConverter.readToImagePlus(ig);
                            else 
                            {
                                if(ig.isStack())
                                    imp = XmippImageConverter.readToImagePlus(ig, index);
                                else
                                    imp = XmippImageConverter.readToImagePlus(ig, (int)index);//read slice
                            }
                        }
                        else if(ig != null)
                        {
                             if(index == -1 || !ig.isStackOrVolume())
                                imp = XmippImageConverter.convertToImagePlus(ig);
                            else 
                             {
                                 if(ig.isStack())
                                    imp = XmippImageConverter.convertToImagePlus(ig, index);
                                else
                                    imp = XmippImageConverter.convertToImagePlus(ig, (int)index);//read slice
                             }
                        }
                        checkResizeAndGeo();
			if(normalize)
			{
				imp.getProcessor().setMinAndMax(normalize_min, normalize_max);
				imp.updateImage();
			}
			return imp;
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		return imp;
	}
    
        public boolean hasChanged()
	{
		return new File(fileName).lastModified() > modified;
	}
        

	public String getFileName()
	{
		return fileName;
	}

    @Override
    public boolean getAllowsPoll() {
        return true;
    }

    @Override
    public String getName() {

        String name;
        if(index != -1)
            name = String.format("%d@%s", index, fileName);
        else
            name = fileName;
        return name;

    }
        
        
}
