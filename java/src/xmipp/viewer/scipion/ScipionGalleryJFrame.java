/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package xmipp.viewer.scipion;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JButton;
import xmipp.ij.commons.XmippUtil;
import xmipp.jni.MetaData;
import xmipp.utils.Param;
import xmipp.utils.XmippWindowUtil;
import xmipp.viewer.windows.GalleryJFrame;

/**
 *
 * @author airen
 */
public class ScipionGalleryJFrame extends GalleryJFrame {

    private final String type;
    private final String script;
    private final String projectid;
    private final String imagesid;
    private JButton cmdbutton;

    public ScipionGalleryJFrame(String filename, MetaData md, ScipionParams parameters) {
        super(filename, md, parameters);
        type = parameters.type;
        script = parameters.script;
        projectid = parameters.projectid;
        imagesid = parameters.imagesid;
        initComponents();
    }

    private void initComponents() {
        if (type != null) {
            cmdbutton = XmippWindowUtil.getTextButton("Create New Set Of " + type, new ActionListener() {

                @Override
                public void actionPerformed(ActionEvent ae) {
                    try {
                        String selectionmd = "selection.xmd";
                        selectionmd = new File(selectionmd).getAbsolutePath();
                        if(is2DClassSelection())
                            System.out.println("is2DClassification");
                        else
                            saveSelection(selectionmd, true);
                        String command = String.format("%s %s %s %s %s", script, selectionmd, type, projectid, imagesid);
                        XmippUtil.executeCommand(command);
                    } catch (Exception ex) {
                        Logger.getLogger(ScipionGalleryJFrame.class.getName()).log(Level.SEVERE, null, ex);
                        
                    }
                }
            });

            buttonspn.add(cmdbutton);
            cmdbutton.setEnabled(false);
        }
       
    }

    public void selectItem(int row, int col)
    {
        super.selectItem(row, col);
        cmdbutton.setEnabled(isImageSelected());
    }

    protected void tableMouseClicked(MouseEvent evt)
    {
        super.tableMouseClicked(evt);
        cmdbutton.setEnabled(isImageSelected());
    }


}
