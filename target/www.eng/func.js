// Help and Message ==========================================================


var HelpOptionsVar = "width=480,height=420,scrollbars,toolbar,resizable,dependent=yes";
var GlossOptionsVar = "width=420,height=180,scrollbars,toolbar,resizable,dependent=yes";
var bigsub   = "width=700,height=440,scrollbars,menubar,resizable,status,dependent=yes";
var smallsub = "width=440,height=320,scrollbars,resizable,dependent=yes";
var sersub   = "width=500,height=380,scrollbars,resizable,status,dependent=yes";
var multisub   = "width=630,height=470,scrollbars,menubar,resizable,status,dependent=yes";
var helpWinVar = null;
var glossWinVar = null;
var datSubWinVar = null;
var ValidStr = 'abcdefghijklmnopqrstuvwxyz-';
var ValidStr_ddns = 'abcdefghijklmnopqrstuvwxyz-1234567890';
var hex_str = "ABCDEFabcdef0123456789";
var DEBUG = 0;


function showMsg()
{
	var msgVar=document.forms[0].message.value;
	if (msgVar.length > 1)
		alert(msgVar);
}


function closeWin(win_var)
{
	if ( ((win_var != null) && (win_var.close)) || ((win_var != null) && (win_var.closed==false)) )
		win_var.close();
}

function openHelpWin(file_name)
{
   helpWinVar = window.open(file_name,'help_win',HelpOptionsVar);
   if (helpWinVar.focus)
		setTimeout('helpWinVar.focus()',200);
}

function openGlossWin()
{
	glossWinVar = window.open('','gloss_win',GlossOptionsVar);
	if (glossWinVar.focus)
		setTimeout('glossWinVar.focus()',200);
}

function closeSubWins()
{
	closeWin(helpWinVar);
	closeWin(glossWinVar);
	closeWin(datSubWinVar);
}

function openDataSubWin(filename,win_type)
{
	closeWin(datSubWinVar);
	datSubWinVar = window.open(filename,'datasub_win',win_type);
	if (datSubWinVar.focus)
		setTimeout('datSubWinVar.focus()',200);
}

