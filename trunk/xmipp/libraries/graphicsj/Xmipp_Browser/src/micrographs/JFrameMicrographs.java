/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * JFrameMicrographs.java
 *
 * Created on Oct 25, 2010, 10:50:01 AM
 */
package micrographs;

import browser.DEBUG;
import browser.ICONS_MANAGER;
import browser.LABELS;
import browser.imageitems.MicrographsTableImageItem;
import browser.table.models.ImagesRowHeaderModel;
import browser.table.renderers.RowHeaderRenderer;
import browser.windows.ImagesWindowFactory;
import ij.IJ;
import ij.ImagePlus;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.io.File;
import java.util.ArrayList;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JList;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPopupMenu;
import javax.swing.JSeparator;
import javax.swing.JTable;
import javax.swing.LookAndFeel;
import javax.swing.SwingUtilities;
import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.JTableHeader;
import javax.swing.table.TableCellRenderer;
import javax.swing.table.TableColumn;
import javax.swing.table.TableRowSorter;
import metadata.models.XTableColumnModel;
import micrographs.ctf.profile.CTFViewImageWindow;
import micrographs.ctf.tasks.TasksEngine;
import micrographs.ctf.tasks.iCTFGUI;
import micrographs.filters.EnableFilter;
import micrographs.renderers.MicrographDoubleRenderer;
import micrographs.renderers.MicrographFileNameRenderer;
import micrographs.renderers.MicrographImageRenderer;

/**
 *
 * @author Juanjo Vega
 */
public class JFrameMicrographs extends JFrame implements iCTFGUI {

    private final static int CTF_IMAGE_COLUMN = 3;
    private JTable table;
    private MicrographsTableModel tableModel;
    private ImagesRowHeaderModel rowHeaderModel;
    private XTableColumnModel columnModel = new XTableColumnModel();
    private JPopUpMenuMicrograph jPopupMenu = new JPopUpMenuMicrograph();
    private JFileChooser fc = new JFileChooser();
    private JList rowHeader;
    private MicrographFileNameRenderer fileNameRenderer = new MicrographFileNameRenderer();
    private MicrographImageRenderer imageRenderer = new MicrographImageRenderer();
    private MicrographDoubleRenderer doubleRenderer = new MicrographDoubleRenderer();
    private TableRowSorter sorter;
    private EnableFilter enableFilter;
    private TasksEngine tasksEngine = new TasksEngine(this);

    /** Creates new form JFrameMicrographs */
    public JFrameMicrographs(String filename) {
        super();

        setTitle(filename);//ImagesWindowFactory.getSortTitle(filename, getWidth(),
        //getGraphics().getFontMetrics()));

        initComponents();

        // Builds table.
        tableModel = new MicrographsTableModel(filename);

        table = new JTable(tableModel) {

            //Implement table header tool tips.
            @Override
            protected JTableHeader createDefaultTableHeader() {
                return new JTableHeader(columnModel) {

                    @Override
                    public String getToolTipText(MouseEvent e) {
                        java.awt.Point p = e.getPoint();
                        int index = columnModel.getColumnIndexAtX(p.x);
                        int realIndex = columnModel.getColumn(index).getModelIndex();
                        return tableModel.getColumnName(realIndex);
                    }
                };
            }
        };

        // Sets sorter and filter.
/*        sorter = new TableRowSorter<MicrographsTableModel>(tableModel);
        sorter.setSortsOnUpdates(true);
        enableFilter = new EnableFilter();
        sorter.setRowFilter(enableFilter);*/

        enableFilter = new EnableFilter();
        setNewTableRowSorter();
        table.setRowSorter(sorter);

        table.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);

        table.addMouseListener(new java.awt.event.MouseAdapter() {

            @Override
            public void mouseClicked(java.awt.event.MouseEvent evt) {
                tableMouseClicked(evt);
            }
        });

        jsPanel.setViewportView(table);

        table.setColumnModel(columnModel);
        table.createDefaultColumnsFromModel();

        setRenderers();

        // places Defocus columns next to images.
        table.moveColumn(table.getColumnCount() - 2, MicrographsTableModel.INDEX_DEFOCUS_U_COL);
        table.moveColumn(table.getColumnCount() - 1, MicrographsTableModel.INDEX_DEFOCUS_V_COL);

        hideColumns();
        setRowHeader();

