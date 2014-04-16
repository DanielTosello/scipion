 /*****************************************************************************
 *
 * Authors:    Jose Gutierrez (jose.gutierrez@cnb.csic.es)
 * 			   Adrian Quintana (aquintana@cnb.csic.es)
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
 *  e-mail address 'jmdelarosa@cnb.csic.es'
 *
 ******************************************************************************/
/******************************************************************************
 * DESCRIPTION:
 * 
 * Methods used in the project_content template. 
 * Toolbar functions + Manage tabs
 * 
 * ATTRIBUTES LIST:
 * 
 * METHODS LIST:
 * 
 * function launchToolbarList(id, elm)
 * 	->	Toolbar used in the project content template for the run list view.
 * 
 * function launchToolbarTree(id, elm)
 * 	->	Toolbar used in the project content template for the runs tree view.
 * 
 * function checkRunStatus(id)
 * 	->	Function to check a protocol run, depend on the status two button will be
 * 		switching in the toolbar (Stop / Analyze Results).
 * 
 * function fillTabs(id)
 * 	->	Fill the content of the tabs for a protocol run selected 
 * 		(Data / Summary / Methods / Status)
 * 
 * function fillUL(list, ul_id, icon)
 * 	->	Fill an UL element with items from a list items, should contains id and 
 * 		name properties.
 * 
 * function launchViewer(id)
 * 	->	Launch the viewers to analyze the results for a protocol run.
 * 
 * function updateButtons(id, elm)
 * 	->	Function to update the buttons in the toolbar and the tabs, after choose
 *  	a new protocol run.
 * 
 * function updateRow(id, elm, row)
 * 	->	Function to update the row in the protocol run list when an element is
 *		selected.
 * 
 * function updateTree(id, elm, row)
 * 	->	Function to update the node in the protocol run tree when an element is
 * 		selected.
 * 
 * function graphON(graph, icon_graph, list, icon_list)
 * 	->	Function to disable the list view and enable the tree graph view.
 * 
 * function graphOFF(graph, icon_graph, list, icon_list)
 * 	->	Function to disable the tree graph view and enable the list view.
 * 
 * function changeStatusGraph(status, graph, icon_graph, list, icon_list)
 * 	->	Function to switch between the graph/list view depending on the status.
 * 
 * function markElmGraph(node_id, graph)
 * 	->	Function used to mark the same protocol run when one is selected in the
 * 		protocol run list and the view is changed to protocol run graph.
 * 
 * function markElmList(row_id, graph)
 * 	->	Function used to mark the same protocol run when one is selected in the
 * 		protocol run graph and the view is changed to protocol run list.
 * 
 * function switchGraph() 
 * 	->	Main function called to change between graph tree/list views.
 * 
 * function updateGraphView(status)
 * 	->	Method to update the graph view in the internal settings of the project
 * 		passing like argument a boolean.
 * 		If the graph view was active when you closed the project last time will
 * 		be available directly, in the another case, will be inactive.
 * 
 * function editObject(objId)
 * 	->	Method to edit an object given his objId, calling an ajax to get the 
 * 		attributes from the object.
 * 
 * function deleteProtocolForm(protocolId)
 * 	->	Dialog web form based in messi.js to verify the option to delete a protocol. 
 * 
 * function deleteProtocol(elm)
 * 	->	Method to execute a delete for a protocol.
 *
 * function stopProtocolForm(protocolId)
 * 	->	Dialog web form based in messi.js to verify the option to stop a protocol.
 * 
 * function stopProtocol(elm)
 * 	->	Method to stop the run for a protocol
 * 
 * function changeTreeView()
 * 	->	Method to update the protocol tree to run in the web left side.
 * 
 * function refreshRuns(mode)
 * 	->	Method to refresh the run list/graph checking changes. 
 * 
 ******************************************************************************/

 /** METHODS ******************************************************************/

