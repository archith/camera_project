// LCS 3021  May 6, 2003

var PAN_TILT = false;

var HelpOptionsVar = "width=800,height=600,scrollbars,toolbar,resizable,dependent=yes";
var bigsub   = "width=700,height=460,scrollbars,menubar,resizable,status,dependent=yes";

var upg_fw_sub ="width=620,height=270,scrollbars,menubar,resizable,status,dependent=yes";
var wsec_sub   = "width=620,height=390,scrollbars,menubar,resizable,status,dependent=yes";
var systime_sub = "width=620,height=280,scrollbars,menubar,resizable,status,dependent=yes";

var smallsub = "width=440,height=320,scrollbars,resizable,dependent=yes";
var specialsub = "width=440,height=320,scrollbars,resizable,dependent=no";
var sersub   = "width=500,height=380,scrollbars,resizable,status,dependent=yes";
var memsub   = "width=630,height=320,scrollbars,menubar,resizable,status,dependent=yes";
var viewingWinoptions = "width=800,height=500,scrollbars=no,resizable,status";
var motionsub = "width=810,height=540,scrollbars=no,resizable,status,dependent=yes";
var helpWinVar = null;
var glossWinVar = null;
var datSubWinVar = null;
var ValidStr = 'abcdefghijklmnopqrstuvwxyz-';
var hex_str = "ABCDEFabcdef0123456789";

var InValid_Str_UserPassword = ',: ';

var Valid_Str = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
var Valid_Stri = 'abcdefghijklmnopqrstuvwxyz-ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.';
var Valid_Strs = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-';
var Valid_Strii = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.@';
var Valid_Strdn = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.@-';
var Valid_Strnn = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-';
var vali_str = '0123456789.';
var Vali_Stri = 'abcdefghijklmnopqrstuvwxyz-ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
var Vali_Stri_Bonj = 'abcdefghijklmnopqrstuvwxyz-ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ';
var Valid_st = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 "';
var Valid_ch = "~!@#$%^&*()_+|}{:?><,./;'[]\``=-\\";
var Valid_Strd = Valid_st + Valid_ch;

var Valid_blank=" ";
var Valid_str_name=Valid_Strd+Valid_blank;
var Valid_domain= 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-';

function checkDomainName(input_field, field_name,valid_char)
{
	if (checkHostName(input_field.value)==false)
	{
		if(valid_char==Vali_Stri)
				valid_char = 'A-Z,a-z,0-9 and "-"';
			if(valid_char==ValidStr)
				valid_char = 'a-z';
			if(valid_char==Valid_st)
				valid_char = 'A-Z,a-z,0-9,",balnk and "_"';		
			if(valid_char==Valid_Str)
				valid_char = 'A-Z,a-z,0-9,and "_"';					
			if(valid_char==Valid_domain)
				valid_char = 'A-Z,a-z,0-9,"-","." and "_"';	
				
		return addstr(msg_invalid,field_name,valid_char);
	}
	return "";
}

function checkHostName(str)
{
		var at="@";
		var dot=".";
		var dash="-";
		var unline="_";
		var lat=str.indexOf(at);
		var lstr=str.length;
		var ldot=str.indexOf(dot);
		var i;
		var aa;	
		//host name must include "." and "." can't be the first/last char.
		if (str.indexOf(dot)==-1 || str.indexOf(dot)==0 ||str.charAt(lstr-1)==".")
		{
			return false; 
		} 
		//"-" can't be the first/last char
		if(str.indexOf(dash)==0 ||str.charAt(lstr-1)=="-")
		{
			return false; 
		} 
		//"_" can't be the first/last char
		if(str.indexOf(unline)==0 ||str.charAt(lstr-1)=="_")
		{
			return false; 
		} 
		// can't include space
		if (str.indexOf(" ")!=-1)
		{		   
		    return false;
		}
		// xx-.com is invalid 	
		if(str.indexOf("-.")!=-1)
		{					
			return false;
		}
		
		// xx.-com is invalid    
		if(str.indexOf(".-")!=-1 )
		{
			return false;
		}
		// xx..com is invalid
		if(str.indexOf("..")!=-1)
		{
			 return false;
		}
		for (i=0;i<=lstr-1;i++)
		{ 
			aa=str.charAt(i) 
			if (!((aa=='.') || (aa=='-') ||(aa=='_') || (aa>='0' && aa<='9') || (aa>='a' && aa<='z') || (aa>='A' && aa<='Z')))
			{ 
				return false; 
			} 
		}
		return true;	
}


