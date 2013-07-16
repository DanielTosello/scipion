function popup(URL) {
	var popup_width = 470
	var popup_height = 615
	day = new Date();
	id = day.getTime();
	eval("page"
			+ id
			+ " = window.open(URL, '"
			+ id
			+ "', 'toolbar=0,scrollbars=1,location=0,statusbar=0,menubar=0,resizable=0,width='+popup_width+',height='+popup_height+'');");
}

function customPopup(URL, widthValue, heightValue) {
	day = new Date();
	id = day.getTime();
	eval("page"
			+ id
			+ " = window.open(URL, '"
			+ id
			+ "', 'toolbar=0,scrollbars=1,location=0,statusbar=0,menubar=0,resizable=0,width='+widthValue+',height='+heightValue+'');");
}

function closePopup() {
	// opener.location.reload(true);
	// self.close();
	window.opener.location.reload(true);
	window.close();
}

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
	$("a#editTool").attr(
			'href',
			'javascript:popup("/form/?projectName=' + projName + '&protocolId='
					+ id + '")');
	// Action Copy Button
	$("a#copyTool").attr(
			'href',
			'javascript:popup("/form/?projectName=' + projName + '&protocolId='
					+ id + '&action=copy' + '")');

	// Action Delete Button
	$("a#deleteTool").attr('href',
			'javascript:deleteProtocolForm("' + projName + '","' + id + '")');

	// Action Browse Button
	var aux = "javascript:alert('Not implemented yet')";
	$("a#browseTool").attr('href', aux);

	row.show(); // Show toolbar

	fillTabsSummary(projName, id);
}

/*
 * 
 */