function launchToolbarList(id, elm) {
	/*
	 * Toolbar used in the project content template for list view
	 */
	var row = $("div#toolbar");
	updateRow(id, elm, row);
	updateButtons(id, elm);
	row.show(); // Show toolbar
}

var event = jQuery("div#graphActiv").trigger(jQuery.Event("click"));

function enableMultipleMarkGraph(elm){
	if (elm.attr("selected") == "selected"){
		elm.css("border", "");
		elm.removeAttr("selected")
		
		$("div#graphActiv").removeAttr("data-option");
	} else {
		elm.css("border", "2.5px solid Firebrick");
		elm.attr("selected", "selected")
		
		$("div#graphActiv").attr("data-option", "graph_"+elm.attr("id"));
	}
}

function disableMultipleMarkGraph(id){
	
	$.each($(".window"), function(){
		elm = $(this)
		
		if (elm.attr("id") != "graph_"+id && elm.attr("selected") != undefined){
			elm.removeAttr("selected")
			elm.css("border", "");
		}
	}) 
}

function launchToolbarTree(id, elm) {
	/*
	 * Toolbar used in the project content template for list view
	 */
	
	if (event.ctrlKey){
		enableMultipleMarkGraph(elm)
	} else {
		disableMultipleMarkGraph(id)
		
		var row = $("div#toolbar");
		updateTree(id, elm, row);
		updateButtons(id, elm);
		row.show(); // Show toolbar
	}
}


function fillTabs(id) {
	/*
	 * Fill the content of the summary tab for a protocol run selected 
	 * (Data / Summary / Methods / Status)
	 */
	$.ajax({
		type : "GET",
		url : '/protocol_info/?protocolId=' + id,
		dataType : "json",
		success : function(json) {
			
			// DATA SUMMARY
			fillUL("input", json.inputs, "protocol_input", "fa-sign-in");
			fillUL("output", json.outputs, "protocol_output", "fa-sign-out");
			
			var summary = $("#protocol_summary");
			
			summary.empty();
			summary.append(json.summary);

			// METHODS
			$("#tab-methods").empty();
			$("#tab-methods").append(json.methods);
			
			// STATUS
			if(json.status=="running"){
				// Action Stop Button
//				$("span#analyzeTool").hide();
				$("span#buttonAnalyzeResult").hide();
				$("span#stopTool").show();
				$("a#stopTool").attr('href',
				'javascript:stopProtocolForm("' + id + '")');
			} else {
				// Action Analyze Result Button
				$("span#stopTool").hide();
//				$("span#analyzeTool").show();
				$("span#buttonAnalyzeResult").show();
				$("a#analyzeTool").attr('href', 'javascript:launchViewer("'+id +'")');
			}
			
			//LOGS
			$("#tab-logs-output").empty();
			$("#tab-logs-output").append(json.logs_out);
			
			$("#tab-logs-error").empty();
			$("#tab-logs-error").append(json.logs_error);
			
			$("#tab-logs-scipion").empty();
			$("#tab-logs-scipion").append(json.logs_scipion);
		}
	});
}

function showLog(log_type){
	
	if(log_type=="output_log"){
		$("#tab-logs-output").css("display","")
		$("#output_log").attr("class", "elm-header-log_selected")
		html = $("#tab-logs-output").html()
		
		$("#tab-logs-error").css("display","none")
		$("#error_log").attr("class", "elm-header-log")
		
		$("#tab-logs-scipion").css("display","none")
		$("#scipion_log").attr("class", "elm-header-log")
		
	}else if(log_type=="error_log"){
		$("#tab-logs-output").css("display","none")
		$("#output_log").attr("class", "elm-header-log")
		
		$("#tab-logs-error").css("display","")
		$("#error_log").attr("class", "elm-header-log_selected")
		html = $("#tab-logs-error").html()
		
		$("#tab-logs-scipion").css("display","none")
		$("#scipion_log").attr("class", "elm-header-log")
		
	}else if(log_type=="scipion_log"){
		$("#tab-logs-output").css("display","none")
		$("#output_log").attr("class", "elm-header-log")
		
		$("#tab-logs-error").css("display","none")
		$("#error_log").attr("class", "elm-header-log")
		
		$("#tab-logs-scipion").css("display","")
		$("#scipion_log").attr("class", "elm-header-log_selected")
		html = $("#tab-logs-scipion").html()
	}
	
	$("#externalTool").attr("href","javascript:customPopupFileHTML('" + html +"','"+ log_type + "',1024,768)")
}


