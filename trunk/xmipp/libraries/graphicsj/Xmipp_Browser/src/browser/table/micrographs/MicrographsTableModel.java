/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package browser.table.micrographs;

import ij.IJ;
import java.io.File;
import java.util.LinkedList;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import browser.Cache;
import browser.imageitems.MicrographsTableImageItem;
import java.util.ArrayList;
import xmipp.MDLabel;
import xmipp.MetaData;

/**
 *
 * @author Juanjo Vega
 */
public class MicrographsTableModel extends DefaultTableModel implements TableModelListener {

    public final static int ID_COLUMN_INDEX = 0;
    public final static int ENABLED_COLUMN_INDEX = 1;
    public final static int MD_LABELS[] = {
        MDLabel.MDL_ENABLED,
        MDLabel.MDL_IMAGE,
        MDLabel.MDL_PSD,
        MDLabel.MDL_ASSOCIATED_IMAGE1,
        MDLabel.MDL_ASSOCIATED_IMAGE2,
        MDLabel.MDL_ASSOCIATED_IMAGE3,
        MDLabel.MDL_CTF_CRITERION_DAMPING,
        MDLabel.MDL_CTF_CRITERION_FIRSTZEROAVG,
        MDLabel.MDL_CTF_CRITERION_FIRSTZERODISAGREEMENT,
        MDLabel.MDL_CTF_CRITERION_FIRSTZERORATIO,
        MDLabel.MDL_CTF_CRITERION_FITTINGSCORE,
        MDLabel.MDL_CTF_CRITERION_FITTINGCORR13,
        MDLabel.MDL_CTF_CRITERION_PSDCORRELATION90,
        MDLabel.MDL_CTF_CRITERION_PSDRADIALINTEGRAL,
        MDLabel.MDL_CTF_CRITERION_PSDVARIANCE,
        MDLabel.MDL_CTF_CRITERION_PSDPCARUNSTEST
    };
    private static final String COLUMNS_NAMES[] = new String[]{
        "ID",
        "Enabled",
        "Micrograph",
        "PSD",
        "Xmipp CTF",
        "Xmipp CTF",
        "CTFFind",
        "Damping",
        "1st Zero AVG.",
        "1st Zero Disagreement",
        "1st Zero Ratio",
        "Fitting Score",
        "Fitting Correlation 1st-3rd Zero",
        "PSD Correlation At 90 degree",
        "PSD Radial Integral",
        "PSD Variance",
        "PSD PCA Runs Test"
    };
    private static final int EXTRA_COLUMNS_LABELS[] = {
        MDLabel.MDL_CTF_DEFOCUSU,
        MDLabel.MDL_CTF_DEFOCUSV};
    private static final String EXTRA_COLUMNS_NAMES[] = {
        "Defocus U",
        "Defocus V"};
    public final static int INDEX_ID = 0;
    public final static int INDEX_ENABLED = 1;
    public final static int INDEX_IMAGE = 2;
    public final static int INDEX_DEFOCUS_U_COL = 7;
    public final static int INDEX_DEFOCUS_V_COL = 8;
    public final static int INDEX_AUTOSORT_COLUMN = 16;
    protected ArrayList<Integer> busyRows = new ArrayList<Integer>();
    // Data type contained by columns to set renderes properly.
    protected static Cache cache = new Cache();
    private MetaData md;

    public MicrographsTableModel(String filename) {
        super();

        // Builds colunms structure.
        for (int i = 0; i < COLUMNS_NAMES.length; i++) {
            addColumn(COLUMNS_NAMES[i]);
        }
        addColumn(EXTRA_COLUMNS_NAMES[0]);  // DEFOCUS_U
        addColumn(EXTRA_COLUMNS_NAMES[1]);  // DEFOCUS_V

        load(filename);

        addTableModelListener(this);
    }

    public void reload() {
        load(getFilename());    // ...restore data...
    }

    private void load(String filename) {
        try {
            clear();    // Clear the whole data.

            md = new MetaData(filename);

            buildTable(md);
        } catch (Exception ex) {
            ex.printStackTrace();
            throw new RuntimeException(ex.getMessage());
        }
    }

