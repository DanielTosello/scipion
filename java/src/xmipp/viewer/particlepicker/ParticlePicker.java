package xmipp.viewer.particlepicker;

import ij.CommandListener;
import ij.Executer;
import ij.IJ;
import ij.ImageListener;
import ij.ImagePlus;
import ij.plugin.frame.Recorder;

import java.awt.Color;
import java.io.File;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.swing.JOptionPane;
import xmipp.ij.commons.XmippUtil;

import xmipp.jni.Filename;
import xmipp.jni.MDLabel;
import xmipp.jni.MetaData;
import xmipp.jni.Program;
import xmipp.utils.XmippMessage;
import xmipp.viewer.particlepicker.training.model.Mode;
import xmipp.viewer.particlepicker.training.model.SupervisedParticlePickerMicrograph;

/**
 * Business object for ParticlePicker common GUI. SingleParticlePicker and
 * TiltPairPicker inherit from this class.
 *
 * @author airen
 *
 */
public abstract class ParticlePicker {

    protected String macrosfile;
    protected static Logger logger;
    protected String outputdir = ".";
    protected boolean changed;
    protected Mode mode;
    protected List<IJCommand> filters;
    protected String selfile;
    protected String command;
    protected String configfile;


    String[] commonfilters = new String[]{"Install...", "Duplicate", "Bandpass Filter...", "Anisotropic Diffusion...", "Mean Shift",
        "Subtract Background...", "Gaussian Blur...", "Brightness/Contrast...", "Invert LUT"};
    static String xmippsmoothfilter = "Xmipp Smooth Filter";
    public static final String particlesAutoBlock = "particles_auto";

    protected Color color;
    protected int size;

    public static final int sizemax = 1048;
    protected String block;
    Format[] formats = new Format[]{Format.Xmipp24, Format.Xmipp24a, Format.Xmipp24b, Format.Xmipp24c, Format.Xmipp30, Format.Xmipp301, Format.Eman};

    private static Color[] colors = new Color[]{Color.BLUE, Color.CYAN, Color.GREEN, Color.MAGENTA, Color.ORANGE, Color.PINK, Color.YELLOW};

    private static int nextcolor;

    public static Color getNextColor() {
        Color next = colors[nextcolor];
        nextcolor++;
        if (nextcolor == colors.length) {
            nextcolor = 0;
        }
        return next;
    }

    public static String getParticlesBlock(Format f, String file) {
        String blockname = getParticlesBlockName(f);
        if (f == null) {
            return null;
        }
        return blockname + "@" + file;

    }

    public static String getParticlesBlockName(Format f) {
        if (f == Format.Xmipp301) {
            return "particles";
        }
        if (f == Format.Xmipp30) {
            return "DefaultFamily";
        }
        return null;
    }

    public static String getParticlesBlock(String file) {
        return getParticlesBlock(Format.Xmipp301, file);

    }
    private ScipionSave scipionsave;

    public String getParticlesAutoBlock(String file) {
        return particlesAutoBlock + "@" + file;
    }

    public String getParticlesBlock(Micrograph m) {
        return getParticlesBlock(getOutputPath(m.getPosFile()));

    }

    public String getParticlesAutoBlock(Micrograph m) {
        return getParticlesAutoBlock(getOutputPath(m.getPosFile()));
    }

    public ParticlePicker(String selfile, Mode mode) {
        this(null, selfile, ".", mode);
    }

    public ParticlePicker(String selfile, String outputdir, Mode mode) {
        this(null, selfile, outputdir, mode);
    }

    public ParticlePicker(String block, String selfile, String outputdir, Mode mode) {
        this.block = block;
        this.outputdir = outputdir;
        this.selfile = selfile;
        this.mode = mode;
        this.configfile = getOutputPath("config.xmd");

        initFilters();
        loadEmptyMicrographs();
        loadConfig();
    }

