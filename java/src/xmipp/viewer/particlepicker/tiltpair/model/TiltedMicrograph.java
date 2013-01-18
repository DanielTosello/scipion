package xmipp.viewer.particlepicker.tiltpair.model;

import java.util.ArrayList;
import java.util.List;

import xmipp.viewer.particlepicker.Micrograph;

public class TiltedMicrograph extends Micrograph
{

	private List<TiltedParticle> particles;
	private UntiltedMicrograph untiltedmicrograph;

	public TiltedMicrograph(String file)
	{
		super(file, getName(file, 1));
		particles = new ArrayList<TiltedParticle>();
	}

	@Override
	public boolean hasData()
	{
		return !particles.isEmpty();
	}

	@Override
	public void reset()
	{
		getParticles().clear();
	}

	public List<TiltedParticle> getParticles()
	{
		return particles;
	}

	public void removeParticle(UntiltedParticle p)
	{
		particles.remove(p);

	}

	public void addParticle(TiltedParticle p)
	{
		particles.add(p);

	}

	public TiltedParticle getParticle(int x, int y, int size)
	{
		for (TiltedParticle p : particles)
			if (p.contains(x, y, size))
				return p;
		return null;
	}

	public void removeParticle(TiltedParticle p)
	{
		if (p != null)
		{
			particles.remove(p);
			p.getUntiltedParticle().setTiltedParticle(null);
		}
	}

	public UntiltedMicrograph getUntiltedMicrograph()
	{
		return untiltedmicrograph;
	}

	public void setUntiltedMicrograph(UntiltedMicrograph untiltedmicrograph)
	{
		this.untiltedmicrograph = untiltedmicrograph;
	}

	

}