function showHelp(helpfile) 
{
	if(top.frames.length == 0)
		return;
	top.helpframe.location.href = helpfile;
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

function checkMsg(msg)
{
	if(msg.length > 1)
	{
		alert(msg);
		return false;
	}
	return true;
}	

function setHTML(windowObj, el, htmlStr)  // el must be str, not reference
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


//  High-level test functions - generate messages

function checkBlank(fieldObj, fname)
{
	var msg = "";
	if (fieldObj.value.length < 1){
		msg = addstr(msg_blank,fname);
        }
	return msg;
}

function checkNoBlanks(fObj, fname)
{
	var space = " ";
 	if (fObj.value.indexOf(space) >= 0 )
		return addstr(msg_space, fname);
	else return "";
}

function checkAllSpaces(fieldObj, fname)
{
	var msg = "";
	if(fieldObj.value.length == 0)
		return "";
	var tstr = makeStr(fieldObj.value.length," ");
	if (tstr == fieldObj.value)
		msg = addstr(msg_allspaces,fname);
	return msg;
}

function checkValid(text_input_field, field_name, Valid_Str, max_size, mustFill)
{
	var error_msg= "";
	var size = text_input_field.value.length;
	var str = text_input_field.value;

	if ((mustFill) && (size != max_size) )
		error_msg = addstr(msg_blank_in,field_name);
 	for (var i=0; i < size; i++)
  	{
    	if (!(Valid_Str.indexOf(str.charAt(i)) >= 0))
    	{
			error_msg = addstr(msg_invalid,field_name,Valid_Str);
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
			if (int_value < min_value || int_value > max_value)
				error_msg = addstr(msg_valid_range,field_name,min_value,max_value);
		}
	}
	return(error_msg);
}


// Low-level test functions - return true or false ============================


function blankIP(ip1, ip2, ip3, ip4) // ip fields, true if 0 or blank
{
return ((ip1.value == "" || ip1.value == "0")
	 && (ip2.value == "" || ip2.value == "0")
	 && (ip3.value == "" || ip3.value == "0")
	 && (ip4.value == "" || ip4.value == "0"))
}

function badIP(ip1, ip2, ip3, ip4, max)   // ip fields, 1.0.0.1 to 254.255.255.max
{
	if(!(isInteger(ip1.value,1,254,false))) return true;
	if(!(isInteger(ip2.value,0,255,false))) return true;
	if(!(isInteger(ip3.value,0,255,false))) return true;
	if(!(isInteger(ip4.value,1,max,false))) return true;
   	return false;
}
function badSubnetIP(ip1, ip2, ip3, ip4, max)   // ip fields 1.0.0.0. to 255.255.255.max
{
	if(!(isInteger(ip1.value,1,254,false))) return true;
	if(!(isInteger(ip2.value,0,255,false))) return true;
	if(!(isInteger(ip3.value,0,255,false))) return true;
	if(!(isInteger(ip4.value,0,max,false))) return true;
   	return false;
}


function badMask(ip1, ip2, ip3, ip4)   // mask fields 0 to 255
{
	if(!(isInteger(ip1.value,0,255,false))) return true;
	if(!(isInteger(ip2.value,0,255,false))) return true;
	if(!(isInteger(ip3.value,0,255,false))) return true;
	if(!(isInteger(ip4.value,0,255,false))) return true;
   	return false;
}


function badMac(macfld, removeSeparators) // macfld is form field, removeSeparators true/false
{
	var myRE = /[0-9a-fA-F]{12}/;
	var MAC = macfld.value;	
	
	MAC = MAC.replace(/:/g,"");
	MAC = MAC.replace(/-/g,"");
	if (removeSeparators)
		macfld.value = MAC;	
	if((MAC.length != 12) || (MAC == "000000000000")||(myRE.test(MAC)!=true))
		return true;
	else
	 	return false;
}

function ValidMacAddress(macAddr)
{
//	alert("ValidMacAddress(): Use badMac(macfld, removeSeparators) instead!");
	return;
	
	var i;
	if ((macAddr.indexOf(':')!=-1)||(macAddr.indexOf('-')!=-1))
	{     
        macAddr = macAddr.replace(/:/g,"");
		macAddr = macAddr.replace(/-/g,"");
	}
	
	if ((macAddr.length == 12) && (macAddr != "000000000000"))
	{
		for(i=0; i<macAddr.length;i++) 
		{
			var c = macAddr.substring(i, i+1);
			if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) 
				continue;
			else
				return false;
		}

		return true;
	}

	return false;	  
}



function badIpRange(from1,from2,from3,from4,to1,to2,to3,to4)
// parameters are form fields, returns true if invalid ( from > to )
{
    var total1 = 0;
    var total2 = 0;
    
    total1 += parseInt(from4.value,10);
    total1 += parseInt(from3.value,10)*256;
    total1 += parseInt(from2.value,10)*256*256;
    total1 += parseInt(from1.value,10)*256*256*256;
    
    total2 += parseInt(to4.value,10);
    total2 += parseInt(to3.value,10)*256;
    total2 += parseInt(to2.value,10)*256*256;
    total2 += parseInt(to1.value,10)*256*256*256;
    if(total1 >= total2)
        return true;
    return false;
}


function isBlank(str) 
{
	return (str.length == 0 );
}


function isBigger(str_a, str_b)
//  true if a bigger than b
{
	var int_value_a = parseInt(str_a);
	var int_value_b = parseInt(str_b);
	return (int_value_a > int_value_b);
}

