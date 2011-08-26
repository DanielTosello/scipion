package gui;

import ij.gui.ImageCanvas;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.MouseEvent;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.util.List;

import javax.swing.SwingUtilities;

import model.State;
import model.AutomaticParticle;
import model.Micrograph;
import model.MicrographFamilyData;
import model.Particle;
import model.Step;

public class ParticlePickerCanvas extends ImageCanvas implements
		MouseWheelListener {

	private ParticlePickerJFrame frame;
	private Micrograph micrograph;
	private Particle dragged;
	final static BasicStroke dashedst = new BasicStroke(1.0f,
			BasicStroke.CAP_BUTT, BasicStroke.JOIN_MITER, 10.0f,
			new float[] { 10.0f }, 0.0f);
	final static BasicStroke continuousst = new BasicStroke();

	public ParticlePickerCanvas(ParticlePickerJFrame frame) {
		super(frame.getMicrograph().getImage());
		this.micrograph = frame.getMicrograph();
		this.frame = frame;
		addMouseWheelListener(this);
	}
	
	public void updateMicrograph() {
		this.micrograph = frame.getMicrograph();
		imp = micrograph.getImage();
		setImageUpdated();
		repaint();
	}

	/**
	 * Adds particle or updates its position if onpick. If ondeletepick removes
	 * particle. Considers owner for selection to the first particle containing
	 * point. Sets dragged if onpick
	 */

	public void mousePressed(MouseEvent e) {
		if (frame.getTool() != Tool.PICKER) {
			super.mousePressed(e);
			return;
		}
		int x = super.offScreenX(e.getX());
		int y = super.offScreenY(e.getY());

		if (SwingUtilities.isRightMouseButton(e)) {
			setupScroll(x, y);
			return;
		}
		if (frame.getFamilyData().isPickingAvailable()) {
			Particle p = micrograph.getParticle(x, y);
			if (p != null) {
				if (SwingUtilities.isLeftMouseButton(e) && e.isControlDown()) {
					micrograph.removeParticle(p);
					frame.updateMicrographsModel();
				} else if (SwingUtilities.isLeftMouseButton(e)) {
					dragged = p;
				}
			} else if (SwingUtilities.isLeftMouseButton(e)
					&& Particle.boxContainedOnImage(x, y, frame.getFamily()
							.getSize(), imp)) {
				p = new Particle(x, y, frame.getFamily(), micrograph);
				micrograph.addManualParticle(p);
				dragged = p;
				frame.updateMicrographsModel();
			}
			frame.setChanged(true);
			repaint();
		}
	}

	
	/**
	 * Updates particle position and repaints. Sets dragged to null at the end
	 */
	public void mouseReleased(MouseEvent e) {
		if (!frame.getFamilyData().isPickingAvailable() || frame.getTool() != Tool.PICKER) {
			super.mouseReleased(e);
			return;
		}
		dragged = null;
	}

	/**
	 * Updates particle position and repaints if onpick.
	 */
	@Override
	public void mouseDragged(MouseEvent e) {

		if (frame.getTool() != Tool.PICKER) {
			super.mouseDragged(e);
			return;
		}
		int x = super.offScreenX(e.getX());
		int y = super.offScreenY(e.getY());
		if (SwingUtilities.isRightMouseButton(e)) {
			scroll(e.getX(), e.getY());
			return;
		}
		if (frame.getFamilyData().isPickingAvailable()) {
			if (dragged == null)
				return;

			if (!Particle.boxContainedOnImage(x, y, dragged.getFamily()
					.getSize(), imp))
				return;
			if(dragged instanceof AutomaticParticle)
			{
				micrograph.removeParticle(dragged);
				dragged = new Particle(dragged.getX(), dragged.getY(), dragged.getFamily(), micrograph);
				micrograph.addManualParticle(dragged);
			}
			else
				dragged.setPosition(x, y);
			frame.setChanged(true);
			repaint();
		}
	}
	
	@Override
	public void mouseWheelMoved(MouseWheelEvent e) {
		int x = super.offScreenX(e.getX());
		int y = super.offScreenY(e.getY());

		int rotation = e.getWheelRotation();
		System.out.println(rotation);
		if (rotation < 0)
			zoomIn(x, y);
		else
			zoomOut(x, y);

	}

	public void paint(Graphics g) {
		super.paint(g);
		Graphics2D g2 = (Graphics2D) g;
		if(frame.getFamily().getStep() == Step.Manual)
			for (MicrographFamilyData mfdata : micrograph.getFamiliesData()) 
				drawFamily(g2, mfdata);
		else
			drawFamily(g2, micrograph.getFamilyData(frame.getFamily()));
		
	}
	
	private void drawFamily(Graphics2D g2, MicrographFamilyData mfdata)
	{
		int x0 = (int) getSrcRect().getX();
		int y0 = (int) getSrcRect().getY();
		
		int radius;
		List<Particle> particles;
		int index;
		if(!mfdata.isEmpty())
		{
			particles = mfdata.getManualParticles();
			g2.setColor(mfdata.getFamily().getColor());
			radius = (int) (mfdata.getFamily().getSize() / 2 * magnification);
			g2.setStroke(continuousst);
			for (index = 0; index < particles.size(); index ++) 
				drawShape(g2, particles.get(index), x0, y0, radius, index + 1);
			g2.setStroke(dashedst);
			List<AutomaticParticle> autoparticles = mfdata.getAutomaticParticles();
			for (int i = 0; i < autoparticles.size(); i ++)
				if(!autoparticles.get(i).isDeleted())
					drawShape(g2, autoparticles.get(i), x0, y0, radius, index + i);
		}
	
	}

	private void drawShape(Graphics2D g2, Particle p, int x0, int y0, int radius,
			int label) {
		
		int x = (int) ((p.getX() - x0) * magnification);
		int y = (int) ((p.getY() - y0) * magnification);
		if (frame.isShapeSelected(Shape.Rectangle))
			g2.drawRect(x - radius, y - radius, radius * 2, radius * 2);
		if (frame.isShapeSelected(Shape.Circle))
			g2.drawOval(x - radius, y - radius, radius * 2, radius * 2);
		if (frame.isShapeSelected(Shape.Center)) {
			g2.drawLine(x - 2, y - 2, x + 2, y + 2);
			g2.drawLine(x + 2, y - 2, x - 2, y + 2);
		}
		g2.drawString(Integer.toString(label), x, y - radius);
	}





}