function openViewWin()
{
//RELEASE
	viewWindowVar = window.open("/main.cgi?next_file=main.htm", "viewWin", viewingWinoptions);
//DEMO
//	viewWindowVar = window.open("main.htm", "viewWin", viewingWinoptions);
}

var Vshowit = "visible";
var Vhideit = "hidden";

function setVisible(el,shownow)  // IE & NS6; shownow = true, false
{
  if (document.all)
    document.all(el).style.visibility = (shownow) ? Vshowit : Vhideit ;
  else if (document.getElementById)
   document.getElementById(el).style.visibility = (shownow) ? Vshowit : Vhideit ;
}

var restart_time = 5000; // msecs


function changeTab(url)
{
	location.href=url;
}

/*
function isIE(){
  if(navigator.appName.indexOf("Microsoft") != -1)
		return true;
  else return false;
}*/

function isWindows(){
  if(navigator.platform.indexOf("Win") != -1)
		return true;
  else return false;
}

var showit = "block";
var hideit = "none";

function show_hide(el,shownow)  // IE & NS6; shownow = true, false
{
        if (document.all)
                document.all(el).style.display = (shownow) ? showit : hideit ;
        else if (document.getElementById)
                document.getElementById(el).style.display = (shownow) ? showit : hideit ;
}

function showMsg(caller)
{
	var msgVar=document.forms[0].message.value;
	var serverAdd = "http://"  +  self.location.host + ":" + self.location.port;
	var timeoutStr ;


	if (msgVar == "restart")
	{
		rALERT(caller,addstr(msg_restart, restart_time/1000));
//		setTimeout(timeoutStr,restart_time);
	}
	else if (msgVar == "changeIP")
	{
		rALERT(caller,addstr(msg_changeIP, restart_time/1000));
		top.close();
	}
	else if(msgVar == "@" + "m" + "essage#")
		;
	else if (msgVar == "Logo Upload Failure")
	{
		diALERT(caller,addstr("Logo Upload Failure", restart_time/1000));
	}
	
	else if (msgVar == "Logo Upload Success")
	{
		iALERT(caller,addstr("Logo Upload Success", restart_time/1000));
	}
	
	else if (msgVar.length > 1)
	{
		if(msgVar.indexOf("The settings of web server of camera") != -1)
		{
			rINFO(caller,msgVar);
		}
		//else if(msgVar.indexOf("Success to send e-mail") != -1)
		else if(msgVar.indexOf("SMTP Server Test Passed. E-mail was successfully sent") != -1)
		{
			rINFO(caller,msgVar);
		}
		else
			rALERT(caller,msgVar);
	}

}

function closeWin(win_var)
{
	if   ((win_var != null) && (win_var.closed == false)) 
			win_var.close();
}

function openHelpWin2(file_name)
{
   helpWinVar = window.open(file_name,'help_win',HelpOptionsVar,bigsub);
   if (helpWinVar.focus)
		setTimeout('helpWinVar.focus()',200);
}

function openHelpWin(file_name)
{
	closeWin(helpWinVar);
   helpWinVar = window.open(file_name,'help_win',bigsub);
   if (helpWinVar.focus)
		setTimeout('helpWinVar.focus()',200);
}

function openGlossWin()
{
	glossWinVar = window.open('','gloss_win',GlossOptionsVar);
	if (glossWinVar.focus)
		setTimeout('glossWinVar.focus()',200);
}

var randomn = Math.round(Math.random()*1000);
function openDataSubWin(filename,win_type)
{
	closeWin(datSubWinVar);
	datSubWinVar = window.open(filename,'datasub_win' + randomn,win_type);
	if (datSubWinVar.focus)
		setTimeout('datSubWinVar.focus()',200); 
}

function closeSubWins()
{
	closeWin(helpWinVar);
	closeWin(glossWinVar);
	closeWin(datSubWinVar);
}

function is485Hex(str) {
   var i;
   for(i = 0; i<str.length; i++)
   {
	   var c = str.substring(i, i+1);
	   if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F") || ("," == c)) // &cedil;
	   { continue; }
       return (msg_hex485key);
   }
   return "";
}

function checkBlank(fieldObj, fname)
{
	var msg = "";
	if (fieldObj.value.length < 1)
		msg = addstr(msg_blank,fname);
	return msg;
}