function fillTabsSummary(projName, id) {
	$.ajax({
		type : "GET",
		url : '/protocol_io/?projectName=' + projName + '&protocolId=' + id,
		dataType : "json",
		success : function(json) {
			fillUL(json.inputs, "protocol_input", "db_input.gif", projName);
			fillUL(json.outputs, "protocol_output", "db_output.gif", projName);
		}
	});

	$.ajax({
		type : "GET",
		url : '/protocol_summary/?projectName=' + projName + '&protocolId='
				+ id,
		dataType : "json",
		success : function(json) {
			$("#tab2").empty();
			for ( var i = 0; i < json.length; i++) {
				$("#tab2").append('<p>' + json[i] + '</p>');
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
				+ '"><img src="../../../../resources/' + icon + '" /> '
				+ list[i].name + '</a></li>');
	}
}

/*
 * Toolbar used in the view host template
 */
function launchHostsToolbar(projName, hostId, elm) {
	var row = $("div#toolbarHost");

	if (row.attr('value') != undefined && row.attr('value') != hostId) {
		var rowOld = $("tr#" + row.attr('value'));
		rowOld.attr('style', 'background-color: #fafafa;');
		rowOld.attr('class', 'runtr');
	}
	row.attr('value', hostId);
	elm.attr('style', 'background-color: LightSteelBlue;');
	elm.attr('class', 'selected');

	// Action Edit Button
	$("a#editTool").attr('href', 'javascript:editHost()');
	// Action Copy Button
	$("a#newTool").attr('href', 'javascript:newHost()');
	// Action Delete Button
	$("a#deleteTool").attr('href', 'javascript:deleteHost()');
	// Action Browse Button
	// $("a#browseTool").attr(
	// 'href',
	// 'javascript:popup("/form/?projectName=' + projName + '&protocolId='
	// + id + '")');

	row.show(); // Show toolbar
}

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

function selTableMessi(elm) {

	var row = $("table.content");
	var id = elm.attr('id');

	if (row.attr('value') != undefined && row.attr('value') != id) {
		var rowOld = $("td#" + row.attr('value'));
		rowOld.attr('style', '');
	}
	row.attr('value', id);
	elm.attr('style', 'background-color: LightSteelBlue;');
}

function switchGraph() {
	var status = $("div#graphActiv").attr("data-mode");
	if (status == 'inactive') {
		$("div#graphActiv").attr("data-mode", "active");
		$("div#graphActiv").attr("style", "");
		$("div#runTable").attr("data-mode", "inactive");
		$("div#runTable").attr("style", "display:none;");
	} else if (status == 'active') {
		$("div#runTable").attr("data-mode", "active");
		$("div#runTable").attr("style", "");
		$("div#graphActiv").attr("data-mode", "inactive");
		$("div#graphActiv").attr("style", "display:none;");
	}
	if ($("div#graphActiv").attr("data-time") == 'first') {
		callPaintGraph();
		$("div#graphActiv").attr("data-time", "not");
	}
}

/*
 * Graph methods Functions to use the jsPlumb plugin
 */

/** **** Settings to connect nodes *********** */
// Setting up drop options
var targetDropOptions = {
	tolerance : 'touch',
	hoverClass : 'dropHover',
	activeClass : 'dragActive'
};

// Setting up a Target endPoint
var targetColor = "black";
// var targetColor = "black";
var targetEndpoint = {
	endpoint : [ "Dot", {
		radius : 5
	} ],
	paintStyle : {
		fillStyle : targetColor
	},
	// isSource:true,
	scope : "green dot",
	connectorStyle : {
		strokeStyle : targetColor,
		lineWidth : 2
	},
	connector : [ "Bezier", {
		curviness : 5
	} ],
	maxConnections : 10,
	isTarget : true,
	dropOptions : targetDropOptions
};

// Setting up a Source endPoint
var sourceColor = "black";
// var sourceColor = "black";
var sourceEndpoint = {
	endpoint : [ "Dot", {
		radius : 5
	} ],
	paintStyle : {
		fillStyle : sourceColor
	},
	isSource : true,
	scope : "green dot",
	connectorStyle : {
		strokeStyle : sourceColor,
		lineWidth : 2
	},
	connector : [ "Bezier", {
		curviness : 5
	} ],
	maxConnections : 10
// isTarget:true,
// dropOptions : targetDropOptions
};

function callPaintGraph() {
	// Draw the boxes
	var nodeSource = $("div#graphActiv");
	var status = "finished";
	var aux = [];

	// Paint the first node
	paintBox(nodeSource, "graph_PROJECT", "PROJECT", "");
	var width = $("div#" + "graph_PROJECT" + ".window").width();
	var height = $("div#" + "graph_PROJECT" + ".window").height();
	aux.push("PROJECT" + "-" + width + "-" + height);

	// Paint the other nodes (selected include)
	$("tr.runtr,tr.selected").each(function() {
		var id = jQuery(this).attr('id');
		var idNew = "graph_" + id;

		var name = jQuery(this).attr('data-name');

		paintBox(nodeSource, idNew, name, status);
		var width = $("div#" + idNew + ".window").width();
		var height = $("div#" + idNew + ".window").height();

		aux.push(id + "-" + width + "-" + height);
	});

	$.ajax({
		type : "GET",
		url : '/project_graph/?list=' + aux,
		dataType : "json",
		success : function(json) {
			// Iterate over the nodes and position in the screen
			// coordinates should come in the json response
			for ( var i = 0; i < json.length; i++) {
				var top = json[i].y * 1.2;
				var left = json[i].x;
				addStatusBox(nodeSource, "graph_" + json[i].id,
						json[i].status);
				$("div#graph_" + json[i].id + ".window").attr(
						"style",
						"top:" + top + "px;left:" + left
								+ "px;background-color:"
								+ json[i].color + ";");
			}
			// After all nodes are positioned, then create the edges
			// between
			// them

			for ( var i = 0; i < json.length; i++) {
				for ( var j = 0; j < json[i].childs.length; j++) {
					var source = $("div#graph_" + json[i].id
							+ ".window");
					var target = $("div#graph_" + json[i].childs[j]
							+ ".window");
					connectNodes(source, target);
				}
			}
		}
	});

	jsPlumb.draggable($(".window"));
}

function paintBox(nodeSource, id, msg) {

	if (id != "graph_PROJECT") {
		var objId = id.replace("graph_", "");
		var href = "javascript:popup('/form/?protocolId="+objId+"')";
		var projName = $("div#graphActiv").attr("data-project");
		var onclick = "fillTabsSummary('"+ projName +"', '" + objId +"')";
		var aux = '<div class="window" style="" onclick="'+ onclick +'" id="' + id
				+ '"><a href="' + href
				+ '"><strong>' + msg + '</strong></a><br /></div>';
	} else {
		var aux = '<div class="window" style="" id="' + id + '"><strong>' + msg
				+ '</strong><br />' + "" + '</div>';
	}

	nodeSource.append(aux);
}

function addStatusBox(nodeSource, id, status) {
	$("div#" + id + ".window").append(status);
}

function connectNodes(elm1, elm2) {
	// alert($(elm1).attr('id') + " - " + $(elm2).attr('id'));
	var a = jsPlumb.addEndpoint($(elm1), {
		anchor : "Center"
	}, sourceEndpoint);
	var b = jsPlumb.addEndpoint($(elm2), {
		anchor : "Center"
	}, targetEndpoint);

	jsPlumb.connect({
		source : a,
		target : b
	});
}

// function putEndPoints(elm) {
// jsPlumb.addEndpoint($(elm + ".window"), {
// anchor : "TopCenter"
// }, targetEndpoint);
// jsPlumb.addEndpoint($(elm + ".window"), {
// anchor : "BottomCenter"
// }, sourceEndpoint);
// }
