 /**************************************************************************
 *
 * Authors:    Jose Gutierrez (jose.gutierrez@cnb.csic.es)
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
 **************************************************************************/



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

function showErrorValidation(json) {
	var msg = JSON.stringify(json);
	msg = msg.replace("<", "");
	msg = msg.replace(">", "");
	msg = msg.replace("[", "");
	msg = msg.replace("]", "");

	var msg = "<table><tr><td><img src='/resources/error.gif' width='45' height='45' />"
			+ "</td><td class='content'>" + msg + "</td></tr></table>";

	new Messi(msg, {
		title : 'Errors found',
		modal : true,
		buttons : [ {
			id : 0,
			label : 'close',
			val : 'Y',
			btnClass : 'btn-close'
		} ]
	});
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
