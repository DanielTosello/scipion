package xmipp.viewer.particlepicker.training.gui;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Rectangle;
import java.awt.event.MouseEvent;
import java.util.List;
import javax.swing.SwingUtilities;
import xmipp.jni.Particle;
import xmipp.utils.TasksManager;
import xmipp.viewer.particlepicker.Micrograph;
import xmipp.viewer.particlepicker.ParticlePickerCanvas;
import xmipp.viewer.particlepicker.ParticlePickerJFrame;
import xmipp.viewer.particlepicker.ParticleToTemplatesTask;
import xmipp.viewer.particlepicker.training.model.AutomaticParticle;
import xmipp.viewer.particlepicker.training.model.SingleParticlePicker;
import xmipp.viewer.particlepicker.training.model.SingleParticlePickerMicrograph;
import xmipp.viewer.particlepicker.training.model.ManualParticle;

public class SingleParticlePickerCanvas extends ParticlePickerCanvas
{

	private SingleParticlePickerJFrame frame;
	private SingleParticlePickerMicrograph micrograph;
	private ManualParticle active;
	private SingleParticlePicker ppicker;

	public SingleParticlePickerCanvas(SingleParticlePickerJFrame frame)
	{
		super(frame.getMicrograph().getImagePlus(frame.getParticlePicker().getFilters()));

		this.micrograph = frame.getMicrograph();
		this.frame = frame;
		this.ppicker = frame.getParticlePicker();
		micrograph.runImageJFilters(ppicker.getFilters());
		active = getLastParticle();

	}

	protected ManualParticle getLastParticle()

	{
		if (micrograph.getParticles().isEmpty())
			return null;
		return micrograph.getLastAvailableParticle(frame.getThreshold());
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

			ManualParticle p = micrograph.getParticle(x, y);
			if (p == null)
				p = micrograph.getAutomaticParticle(x, y, frame.getThreshold());
			if (p != null && SwingUtilities.isLeftMouseButton(e))
			{
				active = p;
				repaint();

			}
			else if (SwingUtilities.isLeftMouseButton(e) && micrograph.fits(x, y, frame.getParticlePicker().getSize()))
			{
				p = new ManualParticle(x, y, frame.getParticlePicker(), micrograph);
				micrograph.addManualParticle(p, ppicker, frame.isCenterParticle());
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
		if (SwingUtilities.isLeftMouseButton(e))
		{
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

				if (!micrograph.fits(x, y, active.getParticlePicker().getSize()))
					return;
				if (active instanceof AutomaticParticle)
				{
					micrograph.removeParticle(active, ppicker);
					active = new ManualParticle(active.getX(), active.getY(), ppicker, micrograph);
					micrograph.addManualParticle(active, ppicker, frame.isCenterParticle());
				}
				else
				{
					setActiveMoved(true);
					moveActiveParticle(x, y);
				}

				manageActive(x, y);

			}
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
			if (frame.isEraserMode())
			{
				erase(x, y);
				return;
			}
			//deleting when mouse released, takes less updates to templates and frame
			if (active != null && SwingUtilities.isLeftMouseButton(e) && e.isShiftDown())
			{
				micrograph.removeParticle(active, ppicker);
				active = getLastParticle();
				refresh();

			}
			else
				manageActive(x, y);
			if (activemoved)
			{
				setActiveMoved(false);
				TasksManager.getInstance().addTask(new ParticleToTemplatesTask(active));
			}
		}
	}

	public void manageActive(int x, int y)
	{
		if (!activemoved)
			return;

		if (!micrograph.fits(x, y, active.getSize()))
			return;

		if (active instanceof AutomaticParticle && !((AutomaticParticle) active).isDeleted())
		{

			micrograph.removeParticle(active, ppicker);
			active = new ManualParticle(active.getX(), active.getY(), active.getParticlePicker(), micrograph);
			micrograph.addManualParticle(active, ppicker, frame.isCenterParticle());
			repaint();
		}
		else
		{
			//changing particle position requires to remove it from template and add it again
			moveActiveParticle(x, y);
			repaint();

		}
		frame.setChanged(true);
	}

	protected void doCustomPaint(Graphics2D g2)
	{
		List<ManualParticle> particles;
		int index;
		if (!micrograph.isEmpty())
		{
			particles = micrograph.getManualParticles();
			g2.setColor(ppicker.getColor());

			for (index = 0; index < particles.size(); index++)
				drawShape(g2, particles.get(index), index == particles.size() - 1, continuousst);

			List<AutomaticParticle> autoparticles = micrograph.getAutomaticParticles();
			for (int i = 0; i < autoparticles.size(); i++)
				if (!autoparticles.get(i).isDeleted() && autoparticles.get(i).getCost() >= frame.getThreshold())
					drawShape(g2, autoparticles.get(i), false, dashedst);

		}
		if (active != null)
		{
			g2.setColor(Color.red);
			BasicStroke activest = (active instanceof AutomaticParticle) ? activedst : activecst;
			drawShape(g2, active, true, activest);
		}
		Rectangle autopickout = frame.getParticlesRectangle();
		if (autopickout != null)
		{
			g2.setColor(Color.yellow);
			g2.setStroke(continuousst);
			int x = getXOnImage((int) autopickout.getX());
			int y = getYOnImage((int) autopickout.getY());
			g2.drawRect(x, y, (int) (autopickout.getWidth() * magnification), (int) (autopickout.getHeight() * magnification));
		}

	}

	@Override
	public void refreshActive(Particle p)
	{
		if (p == null)
			active = null;
		else
			active = (ManualParticle) p;
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
	public ManualParticle getActive()
	{
		return active;
	}

	@Override
	public void setMicrograph(Micrograph m)
	{
		micrograph = (SingleParticlePickerMicrograph) m;

	}

}
