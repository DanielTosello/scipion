
package xmipp.viewer.particlepicker.training.gui;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.event.MouseEvent;
import java.util.List;
import javax.swing.SwingUtilities;
import xmipp.viewer.particlepicker.Micrograph;
import xmipp.viewer.particlepicker.ParticlePickerCanvas;
import xmipp.viewer.particlepicker.ParticlePickerJFrame;
import xmipp.viewer.particlepicker.training.model.AutomaticParticle;
import xmipp.viewer.particlepicker.training.model.FamilyState;
import xmipp.viewer.particlepicker.training.model.MicrographFamilyData;
import xmipp.viewer.particlepicker.training.model.TrainingMicrograph;
import xmipp.viewer.particlepicker.training.model.TrainingParticle;
import xmipp.viewer.particlepicker.training.model.TrainingPicker;

public class TrainingCanvas extends ParticlePickerCanvas
{

	private TrainingPickerJFrame frame;
	private TrainingMicrograph micrograph;
	private TrainingParticle active;
	private TrainingPicker ppicker;

	

	public TrainingCanvas(TrainingPickerJFrame frame)
	{
		super(frame.getMicrograph().getImagePlus(frame.getParticlePicker().getFilters()));
		
		this.micrograph = frame.getMicrograph();
		this.frame = frame;
		this.ppicker = frame.getParticlePicker();
		micrograph.runImageJFilters(ppicker.getFilters());
		if(!frame.getFamilyData().getParticles().isEmpty())
			active = getLastParticle();
		else
			active = null;

	}

	public void updateMicrograph()
	{
		this.micrograph = frame.getMicrograph();
		updateMicrograph();
		if(!frame.getFamilyData().getParticles().isEmpty())
			refreshActive(getLastParticle());
		else
			active = null;
		
	}
	
	TrainingParticle getLastParticle()
	{
		return frame.getFamilyData().getLastAvailableParticle(frame.getThreshold());
	}


	public void mousePressed(MouseEvent e)
	{
		super.mousePressed(e);
		int x = super.offScreenX(e.getX());
		int y = super.offScreenY(e.getY());
		if (frame.isPickingAvailable(e))
		{
			if (frame.isEraserMode())
			{
				erase(x, y);
				return;
			}


			TrainingParticle p = micrograph.getParticle(x, y);
			if (p == null)
				p = micrograph.getAutomaticParticle(x, y, frame.getThreshold());
			if (p != null && SwingUtilities.isLeftMouseButton(e))
			{
				active = p;
				repaint();

			}
			else if (SwingUtilities.isLeftMouseButton(e) && micrograph.fits(x, y, frame.getFamily().getSize()))
			{
				p = new TrainingParticle(x, y, frame.getFamily(), micrograph);
				micrograph.addManualParticle(p);
				ppicker.addParticleToTemplates(p, frame.isCenterPick());
				active = p;
				refresh();
			}
		}
	}

	protected void erase(int x, int y)
	{
		micrograph.removeParticles(x, y, ppicker);
		active = getLastParticle();
		refresh();
	}
	
	/**
	 * Updates particle position and repaints if onpick.
	 */
	@Override
	public void mouseDragged(MouseEvent e)
	{

		super.mouseDragged(e);
		int x = super.offScreenX(e.getX());
		int y = super.offScreenY(e.getY());

		if (frame.isPickingAvailable(e))
		{
			if (frame.isEraserMode())
			{
				erase(x, y);
				return;
			}
			if (active == null)
				return;

			if (!micrograph.fits(x, y, active.getFamily().getSize()))
				return;
			if (active instanceof AutomaticParticle)
			{
				micrograph.removeParticle(active, ppicker);
				active = new TrainingParticle(active.getX(), active.getY(), active.getFamily(), micrograph);
				micrograph.addManualParticle(active);
			}
			else
			{
				setActiveMoved(true);
				moveActiveParticle(x, y);
			}
			frame.setChanged(true);
			repaint();
		}
	}
	
	@Override
	public void mouseReleased(MouseEvent e)
	{

		super.mouseReleased(e);
		int x = super.offScreenX(e.getX());
		int y = super.offScreenY(e.getY());
		if (frame.isPickingAvailable(e))
		{
			
			//deleting when mouse released, takes less updates to templates and frame
			if (active != null && SwingUtilities.isLeftMouseButton(e) && e.isShiftDown())
			{
				micrograph.removeParticle(active, ppicker);
				active = getLastParticle();
				refresh();
				if (!(active instanceof AutomaticParticle))
					frame.updateTemplates();
			}
			else
				manageActive(x, y);
			if (activemoved)
			{
				frame.updateTemplates();
				setActiveMoved(false);
			}

		}
	}
	
	public void manageActive(int x, int y)
	{
		if (!activemoved)
			return;

		if (!micrograph.fits(x, y, active.getFamily().getSize()))
			return;
		
		if (active instanceof AutomaticParticle && !((AutomaticParticle)active).isDeleted())
		{

			micrograph.removeParticle(active, ppicker);
			active = new TrainingParticle(active.getX(), active.getY(), active.getFamily(), micrograph);
			micrograph.addManualParticle(active);
			repaint();
		}
		else
		{
			moveActiveParticle(x, y);
			repaint();

		}
		frame.setChanged(true);
		setActiveMoved(false);
	}

	
	

	protected void doCustomPaint(Graphics2D g2)
	{
		if (frame.getFamily().getStep() == FamilyState.Manual)
			for (MicrographFamilyData mfdata : micrograph.getFamiliesData())
				drawFamily(g2, mfdata);
		else
			drawFamily(g2, micrograph.getFamilyData(frame.getFamily()));
		if (active != null)
		{
			g2.setColor(Color.red);
			BasicStroke activest = (active instanceof AutomaticParticle) ? activedst : activecst;

			drawShape(g2, active, true, activest);
		}

	}

	private void drawFamily(Graphics2D g2, MicrographFamilyData mfdata)
	{
		List<TrainingParticle> particles;
		int index;
		if (!mfdata.isEmpty())
		{
			particles = mfdata.getManualParticles();
			g2.setColor(mfdata.getFamily().getColor());

			for (index = 0; index < particles.size(); index++)
				drawShape(g2, particles.get(index), index == particles.size() - 1, continuousst);

			List<AutomaticParticle> autoparticles = mfdata.getAutomaticParticles();
			for (int i = 0; i < autoparticles.size(); i++)
				if (!autoparticles.get(i).isDeleted() && autoparticles.get(i).getCost() >= frame.getThreshold())
					drawShape(g2, autoparticles.get(i), false, dashedst);

		}
	}

	@Override
	public void refreshActive(TrainingParticle p)
	{
		if (p == null)
			active = null;
		else
			active = (TrainingParticle) p;
		repaint();

	}
	
	@Override
	public ParticlePickerJFrame getFrame()
	{
		return frame;
	}

	@Override
	public Micrograph getMicrograph()
	{
		return micrograph;
	}

	@Override
	public TrainingParticle getActive()
	{
		return active;
	}

	@Override
	public void setMicrograph(Micrograph m)
	{
		micrograph = (TrainingMicrograph) m;

	}

}
