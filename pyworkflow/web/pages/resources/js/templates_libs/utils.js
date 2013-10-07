/**
 * Generic lib with commons methods
 * 
 * popup(URL); 
 * customPopup(URL, widthValue, heightValue);
 * customPopupHTML(html);  
 * closePopup();
 * launchMessiSimple(title, msg);
 * messiError(msg);
 * messiWarning(msg);
 * messiInfo(msg);
 * 
 */

function popup(URL) {
	var popup_width = 500;
	var popup_height = 490;
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

function customPopupHTML(html, widthValue, heightValue) {
	day = new Date();
	id = day.getTime();
	var popup = window.open('', id, 'height='+heightValue+',width='+widthValue);
	popup.document.write(html);
	popup.document.close();
	
}

function closePopup() {
	// opener.location.reload(true);
	// self.close();
	window.opener.location.reload(true);
	window.close();
}

function launchMessiSimple(title, msg){
	new Messi(msg, {
		title : title,
		modal : true,
		buttons : [ {
			id : 0,
			label : 'Ok',
			val : 'Y',
			btnClass : 'btn-select'
		}]
	});
}

function messiError(msg){
	var res = "<table><tr><td><img src='/resources/error.gif' width='45' height='45' />"
	+ "</td><td>"+ msg +"</td></tr></table>";

	return res;
}

function messiWarning(msg){
	var res = "<table><tr><td><img src='/resources/warning.gif' width='45' height='45' />"
	+ "</td><td>"+ msg +"</td></tr></table>";

	return res;
}

function messiInfo(msg){
	var res = "<table><tr><td><img src='/resources/info.gif' width='45' height='45' />"
	+ "</td><td>"+ msg +"</td></tr></table>";

	return res;
}
