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

/**
 * Why?
 * Workflow is the model for WorkflowView, a DefaultTreeModel capable of handling UserActions
 */

import java.util.Enumeration;
import java.util.LinkedList;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;


public class Workflow extends DefaultTreeModel{
	
	// TODO: check what protocol workflow code (from python) can be reused around here...
	
	private UserAction selectedUserAction;
	
	public UserAction getSelectedUserAction() {
		return selectedUserAction;
	}


	public void setSelectedUserAction(UserAction selectedUserAction) {
		this.selectedUserAction = selectedUserAction;
	}


	public DefaultMutableTreeNode getRoot() {
		return (DefaultMutableTreeNode)super.getRoot();
	}


	public Workflow(){
		super(new DefaultMutableTreeNode(UserAction.start()));
	}
	
	public static Workflow getTestWorkflow(){
		// Use workflow class to encapsulate the tree and allow
		// this method to be static
		UserAction a1=new UserAction(0,"Load","Load" , "g1ta.spi");
		UserAction a2=new UserAction(0,"Gaussian", "Gaussian Blur..." , "Sigma (Radius)=2.0");
		UserAction a3=new UserAction(0,"Median","Median..." , "Sigma (Radius)=2.0");
		UserAction a4=new UserAction(0,"Bandpass Filter","Bandpass Filter..." , "Filter_Large Structures Down to=40.0Filter_Small Structures Up to=3.0Suppress Stripes:=Tolerance of Direction:=5.0Autoscale After Filtering=trueSaturate Image when AutoscalingtrueDisplay Filterfalse");
		
		Workflow testWorkflow=new Workflow();
		DefaultMutableTreeNode n1=testWorkflow.addUserAction(testWorkflow.getRoot(), a1);
		DefaultMutableTreeNode n2=testWorkflow.addUserAction(n1, a2);
		DefaultMutableTreeNode n3=testWorkflow.addUserAction(n2, a3);
		DefaultMutableTreeNode n4=testWorkflow.addUserAction(n2, a4);
		
		return testWorkflow;
	}
	
	public DefaultMutableTreeNode addUserAction(DefaultMutableTreeNode parent, UserAction newAction){
		DefaultMutableTreeNode child = new DefaultMutableTreeNode(newAction);
		if(parent == null){
			Logger.debug("Workflow.addUserAction - parent is null");
		}else{
			insertNodeInto(child, parent, parent.getChildCount());
		}
		return child;
	}
	
	// TODO: clearWorkflow
	public void clearWorkflow(){
		
	}
	
	// TODO: cast problem - string to useraction
	public String toString(){
		String result = "";
		LinkedList<DefaultMutableTreeNode> queue = new LinkedList<DefaultMutableTreeNode>();
		queue.addLast(getRoot());
		while(queue.size() > 0){
			DefaultMutableTreeNode current = queue.pop();
			Object currentObject = current.getUserObject();
			if(currentObject != null)
				result = result + currentObject.toString() + "\n";
			for (Enumeration children=current.children(); children.hasMoreElements(); ) {
				DefaultMutableTreeNode child = (DefaultMutableTreeNode) children.nextElement();
				queue.addLast(child);
			}
		}
		return result;
	}
	
	public UserActionIO getCurrentUserActionIO(){
		return getSelectedUserAction().getIoDetails();
	}
	

}
