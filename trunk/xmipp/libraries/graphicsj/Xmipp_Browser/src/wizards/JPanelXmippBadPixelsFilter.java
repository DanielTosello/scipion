/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package wizards;

import browser.LABELS;
import browser.imageitems.listitems.AbstractImageItem;
import ij.IJ;
import ij.ImagePlus;
import ij.process.FloatProcessor;
import xmipp.ImageDouble;

/**
 *
 * @author Juanjo Vega
 */
public class JPanelXmippBadPixelsFilter extends JPanelXmippGaussianFilter {

    public JPanelXmippBadPixelsFilter(String metadata) {
        this(metadata, 1.0);
    }

    public JPanelXmippBadPixelsFilter(String metadata, double factor) {
        super(metadata, factor);

        jlW1.setText(LABELS.LABEL_BAD_PIXELS_FACTOR);
    }

    public double getFactor() {
        return getW1();
    }

    @Override
    ImagePlus getFilteredPreview(AbstractImageItem item) {
        ImagePlus imp = null;

        try {
            double factor = (Double) jsW1.getValue();
            double pixels[] = ImageDouble.badPixelsFilter(item.getAbsoluteFileName(),
                    factor, previewWidth, previewHeight);

            FloatProcessor ip = new FloatProcessor(
                    previewWidth, previewHeight, pixels);
            imp = new ImagePlus(item.getFileName(), ip);
        } catch (Exception e) {
            IJ.error(e.getMessage());
        }

        return imp;
    }
}
