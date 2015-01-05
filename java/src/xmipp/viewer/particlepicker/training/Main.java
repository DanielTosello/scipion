package xmipp.viewer.particlepicker.training;

import java.util.logging.Level;

import javax.swing.SwingUtilities;

import xmipp.utils.XmippDialog;
import xmipp.viewer.particlepicker.ParticlePicker;
import xmipp.viewer.particlepicker.training.gui.SupervisedPickerJFrame;
import xmipp.viewer.particlepicker.training.model.Mode;
import xmipp.viewer.particlepicker.training.model.SupervisedParticlePicker;

public class Main
{
    public static String selfile, outputdir;
	// 0 --> input metadata
	// 1 --> output dir
	// 2 --> mode

	// On Supervised
	// 3 --> number of threads for supervised mode
	// 4 --> fast mode for supervised mode
	// 5 --> incore for supervised mode

	// On Review
	// 3 -->external auto dir
	public static void main(String[] args)
	{

		final String[] myargs = args;

		SwingUtilities.invokeLater(new Runnable()
		{

			@Override
			public void run()
			{
				try
				{
					SupervisedParticlePicker ppicker = null;
					selfile = myargs[0];
					outputdir = myargs[1];
					//mode is the third argument 
					Mode mode = Mode.getMode(myargs[2]);

					if (mode == Mode.ReadOnly)
						ppicker = new SupervisedParticlePicker(selfile, outputdir, mode);

					else if (mode == Mode.Manual)
					{
						if (myargs.length > 3)
						{
							int index = 3;
							int threads = Integer.parseInt(myargs[index]);
							boolean fastmode = Boolean.parseBoolean(myargs[index + 1]);
							boolean incore = Boolean.parseBoolean(myargs[index + 2]);
							ppicker = new SupervisedParticlePicker(selfile, outputdir, threads, fastmode, incore);
						}
						else
							ppicker = new SupervisedParticlePicker(selfile, outputdir, Mode.Manual);
					}

					else if (mode == Mode.Review)
						ppicker = new SupervisedParticlePicker(selfile, outputdir, Mode.Review);

					new SupervisedPickerJFrame(ppicker);
				}
				catch (Exception e)
				{
					System.out.println("Error catched on main");
					ParticlePicker.getLogger().log(Level.SEVERE, e.getMessage(), e);
                                        XmippDialog.showError(null, e.getMessage());

				}
			}
		});

	}

}