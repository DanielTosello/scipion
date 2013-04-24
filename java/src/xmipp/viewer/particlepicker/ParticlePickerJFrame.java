package xmipp.viewer.particlepicker;

import ij.IJ;
import ij.WindowManager;
import ij.gui.ImageCanvas;
import ij.gui.ImageWindow;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.InputEvent;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.KeyEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JColorChooser;
import javax.swing.JDialog;
import javax.swing.JFormattedTextField;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.JTable;
import javax.swing.JToggleButton;
import javax.swing.JToolBar;
import javax.swing.KeyStroke;
import javax.swing.SwingUtilities;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.MenuEvent;
import javax.swing.event.MenuListener;

import xmipp.ij.commons.Tool;
import xmipp.ij.commons.XmippApplication;
import xmipp.ij.commons.XmippIJUtil;
import xmipp.utils.ColorIcon;
import xmipp.utils.QuickHelpJDialog;
import xmipp.utils.XmippDialog;
import xmipp.utils.XmippFileChooser;
import xmipp.utils.XmippMessage;
import xmipp.utils.XmippQuestionDialog;
import xmipp.utils.XmippResource;
import xmipp.utils.XmippWindowUtil;
import xmipp.viewer.particlepicker.Family;
import xmipp.viewer.particlepicker.Format;
import xmipp.viewer.particlepicker.ImportParticlesJDialog;
import xmipp.viewer.particlepicker.Micrograph;
import xmipp.viewer.particlepicker.ParticlePicker;
import xmipp.viewer.particlepicker.ParticlePickerCanvas;
import xmipp.viewer.particlepicker.ParticlesJDialog;
import xmipp.viewer.particlepicker.Shape;
import xmipp.viewer.particlepicker.tiltpair.gui.TiltPairParticlesJDialog;
import xmipp.viewer.particlepicker.training.gui.TemplatesJDialog;
import xmipp.viewer.particlepicker.training.gui.TrainingPickerJFrame;
import xmipp.viewer.particlepicker.training.model.FamilyState;
import xmipp.viewer.particlepicker.training.model.TrainingParticle;

public abstract class ParticlePickerJFrame extends JFrame implements ActionListener
{

	protected ParticlesJDialog particlesdialog;

	protected JMenuItem ijmi;
	protected JCheckBox circlechb;
	protected JCheckBox rectanglechb;
	protected JFormattedTextField sizetf;
	protected JCheckBox centerchb;
	protected JPanel symbolpn;
	protected JMenuItem savemi;
	protected JMenuItem hcontentsmi;
	protected JMenuItem pmi;
	// protected JMenuItem importffilemi;
	protected JMenuItem exportmi;
	protected JMenu filtersmn;
	protected String activefilter;
	protected JSlider sizesl;
	protected JPanel sizepn;

	private List<JCheckBoxMenuItem> mifilters;
	protected JMenu filemn;
	protected JMenuItem importffmi;
	protected JButton colorbt;
	protected Color color;
	protected JPanel colorpn;
	protected JButton resetbt;
	protected JTable micrographstb;
	protected ImportParticlesJDialog importpjd = null;

	private JMenuItem exitmi;
	protected JPanel imagepn;
	protected JLabel positionlb;
	protected JToggleButton usezoombt;

	public TemplatesJDialog templatesdialog;

	private JToggleButton eraserbt;
	protected JButton exitbt;

	private JMenuItem keyassistmi;

	protected JMenu helpmn;