function isInteger(str,min_value,max_value,allowBlank)  // allowBlank = true or false
// return true if positive Integer, false otherwise
{
	if(str.length == 0)
		if(allowBlank)
			return true;
		else
			return false;
	for (var i=0; i < str.length; i++)
	{
		if ((str.charAt(i) < '0') || (str.charAt(i) > '9'))
				return false;
	}
	var int_value = parseInt(str,10);
	if ((int_value < min_value) || (int_value > max_value))
		return false;
	return true;
}


function isHex(str) {
    var i;
    for(i = 0; i<str.length; i++) {
        var c = str.substring(i, i+1);
        if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) {
            continue;
        }
        return false;
    }
    return true;
}

function isTelephoneNum(str) 
{
	var c;
    if(str.length == 0) 
        return false;
    for (var i = 0; i < str.length; i++) 
	{
        c = str.substring(i, i+1);
        if (c>= "0" && c <= "9")
            continue;
        if ( c == '-' && i !=0 && i != (str.length-1) )
            continue;
        if ( c == ',' ) continue;
        if (c == ' ') continue;
        if (c>= 'A' && c <= 'Z') continue;
        if (c>= 'a' && c <= 'z') continue;
        return false;
    }
    return true;
}

function checkDay(year,month,day)  // check if valid date
{
	var isleap = false;
	if(year%400 == 0 || (year%4 == 0 && year%100 != 0))
		isleap = true;
	if(month%2)
	{
		if((month<=7)&&(day>31))
			return false;
		if((month>7)&&(day>30))
			return false;
	}
	else
	{
		if(month<=6)
		{
			if(month == 2)
			{
				if((isleap)&&(day>29))
				{
					return false;
				}
				if((!isleap)&&(day>28))
				{			
					return false;	
				}		
			}
			else
			{
				if(day > 30)
					return false;
			}
		}
		else
			if(day>31)
				return false;
	}
	return true;
}

function CheckSpaceInName(text_input_field)
//not allow space in name,
{
	if (text_input_field.value.length>1)
	{
		for (var i=0;i<text_input_field.value.length;i++)
		{
			if (text_input_field.value.charAt(i) == ' ')
				return false;
		}
	}
	return true;
}

// Utility & Misc functions ===================================================


function isIE()
{
    if(navigator.appName.indexOf("Microsoft") != -1)
        return true;
    else return false;
}

function setDisabled(OnOffFlag,formFields)
{
	for (var i = 1; i < setDisabled.arguments.length; i++)
		setDisabled.arguments[i].disabled = OnOffFlag;
}

function makeStr(strSize, fillChar)
{
	var temp = "";
	for (i=0; i < strSize ; i ++)
		temp = temp + fillChar;
	return temp;
}

var showit = "block";
var hideit = "none";

function show_hide(el,shownow)  // IE & NS6; shownow = true, false
{
//	alert("el = " + el);
	if (document.all)
		document.all(el).style.display = (shownow) ? showit : hideit ;
	else if (document.getElementById)
		document.getElementById(el).style.display = (shownow) ? showit : hideit ;
}


function printPage()
{
    location.href="javascript:print();";
}



ie4 = ((navigator.appName == "Microsoft Internet Explorer") && (parseInt(navigator.appVersion) >= 4 ))
ns4 = ((navigator.appName == "Netscape") && (parseInt(navigator.appVersion) < 6 ))
ns6 = ((navigator.appName == "Netscape") && (parseInt(navigator.appVersion) >= 6 ))

// 0.0.0.0
var ZERO_NO = 1;	// 0x0000 0001
var ZERO_OK = 2;	// 0x0000 0010
// x.x.x.0
var MASK_NO = 4;	// 0x0000 0100
var MASK_OK = 8;	// 0x0000 1000
// 255.255.255.255
var BCST_NO = 16;	// 0x0001 0000
var BCST_OK = 32;	// 0x0010 0000

var SPACE_NO = 1;
var SPACE_OK = 2;

function choose_disable(dis_object)
{
	if(!dis_object)	return;
	dis_object.disabled = true;

	if(!ns4)
		dis_object.style.backgroundColor = "#e0e0e0";
}
