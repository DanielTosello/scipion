package xmipp.viewer.particlepicker.tiltpair;


import java.util.logging.Level;

import javax.swing.SwingUtilities;
import org.apache.commons.cli.BasicParser;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import xmipp.ij.commons.XmippApplication;
import xmipp.utils.XmippDialog;
import xmipp.viewer.particlepicker.ParticlePicker;
import xmipp.viewer.particlepicker.ParticlePickerParams;
import xmipp.viewer.particlepicker.tiltpair.gui.TiltPairPickerJFrame;
import xmipp.viewer.particlepicker.tiltpair.model.TiltPairPicker;
import xmipp.viewer.particlepicker.training.model.Mode;

public class TiltPairPickerRunner implements Runnable {

    
    
    private final ParticlePickerParams params;


    public TiltPairPickerRunner(String[] args) throws ParseException {

        params = new ParticlePickerParams(args);

    }
	// 0 --> input metadata
    // 1 --> output dir
    // 2 --> mode


    

    @Override
    public void run() {
        
            TiltPairPicker ppicker = null;
            ppicker = new TiltPairPicker(params.inputfile, params.outputdir, params.mode, params);
            
            if(params.isScipion())
                XmippApplication.setIsScipion(true);

            new TiltPairPickerJFrame(ppicker);
        
    }

    public static void main(String[] args) {
        try {
            TiltPairPickerRunner spr = new TiltPairPickerRunner(args);
            SwingUtilities.invokeLater(spr);
        } catch (Exception e) {
            System.out.println("Error catched on main");
            ParticlePicker.getLogger().log(Level.SEVERE, e.getMessage(), e);
            XmippDialog.showException(null, e);

        }

    }

}
