/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package browser.imageitems.listitems;

import browser.Cache;
import browser.DEBUG;
import browser.imageitems.ImageConverter;
import ij.IJ;
import ij.ImagePlus;
import java.io.File;
import xmipp.Filename;
import xmipp.MDLabel;
import xmipp.MetaData;

/**
 *
 * @author Juanjo Vega
 */
public class MetadataFileItem extends XmippImageItem {

    protected String previewFile;
    protected MetaData md;

    public MetadataFileItem(File file, Cache cache) {
        super(file, cache);
    }

    @Override
    protected void loadImageData() {
        try {
            String path = file.getAbsolutePath();
            md = new MetaData();
            md.read(path);

            previewFile = getPreviewFile(md);
            if (previewFile != null) {
                loadPreviewFileData();
            }

            dimension.setNImages(md.size());
        } catch (Exception ex) {
            DEBUG.printException(ex);
            IJ.error(ex.getMessage());
        }
    }

    // Gets first available filename if any.
    protected String getPreviewFile(MetaData md) {
        if (md.containsLabel(MDLabel.MDL_IMAGE)) {
            File f;

            long objs[] = md.findObjects();

            for (long id : objs) {
                String field = md.getValueString(MDLabel.MDL_IMAGE, id, true);

                field = Filename.getFilename(field);      // Avoids image@filename format ;)
                nimage = Filename.getNimage(field);

                f = new File(field);

                if (f.exists()) {
//                    System.err.println(" *** preview File: " + f.getAbsolutePath());
                    return f.getAbsolutePath();
                }
            }
        }

        return null;
    }

    protected void loadPreviewFileData() {
        // Tricky way to get preview file's data.
        File originalFile = file;

        file = new File(previewFile);

        super.loadImageData();

        file = originalFile;
    }

    @Override
    public ImagePlus getPreview(int w, int h) {
        // Tricky way to get preview.
        File originalFile = file;
        file = new File(Filename.getFilename(previewFile)); // skips image@filename format

        ImagePlus ip = super.getPreview(w, h);

        file = originalFile;

        return ip;
    }

    @Override
    public ImagePlus getImagePlus() {
        return ImageConverter.convertToImagej(md);
    }
}