function checkMustBlank(fieldObj, fname)
{
	var msg = "";
	if (fieldObj.value.length >= 0)
		msg = addstr(msg_fill,fname);
	return msg;
}

function checkNoBlanks(fObj, fname)
{
	var space = " ";
 	if (fObj.value.indexOf(space) >= 0 )
			return msg_nospaces;
	else return "";
}

function checkMailAdd(input_field, field_name)
{
	if (!(input_field.value.indexOf("@") >= 0 ))
		return addstr(msg_mail, field_name);
	if (!(input_field.value.indexOf(".") >= 0 ))
		return addstr(msg_mail, field_name);
	return "";
}

function checkIPAdd(input_field)
{
	var error_msg= "";
	var size = input_field.value.length;
	var str = input_field.value;

	for (var i=0; i < size; i++)
	{
		if (!(vali_str.indexOf(str.charAt(i)) >= 0))
		{
			error_msg = addstr(msg_ipadd);
			break;
		}
	}
  	return error_msg;
}

function checkHyphen(text_input_field, name)
{
	var size = text_input_field.value.length;
	var str = text_input_field.value;
	var hyphen_Str = "-";
	var error_msg= "";
	if ((hyphen_Str.indexOf(str.charAt(0)) >= 0) || (hyphen_Str.indexOf(str.charAt(size-1)) >= 0) )
	{
		error_msg = addstr('%s can not begin or end with "-"', name);
	}
	return error_msg;
}

function checkValid(text_input_field, field_name, Valid_Str, max_size, mustFill)
{
	var error_msg= "";
	var size = text_input_field.value.length;
	var str = text_input_field.value;


	if ((mustFill) && (size != max_size) )
		return (addstr(msg_nospaces,field_name));
 	for (var i=0; i < size; i++)
  	{
    	if (!(Valid_Str.indexOf(str.charAt(i)) >= 0))
    	{
			if(Valid_Str=='abcdefghijklmnopqrstuvwxyz-ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789')
				Valid_Str = 'A-Z,a-z,0-9 and "-"';
			if(Valid_Str=='abcdefghijklmnopqrstuvwxyz-')
				Valid_Str = 'a-z';
			if(Valid_Str=='abcdefghijklmnopqrstuvwxyz_\nABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 "')
				Valid_Str = 'A-Z,a-z,0-9,",balnk and "_"';		
			if(Valid_Str=='abcdefghijklmnopqrstuvwxyz_\nABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789')
				Valid_Str = 'A-Z,a-z,0-9,and "_"';		
			if(Valid_Str=='abcdefghijklmnopqrstuvwxyz_\nABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-')
				Valid_Str = 'A-Z,a-z,0-9,"-","." and "_"';		
				
			error_msg = addstr(msg_invalid,field_name,Valid_Str);
			break;
    	}
  	}
  	return error_msg;
}

function checkHEX(text_input_field, field_name, hex_str, max_size, mustFill)
{
	var error_msg= "";
	var size = text_input_field.value.length;
	var str = text_input_field.value;

	if ((mustFill) && (size != max_size) )
		return (addstr(msg_nospaces,field_name));
 	for (var i=0; i < size; i++)
  	{
    	if (!(hex_str.indexOf(str.charAt(i)) >= 0))
    	{
			error_msg = addstr(msg_invalid,field_name,hex_str);
			break;
    	}
  	}
  	return error_msg;
}

function checkInvalid(text_input_field, field_name, InvalidStr)
// check that no chars in "InvalidStr" appear in input
{
  var str = text_input_field.value;
  var error_msg= "";


  for (var i=0; i < InvalidStr.length; i++)
  {
    if (str.indexOf(InvalidStr.charAt(i)) >= 0)
    {
		if(InvalidStr==',: ')
			InvalidStr = '",", ":" and " "';
		 error_msg = addstr(msg_notallow,field_name,InvalidStr);
		 break;
    }
  }
  return error_msg;
}


