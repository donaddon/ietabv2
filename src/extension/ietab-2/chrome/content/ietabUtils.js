//-----------------------------------------------------------------------------

// Updates:
//
// 2/24/10: ietab.net -- Converted to new ietab2 namespace.
//

IeTab2.prototype.mlog = function(text) {
  Components.classes["@mozilla.org/consoleservice;1"]
    .getService(Components.interfaces.nsIConsoleService)
    .logStringMessage("IeTab: "+text);
}

//-----------------------------------------------------------------------------

IeTab2.prototype.addEventListener = function(obj, type, listener) {
   if (typeof(obj) == "string") obj = document.getElementById(obj);
   if (obj) obj.addEventListener(type, listener, false);
}
IeTab2.prototype.removeEventListener = function(obj, type, listener) {
   if (typeof(obj) == "string") obj = document.getElementById(obj);
   if (obj) obj.removeEventListener(type, listener, false);
}

IeTab2.prototype.addEventListenerByTagName = function(tag, type, listener) {
   var objs = document.getElementsByTagName(tag);
   for (var i = 0; i < objs.length; i++) {
      objs[i].addEventListener(type, listener, false);
   }
}
IeTab2.prototype.removeEventListenerByTagName = function(tag, type, listener) {
   var objs = document.getElementsByTagName(tag);
   for (var i = 0; i < objs.length; i++) {
      objs[i].removeEventListener(type, listener, false);
   }
}

//-----------------------------------------------------------------------------

IeTab2.prototype.hookCode = function(orgFunc, orgCode, myCode) {
   try{ if (eval(orgFunc).toString() == eval(orgFunc + "=" + eval(orgFunc).toString().replace(orgCode, myCode))) throw orgFunc; }
   catch(e){ Components.utils.reportError("Failed to hook function: "+orgFunc); }
}

IeTab2.prototype.hookAttr = function(parentNode, attrName, myFunc) {
   if (typeof(parentNode) == "string") parentNode = document.getElementById(parentNode);
   try { parentNode.setAttribute(attrName, myFunc + parentNode.getAttribute(attrName)); }catch(e){ Components.utils.reportError("Failed to hook attribute: "+attrName); }
}