	public ParticlePickerJFrame(ParticlePicker picker)
	{
		XmippApplication.addInstance();
		setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
		addWindowListener(new WindowAdapter()
		{
			public void windowClosing(WindowEvent winEvt)
			{

				if (getParticlePicker().isChanged())
				{
					XmippQuestionDialog qd = new XmippQuestionDialog(ParticlePickerJFrame.this, "Save changes before closing?");
					boolean save = qd.showDialog();
					if (save)
						saveChanges();
					else if (qd.isCanceled())
						return;
				}
				close();
//				if (getParticlePicker().getMode() == FamilyState.Supervised)
//					System.exit(0);// temporarily
				
			}
		});

		initMenuBar(picker);

		resetbt = XmippWindowUtil.getTextButton("Reset", new ActionListener()
		{
			@Override
			public void actionPerformed(ActionEvent e)
			{
				XmippQuestionDialog qd = new XmippQuestionDialog(ParticlePickerJFrame.this, "Are you sure to remove all particles from micrograph\n",
						false);
				if (qd.showDialog())
					resetMicrograph();
			}
		});
		
		exitbt = XmippWindowUtil.getTextButton("Save and Exit", new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent arg0)
			{
				saveChanges();
				System.exit(0);
				
			}
		});
		
		micrographstb = new JTable();
		micrographstb.getSelectionModel().addListSelectionListener(new ListSelectionListener()
		{

			@Override
			public void valueChanged(ListSelectionEvent e)
			{
				if (e.getValueIsAdjusting())
					return;
				if (micrographstb.getSelectedRow() == -1)
					return;// Probably
							// from
							// fireTableDataChanged
							// raised
				loadMicrograph();
			}
		});
		micrographstb.addMouseListener(new MouseListener()
		{

			@Override
			public void mouseReleased(MouseEvent arg0)
			{
				// TODO Auto-generated method stub

			}

			@Override
			public void mousePressed(MouseEvent arg0)
			{
				// TODO Auto-generated method stub

			}

			@Override
			public void mouseExited(MouseEvent arg0)
			{
				// TODO Auto-generated method stub

			}

			@Override
			public void mouseEntered(MouseEvent arg0)
			{
				// TODO Auto-generated method stub

			}

			@Override
			public void mouseClicked(MouseEvent arg0)
			{
				if (micrographstb.getSelectedRow() == -1)
					return;
				loadMicrograph();
			}
		});
	}

	protected abstract void loadMicrograph();

	private void initMenuBar(ParticlePicker picker)
	{
		filemn = new JMenu("File");
		helpmn = new JMenu("Help");
		savemi = new JMenuItem("Save", XmippResource.getIcon("save.gif"));
		savemi.setMnemonic('S');
		savemi.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, InputEvent.CTRL_DOWN_MASK));
		savemi.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				saveChanges();
				showMessage("Data saved successfully");
				((JMenuItem) e.getSource()).setEnabled(false);
				exitbt.setEnabled(false);
			}
		});
		filemn.add(savemi);
		importffmi = new JMenuItem("Import Particles...", XmippResource.getIcon("import_wiz.gif"));
		filemn.add(importffmi);
		if (picker.getFamily().getStep() != FamilyState.Manual)
			importffmi.setEnabled(false);

		importffmi.addActionListener(new ActionListener()
		{
			@Override
			public void actionPerformed(ActionEvent e)
			{
				showImportDialog();
			}
		});

		exitmi = new JMenuItem("Exit");
		exitmi.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent arg0)
			{
				close();
			}
		});
		filemn.add(exitmi);

		ijmi = new JMenuItem("ImageJ", XmippResource.getIcon("ij.gif"));
		ijmi.setEnabled(picker.getMode() != FamilyState.ReadOnly);
		ijmi.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				XmippIJUtil.showImageJ(Tool.PICKER);
			}
		});

		hcontentsmi = new JMenuItem("Online help", XmippResource.getIcon("online_help.gif"));
		hcontentsmi.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				try
				{
					openHelpURl();

				}
				catch (Exception ex)
				{
					showException(ex);
					// JOptionPane.showMessageDialog(ParticlePickerJFrame.this,
					// ex.getMessage());
				}
			}
		});
		helpmn.add(hcontentsmi);

		keyassistmi = new JMenuItem("Key Assist...");
		keyassistmi.addActionListener(new ActionListener()
		{

			private QuickHelpJDialog keyassistdlg;

			@Override
			public void actionPerformed(ActionEvent e)
			{
				if (keyassistdlg == null)
					keyassistdlg = new QuickHelpJDialog(ParticlePickerJFrame.this, false, "Key Assist", getKeyAssist());
				keyassistdlg.setVisible(true);

			}
		});
		helpmn.add(keyassistmi);

		pmi = new JMenuItem("Particles", XmippResource.getIcon("table_view.gif"));
		pmi.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				loadParticles();
			}
		});

		mifilters = new ArrayList<JCheckBoxMenuItem>();
		filtersmn = new JMenu("Filters");
		filtersmn.addMenuListener(new MenuListener()
		{

			@Override
			public void menuCanceled(MenuEvent arg0)
			{
				// TODO Auto-generated method stub

			}

			@Override
			public void menuDeselected(MenuEvent arg0)
			{
				// TODO Auto-generated method stub

			}

			@Override
			public void menuSelected(MenuEvent arg0)
			{
				for (JCheckBoxMenuItem mi : mifilters)
					mi.setSelected(getParticlePicker().isFilterSelected(mi.getText()));

			}
		});

		addFilterMenuItem("Smooth Filter", true, picker);
		addFilterMenuItem("Bandpass Filter...", true, picker);

		JCheckBoxMenuItem admi = addFilterMenuItem("Anisotropic Diffusion...", false, picker);
		admi.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				activefilter = "8-bit";
				IJ.run(activefilter);
				activefilter = ((JCheckBoxMenuItem) e.getSource()).getText();
				IJ.run(activefilter);
			}
		});
		addFilterMenuItem("Mean Shift", true, picker);
		addFilterMenuItem("Subtract Background...", true, picker);
		addFilterMenuItem("Gaussian Blur...", true, picker);
		addFilterMenuItem("Brightness/Contrast...", true, picker);
		addFilterMenuItem("Invert LUT", true, picker);
	}

	protected abstract void openHelpURl();

	protected abstract void resetMicrograph();

	protected void enableEdition(boolean enable)
	{
		importffmi.setEnabled(enable);
		savemi.setEnabled(enable);
		sizesl.setEnabled(enable);
		colorbt.setEnabled(enable);
		resetbt.setEnabled(enable);
		exitbt.setEnabled(enable);
	}

	private JCheckBoxMenuItem addFilterMenuItem(String command, boolean defaultlistener, ParticlePicker picker)
	{
		JCheckBoxMenuItem mi = new JCheckBoxMenuItem(command);
		mifilters.add(mi);
		mi.setSelected(picker.isFilterSelected(command));
		if (defaultlistener)
			mi.addActionListener(this);
		filtersmn.add(mi);
		mi.setEnabled(picker.getMode() != FamilyState.ReadOnly);
		return mi;
	}

	@Override
	public void actionPerformed(ActionEvent e)
	{
		try
		{
			JCheckBoxMenuItem item = (JCheckBoxMenuItem) e.getSource();
			activefilter = item.getText();
			if (item.isSelected())// filter added, will be registered by picker
									// with options if needed
				if (activefilter.equals("Smooth Filter"))
				{
					getParticlePicker().addFilter("Smooth Filter", "xmipp");
					reloadImage();
				}
				else
				{
					for (int i = 0; i < WindowManager.getImageCount(); i++)
						IJ.run(WindowManager.getImage(i), activefilter, "");
				}
			else
			{
				// filter removed
				getParticlePicker().removeFilter(activefilter);
				reloadImage();
			}

			getParticlePicker().saveFilters();
		}
		catch (Exception ex)
		{

			ex.printStackTrace();
			showException(ex);
		}

	}

	protected void reloadImage()
	{
		getCanvas().getMicrograph().releaseImage();
		getCanvas().updateMicrograph();
		getCanvas().display();
	}

	protected void saveChanges()
	{
		getParticlePicker().saveData();
		setChanged(false);
	}

	public int getSide(int size)
	{
		return 100;
	}

	public Family getFamily()
	{
		return getParticlePicker().getFamily();
	}

	public abstract ParticlesJDialog initParticlesJDialog();

	public abstract ParticlePickerCanvas getCanvas();

	public void loadParticles()
	{
		try
		{
			if (particlesdialog == null)
				if (ParticlePickerJFrame.this instanceof TrainingPickerJFrame)
					particlesdialog = new ParticlesJDialog(ParticlePickerJFrame.this);
				else
					particlesdialog = new TiltPairParticlesJDialog(ParticlePickerJFrame.this);
			else
			{

				particlesdialog.loadParticles(false);
				particlesdialog.setVisible(true);
			}
		}
		catch (Exception ex)
		{
			showException(ex);
			if (particlesdialog != null)
				particlesdialog.close();
			particlesdialog = null;
		}

	}

	public void updateMicrographsModel()
	{
		updateMicrographsModel(false);

	}

	public abstract void updateMicrographsModel(boolean all);

	public ParticlesJDialog getParticlesJDialog()
	{
		return particlesdialog;
	}

	public abstract Micrograph getMicrograph();

	public abstract List<? extends TrainingParticle> getAvailableParticles();

	public boolean isPickingAvailable(MouseEvent e)
	{
		if (getCanvas().getTool() != Tool.PICKER)
			return false;
		if (SwingUtilities.isRightMouseButton(e))
			return false;
		if (getParticlePicker().getMode() == FamilyState.ReadOnly)
			return false;
		return true;
	}

	public boolean isPickingAvailable()
	{
		if (getCanvas().getTool() != Tool.PICKER)
			return false;
		if (getParticlePicker().getMode() == FamilyState.ReadOnly)
			return false;
		return true;
	}

	public class ColorActionListener implements ActionListener
	{
		JColorChooser colorChooser;

		@Override
		public void actionPerformed(ActionEvent e)
		{
			// Set up the dialog that the button brings up.
			colorChooser = new JColorChooser();
			JDialog dialog = JColorChooser.createDialog(colorbt, "Pick a Color", true, // modal
					colorChooser, new ActionListener()
					{

						@Override
						public void actionPerformed(ActionEvent e)
						{
							updateColor(colorChooser.getColor());
							getParticlePicker().setColor(colorChooser.getColor());
						}
					}, // OK button handler
					null); // no CANCEL button handler
			XmippWindowUtil.setLocation(0.5f, 0.5f, dialog);
			dialog.setVisible(true);
		}
	}

	public void updateColor(Color color)
	{
		if (colorbt != null)
			colorbt.setIcon(new ColorIcon(color));
		getParticlePicker().setColor(color);
		getCanvas().repaint();
		getParticlePicker().saveFamilies();
	}

	protected void initImagePane()
	{
		imagepn = new JPanel(new FlowLayout(FlowLayout.LEFT));
		imagepn.setBorder(BorderFactory.createTitledBorder("Image"));

		JPanel paintpn = new JPanel();
		usezoombt = new JToggleButton("-1", XmippResource.getIcon("zoom.png"));
		usezoombt.setToolTipText("Keep zoom");
		usezoombt.setFocusable(false);
		eraserbt = new JToggleButton("Eraser", XmippResource.getIcon("clean.gif"));

		JToolBar tb = new JToolBar();

		tb.setFloatable(false);
		tb.add(usezoombt);
		tb.add(eraserbt);
		// usezoombt.setBorderPainted(false);
		paintpn.add(tb);

		symbolpn = new JPanel(new FlowLayout(FlowLayout.LEFT));
		symbolpn.add(new JLabel("Symbol:"));
		// symbolpn.setBorder(BorderFactory.createTitledBorder("Symbol"));
		ShapeItemListener shapelistener = new ShapeItemListener();

		circlechb = new JCheckBox(Shape.Circle.toString());
		circlechb.setSelected(true);
		circlechb.addItemListener(shapelistener);

		rectanglechb = new JCheckBox(Shape.Rectangle.toString());
		rectanglechb.setSelected(true);
		rectanglechb.addItemListener(shapelistener);

		centerchb = new JCheckBox(Shape.Center.toString());
		centerchb.setSelected(true);
		centerchb.addItemListener(shapelistener);

		symbolpn.add(circlechb);
		symbolpn.add(rectanglechb);
		symbolpn.add(centerchb);

		imagepn.add(symbolpn);
		imagepn.add(paintpn);

	}

	protected void updateZoom()
	{
		double zoom = getZoom();
		if (zoom == -1. || (zoom != -1. && !usezoombt.isSelected()))
		{
			zoom = getCanvas().getMagnification();
			usezoombt.setText(String.format("%.2f", zoom));
		}
		else if (usezoombt.isSelected())
			getCanvas().setZoom(zoom);
	}

	public double getZoom()
	{
		return Double.parseDouble(usezoombt.getText());
	}

	public boolean isEraserMode()
	{
		return eraserbt.isSelected();
	}

	protected void displayZoom()
	{

		usezoombt.setText(String.format("%.2f", getCanvas().getMagnification()));
		pack();
	}

	class ShapeItemListener implements ItemListener
	{
		@Override
		public void itemStateChanged(ItemEvent e)
		{
			changeShapes();
		}
	}

	public void changeShapes()
	{
		getCanvas().repaint();

	}

	public boolean isShapeSelected(Shape shape)
	{
		switch (shape)
		{
		case Rectangle:
			return rectanglechb.isSelected();
		case Circle:
			return circlechb.isSelected();
		case Center:
			return centerchb.isSelected();
			// case OnlyLast:
			// return onlylastchb.isSelected();
		}
		return false;
	}

	public abstract ParticlePicker getParticlePicker();

	public abstract void setChanged(boolean changed);

	protected void initColorPane(Color color)
	{
		colorpn = new JPanel();
		this.color = color;
		colorpn.add(new JLabel("Color:"));
		colorbt = new JButton();
		colorbt.setIcon(new ColorIcon(color));
		colorbt.setBorderPainted(false);
		colorpn.add(colorbt);
	}

	protected void initSizePane()
	{
		sizepn = new JPanel();

		int size = getFamily().getSize();
		sizepn.add(new JLabel("Size:"));
		sizesl = new JSlider(0, 1000, size);
		sizesl.setPaintTicks(true);
		sizesl.setMajorTickSpacing(100);
		int height = (int) sizesl.getPreferredSize().getHeight();

		sizesl.setPreferredSize(new Dimension(100, height));
		sizepn.add(sizesl);

		sizetf = new JFormattedTextField(NumberFormat.getIntegerInstance());
		sizetf.setColumns(3);
		sizetf.setValue(size);
		sizepn.add(sizetf);
		sizetf.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				if(sizesl.getValue() ==((Number)sizetf.getValue()).intValue())//event from sizesl
					return;
				
				int size = ((Number) sizetf.getValue()).intValue();
				if (!getParticlePicker().isValidSize(size))
				{
					int prevsize = getFamily().getSize();
					JOptionPane.showMessageDialog(ParticlePickerJFrame.this, XmippMessage.getOutOfBoundsMsg("Family size " + size));
					sizetf.setText(Integer.toString(prevsize));
					return;
				}
				updateSize(size);
			}
		});

		sizesl.addChangeListener(new ChangeListener()
		{

			@Override
			public void stateChanged(ChangeEvent e)
			{
				if(sizesl.getValue() ==((Number)sizetf.getValue()).intValue())//event from sizetf
					return;
				
				int size = sizesl.getValue();
				if (!getParticlePicker().isValidSize(size))
				{
					int prevsize = getFamily().getSize();
					JOptionPane.showMessageDialog(ParticlePickerJFrame.this, XmippMessage.getOutOfBoundsMsg("Family size " + size));
					sizesl.setValue(prevsize);
					return;
				}
				updateSize(size);
			}
		});

	}

	public void updateSize(int size)
	{

		sizetf.setValue(size);
		sizesl.setValue(size);
		getCanvas().repaint();
		getFamily().setSize(size);
		if (particlesdialog != null)
		{
			for (TrainingParticle p : getAvailableParticles())
				p.resetParticleCanvas();
			loadParticles();
		}
		getParticlePicker().saveFamilies();
	}

	/** Shortcut function to show messages */
	public boolean showMessage(String message)
	{
		return XmippDialog.showInfo(this, message);
	}

	public boolean showException(Exception e)
	{
		return XmippDialog.showException(this, e);
	}

	public void close()
	{
		setVisible(false);
		dispose();
		if(getCanvas() != null)
			getCanvas().getIw().close();
		if(XmippIJUtil.getXmippImageJ() != null)
			XmippIJUtil.getXmippImageJ().close();
		XmippApplication.removeInstance();
	}

	protected abstract void resetData();

	public abstract String importParticles(Format format, String dir, float scale, boolean invertx, boolean inverty);

	protected void showImportDialog()
	{
		if (importpjd == null)
			importpjd = new ImportParticlesJDialog(ParticlePickerJFrame.this);
		importpjd.showDialog();
	}

	public Map<String, String> getKeyAssist()
	{
		Map<String, String> map = Collections.synchronizedMap(new LinkedHashMap<String, String>());
		map.put("Shift + Scroll Up", "Zoom in");
		map.put("Shift + Scroll Down", "Zoom out");
		map.put("Right click + Mouse move", "Moves image previously expanded");
		map.put("Left click", "Adds or selects a particle. If erase mode setted, deletes or disables selected particle");
		map.put("Shift + Left click", "Deletes or disables selected particle");
		map.put("Left click + Mouse move", "Moves selected particle. If erase mode setted, deletes or disables particle");
		map.put("Left click + Mouse move", "Moves selected particle. If erase mode setted, deletes or disables particle");
		map.put("Left", "Moves selected particle to the left");
		map.put("Right", "Moves selected particle to the right");
		map.put("Up", "Moves selected particle up");
		map.put("Down", "Moves selected particle down");
		return map;
	}

}
