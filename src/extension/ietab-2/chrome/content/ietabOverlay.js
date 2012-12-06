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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
//
// Updates:
//
// Many updates have been made to this code, there is a public source code
// repository that can be viewed to see the updates:  http://code.google.com/p/ietabv2/
//
//
const gIeTab2ChromeStr = "chrome://ietab2/content/reloaded.html?url=";
const gIeTab2Version = "4.12.6.1";

IeTab2.prototype.QueryInterface = function(aIID) {
   if (aIID.equals(Components.interfaces.nsIIeTab) || aIID.equals(Components.interfaces.nsISupports))
      return gIeTab2;
   throw Components.results.NS_NOINTERFACE;
}

//
// getIeTabURL
//
// Takes the provided URL and creates a chrome://ietab/reload/url= url out of it
//
IeTab2.prototype.getIeTabURL = function(url) {
   if (gIeTab2.startsWith(url, gIeTab2ChromeStr)) return url;
   if (/^file:\/\/.*/.test(url)) try {url = decodeURI(url).substring(8).replace(/\|/g,":").replace(/\//g,"\\");}catch(e){}
   return gIeTab2ChromeStr + encodeURI(url);
}

//
// getSubjectURLFromIeTabURL
//
// Given an ietab chrome:// URL, parses the actual URL that is loaded by that IE Tab instance
//
IeTab2.prototype.getLoadedURLFromIeTabURL = function(url) {
   if (url && url.length>0) {
      url = url.replace(/^\s+/g,"").replace(/\s+$/g,"");
      if (/^file:\/\/.*/.test(url)) url = url.replace(/\|/g,":");
      if (url.indexOf(gIeTab2ChromeStr) == 0) {
         url = decodeURI(url.substring(gIeTab2ChromeStr.length));
      }
   }
   return url;
}

IeTab2.prototype.doesIeTabElmtExist = function(aTab) {
   var aBrowser = (aTab ? aTab.linkedBrowser : gBrowser);
   if (aBrowser && aBrowser.contentDocument && aBrowser.contentDocument.getElementById('IETab2')){
      var obj = aBrowser.contentDocument.getElementById('IETab2');
      return !!obj;
   }
   return false;
}

IeTab2.prototype.getIeTabElmt = function(aTab) {
   var aBrowser = (aTab ? aTab.linkedBrowser : gBrowser);
   if (aBrowser && aBrowser.currentURI && gIeTab2.startsWith(aBrowser.currentURI.spec, gIeTab2ChromeStr)) {
      if (aBrowser.contentDocument && aBrowser.contentDocument.getElementById('IETab2')){
         var obj = aBrowser.contentDocument.getElementById('IETab2');
         return (obj.wrappedJSObject ? obj.wrappedJSObject : obj);
      }
   }
   return null;
}

IeTab2.prototype.getIeTabCmdElmt = function(aTab) {
   var aBrowser = (aTab ? aTab.linkedBrowser : gBrowser);
   if (aBrowser && aBrowser.contentDocument && aBrowser.contentDocument.getElementById('IETab2')) {
       return aBrowser.contentWindow.wrappedJSObject.IETabCalls;
   }
/* Old logic
   if (aBrowser && aBrowser.currentURI && gIeTab2.startsWith(aBrowser.currentURI.spec, gIeTab2ChromeStr)) {
      if (aBrowser.contentDocument && aBrowser.contentDocument.getElementById('IETab2')) {
          return aBrowser.contentWindow.wrappedJSObject.IETabCalls;
      }
   }
*/
   return null;
}

//
// getIeTabElmtURL
//
// Get the currently loaded URL, regardless of whether it is loaded with IE Tab or not
// e.g. If the tab is using IE Tab then the URL returned would be http://www.google.com, not chrome://ietab/...
//
IeTab2.prototype.getIeTabElmtURL = function(aTab) {
   var aBrowser = (aTab ? aTab.linkedBrowser : gBrowser);
   var url = gIeTab2.getLoadedURLFromIeTabURL(aBrowser.currentURI.spec);
   var ietab = gIeTab2.getIeTabElmt(aTab);
   if (ietab && ietab.url && ietab.url != "") {
      url = (/^file:\/\/.*/.test(url) ? encodeURI(gIeTab2.convertToUTF8(ietab.url)) : ietab.url);
   }
   return url;
}

IeTab2.prototype.isIeForceable = function(url) {
   return(url && (url.length>0) &&
             ((url=="about:blank") ||
              gIeTab2.startsWith(url, 'http://') ||
              gIeTab2.startsWith(url, 'https://') ||
              gIeTab2.startsWith(url, 'file://') ||
              gIeTab2.startsWith(url, 'ftp://')
             )
         );
}

//
// isIeEngine
//
// Returns true if the currently active tab is using IE Tab
//
IeTab2.prototype.isIeEngine = function() {
   return gIeTab2.getIeTabElmt();
}

IeTab2.prototype.switchTabEngine = function(aTab, isOpenNewTab) {
   if (aTab && aTab.localName == "tab") {
      // If we have an ietab element then the url to open is the page url, not the ie url
      var url = gIeTab2.getIeTabElmtURL(aTab);
      var ietab = gIeTab2.getIeTabElmt(aTab);
      // IF we do not have an ietab element, then the url to open is the ietab url
      if (!ietab)
          url = gIeTab2.getIeTabURL(url);
      gBrowser.mIeTab2SwitchURL = url;
      if (isOpenNewTab) {
         var newTab = gBrowser.addTab(url);
         var focustab = gIeTab2.getBoolPref("extensions.ietab2.focustab", true);
         if (focustab) gBrowser.selectedTab = newTab;
      } else {
         if (aTab.linkedBrowser) aTab.linkedBrowser.loadURI(url);
      }
      gBrowser.mIeTab2SwitchURL = null;
   }
    window.setTimeout(function() {
        // Make sure the focus gets set to the right place, by:
        // 1. focus the tab.
        // 2. focus the XUL element.
        // 3. focus the control.

        gBrowser.selectedTab.focus();

        var el = gBrowser.contentDocument.getElementById("IETab2");
        if(el)
            el.focus();
        window.setTimeout(function() {
            gIeTab2.focusIeTab();
        }, 0);
    }, 0);
}

IeTab2.prototype.switchEngine = function(isOpenNewTab) {
   gIeTab2.switchTabEngine(gBrowser.mCurrentTab, isOpenNewTab);
}

IeTab2.prototype.openPrefDialog = function(url) {
   if (!url) url = gIeTab2.getIeTabElmtURL();
   var icon = document.getElementById('ietab2-status');
   window.openDialog('chrome://ietab2/content/ietabSetting.xul', null,
      'chrome,centerscreen,dependent', gIeTab2.getUrlDomain(url), icon);
}

IeTab2.prototype.loadInExtApp = function(url) {
   if (/^file:\/\/.*/.test(url)) try {url = decodeURI(url).substring(8).replace(/\//g, "\\");}catch(e){}
   url = gIeTab2.convertToASCII(url);
   var param = gIeTab2.getStrPref("extensions.ietab2.extAppParam", "%1").replace(/%1/g, url);
   var path = gIeTab2.getStrPref("extensions.ietab2.extAppPath", "");
   return IeTab2ExtApp.runApp(path, param);
}

IeTab2.prototype.viewPageInExtApp = function(aTab) {
   return gIeTab2.loadInExtApp(gIeTab2.getIeTabElmtURL(aTab));
}

IeTab2.prototype.viewLinkInExtApp = function() {
   return gIeTab2.loadInExtApp(gIeTab2.getContextLinkURL());
}

IeTab2.prototype.clickButton = function(e) {
   if (e.button == 0) {
      if (e.ctrlKey) {
         var ctrlExtApp = gIeTab2.getBoolPref("extensions.ietab2.ctrlclick", true);
         if (ctrlExtApp ? gIeTab2.viewPageInExtApp() : false) return;
      }
      gIeTab2.switchEngine(e.ctrlKey || gIeTab2.getBoolPref("extensions.ietab2.alwaysNewTab", false));
   }
   if (e.button == 1) gIeTab2.switchEngine(true);
   if (e.button == 2) gIeTab2.openPrefDialog();
   e.preventDefault();
}

IeTab2.prototype.clickStatusButton = function(e) {
   this.clickButton(e);
}

IeTab2.prototype.getContextLinkURL = function() {
   return (gContextMenu ? gContextMenu.link.toString() : null);
}

IeTab2.prototype.loadIeTab = function(url) {
   url = gIeTab2.getLoadedURLFromIeTabURL(url);
   gBrowser.loadURI(gIeTab2.getIeTabURL(url));
}

IeTab2.prototype.addIeTab = function(url) {
   url = gIeTab2.getLoadedURLFromIeTabURL(url);
   var newTab = gBrowser.addTab(gIeTab2.getIeTabURL(url));
   var focustab = gIeTab2.getBoolPref("extensions.ietab2.focustab", true);
   if (focustab) {
      gBrowser.selectedTab = newTab;
      if (gURLBar && (url == 'about:blank'))
         window.setTimeout(function(){gURLBar.focus();}, 0);
   }
}

IeTab2.prototype.ietabContextMenuPopup = function(e) {
   if (e.originalTarget != document.getElementById("contentAreaContextMenu")) return;
   if (!gContextMenu) return;

   var hide4Page = gContextMenu.isTextSelected || gContextMenu.onLink || gContextMenu.onImage || gContextMenu.onTextInput;
   var hide4Link = (!gContextMenu.onLink) || (!gIeTab2.isIeForceable(gIeTab2.getContextLinkURL())); //if link is javascript

   var internal = gIeTab2.getBoolPref("extensions.ietab2.pagelink", true);
   var external = gIeTab2.getBoolPref("extensions.ietab2.pagelink.extapp", true);
   var showicon = gIeTab2.getBoolPref("extensions.ietab2.icon.pagelink", false);

   var menuitem = null;

   //click on page
   menuitem = document.getElementById("ietab2-viewpage");
   menuitem.hidden = hide4Page || !internal;
   menuitem.setAttribute("class", (showicon?menuitem.getAttribute("iconic"):""));

   menuitem = document.getElementById("ietab2-viewpage-extapp");
   menuitem.hidden = hide4Page || !external;
   menuitem.setAttribute("class", (showicon?menuitem.getAttribute("iconic"):""));

   menuitem = document.getElementById("ietab2-viewpage-sep");
   menuitem.hidden = hide4Page || (!internal && !external);

   //click on link
   menuitem = document.getElementById("ietab2-viewlink");
   menuitem.hidden = hide4Link || !internal;
   menuitem.setAttribute("class", (showicon?menuitem.getAttribute("iconic"):""));

   menuitem = document.getElementById("ietab2-viewlink-extapp");
   menuitem.hidden = hide4Link || !external;
   menuitem.setAttribute("class", (showicon?menuitem.getAttribute("iconic"):""));
}

//
// getHandledURL
//
// Updates the URL to an IE Tab URL if the URL should be handled by IE Tab.
// This is not what handles Auto URLs, it is used to determine how to behave if IE Tab
// is the current engine, as follows:
//
// If IE Tab is the current engine, and
//   The option to "use current engine" (i.e. always stay with the current engine) is on   - OR -
//   The URL's domain matches the domain currently loaded in IE Tab.
//
// Then the URL is converted to use IE Tab, otherwise it is just trimmed
//
IeTab2.prototype.getHandledURL = function(url) {
   url = gIeTab2.trim(url);
   if ( gIeTab2.isIeEngine()
      && (!gIeTab2.startsWith(url, "about:"))
      && (!gIeTab2.startsWith(url, "view-source:"))
      ) {
      var isBlank = (gIeTab2.getLoadedURLFromIeTabURL(gBrowser.currentURI.spec)=="about:blank");
      var handleUrlBar = gIeTab2.getBoolPref("extensions.ietab2.handleUrlBar", false);
      var isSimilar = (gIeTab2.getUrlDomain(gIeTab2.getIeTabElmtURL()) == gIeTab2.getUrlDomain(url));
      if (isBlank || handleUrlBar || isSimilar) return gIeTab2.getIeTabURL(url);
   }
   return url;
}

IeTab2.prototype.updateUrlBar = function() {
   if (!gURLBar || !gIeTab2.isIeEngine()) return;
   if (gBrowser.userTypedValue) {
      if (gURLBar.selectionEnd != gURLBar.selectionStart)
         window.setTimeout(function(){gURLBar.focus();}, 0);
   } else {
      var url = gIeTab2.getIeTabElmtURL();
      if (url == "about:blank") url = "";
      if (gURLBar.value != url) gURLBar.value = url;
   }
}

IeTab2.prototype.updateToolButton = function() {
   var btn = document.getElementById("ietab2-button");
   if (btn) {
      btn.setAttribute("engine", (gIeTab2.isIeEngine()?"ie":"fx"));
   }
}

IeTab2.prototype.updateStatusIcon = function() {
   var img = document.getElementById("ietab2-status-image");
   if (img) {
      img.setAttribute("engine", (gIeTab2.isIeEngine()?"ie":"fx"));

      var show = gIeTab2.getBoolPref("extensions.ietab2.statusbar", true);
      var icon = document.getElementById('ietab2-status');
      if (icon && show) {
         icon.removeAttribute("hidden");
      }else{
         icon.setAttribute("hidden", true);
      }
   }
}

IeTab2.prototype.updateObjectDisabledStatus = function(objId, isEnabled) {
   var obj = ( typeof(objId)=="object" ? objId : document.getElementById(objId) );
   if (obj) {
      var d = obj.hasAttribute("disabled");
      if (d == isEnabled) {
         if (d) obj.removeAttribute("disabled");
         else obj.setAttribute("disabled", true);
      }
   }
}

IeTab2.prototype.updateBackForwardButtons = function() {
   try {
      var ietab = gIeTab2.getIeTabElmt();
      var canBack = (ietab ? ietab.canBack : false) || gBrowser.webNavigation.canGoBack;
      var canForward = (ietab ? ietab.canForward : false) || gBrowser.webNavigation.canGoForward;
      gIeTab2.updateObjectDisabledStatus("Browser:Back", canBack);
      gIeTab2.updateObjectDisabledStatus("Browser:Forward", canForward);
   }catch(e){}
}

IeTab2.prototype.updateStopReloadButtons = function() {
   try {
      var ietab = gIeTab2.getIeTabElmt();
      var isBlank = (gBrowser.currentURI.spec == "about:blank");
      var isLoading = gBrowser.mIsBusy;
      gIeTab2.updateObjectDisabledStatus("Browser:Reload", ietab ? ietab.canRefresh : !isBlank);
      gIeTab2.updateObjectDisabledStatus("Browser:Stop", ietab ? ietab.canStop : isLoading);
   }catch(e){}
}

IeTab2.prototype.updateGoMenuItems = function(e) {
   var goPopup = document.getElementById("goPopup");
   if (!goPopup || (e.originalTarget != goPopup)) return;
   try {
      var ietab = gIeTab2.getIeTabElmt();
      var canBack = (ietab ? ietab.canBack : false) || gBrowser.webNavigation.canGoBack;
      var canForward = (ietab ? ietab.canForward : false) || gBrowser.webNavigation.canGoForward;
      var goBack = goPopup.getElementsByAttribute("key","goBackKb");
      if (goBack) gIeTab2.updateObjectDisabledStatus(goBack[0], canBack);
      var goForward = goPopup.getElementsByAttribute("key","goForwardKb");
      if (goForward) gIeTab2.updateObjectDisabledStatus(goForward[0], canForward);
   }catch(e){}
}

IeTab2.prototype.updateEditMenuItems = function(e) {
   if (e.originalTarget != document.getElementById("menu_EditPopup")) return;
   var ietab = gIeTab2.getIeTabElmt();
   if (ietab) {
      gIeTab2.updateObjectDisabledStatus("cmd_cut", ietab.canCut);
      gIeTab2.updateObjectDisabledStatus("cmd_copy", ietab.canCopy);
      gIeTab2.updateObjectDisabledStatus("cmd_paste", ietab.canPaste);
   }
}

IeTab2.prototype.updateToolsMenuItem = function(e) {
   if (e.originalTarget != document.getElementById("menu_ToolsPopup")) return;
   var menuitem = document.getElementById("ietab2-toolsmenu");
   if (menuitem) {
      var showitem = gIeTab2.getBoolPref("extensions.ietab2.toolsmenu", true);
      var showicon = gIeTab2.getBoolPref("extensions.ietab2.toolsmenu.icon", false);
      menuitem.hidden = !showitem;
      menuitem.setAttribute("class", (showicon?menuitem.getAttribute("iconic"):""));
   }
}

IeTab2.prototype.updateSecureLockIcon = function() {
// Note:  See bug #142.  This code has been commented out because it was broken by FF 3.6 changes.
//        We can no longer call onSecurityChange at this time because this._lastStatus is null in
//        getIdentityData, causing a null-reference error due to the fact that the browser now looks
//        at the current URL instead of relying on previous state data, and the current URL is not
//        an HTTPs URL.  We will have to come up with a new, more reliable approach in a future release..
//   var ietab = gIeTab2.getIeTabElmt();
//   if (ietab) {
//      var url = ietab.url;
//      const wpl = Components.interfaces.nsIWebProgressListener;
//      var state = (gIeTab2.startsWith(url, "https://") ? wpl.STATE_IS_SECURE | wpl.STATE_SECURE_HIGH : wpl.STATE_IS_INSECURE);
//      window.XULBrowserWindow.onSecurityChange(null, null, state);
//      var securityButton = document.getElementById("security-button");
//      securityButton.setAttribute("label", gIeTab2.getUrlHost(ietab.url));
//   }
}

IeTab2.prototype.updateInterface = function() {
   gIeTab2.updateStatusIcon();
   gIeTab2.updateToolButton();
   gIeTab2.updateBackForwardButtons();
   gIeTab2.updateStopReloadButtons();
   gIeTab2.updateSecureLockIcon();
   gIeTab2.updateUrlBar();
}

IeTab2.prototype.updateAll = function() {
   if (gIeTab2.updating) return;
   try {
      gIeTab2.updating = true;
      gIeTab2.updateInterface();
   } finally {
      delete gIeTab2.updating;
   }
}

/* Not currently used

IeTab2.prototype.gotFocus = function() {
    // When the control gets the Win32 focus, we put the DOM focus on the IE Tab XUL element.
    // We need to do this because Firefox gets confused about
    // which element has the focus, preventing you from being able to re-activate an alement (like the URL bar)
    // because it thinks it already has the focus when it doesn't.

    // Only do this for FF3.x
    if(!document.activeElement)
        return;

    // Avoid looping if older FF versions end up recursing between the element and the Win32 focus
    var now = (new Date()).getTime();
    if(this.lastGotFocus)
    {
        if(now - this.lastGotFocus < 100)
            return;
    }
    this.lastGotFocus = now;

    // Go ahead and change the focus of the element
    window.setTimeout(function() {
         gBrowser.contentDocument.getElementById("IETab2").focus();
    }, 50);
}
*/

IeTab2.prototype.updateProgressStatus = function() {
   var mTabs = gBrowser.mTabContainer.childNodes;
   for(var i = 0 ; i < mTabs.length ; i++) {
      if (mTabs[i].localName == "tab") {
         var ietab = gIeTab2.getIeTabElmt(mTabs[i]);
         if (ietab) {
            var aCurTotalProgress = ietab.progress;
            if (aCurTotalProgress != mTabs[i].mProgress) {
               const ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
               const wpl = Components.interfaces.nsIWebProgressListener;
               var aMaxTotalProgress = (aCurTotalProgress == -1 ? -1 : 100);
               var aTabListener = gBrowser.mTabListeners[mTabs[i]._tPos];
               var aWebProgress = mTabs[i].linkedBrowser.webProgress;
               var aRequest = ios.newChannelFromURI(mTabs[i].linkedBrowser.currentURI);
               var aStateFlags = (aCurTotalProgress == -1 ? wpl.STATE_STOP : wpl.STATE_START) | wpl.STATE_IS_NETWORK;
               aTabListener.onStateChange(aWebProgress, aRequest, aStateFlags, 0);
               aTabListener.onProgressChange(aWebProgress, aRequest, 0, 0, aCurTotalProgress, aMaxTotalProgress);
               mTabs[i].mProgress = aCurTotalProgress;
            }
         }
      }
   }
}

IeTab2.prototype.onProgressChange = function(progress) {
   if (progress==0) gBrowser.userTypedValue = null;
   gIeTab2.updateProgressStatus();
   gIeTab2.updateAll();
}

IeTab2.prototype.onSecurityChange = function(security) {
   gIeTab2.updateSecureLockIcon();
}

IeTab2.prototype.goDoCommand = function(cmd) {
   try {
      if(gIeTab2.forceDefaultCmd)
         return false;

      if(!gIeTab2.doesIeTabElmtExist())
         return false;

      switch(cmd) {
          case "goBack":
          case "goForward":
          case "stop":
          case "refresh":
          case "saveAs":
          case "print":
          case "printSetup":
          case "printPreview":
          case "viewSource":
          case "find":
          case "cmd_cut":
          case "cmd_copy":
          case "cmd_paste":
          case "cmd_selectAll":
          case "displaySecurityInfo":
              window.setTimeout(function() {
                  gIeTab2.delayedGoDoCommand(cmd);
              }, 100);
             return true;
      }
  }
  catch(ex) {
  }
  return false;
}

IeTab2.prototype.delayedGoDoCommand = function(cmd) {
   try {
      var ietabProps = gIeTab2.getIeTabElmt();
      var ietab = gIeTab2.getIeTabCmdElmt();

      switch (cmd) {
      case "goBack":
         if (!ietab || !ietabProps.canBack)
         {
            // We had to check this asynchronously to avoid a deadlock, re-invoke the browser back command
            // ignoring our own operation this time.
            gIeTab2.forceDefaultCmd = true;
            BrowserBack();
            gIeTab2.forceDefaultCmd = false;
         }
         else
         {
            ietab.goBack();
         }
         break;
      case "goForward":
         if (!ietab || !ietabProps.canForward)
         {
            // We had to check this asynchronously to avoid a deadlock, re-invoke the browser back command
            // ignoring our own operation this time.
            gIeTab2.forceDefaultCmd = true;
            BrowserForward();
            gIeTab2.forceDefaultCmd = false;
         }
         else
         {
            ietab.goForward();
         }
         break;
      case "stop":
         ietab.stop();
         break;
      case "refresh":
         ietab.refresh();
         break;
      case "saveAs":
         ietab.saveAs();
         break;
      case "print":
         ietab.print();
         break;
      case "printSetup":
         ietab.printSetup();
         break;
      case "printPreview":
         ietab.printPreview();
         break;
      case "viewSource":
         ietab.viewSource();
         break;
      case "find":
         ietab.find();
         break;
      case "cmd_cut":
         ietab.cut();
         break;
      case "cmd_copy":
         ietab.copy();
         break;
      case "cmd_paste":
         ietab.paste();
         break;
      case "cmd_selectAll":
         ietab.selectAll();
         break;
      }
   } finally {
      window.setTimeout(function() {
         gIeTab2.updateAll();
      }, 0);
   }
}

IeTab2.prototype.addBookmarkMenuitem = function(e) {
   var popupMenu = e.originalTarget;
   if (popupMenu.id != "placesContext") return;

   var miInt = document.getElementById("ietab2-bookmark");
   var miExt = document.getElementById("ietab2-bookmark-extapp");

   var popupNode = document.popupNode;
   // Use _placesNode in Fx 4+
   var bmNode = popupNode.node || popupNode._placesNode;
   var isBookmark = bmNode && PlacesUtils.nodeIsBookmark(bmNode);
   var isShowIcon = gIeTab2.getBoolPref("extensions.ietab2.icon.bookmark", false);

   miInt.hidden = !isBookmark || !gIeTab2.getBoolPref("extensions.ietab2.bookmark", true);
   miExt.hidden = !isBookmark || !gIeTab2.getBoolPref("extensions.ietab2.bookmark.extapp", true);
   if (!miInt.hidden) {
       miInt.addEventListener('command', function() {
           gIeTab2.addIeTab(bmNode.uri);
       }, false);
       miInt.setAttribute("class", (isShowIcon?miInt.getAttribute("iconic"):""));
   }
   if (!miExt.hidden) {
       miExt.addEventListener('command', function() {
           gIeTab2.loadInExtApp(bmNode.uri);
       }, false);
       miExt.setAttribute("class", (isShowIcon?miExt.getAttribute("iconic"):""));
   }
}

function removeTab(tab) {
    window.setTimeout(function() {gBrowser.removeTab(tab)}, 0);
}

IeTab2.prototype.closeIeTab = function() {
   var mTabs = gBrowser.mTabContainer.childNodes;
   for(var i = mTabs.length-1 ; i>=0 ; i--) {
      if (mTabs[i].localName == "tab") {

         var ietab = gIeTab2.getIeTabElmt(mTabs[i]);
         if (ietab && (ietab.canClose))
         {
            removeTab(mTabs[i]);
         }
      }
   }
}

IeTab2.prototype.getContextTab = function() {
   return  (gBrowser && gBrowser.mContextTab && (gBrowser.mContextTab.localName == "tab") ? gBrowser.mContextTab : null);
}

IeTab2.prototype.viewLink = function(e) {
   if (!gContextMenu) return;
   var url = gIeTab2.getContextLinkURL();

   switch (e.button) {
   case 1:
      var menu = e.originalTarget;
      while (menu) {
         if (menu.localName == "menupopup") break;
         if (menu.localName == "popup") break;
         menu = menu.parentNode;
      }
      if (menu) menu.hidePopup();
   case 0:
      if (e.ctrlKey) {
         var ctrlExtApp = gIeTab2.getBoolPref("extensions.ietab2.ctrlclick", true);
         if (ctrlExtApp ? gIeTab2.loadInExtApp(url) : false) return;
      }
      gIeTab2.addIeTab(url);
      break;
   case 2:
      gIeTab2.openPrefDialog(url);
      break;
   }
}

IeTab2.prototype.viewPage = function(e) {
   var aTab = null;
   switch (e.originalTarget.id) {
   case "ietab2-viewpage":
      aTab = gBrowser.mCurrentTab;
      break;
   case "ietab2-tabbar-switch":
      aTab = gIeTab2.getContextTab();
      break;
   }
   if (!aTab) return;

   switch (e.button) {
   case 0:
      if (e.ctrlKey) {
         var ctrlExtApp = gIeTab2.getBoolPref("extensions.ietab2.ctrlclick", true);
         if (ctrlExtApp ? gIeTab2.viewPageInExtApp(aTab) : false) return;
      }
      gIeTab2.switchTabEngine(aTab, e.ctrlKey || gIeTab2.getBoolPref("extensions.ietab2.alwaysNewTab", false));
      break;
   case 1:
      var menu = e.originalTarget;
      while (menu) {
         if (menu.localName == "menupopup") break;
         if (menu.localName == "popup") break;
         menu = menu.parentNode;
      }
      if (menu) menu.hidePopup();
      gIeTab2.switchTabEngine(aTab, true);
      break;
   case 2:
      gIeTab2.openPrefDialog(gIeTab2.getIeTabElmtURL(aTab));
      break;
   }
}

IeTab2.prototype.updateTabbarMenu = function(e) {
   if (e.originalTarget != gBrowser.mStrip.firstChild.nextSibling) return;

   var aTab = gIeTab2.getContextTab();
   var hide = (aTab == null);

   var internal = gIeTab2.getBoolPref("extensions.ietab2.tabsmenu", true);
   var external = gIeTab2.getBoolPref("extensions.ietab2.tabsmenu.extapp", true);
   var showicon = gIeTab2.getBoolPref("extensions.ietab2.icon.tabsmenu", false);

   var menuitem = null;

   //switch
   menuitem = document.getElementById("ietab2-tabbar-switch");
   menuitem.hidden = hide || !internal;
   menuitem.setAttribute("class", (showicon?menuitem.getAttribute("iconic"):""));

   //extapp
   menuitem = document.getElementById("ietab2-tabbar-extapp");
   menuitem.hidden = hide || !external;
   menuitem.setAttribute("class", (showicon?menuitem.getAttribute("iconic"):""));

   //sep
   menuitem = document.getElementById("ietab2-tabbar-sep");
   menuitem.hidden = hide || (!internal && !external);

   if (aTab) {
      var ietab = gIeTab2.getIeTabElmt(aTab);
      document.getElementById("ietab2-tabbar-switch").setAttribute("engine", (ietab ? "ie" : "fx"));
   }
}

IeTab2.prototype.createTabbarMenu = function() {
   var tabbarMenu = gBrowser.mStrip.firstChild.nextSibling;
   var menuitems = tabbarMenu.childNodes;
   var separator = null;
   for(var i=0, c=0 ; i < menuitems.length-1 ; i++) {
      if (menuitems[i].localName=="menuseparator")
         if (++c==2) {separator=menuitems[i];break;}
   }
   tabbarMenu.insertBefore(document.getElementById("ietab2-tabbar-sep"), separator);
   tabbarMenu.insertBefore(document.getElementById("ietab2-tabbar-switch"), separator);
   tabbarMenu.insertBefore(document.getElementById("ietab2-tabbar-extapp"), separator);
}

IeTab2.prototype.getTitleEnding = function(oldModifier) {
   var ietab = gIeTab2.getIeTabElmt();
   if (ietab) {
      var titleEnding = gIeTab2.getStrPref("extensions.ietab2.titleEnding", "");
      if (titleEnding != "") return titleEnding;
   }
   return oldModifier;
}

IeTab2.prototype.focusIeTab = function() {
   var ietab = gIeTab2.getIeTabCmdElmt();
   if (ietab) ietab.focus();
}

IeTab2.prototype.onTabSelected = function(e) {
   if ((e.originalTarget.localName == "tabs") || (e.originalTarget.localName == "tabpanels")) {
      gIeTab2.updateAll();
   }
}

IeTab2.prototype.assignJSObject = function(aDoc) {
   if (aDoc instanceof HTMLDocument) {
      var aBrowser = getBrowser().getBrowserForDocument(aDoc);
      if (aBrowser && aBrowser.currentURI && aBrowser.currentURI.spec.indexOf(gIeTab2ChromeStr) == 0) {
         if (aDoc && aDoc.getElementById('IETab2')) {
            var ietab = aDoc.getElementById('IETab2');
            if (ietab.wrappedJSObject) ietab = ietab.wrappedJSObject;
            ietab.requestTarget = gIeTab2;
         }
      }
   }
}

/**
 * Reviewer Note: Many users are unfamiliar with toolbar customization, so we provide a
 *          one click opt-in solution for installing the button on a toolbar, using
 *          the technique documented at developers.mozilla.org
 *
 * Installs the toolbar button with the given ID into the given
 * toolbar, if it is not already present in the document.
 *
 * @param {string} toolbarId The ID of the toolbar to install to.
 * @param {string} id The ID of the button to install.
 * @param {string} afterId The ID of the element to insert after. @optional
 */
IeTab2.prototype.installButton = function(toolbarId, id, afterId) {
    if (!document.getElementById(id)) {
        var toolbar = document.getElementById(toolbarId);
        // Can't do anything if the toolbar doesn't exist
        if (!toolbarId)
            return;

        // If no afterId is given, then append the item to the toolbar
        var before = null;
        if (afterId) {
            var elem = document.getElementById(afterId);
            if (elem && elem.parentNode == toolbar)
                before = elem.nextElementSibling;
        }

        toolbar.insertItem(id, before);
        toolbar.setAttribute("currentset", toolbar.currentSet);
        document.persist(toolbar.id, "currentset");

        if (toolbarId == "addon-bar")
            toolbar.collapsed = false;
    }
}

IeTab2.prototype.onPageShowOrLoad = function(e) {
   window.setTimeout(function() {gIeTab2.assignJSObject(e.target);}, 0);
   gIeTab2.updateAll();
}

IeTab2.prototype.getCurrentIeTabURI = function(aBrowser) {
   try {
      var docShell = aBrowser.boxObject.QueryInterface(Components.interfaces.nsIBrowserBoxObject).docShell;
      var wNav = docShell.QueryInterface(Components.interfaces.nsIWebNavigation);
      if (wNav.currentURI && wNav.currentURI.spec.indexOf(gIeTab2ChromeStr) == 0) {
         var ietab = wNav.document.getElementById("IETab2");
         if (ietab) {
            if (ietab.wrappedJSObject) ietab = ietab.wrappedJSObject;
            var url = ietab.url;
            if (url) {
               const ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
               return ios.newURI(gIeTab2ChromeStr + encodeURI(url), null, null);
            }
         }
      }
   } catch(e) {}
   return null;
}

IeTab2.prototype.hookBrowserGetter = function(aBrowser) {
   if (aBrowser.localName != "browser") aBrowser = aBrowser.getElementsByTagNameNS(kXULNS, "browser")[0];
   // hook aBrowser.currentURI
   gIeTab2.hookProp(aBrowser, "currentURI", function(oldGetter) {
      var uri = gIeTab2.getCurrentIeTabURI(this);
      if (uri)
          return uri;
      else
          return oldGetter.call(this);
   });
   // hook aBrowser.sessionHistory
   gIeTab2.hookProp(aBrowser, "sessionHistory", function(oldGetter) {
      var history = this.webNavigation.sessionHistory;
      var uri = gIeTab2.getCurrentIeTabURI(this);
      if (uri) {
         var entry = history.getEntryAtIndex(history.index, false);
         if (entry.URI.spec != uri.spec) {
            entry.QueryInterface(Components.interfaces.nsISHEntry).setURI(uri);
            if (this.parentNode.__SS_data) delete this.parentNode.__SS_data;
         }
      }
      return oldGetter.call(this);
   });
}

IeTab2.prototype.hookURLBarSetter = function(aURLBar) {
   if (!aURLBar) aURLBar = document.getElementById("urlbar");
   if (!aURLBar) return;

   gIeTab2.hookProp(aURLBar, "value", null, function(oldSetter, value) {
      this.isModeIE = value && (value.indexOf(gIeTab2ChromeStr) == 0);
      if (this.isModeIE) {
         value = decodeURI(value.substring(gIeTab2ChromeStr.length));
         if (value == "about:blank") value = "";
      }
      oldSetter.call(this, value);
   });
}

IeTab2.prototype.log = function(str) {
	var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
		 getService(Components.interfaces.nsIConsoleService);

	aConsoleService.logStringMessage(str);
}

IeTab2.prototype.checkFilter = function(aBrowser, aRequest, aLocation) {

   var ietabwatch = Components.classes["@mozilla.org/ietab2watch;1"].getService().wrappedJSObject;
   if (ietabwatch && ietabwatch.shouldFilter(aLocation.spec)) {
      aRequest.cancel(0x804b0002); //NS_BINDING_ABORTED
      aBrowser.loadURI(aLocation.spec);
   }
}

IeTab2.prototype.safeMakeURI = function(uri, alternative) {
   if(uri) return makeURI(uri);
   return alternative;
}

/* ================================================
// DEPRECATED / OBSOLETE HOOK CODE
//
// 11/20/12 - Moving away from the string-based function customization.
//
// From what I can tell the ones in this comment block have all either been broken for a long time and are probably not necessary
// or they are no longer appropriate with current versions of Fx.  Keep these comments here for a few releases in case we find we overlooked some
// functionality

// FIRST, THE HOOK FUNCTION

// IeTab2.prototype.hookCode = function(orgFunc, orgCode, myCode) {
//    try{ if (eval(orgFunc).toString() == eval(orgFunc + "=" + eval(orgFunc).toString().replace(orgCode, myCode))) throw orgFunc; }
//   catch(e){ Components.utils.reportError("Failed to hook function: "+orgFunc); }
// }


   // The following one disabled typeAhead when IE was installed, it's a questionable feature to begin with.
   // gIeTab2.hookCode("gFindBar._onBrowserKeypress", "this._useTypeAheadFind &&", "$& !gIeTab2.isIeEngine() &&");

   // The following converted chrome://ietab2/url=http%25... links to the final target link when a bookmark is created.  We _could_ do this
   // using nsINavBookmarkObserver::onItemAdded and change the value on save.  But again, I don't think this functionality is really necessary, if the
   // URL was opened in IE Tab wen it was bookmarked, what's wrong with bookmarking it to open in IE Tab again?
   // gIeTab2.hookCode("PlacesCommandHook.bookmarkPage", "aBrowser.currentURI", "gIeTab2.safeMakeURI(gIeTab2.getLoadedURLFromIeTabURL($&.spec), getBrowser().currentURI)");

   // This was commented out long ago, _apparently_ no longer needed
   // gIeTab2.hookCode("gBrowser.updateTitlebar", 'docElement.getAttribute("titlemodifier")', 'gIeTab2.getTitleEnding($&)');

   // Not sure what the following did, but URLBarSetURI has not used getWebNavigation() for a while, so it is effectively a NOP
   // gIeTab2.hookCode("URLBarSetURI", "getWebNavigation()", "getBrowser()");

   // 11/20/12:  The following hook is not working in Fx17.  It was trying to hook OnLocationChange, but nsIWebProgressListener now has an aFlags fourth parameter so the hook
   //        wouldn't have worked.  According to the MDN docs, this was added in Gecko 11 / Fx 11, so this has been broken since 3/13/12.
   //        Investigate / test whether there are auto URL scenarios that don't work due to this breakage.
   // gIeTab2.hookCode('gBrowser.mTabProgressListener', "function (aWebProgress, aRequest, aLocation) {", "$& gIeTab2.checkFilter(this.mBrowser, aRequest, aLocation);");

   // ================================================
   // The following two have been broken since FX4
       // This is the hook for the star button, if we decide to keep pretty-izing the URLs, then we may need to re-add this
       // gIeTab2.hookCode("PlacesStarButton.updateState", "getBrowser().currentURI", "gIeTab2.safeMakeURI(gIeTab2.getLoadedURLFromIeTabURL($&.spec), getBrowser().currentURI)");

       // This has been broken since Fx 4.  It was influencing OpenURI by making sure it was called with LOAD_FLAGS_NONE instead of
       // LOAD_FLAGS_FROM_EXTERNAL.  The exact impact of that is not clear to me, it seems to impact userTypedClear, but I don't see that used in the
       // source code.  So we're leaving this one out for now.'
       // gIeTab2.hookCode("nsBrowserAccess.prototype.openURI", "var loadflags = isExternal ?", "var loadflags = false ?");
   // ================================================
// The following functionality has been replaced by a content policy handler
//for(var i=0 ; i<gBrowser.mTabListeners.length ; i++)
 //  gIeTab2.hookCode("gBrowser.mTabListeners["+i+"].onLocationChange", /{/, "$& gIeTab2.checkFilter(this.mBrowser, aRequest, aLocation);");

// END DEPRECATED / OBSOLETE HOOKS
// ================================================
*/

IeTab2.prototype.hookCommands = function() {
   // General command functions that may be handled by IE Tab
   function commandHook(oldFunction, ieTabCmd) {
       if (typeof(oldFunction) === 'undefined') {
           Components.utils.reportError("Failed to apply hook: " + ieTabCmd);
           return null;
       }

       function newFunction() {
           if (gIeTab2.goDoCommand(ieTabCmd))
               return;
           return oldFunction.apply(this, arguments);
       }
       return newFunction;
   }

   window.BrowserBack = commandHook(window.BrowserBack, 'goBack');
   window.BrowserForward = commandHook(window.BrowserForward, 'goForward');
   window.BrowserStop = commandHook(window.BrowserStop, 'stop');
   window.BrowserReload = commandHook(window.BrowserReload, 'refresh');
   window.BrowserReloadSkipCache = commandHook(window.BrowserReloadSkipCache, 'refresh');
   window.saveDocument = commandHook(window.saveDocument, 'saveAs');
   window.BrowserViewSourceOfDocument = commandHook(window.BrowserViewSourceOfDocument, 'viewSource');
   window.displaySecurityInfo = commandHook(window.displaySecurityInfo, 'displaySecurityInfo');

   if (window.PrintUtils) {
       window.PrintUtils.print = commandHook(window.PrintUtils.print, 'print');
       window.PrintUtils.showPageSetup = commandHook(window.PrintUtils.showPageSetup, 'printSetup');
       window.PrintUtils.printPreview = commandHook(window.PrintUtils.printPreview, 'printPreview');
   }
}

//
// hookCodeAll
//
// Hooks into browser functionality.  The bulk of the hooks are simple redirect command hooks that
// are set up by hookCommands function.
//
// The more complex ones are addTab, setTabTitle, and getShortcutOrURI, and their purpose is
// to force the URL bar to display the loaded URL instead of the IE Tab container URL when IE Tab
// is in use, and for other operations to use the loaded URL instead of the chrome:// IE Tab URL.
//
// NOTE:  These functions do NOT handle the use of auto urls / site filters.  That is all handled
//        by content policy in the IE Tab Watcher component.
//
IeTab2.prototype.hookCodeAll = function() {
    //hook properties
   gIeTab2.hookBrowserGetter(gBrowser.mTabContainer.firstChild.linkedBrowser);
   gIeTab2.hookURLBarSetter(gURLBar);

   // Old hook code followed by new hook code
   // gIeTab2.hookCode("gBrowser.addTab", "return t;", "gIeTab2.hookBrowserGetter(t.linkedBrowser); $&");
   if (gBrowser.addTab) {
       var oldAddTab = gBrowser.addTab;
       gBrowser.addTab = function() {
           var tab = oldAddTab.apply(this, arguments);
           gIeTab2.hookBrowserGetter(tab.linkedBrowser);
           return tab;
       }
   } else {
       Components.utils.reportError("Failed to apply addTab hook");
   }

   // Old code for updating the title followed by new implementation that doesn't use toString substitution
   // gIeTab2.hookCode("gBrowser.setTabTitle", "if (browser.currentURI.spec) {", "$& if (browser.currentURI.spec.indexOf(gIeTab2ChromeStr) == 0) return;");
   if (gBrowser.setTabTitle) {
       var oldSetTabTitle = gBrowser.setTabTitle;
       gBrowser.setTabTitle = function(aTab) {
           var browser = this.getBrowserForTab(aTab);
           var title = browser.contentTitle;
           if (!title) {
               if (browser.currentURI.spec && browser.currentURI.spec.indexOf(gIeTab2ChromeStr) == 0)
                   return false;
           }
           return oldSetTabTitle.apply(this, arguments);
       }
   } else {
       Components.utils.reportError("Failed to apply setTabtitle hook");
   }

   // Intercept general URI requests
   // gIeTab2.hookCode("getShortcutOrURI", /return (\S+);/g, "return gIeTab2.getHandledURL($1);");
   if (window.getShortcutOrURI) {
       var oldGetShortcutOrURI = getShortcutOrURI;
       getShortcutOrURI = function() {
           var result = oldGetShortcutOrURI.apply(this, arguments);
           return gIeTab2.getHandledURL(result);
       }
   } else {
       Components.utils.reportError("Failed to apply getShortcutOrURI hook");
   }

   // The functionality below replaces this old hook:
   //   if (gURLBar.handleCommand) gIeTab2.hookCode("gURLBar.handleCommand", "this.value = url;", "url = gIeTab2.getHandledURL(url); $&"); //fx3.1
    if (gURLBar.handleCommand) {
       var oldHandleCommand = gURLBar.handleCommand;
       gURLBar.handleCommand = function() {
           if (this.value) {
               this.value = gIeTab2.getHandledURL(this.value);
           }
           return oldHandleCommand.apply(this, arguments);
       }
   }
   else {
        // The following was for Fx 3.0, and we are just going to ignore this functionality for FX versions that old
        // gIeTab2.hookCode("BrowserLoadURL", "url = gURLBar.value;", "url = gIeTab2.getHandledURL(gURLBar.value);");//fx3.0
   }

   // Modify the URL sent by the "EMail link" menu option
   if (window.MailIntegration && window.MailIntegration.sendMessage) {
       var oldSendMessage = window.MailIntegration.sendMessage;
       window.MailIntegration.sendMessage = function() {
           var ietab = gIeTab2.getIeTabElmt();
           if (ietab) {
               arguments[0] = ietab.url;
               arguments[1] = ietab.title;
           }
           oldSendMessage.apply(this, arguments);
       }
   } else {
       Components.utils.reportError("Failed to apply sendMessage hook");
   }

   if (window.goDoCommand) {
       var oldDoCommand = window.goDoCommand;
       window.goDoCommand = function() {
           if (gIeTab2.goDoCommand(arguments[0]))
               return;
           return oldDoCommand.apply(this, arguments);
       }
   } else {
       Components.utils.reportError("Failed to apply goDoCommand hook");
   }

   gIeTab2.hookCommands();

   gIeTab2.hookAttr("cmd_find", "oncommand", "if(gIeTab2.goDoCommand('find')) return;");
   gIeTab2.hookAttr("cmd_findAgain", "oncommand", "if(gIeTab2.goDoCommand('find')) return;");
   gIeTab2.hookAttr("cmd_findPrevious", "oncommand", "if(gIeTab2.goDoCommand('find')) return;");
}

IeTab2.prototype.addEventAll = function() {
   gIeTab2.addEventListener(window, "DOMContentLoaded", gIeTab2.onPageShowOrLoad);
   gIeTab2.addEventListener(window, "pageshow", gIeTab2.onPageShowOrLoad);

   gIeTab2.addEventListener(gBrowser.mStrip.firstChild.nextSibling, "popupshowing", gIeTab2.updateTabbarMenu);
   gIeTab2.addEventListener("appcontent", "select", gIeTab2.onTabSelected);

   gIeTab2.addEventListener("goPopup", "popupshowing", gIeTab2.updateGoMenuItems);
   gIeTab2.addEventListener("placesContext", "popupshowing", gIeTab2.addBookmarkMenuitem);
   gIeTab2.addEventListener("menu_EditPopup", "popupshowing", gIeTab2.updateEditMenuItems);
   gIeTab2.addEventListener("menu_ToolsPopup", "popupshowing", gIeTab2.updateToolsMenuItem);
   gIeTab2.addEventListener("contentAreaContextMenu", "popupshowing", gIeTab2.ietabContextMenuPopup);
}

IeTab2.prototype.removeEventAll = function() {
   gIeTab2.removeEventListener(window, "DOMContentLoaded", gIeTab2.onPageShowOrLoad);
   gIeTab2.removeEventListener(window, "pageshow", gIeTab2.onPageShowOrLoad);

   gIeTab2.removeEventListener(gBrowser.mStrip.firstChild.nextSibling, "popupshowing", gIeTab2.updateTabbarMenu);
   gIeTab2.removeEventListener("appcontent", "select", gIeTab2.onTabSelected);

   gIeTab2.removeEventListener("goPopup", "popupshowing", gIeTab2.updateGoMenuItems);
   gIeTab2.removeEventListener("placesContext", "popupshowing", gIeTab2.addBookmarkMenuitem);
   gIeTab2.removeEventListener("menu_EditPopup", "popupshowing", gIeTab2.updateEditMenuItems);
   gIeTab2.removeEventListener("menu_ToolsPopup", "popupshowing", gIeTab2.updateToolsMenuItem);
   gIeTab2.removeEventListener("contentAreaContextMenu", "popupshowing", gIeTab2.ietabContextMenuPopup);

   gIeTab2.removeEventListener(window, "load", gIeTab2.init);
   gIeTab2.removeEventListener(window, "unload", gIeTab2.destroy);
}

IeTab2.prototype.checkShowIntro = function() {
    if(this.getBoolPref("extensions.ietab2.hasRun", false))
    {
        var version = this.getStrPref("extensions.ietab2.version", "");
        this.setStrPref("extensions.ietab2.version", gIeTab2Version);
        if(version != gIeTab2Version)
        {
            window.setTimeout(function() {
                var url = "http://www.ietab.net/ie-tab-2-updated-whats-new?";
                url += "oldver=" + encodeURIComponent(version);
                url += "&newVer=" + encodeURIComponent(gIeTab2Version);
                gBrowser.selectedTab = gBrowser.addTab(url);
            }, 100);
        }
        return;
    }

    this.setBoolPref("extensions.ietab2.hasRun", true);
    window.setTimeout(function() {
        gBrowser.selectedTab = gBrowser.addTab("http://www.ietab.net/thanks-installing-ie-tab-2-0");
    }, 100);
}

IeTab2.prototype.init = function() {
    gIeTab2.ffversion = 3.0;
    if (/Firefox[\/\s](\d+\.\d+)/.test(navigator.userAgent))
    {
        gIeTab2.ffversion = new Number(RegExp.$1) // capture x.x portion and store as a number
    }

   gIeTab2.migrateIETab2PrefSettings();
   gIeTab2.hookCodeAll();
   gIeTab2.addEventAll();
   gIeTab2.createTabbarMenu();
   gIeTab2.checkShowIntro();

   // Set the plugin to run in the desired process mode
   var runInProcess = this.getBoolPref("extensions.ietab2.runinprocess", false);
   gIeTab2.setBoolPref("dom.ipc.plugins.enabled.npietab2.dll", !runInProcess);

   // Workaround for the Firefox 6 glass / theme bug
   var elAppContent = document.getElementById("appcontent");
   if(elAppContent)
   {
	   var style = elAppContent.getAttribute("style");
	   if(!style)
		   style = "";

	   if(style.indexOf("-moz-win-exclude-glass") == -1)
		   elAppContent.setAttribute("style", style + ";-moz-appearance:-moz-win-exclude-glass");
   }
}

IeTab2.prototype.destroy = function() {
   gIeTab2.removeEventAll();
   delete gIeTab2;
}


var gIeTab2 = new IeTab2();

gIeTab2.addEventListener(window, "load", gIeTab2.init);
gIeTab2.addEventListener(window, "unload", gIeTab2.destroy);

/*
function watchAllCalls() {
	for(var fn in gIeTab2)
	{
		if(typeof(fn) == "function");
		{
			if(fn.indexOf("log") != -1)
				continue;
			gIeTab2.log("toString = " + fn.toString() + ", eval = " + eval("gIeTab2." + fn).toString());
			var oldfn = eval("gIeTab2." + fn).toString();
			var newfn = oldfn.replace("{", "{ gIeTab2.log('" + fn + " called'); ");
			eval("gIeTab2." + fn + " = " + newfn);
		}
	}
}

watchAllCalls();
*/
