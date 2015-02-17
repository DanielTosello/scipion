/***************************************************************************
 * Authors:     J.M. de la Rosa Trevin (jmdelarosa@cnb.csic.es)
 *
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

package xmipp.viewer.windows;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import javax.swing.BorderFactory;
import javax.swing.DefaultCellEditor;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableColumn;
import org.junit.experimental.theories.ParameterSignature;
import xmipp.jni.MetaData;
import xmipp.utils.ColorEditor;
import xmipp.utils.ColorRenderer;
import xmipp.utils.ScipionParams;
import xmipp.utils.XmippDialog;
import xmipp.utils.XmippWindowUtil;
import xmipp.viewer.models.ColumnInfo;
import xmipp.viewer.models.ImageGalleryTableModel;
import xmipp.viewer.scipion.ScipionGalleryData;
import xmipp.viewer.scipion.ScipionGalleryJFrame;

public class PlotJDialog extends XmippDialog {
	private static final long serialVersionUID = 1L;
	private JTable tableColumns;
	private HashMap<ColumnInfo, ColumnInfo.ColumnExtraInfo> rowsExtra;
	private ColumnsTableModel model;
	private JTextField tfTitle, tfXLabel, tfYLabel, tfBins;
	private JCheckBox jchHist;
	private JComboBox jcbXAxis;
        private List<ColumnInfo> rows;
	// This will be used for check for results from the dialog
	boolean fireEvent = true;
	GridBagConstraints gbc = new GridBagConstraints();
	ImageGalleryTableModel gallery;
	JPanel panelEntries;
	String[] COLORS = { "0000CC", "009900", "CC0000", "000000", "FF6600",
			"FFFF00", "00CCFF" };

	public PlotJDialog(GalleryJFrame parent) {
		super(parent, "Plot options", false);

		rows = new ArrayList<ColumnInfo>();
		rowsExtra = new HashMap<ColumnInfo, ColumnInfo.ColumnExtraInfo>();
		int i = 0;
		for (ColumnInfo ci : parent.getData().getLabelsInfo())
			if (ci.type == MetaData.LABEL_INT || ci.type == MetaData.LABEL_DOUBLE) {// int or double
				
				rows.add(ci);
				rowsExtra.put(ci, ci.new ColumnExtraInfo(COLORS[i % COLORS.length]));
				++i;
			}
		this.gallery = parent.gallery;
		btnOkText = "Plot";
		closeOnAction = false;
		initComponents();
	}// constructor ColumnsJDialog

	private void addPair(String text, Component c, int row) {
		JLabel label = new JLabel(text);
		gbc.anchor = GridBagConstraints.EAST;
		panelEntries.add(label, XmippWindowUtil.getConstraints(gbc, 0, row));
		gbc.anchor = GridBagConstraints.WEST;
		panelEntries.add(c, XmippWindowUtil.getConstraints(gbc, 1, row));
	}

	protected void createEntries() {
		tfTitle = new JTextField(20);
		tfXLabel = new JTextField(20);
		tfYLabel = new JTextField(20);
		tfBins = new JTextField(10);
                tfBins.setEnabled(false);
		jchHist = new JCheckBox();
                jchHist.addActionListener(new ActionListener() {

                    @Override
                    public void actionPerformed(ActionEvent ae) {
                        tfBins.setEnabled(jchHist.isSelected());
                    }
                });

		panelEntries = new JPanel(new GridBagLayout());
		addPair("Title:", tfTitle, 0);
		addPair("X label:", tfXLabel, 1);
		addPair("Y label:", tfYLabel, 2);
		addPair("Histogram:", jchHist, 3);
		addPair("Bins: ", tfBins, 4);
		jcbXAxis = new JComboBox();
		jcbXAxis.addItem("");
		for (ColumnInfo ci : rows)
			jcbXAxis.addItem(ci.labelName);
		addPair("X Axis:", jcbXAxis, 5);
                jcbXAxis.addActionListener(new ActionListener() {

                    @Override
                    public void actionPerformed(ActionEvent ae) {
                            tfXLabel.setText(jcbXAxis.getSelectedItem().toString());
                    }
                });
	}

	@Override
	protected void createContent(JPanel panel) {
		setResizable(false);
		panel.setLayout(new GridBagLayout());
		gbc.anchor = GridBagConstraints.EAST;
		gbc.insets = new Insets(5, 5, 5, 5);

		JPanel groupstbpn = new JPanel();
		JScrollPane sp = new JScrollPane();
		groupstbpn.setBorder(BorderFactory.createTitledBorder("Labels"));
		groupstbpn.add(sp);
		sp.setOpaque(true);
		model = new ColumnsTableModel();
		tableColumns = new JTable(model);
                tableColumns.getColumnModel().getColumn(0).setPreferredWidth(250);
		tableColumns
				.setPreferredScrollableViewportSize(new Dimension(450, 200));
		sp.setViewportView(tableColumns);
		panel.add(groupstbpn, XmippWindowUtil.getConstraints(gbc, 0, 1, 2));
		createEntries();
		panel.add(panelEntries, XmippWindowUtil.getConstraints(gbc, 1, 0));

		tableColumns.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		tableColumns.setDefaultRenderer(Color.class, new ColorRenderer());
		tableColumns.setDefaultEditor(Color.class, new ColorEditor());
		setComboEditor(3, "solid", "dashed", "dotted");
		setComboEditor(4, "none", ".", ",", "o", "v", "^", "<", ">", "1", "2",
				"3", "4", "s", "p", "h", "H", "+", "x", "D", "d", "|", "_");
	}// function initComponents

	private void setComboEditor(int col, String... args) {
		TableColumn column = tableColumns.getColumnModel().getColumn(col);
		JComboBox cb = new JComboBox(args);
		column.setCellEditor(new DefaultCellEditor(cb));
	}

	@Override
	public void handleCancel() {
		closeOnAction = true;
	}

	@Override
	public void handleOk() {

		String labels = "";
		String colors = "";
		String styles = "";
		String markers = "";
		Boolean checked = false;
		String ylabel = tfYLabel.getText().trim();

		for (ColumnInfo ci: rows) {
			ColumnInfo.ColumnExtraInfo cei = rowsExtra.get(ci);
			if (cei.plot) {
				labels += ci.labelName + " ";
				colors += "#" + cei.color + " ";
				styles += cei.linestyle + " ";
				markers += cei.marker + " ";
				checked = true;
				if (ylabel.length() == 0)
					ylabel = rows.get(0).labelName;
			}
		}

		if (!checked)
			return;

		try {
                        String[] argsBasic;
                        if(parent instanceof ScipionGalleryJFrame)
                        {
                            ScipionParams params = (ScipionParams)gallery.data.parameters;
                            //argsBasic = new String[]{params.python, params.getPlotSqliteScript(), gallery.data.getFileName(), ((ScipionGalleryData)gallery.data).getPreffix(), 
                              //  labels, colors, styles, markers, getXColumn(), getYLabel(), getXLabel(), getPlotTitle(), getBins()};
                            ScipionGalleryData data = (ScipionGalleryData)gallery.data;
                            String orderColumn = "id";
                            String orderDirection = "ASC";
                            if(params.sortby != null)
                            {
                                orderColumn = params.sortby[0];
                                orderDirection = params.sortby[1];
                            }   
                            String command = String.format("run function scheduleSqlitePlot '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s' %s %s", 
                                    data.getFileName(), data.getPreffix(), 
                                labels, colors, styles, markers, getXColumn(), getYLabel(), getXLabel(), getPlotTitle(), getBins(), orderColumn, orderDirection);
                            XmippWindowUtil.runCommand(command, params.port);
                        }
                        else
                        {
                            argsBasic = new String[]{ "xmipp_metadata_plot",
                                            gallery.data.getMdFilename(), "-y", labels, "--colors", colors,
                                            "--style", styles, "--markers", markers, "--title",
                                            tfTitle.getText().trim(), "--ytitle", ylabel, "--xtitle",
                                            tfXLabel.getText().trim() };
                            ArrayList<String> argsArray = new ArrayList<String>(Arrays.asList(argsBasic));
                            
                            if (jchHist.isSelected()) {
                                    argsArray.add("--nbins");
                                    argsArray.add(tfBins.getText().trim());
                            }

                            if (jcbXAxis.getSelectedIndex() != 0) {
                                    argsArray.add("-x");
                                    argsArray.add(jcbXAxis.getSelectedItem().toString());
                            }

                            for (String a: argsArray)
                                    System.out.print(a + " ");
                            System.out.println("");
                            argsBasic = argsArray.toArray(argsBasic);
                            String result = XmippWindowUtil.executeCommand(argsBasic, false);
                        }
			
			
			

		} catch (Exception e) {
			e.printStackTrace();
		}
	}
        
        public String getXColumn()
        {
            if (jcbXAxis.getSelectedIndex() != 0) 
                    return jcbXAxis.getSelectedItem().toString();
            return "";
        }
        
        public String getXLabel()
        {
            return tfXLabel.getText().trim();
        }
        
        public String getYLabel()
        {
            return tfYLabel.getText().trim();
        }

        public String getPlotTitle() {
            return tfTitle.getText();
        }

        public String getBins() {
            if(tfBins.isEnabled())
                return tfBins.getText();
            return "";
        }

	class ColumnsTableModel extends AbstractTableModel {
		private static final long serialVersionUID = 1L;

		private String[] columns = { "Label", "Plot", "Color", "Line style",
				"Marker" };

		@Override
		public Class getColumnClass(int column) {
			switch (column) {
			case 0:
			case 3:
			case 4:
				return String.class;
			case 1:
				return Boolean.class;
			case 2:
				return Color.class;
			default:
				return null;
			}
		}

		@Override
		public Object getValueAt(int row, int column) {
			ColumnInfo ci = rows.get(row);
			ColumnInfo.ColumnExtraInfo cei = rowsExtra.get(ci);
			switch (column) {
			case 0:
				return ci.labelName;
			case 1:
				return cei.plot;
			case 2:
				return ColorEditor.stringToColor(cei.color);
			case 3:
				return cei.linestyle;
			case 4:
				return cei.marker;
			}
			return null;
		}

		@Override
		public void setValueAt(Object value, int row, int column) {
			/* We will use labelName to store Color and comments to store marker */
			ColumnInfo ci = rows.get(row);
			ColumnInfo.ColumnExtraInfo cei = rowsExtra.get(ci);
			switch (column) {
			case 1:
				cei.plot = (Boolean) value;
                                
				break;
			case 2:
				cei.color = ColorEditor.colorToString((Color) value);
				break;
			case 3:
				cei.linestyle = (String) value;
				break;
			case 4:
				cei.marker = (String) value;
				break;
			}
		}

		@Override
		public String getColumnName(int columnIndex) {
			return columns[columnIndex];
		}

		@Override
		public int getColumnCount() {
			return columns.length;
		}

		@Override
		public int getRowCount() {
			return rows.size();
			// return frame.getParticlePicker().getFamilies().size();
		}

		@Override
		public boolean isCellEditable(int row, int column) {
			switch (column) {
			case 0:
				return false;
			default:
				return true;

			}
		}

	}// class ColumnsTableModel

}// class ColumnsJDialog
