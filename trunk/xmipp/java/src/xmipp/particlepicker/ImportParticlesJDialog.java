package xmipp.particlepicker;

import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;

import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTextField;
import xmipp.utils.WindowUtil;
import xmipp.utils.XmippMessage;

public class ImportParticlesJDialog extends JDialog
{

	ParticlePickerJFrame parent;
	private JRadioButton xmipp24rb;
	private JRadioButton xmipp30rb;
	private JRadioButton emanrb;
	private JPanel sourcepn;
	private JTextField sourcetf;
	private ButtonGroup formatgroup;
	public Format format;

	public ImportParticlesJDialog(ParticlePickerJFrame parent, boolean modal)
	{
		super(parent, modal);
		setResizable(false);
		this.parent = parent;
		setDefaultCloseOperation(DISPOSE_ON_CLOSE);
		setTitle("Import Particles");
		setLayout(new GridBagLayout());
		GridBagConstraints constraints = new GridBagConstraints();
		constraints.insets = new Insets(5, 5, 5, 5);
		constraints.anchor = GridBagConstraints.WEST;
		initSourcePane();
		add(new JLabel("Format:"), WindowUtil.getConstraints(constraints, 0, 0, 1));
		add(sourcepn, WindowUtil.getConstraints(constraints, 1, 0, 2));
		add(new JLabel("Source:"), WindowUtil.getConstraints(constraints, 0, 1, 1));
		sourcetf = new JTextField(20);
		add(sourcetf, WindowUtil.getConstraints(constraints, 1, 1, 1));
		JButton browsebt = new JButton("Browse");
		add(browsebt, WindowUtil.getConstraints(constraints, 2, 1, 1));
		browsebt.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				browseDirectory();
			}

		});
		JPanel actionspn = new JPanel(new FlowLayout(FlowLayout.RIGHT));
		JButton okbt = new JButton("OK");
		okbt.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent arg0)
			{
				try
				{
					importParticles();
					setVisible(false);
					dispose();
				}
				catch (Exception ex)
				{
					JOptionPane.showMessageDialog(ImportParticlesJDialog.this, ex.getMessage());
				}
			}
		});
		JButton cancelbt = new JButton("Cancel");
		cancelbt.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent arg0)
			{
				setVisible(false);
				dispose();

			}
		});
		actionspn.add(okbt);
		actionspn.add(cancelbt);
		add(actionspn, WindowUtil.getConstraints(constraints, 0, 2, 3));
		pack();
		WindowUtil.setLocation(0.8f, 0.5f, this);
		setVisible(true);

	}

	private void browseDirectory()
	{
		JFileChooser fc = new JFileChooser();
		fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
		int returnVal = fc.showOpenDialog(ImportParticlesJDialog.this);

		try
		{
			if (returnVal == JFileChooser.APPROVE_OPTION)
			{
				String path = fc.getSelectedFile().getAbsolutePath();
				sourcetf.setText(path);
			}
		}
		catch (Exception ex)
		{
			JOptionPane.showMessageDialog(ImportParticlesJDialog.this, ex.getMessage());
		}

	}

	protected void initSourcePane()
	{

		sourcepn = new JPanel(new FlowLayout(FlowLayout.LEFT));

		FormatItemListener shapelistener = new FormatItemListener();

		formatgroup = new ButtonGroup();
		xmipp24rb = new JRadioButton(Format.Xmipp24.toString());
		xmipp24rb.setSelected(true);
		xmipp24rb.addItemListener(shapelistener);

		xmipp30rb = new JRadioButton(Format.Xmipp30.toString());
		xmipp30rb.addItemListener(shapelistener);

		emanrb = new JRadioButton(Format.Eman.toString());
		emanrb.addItemListener(shapelistener);

		sourcepn.add(xmipp24rb);
		sourcepn.add(xmipp30rb);
		sourcepn.add(emanrb);

		formatgroup.add(xmipp24rb);
		formatgroup.add(xmipp30rb);
		formatgroup.add(emanrb);

	}

	class FormatItemListener implements ItemListener
	{

		@Override
		public void itemStateChanged(ItemEvent e)
		{
			JRadioButton formatrb = (JRadioButton) e.getSource();
			format = Format.valueOf(formatrb.getText());
		}
	}

	private void importParticles()
	{

		String path = sourcetf.getText();
		if (path == null || path.equals(""))
			throw new IllegalArgumentException(XmippMessage.getEmptyFieldMsg("directory"));
		switch (format)
		{
		case Xmipp24:
			parent.importParticlesXmipp24(path);
			break;
		case Xmipp30:
			parent.importParticlesXmipp30(path);
			break;
		case Eman:
			parent.importParticlesEman(path);
			break;

		}
		JOptionPane.showMessageDialog(ImportParticlesJDialog.this, "Import successful");

	}
}
