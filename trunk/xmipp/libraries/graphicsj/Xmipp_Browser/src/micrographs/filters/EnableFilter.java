/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package micrographs.filters;

import browser.table.micrographs.MicrographsTableModel;
import javax.swing.RowFilter;
import javax.swing.RowFilter.Entry;

/**
 *
 * @author Juanjo Vega
 */
public class EnableFilter extends RowFilter<MicrographsTableModel, Object> {

    protected boolean filtering = false;

    public void setFiltering(boolean filtering) {
        this.filtering = filtering;
    }

    public boolean isFiltering() {
        return filtering;
    }

    @Override
    public boolean include(Entry<? extends MicrographsTableModel, ? extends Object> entry) {
        return !filtering || entry.getValue(MicrographsTableModel.ENABLED_COLUMN_INDEX).equals(true);
    }
}