    public void loadConfig() {

        String file = configfile;
        filters.clear();
        if (!new File(file).exists()) {
            color = getNextColor();
            size = getDefaultSize();
            setMicrograph(getMicrographs().get(0));

            filters.add(new IJCommand("Gaussian Blur...", "sigma=2"));
            return;
        }

        String mname;
        int rgb;
        try {
            MetaData md = new MetaData("properties@" + file);
            for (long id : md.findObjects()) {

                mname = md.getValueString(MDLabel.MDL_MICROGRAPH, id);
                setMicrograph(getMicrograph(mname));
                rgb = md.getValueInt(MDLabel.MDL_COLOR, id);
                color = new Color(rgb);
                size = md.getValueInt(MDLabel.MDL_PICKING_PARTICLE_SIZE, id);
            }
            md.destroy();
            String command, options;
            if (MetaData.containsBlock(file, "filters")) {
                md = new MetaData("filters@" + file);
                long[] ids = md.findObjects();
                for (long id : ids) {
                    command = md.getValueString(MDLabel.MDL_MACRO_CMD, id).replace('_', ' ');
                    options = md.getValueString(MDLabel.MDL_MACRO_CMD_ARGS, id).replace('_', ' ');
                    if (options.equals("NULL")) {
                        options = "";
                    }
                    filters.add(new IJCommand(command, options));
                }
                md.destroy();
            }
        } catch (Exception e) {
            getLogger().log(Level.SEVERE, e.getMessage(), e);
            throw new IllegalArgumentException(e.getMessage());
        }

    }

    public void saveConfig() {
        try {
            MetaData md;
            String file = configfile;
            md = new MetaData();
            long id = md.addObject();
            saveConfig(md, id);
            md.write("properties@" + file);
            md.destroy();
            String options;
            md = new MetaData();

            for (IJCommand f : filters) {
                id = md.addObject();
                md.setValueString(MDLabel.MDL_MACRO_CMD, f.getCommand().replace(' ', '_'), id);
                options = (f.getOptions() == null || f.getOptions().equals("")) ? "NULL" : f.getOptions().replace(' ', '_');
                md.setValueString(MDLabel.MDL_MACRO_CMD_ARGS, options, id);
            }
            md.writeBlock("filters@" + file);
            md.destroy();

        } catch (Exception e) {
            getLogger().log(Level.SEVERE, e.getMessage(), e);
            throw new IllegalArgumentException(e.getMessage());
        }

    }

    protected void saveConfig(MetaData md, long id) {
        md.setValueString(MDLabel.MDL_MICROGRAPH, getMicrograph().getName(), id);
        md.setValueInt(MDLabel.MDL_COLOR, getColor().getRGB(), id);
        md.setValueInt(MDLabel.MDL_PICKING_PARTICLE_SIZE, getSize(), id);
    }

    public Format detectFormat(String path) {

        String particlesfile;

        for (Format f : formats) {

            for (Micrograph m : getMicrographs()) {
                particlesfile = getImportMicrographName(path, m.getFile(), f);
                if (particlesfile != null && Filename.exists(particlesfile)) {
                    if (f != Format.Xmipp30 && f != Format.Xmipp301) {
                        return f;
                    }
                    if (f == Format.Xmipp30 && Filename.exists(Filename.join(path, "families.xmd"))) {
                        return f;
                    }
                    return Format.Xmipp301;
                }
            }
        }
        particlesfile = getExportFile(path);
        if (particlesfile != null) {
            if (Filename.exists(Filename.join(path, "families.xmd"))) {
                return Format.Xmipp30;
            }
            return Format.Xmipp301;

        }

        return Format.Unknown;
    }

    public String getExportFile(String path) {
        String particlesfile;
        File folderToScan = new File(path);
        File[] listOfFiles = folderToScan.listFiles();

        //Checking file with exported particles
        for (int i = 0; i < listOfFiles.length; i++) {
            if (listOfFiles[i].isFile()) {
                particlesfile = listOfFiles[i].getName();
                if (particlesfile.endsWith("extract_list.xmd")) {
                    return Filename.join(path, particlesfile);
                }
            }
        }
        return null;
    }

    public abstract String getImportMicrographName(String path, String filename, Format f);

    public String getPosFileFromXmipp24Project(String projectdir, String mname) {
        String suffix = ".raw.Common.pos";
        return String.format("%1$s%2$sPreprocessing%2$s%3$s%2$s%3$s%4$s", projectdir, File.separator, mname, suffix);
    }

    public abstract void loadEmptyMicrographs();

    private void initFilters() {

        filters = new ArrayList<IJCommand>();

        Recorder.record = true;

        // detecting if a command is thrown by ImageJ
        Executer.addCommandListener(new CommandListener() {
            public String commandExecuting(String command) {

                if (IJ.getInstance() != null && !Arrays.asList(commonfilters).contains(command) && !isRegisteredFilter(command)) {
                    String msg = String.format("Would you like to add filter: %s to preprocess micrographs?", command);
                    int result = JOptionPane.showConfirmDialog(null, msg);
                    if (result != JOptionPane.YES_OPTION) {
                        return command;
                    }
                }
                ParticlePicker.this.command = command;
                return command;

            }
        });
        ImagePlus.addImageListener(new ImageListener() {

            @Override
            public void imageUpdated(ImagePlus arg0) {
                updateFilters();
            }

            @Override
            public void imageOpened(ImagePlus arg0) {

            }

            @Override
            public void imageClosed(ImagePlus arg0) {
                // TODO Auto-generated method stub

            }
        });
    }

