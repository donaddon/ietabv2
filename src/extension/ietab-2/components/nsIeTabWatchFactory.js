/*
 * MAINTAINED BY IETAB.NET (http://www.ietab.net)
 *
 * Copyright (c) 2005 yuoo2k <yuoo2k@gmail.com>
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Updates:
 *    2/24/10 - 3/8/10:  IETAB.NET
 *       Updated to npruntime support and IE Tab 2 namespace
 *       Added maintenance information to copyright header
 */

const _IETAB2WATCH_CID = Components.ID('{5666F981-6DC7-4270-9B7F-3E064ED7B6C7}');
const _IETAB2WATCH_CONTRACTID = "@mozilla.org/ietab2watch;1";
const gIeTab2ChromeStr = "chrome://ietab2/content/reloaded.html?url=";

if(Components.utils && Components.utils.import)
    Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var IeTab2Watcher = {
   _log: function(str) {
		var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
			 getService(Components.interfaces.nsIConsoleService);

		aConsoleService.logStringMessage(str);
   },

   isIeTabURL: function(url) {
      if (!url) return false;
      return (url.indexOf(gIeTab2ChromeStr) == 0);
   },

   getIeTabURL: function(url) {
      if (this.isIeTabURL(url)) return url;
      if (/^file:\/\/.*/.test(url)) try { url = decodeURI(url).substring(8).replace(/\//g, "\\"); }catch(e){}
      return gIeTab2ChromeStr + encodeURI(url);
   },

   getBoolPref: function(prefName, defval) {
      var result = defval;
      var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
      var prefs = prefservice.getBranch("");
      if (prefs.getPrefType(prefName) == prefs.PREF_BOOL) {
          try { result = prefs.getBoolPref(prefName); }catch(e){}
      }
      return(result);
   },

   getStrPref: function(prefName, defval) {
      var result = defval;
      var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
      var prefs = prefservice.getBranch("");
      if (prefs.getPrefType(prefName) == prefs.PREF_STRING) {
          try { result = prefs.getComplexValue(prefName, Components.interfaces.nsISupportsString).data; }catch(e){}
      }
      return(result);
   },

   setStrPref: function(prefName, value) {
      var prefservice = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
      var prefs = prefservice.getBranch("");
      var sString = Components.classes["@mozilla.org/supports-string;1"].createInstance(Components.interfaces.nsISupportsString);
      sString.data = value;
      try { prefs.setComplexValue(prefName, Components.interfaces.nsISupportsString, sString); } catch(e){}
   },

   isFilterEnabled: function() {
      return (this.getBoolPref("extensions.ietab2.filter", true));
   },

   getPrefFilterList: function() {
      var s = this.getStrPref("extensions.ietab2.filterlist", null);
      return (s ? s.split(" ") : "");
   },

   setPrefFilterList: function(list) {
      this.setStrPref("extensions.ietab2.filterlist", list.join(" "));
   },

   isMatchURL: function(url, pattern) {
      if ((!pattern) || (pattern.length==0)) return false;
      var retest = /^\/(.*)\/$/.exec(pattern);
      if (retest) {
         pattern = retest[1];
      } else {
         pattern = pattern.replace(/\\/g, "/");
         var m = pattern.match(/^(.+:\/\/+[^\/]+\/)?(.*)/);
         m[1] = (m[1] ? m[1].replace(/\./g, "\\.").replace(/\?/g, "[^\\/]?").replace(/\*/g, "[^\\/]*") : "");
         m[2] = (m[2] ? m[2].replace(/\./g, "\\.").replace(/\+/g, "\\+").replace(/\?/g, "\\?").replace(/\*/g, ".*") : "");
         pattern = m[1] + m[2];
         pattern = "^" + pattern.replace(/\/$/, "\/.*") + "$";
      }
      var reg = new RegExp(pattern.toLowerCase());
      return (reg.test(url.toLowerCase()));
   },

   isMatchFilterList: function(url) {
      var aList = this.getPrefFilterList();
      for (var i=0; i<aList.length; i++) {
         var item = aList[i].split("\b");
         var rule = item[0];
         var enabled = (item.length == 1);
         if (enabled && this.isMatchURL(url, rule)) return(true);
      }
      return(false);
   },

   getTopWinBrowser: function() {
      try {
         var winMgr = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
         var topWin = winMgr.QueryInterface(Components.interfaces.nsIWindowMediator).getMostRecentWindow("navigator:browser");
         var mBrowser = topWin.document.getElementById("content");
         return mBrowser;
      } catch(e) {}
      return null;
   },

   autoSwitchFilter: function(url) {
      if (url == "about:blank") return;
      var mBrowser = this.getTopWinBrowser();
      if (!(mBrowser && mBrowser.mIeTab2SwitchURL)) return;
      if (mBrowser.mIeTab2SwitchURL == url) {
         var isMatched = false;
         var aList = this.getPrefFilterList();
         var isIE = this.isIeTabURL(url);
         if (isIE) url = decodeURI(url.substring(gIeTab2ChromeStr.length));
         for (var i=0; i<aList.length; i++) {
            var item = aList[i].split("\b");
            var rule = item[0];
            if (this.isMatchURL(url, rule)) {
               aList[i] = rule + (isIE ? "" : "\b");
               isMatched = true;
            }
         }
         if (isMatched) this.setPrefFilterList(aList);
      }
      mBrowser.mIeTab2SwitchURL = null;
   }
}

// ContentPolicy class
var IeTab2WatchFactoryClass = function() {
}

IeTab2WatchFactoryClass.prototype = {
    // this must match whatever is in chrome.manifest!
    classID: _IETAB2WATCH_CID,

   _log: function(str) {
		var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
			 getService(Components.interfaces.nsIConsoleService);

		aConsoleService.logStringMessage(str);
   },

    QueryInterface: function (iid)
    {
        if (iid.equals(Components.interfaces.nsISupports) ||
            iid.equals(Components.interfaces.nsISupportsWeakReference) ||
            iid.equals(Components.interfaces.nsIContentPolicy))
            return this;
        else
            throw Components.results.NS_ERROR_NO_INTERFACE;
    },

  shouldFilter: function(url) {
    return !IeTab2Watcher.isIeTabURL(url)
         && IeTab2Watcher.isFilterEnabled()
         && IeTab2Watcher.isMatchFilterList(url);
  },

  checkQueueReload: function(contentType, contentLocation, requestOrigin, requestingNode, mimeTypeGuess, extra) {
	// In Firefox 4, when we are called from the content handler, we aren't able to
	// load a chrome:// URL.  If we try again to filter it from a different context, then
	// it will succeed, so we queue those up for a reload.
	var stack = Components.stack;
	while(stack)
	{
		if(stack.filename && stack.filename.indexOf("nsBrowserContentHandler") != -1)
		{
			var browser = requestingNode;
			var me = this;
			var newURI = IeTab2Watcher.getIeTabURL(contentLocation.spec);
			var hwindow = Components.classes["@mozilla.org/appshell/appShellService;1"]
						   .getService(Components.interfaces.nsIAppShellService)
						   .hiddenDOMWindow;

			hwindow.setTimeout(function() {
				var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
								   .getService(Components.interfaces.nsIWindowMediator);
				var browserWindow = wm.getMostRecentWindow("navigator:browser");
				browserWindow.loadURI(newURI);
			}, 1000);
			break;
		}
		stack = stack.caller;
	}
  },

  // nsIContentPolicy interface implementation
  shouldLoad: function(contentType, contentLocation, requestOrigin, requestingNode, mimeTypeGuess, extra) {
    if (contentType == Components.interfaces.nsIContentPolicy.TYPE_DOCUMENT) {
      IeTab2Watcher.autoSwitchFilter(contentLocation.spec);
      // check IeTab FilterList
      if (this.shouldFilter(contentLocation.spec)) {
		this.checkQueueReload(contentType, contentLocation, requestOrigin, requestingNode, mimeTypeGuess, extra);
        contentLocation.spec = IeTab2Watcher.getIeTabURL(contentLocation.spec);
      }
    }
    return (Components.interfaces.nsIContentPolicy.ACCEPT);
  },
  // this is now for urls that directly load media, and meta-refreshes (before activation)
  shouldProcess: function(contentType, contentLocation, requestOrigin, requestingNode, mimeType, extra) {
    return (Components.interfaces.nsIContentPolicy.ACCEPT);
  },

  get wrappedJSObject() {
    return this;
  }
}

// Factory object
var IeTab2WatchFactoryFactory = {
  createInstance: function(outer, iid) {
    if (outer != null) throw Components.results.NS_ERROR_NO_AGGREGATION;
    return new IeTab2WatchFactoryClass();
  }
}

// Module object
var IeTab2WatchFactoryModule = {
  registerSelf: function(compMgr, fileSpec, location, type) {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(_IETAB2WATCH_CID, "IETab2 content policy", _IETAB2WATCH_CONTRACTID, fileSpec, location, type);
    var catman = Components.classes["@mozilla.org/categorymanager;1"].getService(Components.interfaces.nsICategoryManager);
    catman.addCategoryEntry("content-policy", _IETAB2WATCH_CONTRACTID, _IETAB2WATCH_CONTRACTID, true, true);
  },

  unregisterSelf: function(compMgr, fileSpec, location) {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.unregisterFactoryLocation(_IETAB2WATCH_CID, fileSpec);
    var catman = Components.classes["@mozilla.org/categorymanager;1"].getService(Components.interfaces.nsICategoryManager);
    catman.deleteCategoryEntry("content-policy", _IETAB2WATCH_CONTRACTID, true);
  },

  getClassObject: function(compMgr, cid, iid) {
    if (!cid.equals(_IETAB2WATCH_CID))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return IeTab2WatchFactoryFactory;
  },

  canUnload: function(compMgr) {
    return true;
  }
};

// THE FOLLOWING HAS BEEN DEPRECATED IN FF4
// module initialisation
function NSGetModule(comMgr, fileSpec) {
  return IeTab2WatchFactoryModule;
}


// Gecko 2 component registration
if ((typeof(XPCOMUtils) != "undefined") && XPCOMUtils.generateNSGetFactory)
	var NSGetFactory = XPCOMUtils.generateNSGetFactory([IeTab2WatchFactoryClass]);
