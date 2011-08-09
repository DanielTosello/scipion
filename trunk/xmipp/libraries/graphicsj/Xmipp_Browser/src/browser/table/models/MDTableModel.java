/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package browser.table.models;

import browser.LABELS;
import browser.imageitems.tableitems.MDTableItem;
import ij.IJ;
import java.io.File;
import xmipp.MDLabel;
import xmipp.MetaData;

/**
 *
 * @author Juanjo Vega
 */
public class MDTableModel extends AbstractXmippTableModel {

    protected MetaData md;

    public MDTableModel(String filename) {
        super(filename);
    }

    /*
     * For an array of single images.
     */
    public MDTableModel(String filenames[]) {
        super();

        String message = null;

        for (int i = 0; i < filenames.length; i++) {
            String currentFile = filenames[i];

            File f = new File(currentFile);
            if (f.exists()) {
                try {
                    if (md == null) {
                        md = new MetaData();
                    }

                    long id = md.addObject();
                    md.setValueString(MDLabel.MDL_IMAGE, currentFile, id);
                } catch (Exception ex) {
                    message = ex.getMessage();
                }
            } else {
                message += "File not found: " + currentFile + "\n";
            }
        }

        populateTable();

        if (message != null) {
            IJ.error(message);
        }
    }

    public MDTableModel(String filenames[], boolean enabled[]) {
        this(filenames);

        // Set enabled/disabled
        if (enabled != null) {
            for (int i = 0; i < enabled.length; i++) {
                getAllItems().get(i).setEnabled(enabled[i]);
            }
        }
    }

    @Override
    protected String populateTable(String filename) {
        try {
            md = new MetaData(filename);

            // Adds enabled field if not present.
            if (!md.containsLabel(MDLabel.MDL_ENABLED)) {
                md.addLabel(MDLabel.MDL_ENABLED);

                long ids[] = md.findObjects();
                for (long id : ids) {
                    md.setValueInt(MDLabel.MDL_ENABLED, 1, id);
                }
            }

            populateTable();
        } catch (Exception ex) {
            return ex.getMessage();
        }

        return null;
    }

    private void populateTable() {
        // Populate table.
        long ids[] = md.findObjects();

        for (long id : ids) {
            data.add(new MDTableItem(id, md, cache));
        }
    }

    @Override
    public String[] getLabels() {
        labelsValues = md.getActiveLabels();
        String labels[] = new String[labelsValues.length];

        for (int i = 0; i < labelsValues.length; i++) {
            labels[i] = MetaData.label2Str(labelsValues[i]);
        }

        return labels;
    }

    @Override
    public String getFilename() {
        return md.getFilename();
    }

    @Override
    public String getTitle() {
        String title = md.getFilename();
        return (title == null ? LABELS.TITLE_UNTITLED : title) + ": " + getSize() + " images.";
    }

    @Override
    protected void getMinAndMax() {
        double stats[] = md.getStatistics(false);

        min = stats[0];
        max = stats[1];
    }

    @Override
    public boolean isStack() {
        return true;
    }

    @Override
    public boolean isVolume() {
        return false;
    }
}