        updateTableStructure();
        autoSortTable(MicrographsTableModel.INDEX_AUTOSORT_COLUMN);

        pack();
    }

    private void autoSortTable(int column) {
        int view_column = table.convertColumnIndexToView(column);

        if (column < table.getColumnCount()) {
            table.getRowSorter().toggleSortOrder(view_column);
        }
    }

    private void setNewTableRowSorter() {
        sorter = new TableRowSorter<MicrographsTableModel>(tableModel);
        sorter.setSortsOnUpdates(true);
        sorter.setRowFilter(enableFilter);
        table.setRowSorter(sorter);
    }

    public String getFilename() {
        return tableModel.getFilename();
    }

    public void setRowBusy(int row) {
        setRunning(true);
        tableModel.setRowBusy(row);
    }

    public void setRowIdle(int row) {
        tableModel.setRowIdle(row);

        jPopupMenu.refresh();   // If menu is showing it will be refreshed.
    }

    private void hideColumns() {
        ArrayList<Integer> columns = tableModel.getColumnsToHide();

        for (int i = 0; i < columns.size(); i++) {
            TableColumn column = columnModel.getColumnByModelIndex(columns.get(i));
            columnModel.setColumnVisible(column, false);
        }
    }

    private void setRowHeader() {
        rowHeaderModel = new ImagesRowHeaderModel(table);

        rowHeader = new JList();
        rowHeader.setModel(rowHeaderModel);

        LookAndFeel.installColorsAndFont(rowHeader, "TableHeader.background",
                "TableHeader.foreground", "TableHeader.font");

        rowHeader.setFixedCellHeight(MicrographImageRenderer.CELL_HEIGHT);

        rowHeader.setCellRenderer(new RowHeaderRenderer());

        jsPanel.setRowHeaderView(rowHeader);
    }

    private void setRenderers() {
        table.setDefaultRenderer(MicrographsTableImageItem.class, imageRenderer);
        table.setDefaultRenderer(Double.class, doubleRenderer);

        // Image is quite big, so don't show it by default.
        table.getColumnModel().getColumn(MicrographsTableModel.INDEX_IMAGE).
                setCellRenderer(fileNameRenderer);
    }

    private synchronized void updateTableStructure() {
        DEBUG.printMessage(" *** Updating table... " + System.currentTimeMillis());
        packColumns();
        packRows();

        rowHeader.repaint();
        table.getTableHeader().repaint();
    }

    private void packColumns() {
        for (int i = 0; i < table.getColumnCount(); i++) {
            packColumn(table, i);
        }
    }

    // Sets the preferred width of the visible column specified by vColIndex. The column
    // will be just wide enough to show the column head and the widest cell in the column.
    private void packColumn(JTable table, int column) {
        DefaultTableColumnModel colModel = (DefaultTableColumnModel) table.getColumnModel();
        TableColumn col = colModel.getColumn(column);
        int width = 0;

        // Get width of column header
        TableCellRenderer renderer = col.getHeaderRenderer();
        if (renderer == null) {
            renderer = table.getTableHeader().getDefaultRenderer();
        }

        width = MicrographImageRenderer.CELL_WIDTH_MIN;

        // Get maximum width of column data
        for (int r = 0; r < table.getRowCount(); r++) {
            renderer = table.getCellRenderer(r, column);
            Component comp = renderer.getTableCellRendererComponent(
                    table, table.getValueAt(r, column), false, false, r, column);
            width = Math.max(width, comp.getPreferredSize().width);
        }

        // Set the width
        col.setPreferredWidth(width);
    }

    // The height of each row is set to the preferred height of the tallest cell in that row.
    private void packRows() {
        //  Row header.
        rowHeader.setFixedCellHeight(MicrographImageRenderer.CELL_HEIGHT);

        for (int row = 0; row < table.getRowCount(); row++) {
            table.setRowHeight(row, MicrographImageRenderer.CELL_HEIGHT);
        }
    }

    private void tableMouseClicked(java.awt.event.MouseEvent evt) {
        int view_row = table.rowAtPoint(evt.getPoint());
        int view_col = table.columnAtPoint(evt.getPoint());
        int model_row = table.convertRowIndexToModel(view_row);
        int model_col = table.convertColumnIndexToModel(view_col);

        if (SwingUtilities.isLeftMouseButton(evt)) {
            if (evt.getClickCount() > 1) {
                Object item = table.getValueAt(view_row, view_col);

                if (item instanceof MicrographsTableImageItem) {
                    MicrographsTableImageItem tableItem = (MicrographsTableImageItem) item;
                    ImagePlus ip = tableItem.getImagePlus();

                    if (ip != null) {
                        ip.setTitle(tableItem.getOriginalStringValue());

                        ImagesWindowFactory.captureFrame(ip);
                    }
                }
            } else {    // Single click
                if (model_col == MicrographsTableModel.INDEX_ENABLED) {
                    int min = table.getSelectionModel().getMinSelectionIndex();
                    int max = table.getSelectionModel().getMaxSelectionIndex();

                    // Multiple rows selection.
                    if (evt.isShiftDown()) {
                        // Last selection value will be the value for all the selected items.
                        Boolean value = (Boolean) tableModel.getValueAt(model_row, model_col);

                        for (int i = min; i <= max; i++) {
                            tableModel.setValueAt(value, table.convertRowIndexToModel(i),
                                    model_col);
                        }
                    }

                    updateTableStructure();
                }
            }
        } else if (SwingUtilities.isRightMouseButton(evt)) {
            table.setRowSelectionInterval(view_row, view_row);
            table.setColumnSelectionInterval(view_col, view_col);

            jPopupMenu.setCell(model_row, model_col);

            jPopupMenu.refresh();
            jPopupMenu.show(evt.getComponent(), evt.getX(), evt.getY());
        }
    }

    private void save(String fileName) {
        if (tableModel.save(fileName)) {
            IJ.showMessage("File saved: " + fileName);
        }
    }

    private void enableAll(boolean enable) {
        for (int i = 0; i < tableModel.getRowCount(); i++) {
            tableModel.setValueAt(enable, i, MicrographsTableModel.ENABLED_COLUMN_INDEX);
        }

        tableModel.fireTableRowsUpdated(0, tableModel.getRowCount() - 1);

        updateTableStructure();
    }

    private void setFiltering(boolean filtering) {
        enableFilter.setFiltering(filtering);

        table.setColumnModel(columnModel);  // Re sets column model to hide columns.

        tableModel.fireTableDataChanged();

        updateTableStructure();
    }

    private void showCTFImage(MicrographsTableImageItem item, String CTFFilename,
            String PSDfilename, String MicrographFilename, int row) {
        ImagesWindowFactory.openCTFImage(item.getImagePlus(), CTFFilename,
                PSDfilename, tasksEngine, MicrographFilename, row);
    }

    public void setRunning(boolean running) {
        jlStatus.setIcon(running ? ICONS_MANAGER.WAIT_ICON : null);
    }

    public void done() {
//        System.out.println("Done!!");
        setRunning(false);

        refreshTableData();
    }
    /*
    private void refreshTable(final boolean auto) {
    /*        SwingUtilities.invokeLater(new Runnable() {
    
    public void run() {*/
    /*        String autoStr = auto ? "AUTO" : "MANUAL";
    DEBUG.printMessage(" *** Refreshing table [" + autoStr + "]: " + System.currentTimeMillis());
    
    boolean enabled[] = null;
    
    if (auto) {
    enabled = tableModel.getEnabledRows();   // Gets currently enabled rows...
    }
    
    tableModel.reload();
    
    if (auto) {
    tableModel.setEnabledRows(enabled);    // ...sets previously enabled.
    }
    
    table.setColumnModel(columnModel);  // Re sets column model to hide columns.
    
    if (!auto) {
    setNewTableRowSorter();
    }
    
    updateTableStructure();
    /*            }
    });*//*
    }*/


    private void refreshTableData() {
        DEBUG.printMessage(" *** Refreshing table [AUTO]: " + System.currentTimeMillis());

        boolean enabled[] = tableModel.getEnabledRows();   // Gets currently enabled rows...
        tableModel.reload();
        tableModel.setEnabledRows(enabled);    // ...sets previously enabled.
        table.setColumnModel(columnModel);  // Re sets column model to hide columns.

        updateTableStructure();
    }

    private void reloadTableData() {
        DEBUG.printMessage(" *** Refreshing table [RELOAD]: " + System.currentTimeMillis());

        tableModel.reload();
        table.setColumnModel(columnModel);  // Re sets column model to hide columns.
        setNewTableRowSorter();

        updateTableStructure();
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        toolBar = new javax.swing.JToolBar();
        bSave = new javax.swing.JButton();
        bReload = new javax.swing.JButton();
        jpCenter = new javax.swing.JPanel();
        jpCheckAll = new javax.swing.JPanel();
        jcbEnableAll = new javax.swing.JCheckBox();
        jcbFilterEnabled = new javax.swing.JCheckBox();
        jlStatus = new javax.swing.JLabel();
        jsPanel = new javax.swing.JScrollPane();

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);

        toolBar.setRollover(true);

        bSave.setText(LABELS.BUTTON_SAVE);
        bSave.setFocusable(false);
        bSave.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        bSave.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        bSave.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                bSaveActionPerformed(evt);
            }
        });
        toolBar.add(bSave);

        bReload.setText(LABELS.BUTTON_RELOAD_TABLE);
        bReload.setFocusable(false);
        bReload.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        bReload.setVerticalTextPosition(javax.swing.SwingConstants.BOTTOM);
        bReload.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                bReloadActionPerformed(evt);
            }
        });
        toolBar.add(bReload);

        getContentPane().add(toolBar, java.awt.BorderLayout.PAGE_START);

        jpCenter.setLayout(new java.awt.BorderLayout());

        jpCheckAll.setLayout(new javax.swing.BoxLayout(jpCheckAll, javax.swing.BoxLayout.LINE_AXIS));

        jcbEnableAll.setText(LABELS.LABEL_TABLE_ENABLE_ALL);
        jcbEnableAll.addItemListener(new java.awt.event.ItemListener() {
            public void itemStateChanged(java.awt.event.ItemEvent evt) {
                jcbEnableAllItemStateChanged(evt);
            }
        });
        jpCheckAll.add(jcbEnableAll);

        jcbFilterEnabled.setText(LABELS.LABEL_TABLE_HIDE_DISABLED);
        jcbFilterEnabled.addItemListener(new java.awt.event.ItemListener() {
            public void itemStateChanged(java.awt.event.ItemEvent evt) {
                jcbFilterEnabledItemStateChanged(evt);
            }
        });
        jpCheckAll.add(jcbFilterEnabled);
        jpCheckAll.add(jlStatus);

        jpCenter.add(jpCheckAll, java.awt.BorderLayout.PAGE_START);
        jpCenter.add(jsPanel, java.awt.BorderLayout.CENTER);

        getContentPane().add(jpCenter, java.awt.BorderLayout.CENTER);

        pack();
    }// </editor-fold>//GEN-END:initComponents

    private void bSaveActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_bSaveActionPerformed
        // Sets path and filename automatically.
        fc.setSelectedFile(new File(tableModel.getFilename()));

        if (fc.showSaveDialog(this) != JFileChooser.CANCEL_OPTION) {
            boolean response = true;
            if (fc.getSelectedFile().exists()) {
                response = JOptionPane.showConfirmDialog(null,
                        "Overwrite existing file?", "Confirm Overwrite",
                        JOptionPane.OK_CANCEL_OPTION,
                        JOptionPane.QUESTION_MESSAGE) == JOptionPane.OK_OPTION;
            }

            if (response) {
                save(fc.getSelectedFile().getAbsolutePath());
            }
        }
    }//GEN-LAST:event_bSaveActionPerformed

    private void jcbEnableAllItemStateChanged(java.awt.event.ItemEvent evt) {//GEN-FIRST:event_jcbEnableAllItemStateChanged
        enableAll(jcbEnableAll.isSelected());
    }//GEN-LAST:event_jcbEnableAllItemStateChanged

    private void jcbFilterEnabledItemStateChanged(java.awt.event.ItemEvent evt) {//GEN-FIRST:event_jcbFilterEnabledItemStateChanged
        setFiltering(jcbFilterEnabled.isSelected());
}//GEN-LAST:event_jcbFilterEnabledItemStateChanged

    private void bReloadActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_bReloadActionPerformed
        reloadTableData();
}//GEN-LAST:event_bReloadActionPerformed
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JButton bReload;
    private javax.swing.JButton bSave;
    private javax.swing.JCheckBox jcbEnableAll;
    private javax.swing.JCheckBox jcbFilterEnabled;
    private javax.swing.JLabel jlStatus;
    private javax.swing.JPanel jpCenter;
    private javax.swing.JPanel jpCheckAll;
    private javax.swing.JScrollPane jsPanel;
    private javax.swing.JToolBar toolBar;
    // End of variables declaration//GEN-END:variables

    class JPopUpMenuMicrograph extends JPopupMenu {

        private int row, col;   // Current cell where menu is displayed.
        private JMenuItem jmiShowCTF = new JMenuItem(LABELS.LABEL_SHOW_CTF);
        private JMenuItem jmiExtractColumnEnabled = new JMenuItem(LABELS.LABEL_EXTRACT_COLUMN_ENABLED);
        private JMenuItem jmiExtractColumnAll = new JMenuItem(LABELS.LABEL_EXTRACT_COLUMN_ALL);
        private JMenuItem jmiRecalculateCTF = new JMenuItem(LABELS.LABEL_RECALCULATE_CTF);
        private JMenuItem jmiViewCTFProfile = new JMenuItem(LABELS.LABEL_VIEW_CTF_PROFILE);

        public JPopUpMenuMicrograph() {
            super();

            add(jmiShowCTF);
            add(new JSeparator());
            add(jmiExtractColumnEnabled);
            add(jmiExtractColumnAll);
            add(new JSeparator());
            add(jmiViewCTFProfile);
            add(jmiRecalculateCTF);

            jmiShowCTF.addActionListener(new ActionListener() {

                public void actionPerformed(ActionEvent e) {
                    showCTFFile(table.getSelectedRow());
                }
            });

            jmiExtractColumnEnabled.addActionListener(new ActionListener() {

                public void actionPerformed(ActionEvent e) {
                    extractColumn(table.getSelectedColumn(), true);
                }
            });

            jmiExtractColumnAll.addActionListener(new ActionListener() {

                public void actionPerformed(ActionEvent e) {
                    extractColumn(table.getSelectedColumn(), false);
                }
            });

            jmiViewCTFProfile.addActionListener(new ActionListener() {

                public void actionPerformed(ActionEvent e) {
                    showCTFProfile();
                }
            });

            jmiRecalculateCTF.addActionListener(new ActionListener() {

                public void actionPerformed(ActionEvent e) {
                    showRecalculateCTFWindow();
                }
            });
        }

        private void setCell(int row, int col) {
            this.row = row;
            this.col = col;
        }

        private void refresh() {
            jmiShowCTF.setEnabled(tableModel.hasCtfData());

            // Column extraction only allowed for images
            jmiExtractColumnEnabled.setEnabled(tableModel.getValueAt(row, col) instanceof MicrographsTableImageItem);
            jmiExtractColumnAll.setEnabled(tableModel.getValueAt(row, col) instanceof MicrographsTableImageItem);

            boolean busy = tableModel.isRowBusy(row);
            jmiRecalculateCTF.setIcon(busy ? ICONS_MANAGER.WAIT_MENU_ICON : null);
            jmiRecalculateCTF.setEnabled(!busy);

//            repaint();
            updateUI();

            pack();
        }

        private void showCTFFile(int row) {
            String CTFfile = tableModel.getCTFfile(row);

            ImagesWindowFactory.openFileAsText(CTFfile, this);
        }

        private void extractColumn(int column, boolean onlyenabled) {
            if (onlyenabled) {
                ImagesWindowFactory.openFilesAsTable(tableModel.extractColumn(column, onlyenabled));
            } else {
                ImagesWindowFactory.openTable(
                        tableModel.extractColumn(column, onlyenabled),
                        tableModel.getEnabledRows());
            }
        }

        private void showCTFProfile() {
            String CTFFilename = tableModel.getCTFfile(row);
            String displayFilename = tableModel.getCTFDisplayfile(row);
            String PSDFilename = tableModel.getPSDfile(row);

            ImagePlus ip = IJ.openImage(displayFilename);

            CTFViewImageWindow frame = new CTFViewImageWindow(ip, CTFFilename, PSDFilename);
            frame.setVisible(true);
        }

        private void showRecalculateCTFWindow() {
            Object item = tableModel.getValueAt(row, CTF_IMAGE_COLUMN);

            if (item instanceof MicrographsTableImageItem) {
                showCTFImage((MicrographsTableImageItem) item, tableModel.getCTFfile(row),
                        tableModel.getPSDfile(row), tableModel.getFilename(), row);
            }
        }
    }
}