    private void clear() {
        cache.clear();

        while (getRowCount() > 0) {
            removeRow(0);
        }
    }

    private void buildTable(MetaData md) {
        try {
            // Read metadata.
            Object row[] = new Object[MD_LABELS.length + 1];    // Field #0 is used for MDRow id.

            // Contains field enabled ?
            boolean hasEnabledField = true;
            if (!md.containsLabel(MDLabel.MDL_ENABLED)) {
                md.addLabel(MDLabel.MDL_ENABLED);
                hasEnabledField = false;
            }

            long objs[] = md.findObjects();

            for (long id : objs) {
                // Stores id.
                row[0] = id;

                // Field enabled does not exist, so adds it (true by default).
                if (!hasEnabledField) {
                    md.setValueInt(MDLabel.MDL_ENABLED, 1, id);
                }

                for (int i = 0, col = 1; i < MD_LABELS.length; i++, col++) {
                    int label = MD_LABELS[i];

                    if (md.containsLabel(label)) {
                        switch (label) {
                            case MDLabel.MDL_ENABLED:
                                Integer enabled = md.getValueInt(label, id);
                                row[col] = enabled > 0;
                                break;
                            case MDLabel.MDL_IMAGE:
                            case MDLabel.MDL_PSD:
                            case MDLabel.MDL_ASSOCIATED_IMAGE1:
                            case MDLabel.MDL_ASSOCIATED_IMAGE2:
                            case MDLabel.MDL_ASSOCIATED_IMAGE3:
                                String filename = md.getValueString(label, id, true);
                                File f = new File(filename);
                                String originalValue = md.getValueString(label, id);

                                row[col] = new MicrographsTableImageItem(f, originalValue, cache);
                                break;
                            case MDLabel.MDL_CTF_CRITERION_DAMPING:
                            case MDLabel.MDL_CTF_CRITERION_FIRSTZEROAVG:
                            case MDLabel.MDL_CTF_CRITERION_FIRSTZERODISAGREEMENT:
                            case MDLabel.MDL_CTF_CRITERION_FIRSTZERORATIO:
                            case MDLabel.MDL_CTF_CRITERION_FITTINGSCORE:
                            case MDLabel.MDL_CTF_CRITERION_FITTINGCORR13:
                            case MDLabel.MDL_CTF_CRITERION_PSDCORRELATION90:
                            case MDLabel.MDL_CTF_CRITERION_PSDRADIALINTEGRAL:
                            case MDLabel.MDL_CTF_CRITERION_PSDVARIANCE:
                            case MDLabel.MDL_CTF_CRITERION_PSDPCARUNSTEST:
                                //case MDLabel.MDL_CTF_CRITERION_COMBINED:
                                row[col] = md.getValueDouble(label, id);
                                break;
                            default:
                                row[col] = md.getValueString(label, id, true);
                        }
                    }
                }
                // Adds a new row data to table.
                addRow(row);
            }

            addExtraColumns();
        } catch (Exception ex) {
            System.out.println("Exception: " + ex.getMessage());
            ex.printStackTrace();
            throw new RuntimeException(ex);
        }
    }

