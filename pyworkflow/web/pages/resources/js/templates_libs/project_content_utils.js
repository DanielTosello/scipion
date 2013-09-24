/**
 * Methods used in the project_content template. 
 * Toolbar functions + Manage tabs
 * 
 * launchToolbar(projName, id, elm);
 * fillTabsSummary(projName, id);
 * fillUL(list, ulId, icon, projName);
 * updateTabs(projName, id, elm);
 * switchGraph();
 * deleteProtocolForm(projName, protocolId);
 * deleteProtocol(elm);
 *  
 * 
 **/

/*
 * Toolbar used in the project content template
 */
function launchToolbar(projName, id, elm) {
	var row = $("div#toolbar");

	if (row.attr('value') != undefined && row.attr('value') != id) {
		var rowOld = $("tr#" + row.attr('value'));
		rowOld.attr('style', 'background-color: #fafafa;');
		rowOld.attr('class', 'runtr');
	}
	row.attr('value', id);
	elm.attr('style', 'background-color: LightSteelBlue;');
	elm.attr('class', 'selected');

	// Action Edit Button
	$("a#editTool").attr('href',
	// 'javascript:popup("/form/?projectName=' + projName + '&protocolId='
	'javascript:popup("/form/?=&protocolId=' + id + '")');
	// Action Copy Button
	$("a#copyTool").attr('href',
	// 'javascript:popup("/form/?projectName=' + projName + '&protocolId='
	// + id + '&action=copy' + '")');
	'javascript:popup("/form/?&protocolId=' + id + '&action=copy' + '")');

	// Action Delete Button
	$("a#deleteTool").attr('href',
			'javascript:deleteProtocolForm("' + projName + '","' + id + '")');

	// Action Browse Button
	var aux = "javascript:alert('Not implemented yet')";
	$("a#browseTool").attr('href', aux);

	fillTabsSummary(projName, id);

	row.show(); // Show toolbar
}

/*
 * Fill the content of the summary tab
 */
function fillTabsSummary(projName, id) {
	$.ajax({
		type : "GET",
		url : '/protocol_io/?protocolId=' + id,
		dataType : "json",
		success : function(json) {
			fillUL(json.inputs, "protocol_input", "db_input.gif", projName);
			fillUL(json.outputs, "protocol_output", "db_output.gif", projName);
			fillAnalyze(json.outputs, projName);
		}
	});

	$.ajax({
		type : "GET",
		url : '/protocol_summary/?protocolId='+ id,
		dataType : "json",
		success : function(json) {
			$("#tab-summary").empty();
			for ( var i = 0; i < json.length; i++) {
				$("#tab-summary").append('<p>' + json[i] + '</p>');
			}
		}
	});
}

/*
 * Fill an UL element with items from a list items should contains id and name
 * properties
 */
function fillUL(list, ulId, icon, projName) {
	ul = $("#" + ulId)
	ul.empty()
	for ( var i = 0; i < list.length; i++) {
		ul.append('<li><a href="/visualize_object/?projectName=' + projName
				+ '&objectId=' + list[i].id
				+ '"target="_blank"><img src="../../../../resources/' + icon + '" /> '
				+ list[i].name + '</a></li>');
	}
}

function fillAnalyze(list, projName) {
	$("a#analyzeTool").attr('href', 
			'javascript:launchViewer("' + projName + '","' + list[0].id + '")');
}

function launchViewer(projName, id){
	
//	var uri = '/visualize_object/?projectName=' + projName+ '&objectId=' + id;
	
	/* Execute the viewer */
	$.ajax({
		type : "GET",
		url : "/viewer/?projectName=" + projName+ '&objectId=' + id ,
		success : function(html) {
			
			if(html=="errorInput"){
			
			}
			else {
				customPopupHTML(html,520,460);
			}
		}
	});	
}


function updateTabs(projName, id, elm) {
	var oldSelect = $("div#graphActiv").attr("data-option");
	var selected = "graph_" + id;

	if (oldSelect != selected) {
		if (oldSelect != "") {
			var aux = "div#" + oldSelect + ".window";
			$(aux).css("border", "");
		}
		$("div#graphActiv").attr("data-option", selected);
		elm.css("border", "2.5px solid Firebrick");

		fillTabsSummary(projName, id);
	}
}

