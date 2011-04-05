/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package browser.table;

import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumn;

/**
 *
 * @author Juanjo Vega
 */
public class ImagesTableColumnModel extends DefaultTableColumnModel {

    protected int width;

    @Override
    public void addColumn(TableColumn tc) {
        tc.setPreferredWidth(width);
        super.addColumn(tc);
    }

    @Override
    public TableColumn getColumn(int columnIndex) {
        try {
            return super.getColumn(columnIndex);
        } catch (Exception ex) {
            ex.printStackTrace();
            System.out.println("*** Exception getting column: " + columnIndex + " / " + getColumnCount());
            return super.getColumn(columnIndex);
        }
    }

    public void setWidth(int width) {
        this.width = width;

        adjustColumnsWidth();
    }

    private void adjustColumnsWidth() {
        for (int i = 0; i < getColumnCount(); i++) {
            getColumn(i).setPreferredWidth(width);
        }
    }
}