IeTab2.prototype.hookProp = function(parentNode, propName, myGetter, mySetter) {
   var oGetter = parentNode.__lookupGetter__(propName);
   var oSetter = parentNode.__lookupSetter__(propName);
   if (oGetter && myGetter) myGetter = oGetter.toString().replace(/{/, "{"+myGetter.toString().replace(/^.*{/,"").replace(/.*}$/,""));
   if (oSetter && mySetter) mySetter = oSetter.toString().replace(/{/, "{"+mySetter.toString().replace(/^.*{/,"").replace(/.*}$/,""));
   if (!myGetter) myGetter = oGetter;
   if (!mySetter) mySetter = oSetter;
   if (myGetter) try { eval('parentNode.__defineGetter__(propName, '+ myGetter.toString() +');'); }catch(e){ Components.utils.reportError("Failed to hook property Getter: "+propName); }
   if (mySetter) try { eval('parentNode.__defineSetter__(propName, '+ mySetter.toString() +');'); }catch(e){ Components.utils.reportError("Failed to hook property Setter: "+propName); }
}

//-----------------------------------------------------------------------------

IeTab2.prototype.trim = function(s) {
   if (s) return s.replace(/^\s+/g,"").replace(/\s+$/g,""); else return "";
}

IeTab2.prototype.startsWith = function(s, prefix) {
   if (s) return( (s.substring(0, prefix.length) == prefix) ); else return false;
}

//-----------------------------------------------------------------------------

IeTab2.prototype.getBoolPref = function(prefName, defval) {
   var result = defval;
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = prefservice.getBranch("");
   if (prefs.getPrefType(prefName) == prefs.PREF_BOOL) {
       try { result = prefs.getBoolPref(prefName); }catch(e){}
   }
   return(result);
}

IeTab2.prototype.getIntPref = function(prefName, defval) {
   var result = defval;
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = prefservice.getBranch("");
   if (prefs.getPrefType(prefName) == prefs.PREF_INT) {
       try { result = prefs.getIntPref(prefName); }catch(e){}
   }
   return(result);
}

IeTab2.prototype.getStrPref = function(prefName, defval) {
   var result = defval;
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = prefservice.getBranch("");
   if (prefs.getPrefType(prefName) == prefs.PREF_STRING) {
       try { result = prefs.getComplexValue(prefName, Components.interfaces.nsISupportsString).data; }catch(e){}
   }
   return(result);
}

//-----------------------------------------------------------------------------

IeTab2.prototype.flushPrefs = function() {
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   prefservice.savePrefFile(null);
}

IeTab2.prototype.setBoolPref = function(prefName, value) {
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = prefservice.getBranch("");
   try { prefs.setBoolPref(prefName, value); } catch(e){}
}

IeTab2.prototype.setIntPref = function(prefName, value) {
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = prefservice.getBranch("");
   try { prefs.setIntPref(prefName, value); } catch(e){}
}

IeTab2.prototype.setStrPref = function(prefName, value) {
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = prefservice.getBranch("");
   var sString = Components.classes["@mozilla.org/supports-string;1"].createInstance(Components.interfaces.nsISupportsString);
   sString.data = value;
   try { prefs.setComplexValue(prefName, Components.interfaces.nsISupportsString, sString); } catch(e){}
}

//-----------------------------------------------------------------------------

IeTab2.prototype.getDefaultCharset = function(defval) {
   var charset = this.getStrPref("extensions.ietab2.intl.charset.default", "");
   if (charset.length) return charset;
	var gPrefs = Components.classes['@mozilla.org/preferences-service;1'].getService(Components.interfaces.nsIPrefBranch);
	if(gPrefs.prefHasUserValue("intl.charset.default")) {
	   return gPrefs.getCharPref("intl.charset.default");
	} else {
	   var strBundle = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
	   var intlMess = strBundle.createBundle("chrome://global-platform/locale/intl.properties");
	   try {
	      return intlMess.GetStringFromName("intl.charset.default");
	   } catch(e) {
   	   return defval;
      }
	}
}

IeTab2.prototype.convertToUTF8 = function(data, charset) {
   try {
      data = decodeURI(data);
   }catch(e){
      if (!charset) charset = gIeTab2.getDefaultCharset();
      if (charset) {
         var uc = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
         try {
            uc.charset = charset;
            data = uc.ConvertToUnicode(unescape(data));
            data = decodeURI(data);
         }catch(e){}
         uc.Finish();
      }
   }
   return data;
}

IeTab2.prototype.convertToASCII = function(data, charset) {
   if (!charset) charset = gIeTab2.getDefaultCharset();
   if (charset) {
      var uc = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
      uc.charset = charset;
      try {
         data = uc.ConvertFromUnicode(data);
      }catch(e){
         data = uc.ConvertToUnicode(unescape(data));
         data = decodeURI(data);
         data = uc.ConvertFromUnicode(data);
      }
      uc.Finish();
   }
   return data;
}

//-----------------------------------------------------------------------------

IeTab2.prototype.getUrlDomain = function(url) {
   if (url && !gIeTab2.startsWith(url, "about:")) {
      if (/^file:\/\/.*/.test(url)) return url;
      var matches = url.match(/^([A-Za-z]+:\/+)*([^\:^\/]+):?(\d*)(\/.*)*/);
      if (matches) url = matches[1]+matches[2]+(matches[3]==""?"":":"+matches[3])+"/";
   }
   return url;
}

IeTab2.prototype.getUrlHost = function(url) {
   if (url && !gIeTab2.startsWith(url, "about:")) {
      if (/^file:\/\/.*/.test(url)) return url;
      var matches = url.match(/^([A-Za-z]+:\/+)*([^\:^\/]+):?(\d*)(\/.*)*/);
      if (matches) url = matches[2];
   }
   return url;
}

//-----------------------------------------------------------------------------

IeTab2.prototype._getAllSettings = function(prefix, isDefault) {
   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = (isDefault ? prefservice.getDefaultBranch("") : prefservice.getBranch("") );
   var preflist = prefs.getChildList(prefix, {});

   var aList = ["IETabPref"];
   for (var i = 0 ; i < preflist.length ; i++) {
      try {
         var value = null;
         switch (prefs.getPrefType(preflist[i])) {
         case prefs.PREF_BOOL:
            value = prefs.getBoolPref(preflist[i]);
            break;
         case prefs.PREF_INT:
            value = prefs.getIntPref(preflist[i]);
            break;
         case prefs.PREF_STRING:
            value = prefs.getComplexValue(preflist[i], Components.interfaces.nsISupportsString).data;
            break;
         }
         aList.push(preflist[i] + "=" + value);
      } catch (e) {}
   }
   return aList;
}

IeTab2.prototype.getAllSettings = function(isDefault) {
    return this._getAllSettings("extensions.ietab2.", isDefault);
}

IeTab2.prototype.getAllOldIETabSettings = function() {
    return this._getAllSettings("ietab.");
}

IeTab2.prototype.getAllOldIETab2Settings = function() {
    return this._getAllSettings("ietab2.");
}

IeTab2.prototype.setAllSettings = function(aList) {
   if (!aList) return;
   if (aList.length == 0) return;
   if (aList[0] != "IETabPref") return;

   var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
   var prefs = prefservice.getBranch("");

   var aPrefs = [];
   for (var i = 1 ; i < aList.length ; i++){
      var index = aList[i].indexOf("=");
      if (index > 0){
         var name = aList[i].substring(0, index);
         var value = aList[i].substring(index+1, aList[i].length);
         if (this.startsWith(name, "ietab."))
            name = "ietab2." + name.substr(6);
         if (this.startsWith(name, "ietab2."))
            name = "extensions.ietab2." + name.substr(7);
         if (this.startsWith(name, "extensions.ietab2."))
            aPrefs.push([name, value]);
      }
   }
   for (var i = 0 ; i < aPrefs.length ; i++) {
      try {
         var name = aPrefs[i][0];
         var value = aPrefs[i][1];
         switch (prefs.getPrefType(name)) {
         case prefs.PREF_BOOL:
            prefs.setBoolPref(name, /true/i.test(value));
            break;
         case prefs.PREF_INT:
            prefs.setIntPref(name, value);
            break;
         case prefs.PREF_STRING:
            if (value.indexOf('"') == 0) value = value.substring(1, value.length-1);
            var sString = Components.classes["@mozilla.org/supports-string;1"].createInstance(Components.interfaces.nsISupportsString);
            sString.data = value;
            prefs.setComplexValue(name, Components.interfaces.nsISupportsString, sString);
            break;
         }
      } catch (e) {}
   }
}

IeTab2.prototype.migrateIETab2PrefSettings = function() {
    // Per AMO review, add "extensions." prefix to ietab2 settings
    if(this.getBoolPref("extensions.ietab2.ietab2PrefsMigrated", false))
        return;
        
    var aList = this.getAllOldIETab2Settings();
    this.setAllSettings(aList);
    this.setBoolPref("extensions.ietab2.ietab2PrefsMigrated", true);
}

//-----------------------------------------------------------------------------