function fillUL(type, list, ulId, icon) {
	/*
	 * Fill an UL element with items from a list items should contains id and name
	 * properties
	 */
	var ul = $("#" + ulId);
	ul.empty();
	for ( var i = 0; i < list.length; i++) {
		
//		inihtml = "<table><tr><td><strong>Attribute</strong></td><td><strong>&nbsp;&nbsp;&nbsp;Info</strong></td></tr>"
		
		// Visualize Object
		var visualize_html = '<a href="javascript:launchViewer(' + list[i].id
		+ ');"><i class="fa ' + icon + '" style="margin-right:10px;"></i>'+ list[i].name

		if(type=="input"){
			visualize_html += ' (from ' + list[i].nameId +')</a>'
		}
		else if(type=="output"){
			visualize_html += '</a>'
		}
		
		// Edit Object
		var edit_html = '<a href="javascript:editObject('+ list[i].id + ');"> '+
		'<i class="fa fa-pencil" style="margin-left:0px;"></i></a>'
		
//		endhtml = "</table>"
		
		ul.append('<li>' + visualize_html + edit_html +"&nbsp;&nbsp;&nbsp;" +list[i].info+ '</li>');
//		ul.append(inihtml+'<tr><td>' + visualize_html +"</td><td>&nbsp;&nbsp;&nbsp;" + list[i].info + edit_html + '</td></tr>' + endhtml);
		
	}
}

function launchViewer(id){
	/*
	 * Launch the viewers to analyze the results of the protocol run
	 */
	$.ajax({
		type : "GET",
		// Execute the viewer 
		url : "/launch_viewer/?protocolId=" + id,
		dataType : "json",
		success : function(json) {
			popUpJSON(json);
		}
	});	
}

function updateButtons(id, elm){
	/*
	 * Function to update the buttons in the toolbar and the tabs, after choose a new protocol run.
	 */
	// Action Edit Button
	$("a#editTool").attr('href',
	'javascript:popup("/form/?protocolId=' + id + '")');
	
	// Action Copy Button
	$("a#copyTool").attr('href',
	'javascript:popup("/form/?protocolId=' + id + '&action=copy' + '")');

	// Action Delete Button
	$("a#deleteTool").attr('href',
			'javascript:deleteProtocolForm("' + id + '")');

	// Action Browse Button
	var aux = "javascript:alert('Not implemented yet')";
	$("a#browseTool").attr('href', aux);
	
	fillTabs(id);
}

function updateRow(id, elm, row){	
	/*
	 * Function to update the row in the protocol run list when an element is
	 * selected.
	 */
	if (row.attr('value') != undefined && row.attr('value') != id) {
		var rowOld = $("tr#" + row.attr('value'));
		rowOld.removeClass('selected')
	}
	elm.addClass('selected')

	// add id value into the toolbar
	row.attr('value', id);
}

function updateTree(id, elm, row){
	/*
	 * Function to update the node in the protocol run tree when an element is
	 * selected.
	 */
	var oldSelect = $("div#graphActiv").attr("data-option");
	var selected = "graph_" + id;

	if (oldSelect != selected) {
		if (oldSelect != "") {
			var aux = "div#" + oldSelect + ".window";
			$(aux).css("border", "");
			elm.removeAttr("selected")
		}
		row.attr('value', id);
		$("div#graphActiv").attr("data-option", selected);
		elm.css("border", "2.5px solid Firebrick");
		elm.attr("selected", "selected")
	}
}

