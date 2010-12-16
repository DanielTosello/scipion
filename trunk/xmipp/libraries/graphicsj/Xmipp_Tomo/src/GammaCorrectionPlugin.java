/***************************************************************************
 *
 * @author: Jesus Cuenca (jcuenca@cnb.csic.es)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

import ij.gui.GenericDialog;


public class GammaCorrectionPlugin extends Plugin {
	// default value - @see xmipptomo.Plugin.collectParameters, ij.plugin.filter.ImageMath
	private double gammaValue=2.0;
	private static String COMMAND="Gamma...";
	
	
	public String getCommand(){
		return COMMAND;
	}
	
	// adapt radius if the original image was resized 
	public String getOptions(){
		return "Gamma=" + gammaValue;
	}
	
	@Override
	public void collectParameters(GenericDialog gd) {
		if(gd != null)
			gammaValue=gd.getNextNumber();
	}

	
	public String toString(){
		return getOptions();
	}

}
