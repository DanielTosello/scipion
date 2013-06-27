package xmipp.viewer.particlepicker.training.gui;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.text.NumberFormat;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFormattedTextField;
import javax.swing.JLabel;

import xmipp.utils.XmippDialog;
import xmipp.utils.XmippMessage;
import xmipp.utils.XmippWindowUtil;
import xmipp.viewer.particlepicker.training.model.Mode;

public class AdvancedOptionsJDialog extends JDialog
{

	protected SingleParticlePickerJFrame frame;
	protected int width, height;
	private JFormattedTextField templatestf;
	private JLabel checkpercentlb;
	private JFormattedTextField autopickpercenttf;
	private JButton okbt;

	public AdvancedOptionsJDialog(SingleParticlePickerJFrame frame)
	{
		super(frame);
		this.frame = frame;
		initComponents();

		addWindowListener(new WindowAdapter()
		{
			public void windowClosing(WindowEvent winEvt)
			{
				resetTemplatesJDialog();
			}

		});
	}

	protected void resetTemplatesJDialog()
	{
		frame.optionsdialog = null;
	}

	private void initComponents()
	{
		setDefaultCloseOperation(HIDE_ON_CLOSE);
		setTitle("Advanced Options");
		GridBagConstraints constraints = new GridBagConstraints();
		constraints.insets = new Insets(0, 5, 0, 5);
		constraints.anchor = GridBagConstraints.WEST;
		setLayout(new GridBagLayout());

		add(new JLabel("Templates:"), XmippWindowUtil.getConstraints(constraints, 0, 0));

		templatestf = new JFormattedTextField(NumberFormat.getNumberInstance());
		templatestf.setColumns(3);
		templatestf.setValue(frame.getParticlePicker().getTemplatesNumber());

		templatestf.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent arg0)
			{
				setTemplates();
			}
		});
	
		add(templatestf, XmippWindowUtil.getConstraints(constraints, 1, 0));
		checkpercentlb = new JLabel("Autopick Check (%):");
		add(checkpercentlb, XmippWindowUtil.getConstraints(constraints, 0, 1));
		autopickpercenttf = new JFormattedTextField(NumberFormat.getIntegerInstance());
		autopickpercenttf.setColumns(3);
		autopickpercenttf.setValue(frame.getParticlePicker().getAutopickpercent());
		autopickpercenttf.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent arg0)
			{

				setAutopickPercent();

			}
		});
		
		add(autopickpercenttf, XmippWindowUtil.getConstraints(constraints, 1, 1));

		okbt = XmippWindowUtil.getTextButton("Ok", new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent arg0)
			{
				setTemplates();
				setAutopickPercent();
				setVisible(false);

			}
		});
		add(okbt, XmippWindowUtil.getConstraints(constraints, 1, 2));
		enableOptions();
		XmippWindowUtil.setLocation(0.5f, 0.5f, this);
		setVisible(true);
		setAlwaysOnTop(true);
		pack();
	}

	void enableOptions()
	{
		templatestf.setEnabled(frame.getParticlePicker().getMode() == Mode.Manual);
		autopickpercenttf.setEnabled(frame.getParticlePicker().getMode() == Mode.Supervised || 
				frame.getParticlePicker().getMode() == Mode.Manual);
	}

	protected void setAutopickPercent()
	{
		if (autopickpercenttf.getValue() == null)
		{
			XmippDialog.showInfo(frame, XmippMessage.getEmptyFieldMsg("Check (%)"));
			autopickpercenttf.setValue(frame.getMicrograph().getAutopickpercent());
			return;
		}

		int autopickpercent = ((Number) autopickpercenttf.getValue()).intValue();
		frame.getMicrograph().setAutopickpercent(autopickpercent);
		frame.getParticlePicker().setAutopickpercent(autopickpercent);
		frame.getParticlePicker().saveConfig();
	}

	protected void setTemplates()
	{
		if (templatestf.getValue() == null)
		{
			XmippDialog.showInfo(frame, XmippMessage.getEmptyFieldMsg("Templates"));
			templatestf.setValue(frame.getParticlePicker().getTemplatesNumber());
			return;
		}

		int templates = ((Number) templatestf.getValue()).intValue();
		if (templates != frame.getParticlePicker().getTemplatesNumber())
		{
			frame.getParticlePicker().setTemplatesNumber(templates);
			frame.loadTemplates();
		}
		else if (frame.templatesdialog == null)
			frame.loadTemplates();
	}



	public void close()
	{
		setVisible(false);
		dispose();

	}

}