function selectElmGraph(elm){
	if(elm.css("border") == ""){
		elm.css("border", "2.5px solid Firebrick");
	} else {
		elm.css("border", "");
	}
}


function graphON(graph, icon_graph, list, icon_list){
	/*
	 * Function to disable the list view and enable the tree graph view.
	 */
	
	// Graph ON
	graph.attr("data-mode", "active");
//	graph.removeAttr("style");
	graph.show();
	graph.css("margin-top","-50em")
	icon_graph.hide();
	
	// Table OFF
	list.attr("data-mode", "inactive");
//	list.attr("style", "display:none;");
	list.hide();
	icon_list.show();

	// Update Graph View
	updateGraphView("True");
	
}

function graphOFF(graph, icon_graph, list, icon_list){
	/*
	 * Function to disable the tree graph view and enable the list view.
	 */
	
	// Table ON	
	list.attr("data-mode", "active");
//	list.removeAttr("style");
	list.show();
	icon_list.hide();
	
	// Graph OFF
	graph.attr("data-mode", "inactive");
//	graph.attr("style", "display:none;");
	graph.hide();
	$("img#loading").hide();
	icon_graph.show();
	
	// Update Graph View
	updateGraphView("False")
}

function changeStatusGraph(status, graph, graphTool, list, listTool){
	/*
	 * Function to switch between the graph/list view depending on the status.
	 */
	if (status == 'inactive') {
		// Graph ON & Table OFF
		graphON(graph, graphTool, list, listTool);
	} else if (status == 'active') {
		// Table ON	& Graph OFF
		graphOFF(graph, graphTool, list, listTool);
	}
}

function markElmGraph(node_id, graph){
	/*
	 * Function used to mark the same protocol run when one is selected in the
	 * protocol run list and the view is changed to protocol run tree.
	 */
	var s = "graph_" + node_id;

	if (s != "" || s != undefined) {
		var nodeClear = graph.attr("data-option");
		
		if (nodeClear.length>0 && nodeClear != undefined) {
			// Clear the node
			var elmClear = $("div#" + nodeClear);
			elmClear.css("border", "");
		} 
		// setElement in graph
		graph.attr("data-option", s);
	
		// Highlight the node
		var elm = $("div#" + s);
		elm.css("border", "2.5px solid Firebrick");
	}
}
	
function markElmList(row_id, graph){
	/*
	 * Function used to mark the same protocol run when one is selected in the
	 * protocol run tree and the view is changed to protocol run list.
	 */
	var rowClear = $("tr.selected").attr("id");
	if (rowClear != "") {
		if (rowClear != row_id) {
			// Clear the row selected
			var elmClear = $("tr.selected");
			elmClear.attr("style", "");
			elmClear.attr("class", "runtr");

			// setElement in table
			var elm = $("tr#" + row_id + ".runtr");
			var projName = graph.attr("data-project");
			launchToolbarList(row_id, elm);
		}
	}
}

function switchGraph() {
	/*
	 * Main function called to change between graph tree/list views.
	 */
	
	// graph status (active or inactive) 
	var status = $("div#graphActiv").attr("data-mode");

	// element marked obtained from value in the toolbar
	var id = $("div#toolbar").attr("value");
	
	//get row elements 
	var graph = $("div#graphActiv");
	var graphTool = $("span#treeTool");
	var list = $("div#runTable");
	var listTool = $("span#listTool");
	
	changeStatusGraph(status, graph, graphTool, list, listTool)
		
	// Graph will be painted once
	if (graph.attr("data-time") == 'first') {
		callPaintGraph();
		graph.attr("data-time", "not");
	} 
	markElmGraph(id, graph);
	markElmList(id, graph);
}

function updateGraphView(status) {
	/*
	 * Method to update the graph view in the internal settings of the project
	 * passing like argument a boolean.
	 * If the graph view was active when you closed the project last time will
	 * be available directly, in the another case, will be inactive.
	 */
	$.ajax({
		type : "GET", 
		url : "/update_graph_view/?status=" + status
	});
}

