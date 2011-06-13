/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package browser.imageitems;

import browser.Cache;
import browser.imageitems.listitems.XmippImageItem;
import ij.ImagePlus;
import java.io.File;
import xmipp.ImageDouble;

/**
 *
 * @author Juanjo Vega
 */
public class TableImageItem extends XmippImageItem {

    protected boolean enabled = true;
    protected boolean selected = false;
    protected double scale = 1.0;
    protected String originalValue = null;

    public TableImageItem(File file, String originalValue, Cache cache) {
        this(file, cache);
        this.originalValue = originalValue;
    }

    public TableImageItem(File file, Cache cache) {
        this(file, ImageDouble.FIRST_SLICE, cache);
    }

    public TableImageItem(File file, int slice, Cache cache) {
        this(file, slice, ImageDouble.FIRST_IMAGE, cache);
    }

    public TableImageItem(File file, long nimage, Cache cache) {
        this(file, ImageDouble.FIRST_SLICE, nimage, cache);
    }

    public TableImageItem(File file, int slice, long nimage, Cache cache) {
        super(file, cache);

        this.nslice = slice;
        this.nimage = nimage;
    }

    @Override
    public String getKey() {
        // TODO: (Maybe not) Fix by overriding: return super.getKey() + "[" + scale + "]";
        return file.getAbsolutePath() + "_" + getThumbnailHeight() + "_" + getThumbnailWidth() + "_" + nslice + "_" + nimage;
    }

    public String getOriginalStringValue() {
        return originalValue;
    }

    /*    @Override
    public String getFileName() {
    return file.getAbsolutePath();
    }
     */
    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }

    public void setSelected(boolean selected) {
        this.selected = selected;
    }

    public boolean isEnabled() {
        return enabled;
    }

    public boolean isSelected() {
        return selected;
    }

    public void setZoomScale(double scale) {
        this.scale = scale;
    }

    public int getThumbnailWidth() {
        return (int) ((double) super.getWidth() * scale);
    }

    public int getThumbnailHeight() {
        return (int) ((double) super.getHeight() * scale);
    }

    public ImagePlus getPreview() {
        return getPreview(getThumbnailWidth(), getThumbnailHeight());
    }

    public String getTooltipText() {
        // @TODO: Fix duplicated code.. (see below: getLabel())
        // @TODO: Maybe it's better "n@filename"
        String sliceStr = isVolume() ? String.valueOf(nslice) : "";
        String nimageStr = isStack() ? String.valueOf(nimage) : "";
        String delim = isVolume() && isStack() ? "/" : "";

        String extra = isVolume() || isStack() ? "[" + sliceStr + delim + nimageStr + "]" : "";

        return getAbsoluteFileName() + extra;
    }

    @Override
    public String getLabel() {
        String sliceStr = isVolume() ? String.valueOf(nslice) : "";
        String nimageStr = isStack() ? String.valueOf(nimage) : "";
        String delim = isVolume() && isStack() ? "/" : "";

        String extra = isVolume() || isStack() ? "[" + sliceStr + delim + nimageStr + "]" : "";

        return super.getLabel() + extra;
    }

    @Override
    public String toString() {
        return file.getName() + "@" + nimage + "#" + nslice;
    }
}