function switchGraph() {
	var status = $("div#graphActiv").attr("data-mode");
	// Graph will be painted once
	if ($("div#graphActiv").attr("data-time") == 'first') {
		if (status == 'inactive') {
			// Graph ON
			$("div#graphActiv").attr("data-mode", "active");
			$("div#graphActiv").attr("style", "");
			// Table OFF
			$("div#runTable").attr("data-mode", "inactive");
			$("div#runTable").attr("style", "display:none;");
		} else if (status == 'active') {
			// Table ON
			$("div#runTable").attr("data-mode", "active");
			$("div#runTable").attr("style", "");
			// Graph OFF
			$("div#graphActiv").attr("data-mode", "inactive");
			$("div#graphActiv").attr("style", "display:none;");
		}
		callPaintGraph();
		$("div#graphActiv").attr("data-time", "not");
	} else {
		if (status == 'inactive') {
			// Graph ON
			$("div#graphActiv").attr("data-mode", "active");
			$("div#graphActiv").attr("style", "");
			// Table OFF
			$("div#runTable").attr("data-mode", "inactive");
			$("div#runTable").attr("style", "display:none;");

			// getElement in table
			var s = $("tr.selected").attr("id");
			s = "graph_" + s;

			if (s != "") {
				var nodeClear = $("div#graphActiv").attr("data-option");
				if (nodeClear != "") {
					if (nodeClear != s) {
						// Clear the node selected
						var elmClear = $("div#" + nodeClear + ".window");
						elmClear.css("border", "");

						// setElement in graph
						$("div#graphActiv").attr("data-option", s);

						// Highlight the node
						var elm = $("div#" + s + ".window");
						elm.css("border", "2.5px solid Firebrick");

					}
				}
			}

		} else if (status == 'active') {
			// Table ON
			$("div#runTable").attr("data-mode", "active");
			$("div#runTable").attr("style", "");
			// Graph OFF
			$("div#graphActiv").attr("data-mode", "inactive");
			$("div#graphActiv").attr("style", "display:none;");

			// getElement in graph
			var s = $("div#graphActiv").attr("data-option");
			var s = s.replace("graph_", "");

			if (s != "") {
				var rowClear = $("tr.selected").attr("id");
				if (rowClear != "") {
					if (rowClear != s) {
						// Clear the row selected
						var elmClear = $("tr.selected");
						elmClear.attr("style", "background-color: #fafafa;");
						elmClear.attr("class", "runtr");

						// setElement in table
						var elm = $("tr#" + s + ".runtr");
						var projName = $("div#graphActiv").attr("data-project");
						// elm.attr("style", "background-color:
						// LightSteelBlue;");
						// elm.attr("class","selected");
						launchToolbar(projName, s, elm);
					}
				}
			}
		}
	}
}

/*
 * Dialog form to verify the right option to delete
 */
function deleteProtocolForm(projName, protocolId) {

	var msg = "<table><tr><td><img src='/resources/warning.gif' width='45' height='45' />"
			+ "</td><td class='content' value='"
			+ projName
			+ "-"
			+ protocolId
			+ "'><strong>ALL DATA</strong> related to this <strong>protocol run</strong>"
			+ " will be <strong>DELETED</strong>. Do you really want to continue?</td></tr></table>";

	new Messi(msg, {
		title : 'Confirm DELETE',
		// modal : true,
		buttons : [ {
			id : 0,
			label : 'Yes',
			val : 'Y',
			btnClass : 'btn-select',
			btnFunc : 'deleteProtocol'
		}, {
			id : 1,
			label : 'No',
			val : 'C',
			btnClass : 'btn-cancel'
		} ],
		callback : function(val) {
			if (val == 'Y') {
				window.location.href = "/project_content/?projectName="
						+ projName;
			}
		}
	});
}

/*
 * Method to execute a delete by a protocol
 */
function deleteProtocol(elm) {
	var value = elm.attr('value').split("-");
	var projName = value[0];
	var protId = value[1];
	$.ajax({
		type : "GET",
		url : "/delete_protocol/?projectName=" + projName + "&protocolId="
				+ protId
	});
}