function editObject(objId){
	$.ajax({
		type : "GET",
		url : '/get_attributes/?objId='+ objId,
		dataType: "text",
		success : function(text) {
			var res = text.split("_-_")
			label = res[0]
			comment = res[1]
			editObjParam(objId, "Label", label, "Comment", comment, "Describe your run here...","object")
		}
	});
}

function deleteProtocolForm(protocolId) {
	/*
	 * Dialog web form based in messi.js to verify the option to delete.
	 */
	var msg = "</td><td class='content' value='"
			+ protocolId
			+ "'><strong>ALL DATA</strong> related to this <strong>protocol run</strong>"
			+ " will be <strong>DELETED</strong>. Do you really want to continue?</td></tr></table>";
	
	warningPopup('Confirm DELETE',msg, 'deleteProtocol')
	
}

function deleteProtocol(elm) {
	/*
	 * Method to execute a delete for a protocol
	 */
	var protId = elm.attr('value');
	
	$.ajax({
		type : "GET",
		url : "/delete_protocol/?protocolId=" + protId,
		dataType : "json",
		success : function(json) {
			if(json.errors != undefined){
				// Show errors in the validation
				errorPopup('Errors found',json.errors);
			} else if(json.success!= undefined){
//				launchMessiSimple("Successful", messiInfo(json.success));
//				window.location.reload()
			}
		},
		error: function(){
			alert("error")
		}
	});
}

function stopProtocolForm(protocolId) {
	/*
	 * Dialog web form based in messi.js to verify the option to stop a protocol.
	 */
		
	var msg = "<td class='content' value='"
			+ protocolId
			+ "'>This <strong>protocol run</strong>"
			+ " will be <strong>STOPPED</strong>. Do you really want to continue?</td>";
			
	warningPopup('Confirm STOP',msg, 'stopProtocol');
}

function stopProtocol(elm) {
/*
 * Method to stop the run for a protocol
 */
	var protId = elm.attr('value');
	
	$.ajax({
		type : "GET",
		url : "/stop_protocol/?protocolId=" + protId
	});
}

function changeTreeView(){
	/*
	 * Method to update the protocol tree to run in the web left side.
	 */
	protIndex = $('#viewsTree').val();
	
	$.ajax({
		type : "GET",
		url : '/update_prot_tree/?index='+ protIndex,
		dataType:"text",
		success : function() {
			$.ajax({
				url: '/tree_prot_view/',
				success: function(data) {
					$('div.protFieldsetTree').html(data);
				}
			});
		}
	});
}

function refreshRuns(mode){
	/*
	 * Method to update the run list/graph
	 */
	
	$(function() {
		$.ajax({
			url : '/run_table_graph/',
			success : function(data) {
			
				if (typeof data == 'string' || data instanceof String){
					
					if (data=='stop'){
						window.clearTimeout(updatetimer);
						// stop the script
					}
					else if(data == 'ok'){
						// no changes
					}
					else {
						$('div#runsInfo').html(data);
						// refresh the data keeping the element marked
					}
				}
				else {
					for (var x=0;x< data.length;x++){
						var id = data[x][0];
						var status = data[x][2];
						var time = data[x][3];

						checkStatusNode(id, status)
						checkStatusRow(id, status, time)

					}
				}
			}
		});
  	});
	
	if(mode){
		var updatetimer = setTimeout(function(){ 
			refreshRuns(1);
	  	}, 3000);
	}
}

function checkStatusNode(id, status){
	node = $("#graph_"+ id);
	
	if(status.indexOf('running')==0){
		node.css("background-color", "#FCCE62");
	}
	if(status.indexOf('failed')==0){
		node.css("background-color", "#F5CCCB");
	}
	if(status.indexOf('finished')==0){
		node.css("background-color", "#D2F5CB");
	}
	node.find("#nodeStatus").html(status);
		
}

function checkStatusRow(id, status, time){
	row = $("tr#"+ id);
	
	row.find(".status").html(status);
	row.find(".time").html(time);
		
}





