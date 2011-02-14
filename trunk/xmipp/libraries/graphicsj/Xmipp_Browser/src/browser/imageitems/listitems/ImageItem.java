/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package browser.imageitems.listitems;

import browser.Cache;
import browser.LABELS;
import browser.imageitems.ImageDimension;
import ij.IJ;
import ij.ImagePlus;
import java.awt.Image;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.Iterator;
import javax.imageio.ImageIO;
import javax.imageio.ImageReadParam;
import javax.imageio.ImageReader;
import javax.imageio.stream.ImageInputStream;

/**
 *
 * @author Juanjo Vega
 */
public class ImageItem extends AbstractImageItem {

    public ImageItem(File file, Cache cache) {
        super(file, cache);
    }

    @Override
    protected void loadImageData() {
        try {
            // Tries to get a reader for the given file...
            ImageInputStream iis = ImageIO.createImageInputStream(file);
            Iterator readers = ImageIO.getImageReaders(iis);

            if (readers.hasNext()) {
                // Gets the reader.
                ImageReader imgReader = (ImageReader) readers.next();

                // Creates input stream to read.
                ImageInputStream imageInputStream = ImageIO.createImageInputStream(new BufferedInputStream(new FileInputStream(file)));
                imgReader.setInput(imageInputStream);

                // Stores image info.
                dimension = new ImageDimension();
                dimension.width = imgReader.getWidth(0);
                dimension.height = imgReader.getHeight(0);

                // Closes input stream.
                imageInputStream.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public ImagePlus loadPreview(int w, int h) {
        Image preview = null;

        try {
            // Tries to get a reader for the given file...
            ImageInputStream iis = ImageIO.createImageInputStream(file);
            Iterator readers = ImageIO.getImageReaders(iis);

            if (readers.hasNext()) {
                // Gets the reader.
                ImageReader imgReader = (ImageReader) readers.next();

                // Creates input stream to read.
                ImageInputStream imageInputStream = ImageIO.createImageInputStream(new BufferedInputStream(new FileInputStream(file)));
                imgReader.setInput(imageInputStream);

                // Calculates and sets parameters for subsampling.
                int longEdge = (Math.max(dimension.width, dimension.height));
                int subSampleX = (int) (longEdge / w * 2.f);
                int subSampleY = (int) (longEdge / h * 2.f);

                final ImageReadParam readParam = imgReader.getDefaultReadParam();

                if (subSampleX > 1) {
                    readParam.setSourceSubsampling(subSampleX, subSampleY, 0, 0);
                }

                preview = imgReader.read(0, readParam);

                // Closes input stream.
                imageInputStream.close();
            }
        } catch (Exception ex) {
        }

        return preview != null ? new ImagePlus("", preview) : null;
    }

    public String getImageInfo() {
        loadImageData();

        System.out.println("W=" + dimension.width + " / H=" + dimension.height);

        return "<html>"
                + LABELS.LABEL_WIDTH + dimension.width + "<br>"
                + LABELS.LABEL_HEIGHT + dimension.height + "<br>"
                + "</html>";
    }

    @Override
    public ImagePlus getImagePlus() {
        return IJ.openImage(file.getAbsolutePath());
    }
}