    protected boolean isRegisteredFilter(String command2) {
        for (IJCommand f : filters) {
            if (f.getCommand().equals(command)) {
                return true;
            }
        }
        return false;
    }

    private void updateFilters() {
        if (command != null) {
            String options = "";
            if (Recorder.getCommandOptions() != null) {
                options = Recorder.getCommandOptions();
            }

            if (!isFilterSelected(command)) {
                addFilter(command, options);
            } else if (!(options == null || options.equals(""))) {
                for (IJCommand f : filters) {
                    if (f.getCommand().equals(command)) {
                        f.setOptions(options);
                    }
                }
            }

            saveConfig();
            command = null;

        }
    }

    public String getMicrographsSelFile() {
        return selfile;
    }

    public void addFilter(String command, String options) {
        IJCommand f = new IJCommand(command, options);
        filters.add(f);
    }

    public List<IJCommand> getFilters() {
        return filters;
    }

    public void setChanged(boolean changed) {
        this.changed = changed;
    }

    public boolean isChanged() {
        return changed;
    }

    public Mode getMode() {
        return mode;
    }

    public static Logger getLogger() {
        try {
            if (logger == null) {
				//				FileHandler fh = new FileHandler("PPicker.log", true);
                //				fh.setFormatter(new SimpleFormatter());
                logger = Logger.getLogger("PPickerLogger");
                // logger.addHandler(fh);
            }
            return logger;
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return null;
    }

    public String getOutputPath(String file) {
        return outputdir + File.separator + file;
    }

    public String getOutputDir() {
        return outputdir;
    }

    public abstract List<? extends Micrograph> getMicrographs();

    public int getMicrographIndex() {
        return getMicrographs().indexOf(getMicrograph());
    }

    public String getTemplatesFile(String name) {
        return getOutputPath(name + "_templates.stk");
    }

    public Mode validateState(Mode state) {

        if (mode == Mode.Review && state != Mode.Review) {
            setChanged(true);
            return Mode.Review;
        }
        if (mode == Mode.Manual && !(state == Mode.Manual || state == Mode.Available)) {
            throw new IllegalArgumentException(String.format("Can not use %s mode on this data", mode));
        }
        if (mode == Mode.Supervised && state == Mode.Review) {
            throw new IllegalArgumentException(String.format("Can not use %s mode on this data", mode));
        }
        return state;

    }// function validateState

    public void saveData() {
        saveConfig();
		//saveData(getMicrograph());
        //setChanged(false);
    }// function saveData

    public abstract void saveData(Micrograph m);

    public Micrograph getMicrograph(String name) {
        for (Micrograph m : getMicrographs()) {
            if (m.getName().equalsIgnoreCase(name)) {
                return m;
            }
        }
        return null;
    }

    void removeFilter(String filter) {
        for (IJCommand f : filters) {
            if (f.getCommand().equals(filter)) {
                filters.remove(f);
                saveConfig();
                break;
            }
        }
    }// function removeFilter

    public boolean isFilterSelected(String filter) {
        for (IJCommand f : filters) {
            if (f.getCommand().equals(filter)) {
                return true;
            }
        }
        return false;
    }// function isFilterSelected

    public Format detectFileFormat(String path) {
        if (path.endsWith(".raw.Common.pos")) {
            return Format.Xmipp24;
        }
        if (path.endsWith(".pos")) {
            if (MetaData.containsBlock(path, getParticlesBlockName(Format.Xmipp30))) {
                return Format.Xmipp30;
            }
            return Format.Xmipp301;
        }
        if (path.endsWith(".box")) {
            return Format.Eman;
        }
        return Format.Unknown;
    }

    /**
     * Return the number of particles imported from a file
     */
    public void fillParticlesMdFromFile(String path, Format f, Micrograph m, MetaData md, float scale, boolean invertx, boolean inverty) {
        if (f == Format.Auto) {
            f = detectFileFormat(path);
        }
        String blockname;
        switch (f) {
            case Xmipp24:
            case Xmipp24a:
            case Xmipp24b:
            case Xmipp24c:
                md.readPlain(path, "xcoor ycoor");
                break;
            case Xmipp30:
                blockname = getParticlesBlockName(f); //only used with Xmipp
                if (MetaData.containsBlock(path, blockname)) {
                    md.read(getParticlesBlock(f, path));
                }
                break;
            case Xmipp301:
                fillParticlesMdFromXmipp301File(path, m, md);

                break;
            case Eman:
                fillParticlesMdFromEmanFile(path, m, md, scale);
                break;
            default:
                md.clear();
        }
        int width = (int) (m.width / scale);// original width
        int height = (int) (m.height / scale);// original height
        if (invertx) {
            md.operate(String.format("xcoor=%d-xcoor", width));
        }
        if (inverty) {
            md.operate(String.format("ycoor=%d-ycoor", height));
        }
        if (scale != 1.f) {
            md.operate(String.format("xcoor=xcoor*%f,ycoor=ycoor*%f", scale, scale));
        }

    }// function importParticlesFromFile
    
    public void fillParticlesMdFromXmipp301File(String file, Micrograph m, MetaData md) {
         String blockname = getParticlesBlockName(Format.Xmipp301); //only used with Xmipp
                if (MetaData.containsBlock(file, blockname)) {
                    md.read(getParticlesBlock(Format.Xmipp301, file));
                }
                if (MetaData.containsBlock(file, particlesAutoBlock)) {
                    MetaData mdAuto = new MetaData();
                    mdAuto.read(getParticlesAutoBlock(file));
                    mdAuto.removeDisabled();
                    md.unionAll(mdAuto);
                    mdAuto.destroy();
                }
                
    }

    

    
    public void fillParticlesMdFromEmanFile(String file, Micrograph m, MetaData md, float scale) {
        // inverty = true;
        md.readPlain(file, "xcoor ycoor particleSize");
		//System.err.format("After readPlain: md.size: %d\n", md.size());

        long fid = md.firstObject();
        int size = md.getValueInt(MDLabel.MDL_PICKING_PARTICLE_SIZE, fid);

        int half = size / 2;
        //System.err.format("Operate string: %s\n", String.format("xcoor=xcoor+%d,ycoor=ycoor+%d", half, half));
        md.operate(String.format("xcoor=xcoor+%d,ycoor=ycoor+%d", half, half));

        setSize(Math.round(size * scale));
    }// function fillParticlesMdFromEmanFile

    public void runXmippProgram(String program, String args) {
        try {
            Program.runByName(program, args);
        } catch (Exception e) {
            getLogger().log(Level.SEVERE, e.getMessage(), e);
            throw new IllegalArgumentException(e);
        }
    }

    public abstract Micrograph getMicrograph();

    public abstract void setMicrograph(Micrograph m);

    public abstract boolean isValidSize(int size);

    public int getSize() {
        return size;
    }

    public void setSize(int size) {
        if (size > ParticlePicker.sizemax) {
            throw new IllegalArgumentException(String.format("Max size is %s, %s not allowed", ParticlePicker.sizemax, size));
        }
        this.size = size;
    }

    public Color getColor() {
        return color;
    }

    public void setColor(Color color) {
        this.color = color;
    }

    public static Color getColor(String name) {
        Color color;
        try {
            Field field = Class.forName("java.awt.Color").getField(name);
            color = (Color) field.get(null);// static field, null for parameter
        } catch (Exception e) {
            color = null; // Not defined
        }
        return color;
    }

    public static int getDefaultSize() {
        return 100;
    }

    public int getRadius() {
        return size / 2;
    }
    
    public void importSize(String path, Format f)
    {
        if(f == Format.Xmipp301)
                {
                    String configfile = String.format("%s%s%s", path, File.separator, "config.xmd");
                    if(new File(configfile).exists())
                    {
                        MetaData configmd = new MetaData(configfile);
                        int size = configmd.getValueInt(MDLabel.MDL_PICKING_PARTICLE_SIZE, configmd.firstObject());
                        setSize(size);
                        saveConfig();
                    }
                }
    }


     public void addScipionSave(String script, String projectid, String inputid) {
            scipionsave = new ScipionSave(script, projectid, inputid);
     }
     
     public boolean isScipionSave()
     {
         return scipionsave != null;
     }

    void doScipionSave() {
        try {
            
            String cmd = String.format("%s %s %s %s", scipionsave.script, outputdir, scipionsave.projectid, scipionsave.inputid);;
            XmippUtil.executeCommand(cmd);
        } catch (Exception ex) {
            Logger.getLogger(ParticlePicker.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
     
     

    class ScipionSave {

        public String script, projectid, inputid;
        

        public ScipionSave(String script, String projectid, String inputid) {
            
            this.script = script;
            this.projectid = projectid;
            this.inputid = inputid;
        }
    }
    


}