function checkInt(text_input_field, field_name, min_value, max_value, required)
// NOTE: Doesn't allow negative numbers, required is true/false
{
	var str = text_input_field.value;
	var error_msg= "";
	

	if (text_input_field.value.length==0) // blank
	{
		if (required)
			error_msg = addstr(msg_blank,field_name);
	}
	else // not blank, check contents
	{
		for (var i=0; i < str.length; i++)
		{
			if ((str.charAt(i) < '0') || (str.charAt(i) > '9'))
				error_msg = addstr(msg_check_invalid,field_name);
		}
		if (error_msg.length < 2) // don't parse if invalid
		{

			var int_value = parseInt(str,10);
			if (int_value < min_value)
				error_msg = addstr(msg_outofrange,field_name,min_value,max_value);
			if (int_value > max_value)
				error_msg = addstr(msg_outofrange,field_name,min_value,max_value);
		}
	}
//	if (error_msg.length > 1)
//		error_msg = error_msg + "\n";
	return(error_msg);
}

function blankIP(fn) // true if 0 or blank
{
	return ( (fn.value == "") || (fn.value == "0") )
}

function checkIp(ip1,ip2,ip3,ip4,msg,rq_flag)
{
	if( (rq_flag == false) && blankIP(ip1) && blankIP(ip2) && blankIP(ip3) && blankIP(ip4) )
		return "";
	var errmsg =  checkInt(ip1,msg,1,254,true);
	errmsg =  (errmsg.length > 1) ? errmsg : checkInt(ip2,msg,0,255,true);
	errmsg =  (errmsg.length > 1) ? errmsg : checkInt(ip3,msg,0,255,true);
	errmsg =  (errmsg.length > 1) ? errmsg : checkInt(ip4,msg,1,254,true);
	errmsg =  (errmsg.length > 1) ? addstr(msg_validIP,msg) : "";
	return errmsg;
}

function checkNetMask(ip1,ip2,ip3,ip4,msg)
{
	var errmsg =  checkInt(ip1,msg,0,255,true);
	errmsg =  (errmsg.length > 1) ? errmsg : checkInt(ip2,msg,0,255,true);
	errmsg =  (errmsg.length > 1) ? errmsg : checkInt(ip3,msg,0,255,true);
	errmsg =  (errmsg.length > 1) ? errmsg : checkInt(ip4,msg,0,255,true);
	errmsg =  (errmsg.length > 1) ? addstr(msg_validMask,msg) : "";
	return errmsg;
}

function checkPort(val)
{
	if(val >= 1024 && val < 65535 && val%2 == 0)
		return true;
	else
		return false;
}

function search_string(s_string, sub_string)
{
	var i=0, j;
	var first_char = sub_string.charAt(0);
	var sub_length = sub_string.length;
	
	while (i < s_string.length)
	{
		if (s_string.charAt(i) == first_char)
		{
			j = 0;
			while ((j < sub_length) && (s_string.charAt(i+j) == sub_string.charAt(j)))
					j++;
			if (j == sub_length) // all chars match
				return(i); // match starts at i
		}
		i++;
	}
	return -1; // not found
}

function getRadioCheckedValue(radio_object)
{
	var size = radio_object.length;
	for (var i = 0; i < size; i++)
	{
		if (radio_object[i].checked == true)
			return(radio_object[i].value)
	}
	return (radio_object[0].value); // first value if nothing checked
}

function getRadioIndex(radio_object, checked_value)  
{
	var size = radio_object.length;
	for (var i = 0; i < size; i++)
	{
		if (radio_object[i].value == checked_value)
			return  i;
	}

	return  0;   // if no match
}

function getSelIndex(sel_object, sel_text,caseSensitive)
// caseSensitive: true = exact match, false = ignore case
{
	if (sel_text.length == 0)
		return 0;   //  Nothing may be valid. e.g. New SAP contain errors & returned.
	var size = sel_object.options.length;
	var match;
	for (var i = 0; i < size; i++)
	{
		if (caseSensitive)
		  match = ( (sel_object.options[i].text == sel_text) || (sel_object.options[i].value == sel_text) )
		else
		  match =  ( (sel_object.options[i].text.toLowerCase() == sel_text.toLowerCase()) || (sel_object.options[i].value.toLowerCase() == sel_text.toLowerCase()) );
		if (match)
			return(i);
	}
	return 0;  // if no match
}

function dupSelEntry(sel_object,newvalue,caseSensitive)
// check if renaming current value to existing value
// caseSensitive: true = exact match, false = ignore case
{
	var counter = 0;
	var match;
	var size = sel_object.options.length;
	var index = sel_object.selectedIndex;

	for (var i = 0; i < size; i++)
	{
		if (caseSensitive)
		  match = (sel_object.options[i].text == newvalue) ;
		else
		  match =  (sel_object.options[i].text.toLowerCase() == newvalue.toLowerCase() );
		if ((match) && (i != index))
			return true;
	}
	return false;
}