    private void addExtraColumns() {
        double defocusU, defocusV;

        try {
            if (hasCtfData()) {
                for (int i = 0; i < getRowCount(); i++) {
                    String ctf_file = getCTFfile(i);

                    MetaData ctfMetaData = new MetaData(ctf_file);

                    long firstID = ctfMetaData.findObjects()[0];

                    // DEFOCUS_U
                    defocusU = ctfMetaData.getValueDouble(EXTRA_COLUMNS_LABELS[0], firstID);

                    // DEFOCUS_V
                    defocusV = ctfMetaData.getValueDouble(EXTRA_COLUMNS_LABELS[1], firstID);

                    // Sets values.
                    setValueAt(defocusU, i, getColumnCount() - 2);
                    setValueAt(defocusV, i, getColumnCount() - 1);
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            throw new RuntimeException(ex.getMessage());
        }
    }

    public ArrayList<Integer> getColumnsToHide() {
        ArrayList<Integer> toHide = new ArrayList<Integer>();

        // Hide ID column.
        toHide.add(INDEX_ID);

        // Adds null columns.
        for (int i = 0; i < MD_LABELS.length; i++) {
            if (!md.containsLabel(MD_LABELS[i])) {
                toHide.add(i + 1);
            }
        }

        // Extra columns: DEFOCUS
        if (!hasCtfData()) {
            toHide.add(MD_LABELS.length + 1);
            toHide.add(MD_LABELS.length + 2);
        }

        return toHide;
    }

    public boolean isRowEnabled(int row) {
        return (Boolean) getValueAt(row, ENABLED_COLUMN_INDEX);
    }

    public boolean hasCtfData() {
        return md.containsLabel(MDLabel.MDL_CTFMODEL);
    }

    @Override
    public Class getColumnClass(int column) {
        Object item = getValueAt(0, column);

        return item != null ? item.getClass() : Object.class;
    }

    @Override
    public boolean isCellEditable(int row, int column) {
        return column > 0;
    }

    public int getColumnSize() {
        return COLUMNS_NAMES.length;
    }

    public String getFilename() {
        return md.getFilename();
    }

    public void setRowEnabled(int row, boolean enabled) {
        setValueAt(enabled, row, ENABLED_COLUMN_INDEX);
    }

    public String[] extractColumn(int column, boolean onlyenabled) {
        LinkedList<String> filenames = new LinkedList<String>();

        for (int i = 0; i < getRowCount(); i++) {
            MicrographsTableImageItem item = (MicrographsTableImageItem) getValueAt(i, column);

            if (!onlyenabled || isRowEnabled(i)) {
                filenames.add(item.getAbsoluteFileName());
            }
        }

        return filenames.toArray(new String[filenames.size()]);
    }

    public boolean[] getEnabledRows() {
        boolean enabled[] = new boolean[getRowCount()];

        for (int i = 0; i < enabled.length; i++) {
            enabled[i] = (Boolean) getValueAt(i, ENABLED_COLUMN_INDEX);
        }

        return enabled;
    }

    public void setEnabledRows(boolean enabled[]) {
        for (int i = 0; i < enabled.length; i++) {
            setValueAt(enabled[i], i, ENABLED_COLUMN_INDEX);
        }
    }

    public void setRowBusy(int row) {
        busyRows.add((Integer) row);
    }

    public void setRowIdle(int row) {
        busyRows.remove((Integer) row);
    }

    public boolean isRowBusy(int row) {
        return busyRows.contains((Integer) row);
    }

    public String getCTFfile(int row) {
        long id = (Long) getValueAt(row, 0);
        String file = md.getValueString(MDLabel.MDL_CTFMODEL, id, true);

        return file;
    }

    public String getPSDfile(int row) {
        long id = (Long) getValueAt(row, 0);

        String file = md.getValueString(MDLabel.MDL_PSD, id, true);

        return file;
    }

    public String getCTFDisplayfile(int row) {
        long id = (Long) getValueAt(row, 0);
        String file = md.getValueString(MDLabel.MDL_ASSOCIATED_IMAGE2, id, true);

        return file;
    }

    public void tableChanged(TableModelEvent e) {
        int row = e.getFirstRow();
        int column = e.getColumn();

//        System.out.println(" *** Table changed @ " + row);

        // Updates metadata.
        if (column == ENABLED_COLUMN_INDEX) {
            long id = (Long) getValueAt(row, ID_COLUMN_INDEX);
            boolean enabled = (Boolean) getValueAt(row, ENABLED_COLUMN_INDEX);

            md.setValueInt(MDLabel.MDL_ENABLED, enabled ? 1 : 0, id);
        }
    }

    public boolean save(String fileName) {
        boolean saved = true;

        try {
            md.write(fileName);
        } catch (Exception ex) {
            IJ.error(ex.getMessage());
            saved = false;
        }

        return saved;
    }
}
