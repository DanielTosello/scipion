/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package table;

import ij.ImagePlus;
import ij.gui.Plot;
import ij.process.FloatProcessor;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Vector;
import javax.swing.table.AbstractTableModel;
import sphere.ImageConverter;
import xmipp.ImageDouble;

/**
 *
 * @author Juanjo Vega
 */
public class ImagesTableModel extends AbstractTableModel {

    protected ArrayList<ScoreItem> data = new ArrayList<ScoreItem>();
    protected ImagePlus focusedItem;
    int good, bad;

    public ImagesTableModel() {
        super();
    }

    public void clear() {
        data.clear();
    }

    public void addItem(ScoreItem scoreItem) {
        data.add(scoreItem);

        // Counts "good" and "bad" items.
        if (scoreItem.good) {
            good++;
        } else {
            bad++;
        }

        fireTableStructureChanged();
    }

    @Override
    public String getColumnName(int column) {
        return "" + column;
    }

    public int getGoodCount() {
        return good;
    }

    public int getBadCount() {
        return bad;
    }

    public int getRowCount() {
        return 1;
    }

    public int getColumnCount() {
        return data.size();
    }

    public Object getValueAt(int rowIndex, int columnIndex) {
        return columnIndex < data.size() ? data.get(columnIndex) : null;
    }

    @Override
    public Class getColumnClass(int c) {
        Object object = getValueAt(0, c);

        return object != null ? object.getClass() : null;
    }

    public ArrayList<ScoreItem> getData() {
        return data;
    }

    public void sortItems() {
        Collections.sort(data);
    }

    public static ImagePlus mean(Vector<String> images) {
        try {
            ImageDouble firstImage = new ImageDouble();

            firstImage.readHeader(images.get(0));
            int w = firstImage.getXsize();
            int h = firstImage.getYsize();
            float mean[][] = new float[w][h];

            String currentFileName;

            // For all images...
            for (int k = 0; k < images.size(); k++) {
                ImageDouble currentImage = new ImageDouble();
                currentFileName = images.get(k);

                currentImage.read(currentFileName);
                ImagePlus ip = ImageConverter.convertToImagej(currentImage, currentFileName);

                // Adds current image to sum.
                for (int j = 0; j < h; j++) {
                    for (int i = 0; i < w; i++) {
                        mean[i][j] += ip.getProcessor().getf(i, j);
                    }
                }

                ip.close();
            }

            // Calculates mean...
            for (int j = 0; j < h; j++) {
                for (int i = 0; i < w; i++) {
                    mean[i][j] /= images.size(); // ...by dividing with the #images
                }
            }

            FloatProcessor processor = new FloatProcessor(mean);
            ImagePlus meanIp = new ImagePlus();
            meanIp.setProcessor("", processor);

            return meanIp;
        } catch (Exception ex) {
            throw new RuntimeException(ex);
        }
    }
}