function chkSelected(selObj, err_msg)
{
    if(!(selObj.selectedIndex >= 0 ))
	{
	    rALERT("",err_msg);
	    return false;
	}
	return true;
}
function addOption(selObj, textStr, valueStr)  // value optional
{
	if (addOption.arguments.length == 3)
		selObj.options[selObj.options.length] = new Option(textStr, valueStr);
	else
		selObj.options[selObj.options.length] = new Option(textStr, ""); // value blank
}

function delOption(sel_obj, position)
{
	for (var i = position; i < sel_obj.options.length - 1; i++)
	{
		sel_obj.options[i].text = sel_obj.options[i + 1].text;
		sel_obj.options[i].value = sel_obj.options[i + 1].value;
	}
	sel_obj.options.length = sel_obj.options.length - 1;
}

function getOptionList(sel_obj, strType)  
// return string. strType  = "text", "value" 
{
	var size = sel_obj.options.length; 
	var i; 
	var str = "";
	for(i = 0; i < sel_obj.options.length;  i++)
		str+= (strType=="value")? sel_obj.options[i].value + separator : sel_obj.options[i].text + separator; 
	return str;
}



function makeStr(strchar, strSize)
{
	var newStr = "";
	for (var i = 0; i < strSize; i++)
		newStr+= strchar;
	return newStr;
}

function ignoreSpaces(string)
{
       var temp = "";
       var first=1;
       
       string = '' + string;
       splitstring = string.split(" ");
       for(i = 0; i < splitstring.length; i++)
            if(splitstring[i]!=""){
                if(first==1){
                      if(splitstring[i]!=" "){
                          temp=splitstring[i];
                          first=0;
                      }
                }          
               else
                      temp = temp +  " " + splitstring[i];
            }          
       return temp;
}

function addstr(input_msg)
{
	var last_msg = "";
	var str_location;
	var temp_str_1 = "";
	var temp_str_2 = "";
	var str_num = 0;
	temp_str_1 = addstr.arguments[0];
	while(1)
	{
		str_location = temp_str_1.indexOf("%s");
		if(str_location >= 0)
		{
			str_num++;
			temp_str_2 = temp_str_1.substring(0,str_location);
			last_msg += temp_str_2 + addstr.arguments[str_num];
			temp_str_1 = temp_str_1.substring(str_location+2,temp_str_1.length);
			continue;
		}
		if(str_location < 0)
		{
			last_msg += temp_str_1;
			break;
		}
	}
	return last_msg;
}


function show_data(form_obj)   
// shows form information - used only for debugging
{
	var headvar = "<" + "h" + "e" + "a" + "d" + ">";
	var headend = "<" + "/" + "h" + "e" + "a" + "d" + ">";
	var bodyvar = "<" + "b" + "o" + "d" + "y" + ">";
	var form_size = form_obj.elements.length;
	var debug_win = window.open("","debug","width=540,height=360,menubar=yes,scrollbars=yes,resizable=yes");
	with(debug_win.document)
	{
		open();
		writeln('<html>' + headvar + '<title>Debugging Window</title>' + headend);
		writeln(bodyvar + '<h2>Form being submitted</h2>');
		writeln('<p>Form Name: ' + form_obj.name);
		writeln('<br>Form Action: ' + form_obj.action);
		writeln('<br>Form Target: ' + form_obj.target);
		writeln('</p><h3>Form Data</h3><p>Following table shows ALL fields, even if not submitted.</p>');
		writeln('<p><table border=1><tr bgcolor="#cccccc"><th nowrap>Field Name</th><th>Type</th><th>Value</th></tr>');
		for (var i = 0; i < form_size; i++)
		{
			writeln('<tr><td nowrap>' + form_obj.elements[i].name + '</td>');
			writeln('<td nowrap>' + form_obj.elements[i].type + '</td>');
			writeln('<td nowrap>');
			if ((form_obj.elements[i].type=="select-one") || (form_obj.elements[i].type=="select-multiple"))
				writeln('Selected item: ' + form_obj.elements[i].options[form_obj.elements[i].selectedIndex].text);			
			else 
				writeln(form_obj.elements[i].value);
			if ((form_obj.elements[i].type == "radio") && (form_obj.elements[i].checked))
						write(' (Selected)');
			if ((form_obj.elements[i].type == "checkbox") && (form_obj.elements[i].checked))
					writeln(' (Checked)');
			writeln('</td></tr>');
		}
		writeln('</table></body></html>');
		close();
	}
}


