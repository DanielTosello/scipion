/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package xmipp.viewer.scipion;

import java.util.logging.Level;
import java.util.logging.Logger;
import xmipp.jni.Filename;
import xmipp.jni.ImageGeneric;
import xmipp.jni.MetaData;
import xmipp.utils.DEBUG;
import xmipp.utils.Params;
import xmipp.utils.StopWatch;
import xmipp.utils.XmippDialog;
import xmipp.viewer.Viewer;
import xmipp.viewer.particlepicker.Micrograph;
import xmipp.viewer.windows.GalleryJFrame;
import static xmipp.viewer.windows.ImagesWindowFactory.openFileAsImage;

/**
 *
 * @author airen
 */
public class ScipionViewer extends Viewer {
    
    public static ScipionParams parameters;
    
    
    public static boolean isScipion()
    {
        if(parameters == null)
            return false;
        return parameters.isScipion();
                
    }

    public static void main(String args[]) {
        StopWatch stopWatch = StopWatch.getInstance();
        stopWatch.printElapsedTime("starting app");
        // Schedule a job for the event dispatch thread:
        // creating and showing this application's GUI.
        final String[] myargs = args;

        parameters = new ScipionParams(myargs);
        try {
            if (parameters.debug) {
                DEBUG.enableDebug(true);
            }

            if (parameters.files != null) {
                // DEBUG.enableDebug(true);
                final String[] files = parameters.files;

                for (int i = 0; i < files.length; i++) 
                    openFile(files[i], parameters);
                
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

    }
    


    public static void openFile(String filename, ScipionParams parameters) {
        
        try {
            
            MetaData md;
            if(filename.endsWith(".sqlite") || filename.endsWith(".db"))
            {
                StopWatch.getInstance().printElapsedTime("reading db");
                ScipionGalleryData data = new ScipionGalleryData(null, filename, parameters);
                ScipionGalleryJFrame frame = new ScipionGalleryJFrame(data);
                
            }
            else if (Filename.isMetadata(filename)) {
                if (parameters.mode.equalsIgnoreCase(Params.OPENING_MODE_IMAGE)) {
                    openFileAsImage(null, filename, parameters);
                } else {
                    parameters.mode = Params.OPENING_MODE_GALLERY;
                    md = new MetaData(filename);
                    new GalleryJFrame(md, parameters);
                }
            } else {
                ImageGeneric img = new ImageGeneric(filename);

                if (img.isSingleImage()) {
                    openFileAsImage(null, filename, parameters);
                } else if (img.isStackOrVolume()) {
                    if (parameters.mode
                            .equalsIgnoreCase(Params.OPENING_MODE_IMAGE)) {
                        openFileAsImage(null, filename, parameters);
                    } else {
                        parameters.mode = Params.OPENING_MODE_GALLERY;
                        new GalleryJFrame(filename, parameters);
                    }
                }
            }
            
        } catch (Exception e) {
            XmippDialog.showError(null, String.format(
                    "Couldn't open file: '%s'\nError: %s", filename,
                    e.getMessage()));
            DEBUG.printException(e);
        }
    }

   

}