/* BUTTON FUNCTIONS */
function MM_findObj(n, d) { //v4.01
  var p,i,x;  if(!d) d=document; if((p=n.indexOf("?"))>0&&parent.frames.length) {
    d=parent.frames[n.substring(p+1)].document; n=n.substring(0,p);}
  if(!(x=d[n])&&d.all) x=d.all[n]; for (i=0;!x&&i<d.forms.length;i++) x=d.forms[i][n];
  for(i=0;!x&&d.layers&&i<d.layers.length;i++) x=MM_findObj(n,d.layers[i].document);
  if(!x && d.getElementById) x=d.getElementById(n); return x;
}

function MM_preloadImages() { //v3.0
  var d=document; if(d.images){ if(!d.MM_p) d.MM_p=new Array();
    var i,j=d.MM_p.length,a=MM_preloadImages.arguments; for(i=0; i<a.length; i++)
    if (a[i].indexOf("#")!=0){ d.MM_p[j]=new Image; d.MM_p[j++].src=a[i];}}
}

function MM_swapImgRestore() { //v3.0
  var i,x,a=document.MM_sr; for(i=0;a&&i<a.length&&(x=a[i])&&x.oSrc;i++) x.src=x.oSrc;
}

function MM_swapImage() { //v3.0
  var i,j=0,x,a=MM_swapImage.arguments; document.MM_sr=new Array; for(i=0;i<(a.length-2);i+=3)
   if ((x=MM_findObj(a[i]))!=null){document.MM_sr[j++]=x; if(!x.oSrc) x.oSrc=x.src; x.src=a[i+2];}
}
//


function openPopUp(url, width, height, scrolling, name) { //v2.0
	if (!width) width = 320;
	if (!height) height = 240;
	if (!scrolling) scrolling = "no"; 
	features = "width="+width+","
		+ "height="+height+","
		+ "name="+name+","
		+ "toolbar=no,"
		+ "location=no,"
		+ "status=no,"
		+ "menubar=no,"
		+ "scrollbars="+scrolling+","
		+ "top="+(window.screen.height-height)/2+","
		+ "left="+(window.screen.width-width)/2;
	window.open(url,"win"+Math.round(Math.random()*1000),features);
}


function setHTML(windowObj, el, htmlStr)
{
    if (document.all)
	{
	  if (windowObj.document.all(el) )
        windowObj.document.all(el).innerHTML = htmlStr;
	}
	else if (document.getElementById)
	{
	  if (windowObj.document.getElementById(el) )
	    windowObj.document.getElementById(el).innerHTML = htmlStr;
	}
}

/*
function setHTML(windowObj, el, htmlStr)
{
    if (document.all)
	{
	  if (windowObj.document.all(el) )
        windowObj.document.all(el).innerHTML = htmlStr;
	}
	else if (document.getElementById)
	{
	  if (windowObj.document.getElementById(el) )
	    windowObj.document.getElementById(el).innerHTML = htmlStr;
	}
}
*/

//////////////////////////////New Add/////////////////////////////////

var enablepersist="on" //Enable saving state of content structure using session cookies? (on/off)
var collapseprevious="no" //Collapse previously open content when opening present? (yes/no)

var contractsymbol='&nbsp;- &nbsp;' //HTML for contract symbol. For image, use: <img src="whatever.gif">
var expandsymbol='&nbsp;+&nbsp;' //HTML for expand symbol.

if (document.getElementById)
{
	document.write('<style type="text/css">')
	document.write('.switchcontent{display:none;}')
	document.write('</style>')
}

function getElementbyClass(rootobj, classname)
{
	var temparray=new Array()
	var inc=0
	var rootlength=rootobj.length
	for (i=0; i<rootlength; i++){
	if (rootobj[i].className==classname)
	temparray[inc++]=rootobj[i]
}
	return temparray
}

function sweeptoggle(ec){
var thestate=(ec=="expand")? "block" : "none"
var inc=0
while (ccollect[inc])
{
	ccollect[inc].style.display=thestate
	inc++
}
	revivestatus()
}


function contractcontent(omit)
{
	var inc=0
	while (ccollect[inc])
	{
		if (ccollect[inc].id!=omit)
		ccollect[inc].style.display="none"
		inc++
	}
}

function expandcontent(curobj, cid)
{
	var spantags=curobj.getElementsByTagName("SPAN")
	var showstateobj=getElementbyClass(spantags, "showstate")
	if (ccollect.length>0)
	{
		if (collapseprevious=="yes")
		contractcontent(cid)
		document.getElementById(cid).style.display=(document.getElementById(cid).style.display!="block")? "block" : "none"
			
		if (showstateobj.length>0)
		{ //if "showstate" span exists in header
			if (collapseprevious=="no")
			showstateobj[0].innerHTML=(document.getElementById(cid).style.display=="block")? contractsymbol : expandsymbol
			else
			revivestatus()
		}
	}
}

function revivecontent()
{
	contractcontent("omitnothing")
	selectedItem=getselectedItem()
	selectedComponents=selectedItem.split("|")
	
	for (i=0; i<selectedComponents.length-1; i++)
	document.getElementById(selectedComponents[i]).style.display="block"
}

function revivestatus()
{
	var inc=0
	while (statecollect[inc])
	{
		if (ccollect[inc].style.display=="block")
		statecollect[inc].innerHTML=contractsymbol
		else
		statecollect[inc].innerHTML=expandsymbol
		inc++
	}
}

function get_cookie(Name)
{ 
	var search = Name + "="
	var returnvalue = "";
	if (document.cookie.length > 0)
	{
		offset = document.cookie.indexOf(search)
		if (offset != -1)
		{ 
		offset += search.length
		end = document.cookie.indexOf(";", offset);
		if (end == -1)
		end = document.cookie.length;
		returnvalue=unescape(document.cookie.substring(offset, end))
		}
	}
	return returnvalue;
}

function getselectedItem()
{
	if (get_cookie(window.location.pathname) != "")
	{
		selectedItem=get_cookie(window.location.pathname)
		return selectedItem
	}
	else
	return ""
}

function saveswitchstate()
{
var inc=0, selectedItem=""
while (ccollect[inc])
{
	if (ccollect[inc].style.display=="block")
	selectedItem+=ccollect[inc].id+"|"
	inc++
}
document.cookie=window.location.pathname+"="+selectedItem
}

function do_onload()
{
	uniqueidn=window.location.pathname+"firsttimeload"
	var alltags=document.all? document.all : document.getElementsByTagName("*")
	ccollect=getElementbyClass(alltags, "switchcontent")
	statecollect=getElementbyClass(alltags, "showstate")
	if (enablepersist=="on" && ccollect.length>0)
	{
		document.cookie=(get_cookie(uniqueidn)=="")? uniqueidn+"=1" : uniqueidn+"=0" 
		firsttimeload=(get_cookie(uniqueidn)==1)? 1 : 0 //check if this is 1st page load
		if (!firsttimeload)
		revivecontent()
	}
	if (ccollect.length>0 && statecollect.length>0)
	revivestatus()
}


/////
if (window.addEventListener)
window.addEventListener("load", do_onload, false)
else if (window.attachEvent)
window.attachEvent("onload", do_onload)
else if (document.getElementById)
window.onload=do_onload

if (enablepersist=="on" && document.getElementById)
window.onunload=saveswitchstate
/////

function printPage()
{
    location.href="javascript:print();";
}


// Browser Issue
function isIE()
{
 if(navigator.appName.indexOf("Microsoft Internet Explorer") != -1)
 return true;
 else return false;
}

function isFire()
{
  if(navigator.userAgent.indexOf("Firefox/2.0.0.3") >= 0)
  {
	  return true;
  }
  else {
	  return false;
  }
}

function isNS()
{
 if(navigator.appName.indexOf("Netscape") != -1)
 return true;
 else return false;
}

function isOpera()
{
  if(navigator.userAgent.indexOf("Opera") >= 0)
  {
	  return true;
  }
  else {
	  return false;
  }
}

function isMac()
{
 if(navigator.appVersion.indexOf("Mac") != -1)
 return true;
 else return false;
}

function isWin()
{
 if(navigator.appVersion.indexOf("Win") != -1)
 return true;
 else return false;
}

function isOld()
{
  if(!document.getElementById)
  {
     document.getElementById = function(element)
     { return eval("document.all." + element); }
  }
}

function setClass(id,content,classname)
{
    if (document.all)
	{ document.all(id).className = content; }
	else if (document.getElementById)
	{ document.getElementById(id).className = content; }
} 
