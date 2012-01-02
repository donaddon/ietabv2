//
// Updates:
//
// 2/24/10: ietab.net -- Converted to new ietab2 namespace.
// 3/9/10:  ietab.net -- Support import of settings form ietab
//
function IeTab2() {}

IeTab2.prototype = {
   IExploreExePath: ""
}

IeTab2.prototype.getPrefFilterList = function() {
   var s = this.getStrPref("extensions.ietab2.filterlist", null);
   return (s ? s.split(" ") : "");
}

IeTab2.prototype.addFilterRule = function(rule, enabled) {
   var rules = document.getElementById('filterChilds');
   var item = document.createElement('treeitem');
   var row = document.createElement('treerow');
   var c1 = document.createElement('treecell');
   var c2 = document.createElement('treecell');
   c1.setAttribute('label', rule);
   c2.setAttribute('value', enabled);
   row.appendChild(c1);
   row.appendChild(c2);
   item.appendChild(row);
   rules.appendChild(item);
   return (rules.childNodes.length-1);
}

IeTab2.prototype.initDialog = function() {
   this.checkGetIETab1Settings();
   
   //get iexplore.exe path
   this.IExploreExePath = IeTab2ExtApp.getIExploreExePath();

   //filter
   document.getElementById('filtercbx').checked = this.getBoolPref("extensions.ietab2.filter", true);
   var list = this.getPrefFilterList();
   var rules = document.getElementById('filterChilds');
   while (rules.hasChildNodes()) rules.removeChild(rules.firstChild);
   for (var i = 0; i < list.length; i++) {
      if (list[i] != "") {
         var item = list[i].split("\b");
         var rule = item[0];
         if (!/^\/(.*)\/$/.exec(rule)) rule = rule.replace(/\/$/, "/*");
         var enabled = (item.length == 1);
         this.addFilterRule(rule, enabled);
      }
   }
   //general
   document.getElementById('toolsmenu').checked = this.getBoolPref("extensions.ietab2.toolsmenu", true);
   document.getElementById('toolsmenu.icon').checked = this.getBoolPref("extensions.ietab2.toolsmenu.icon", false);
   document.getElementById('statusbar').checked = this.getBoolPref("extensions.ietab2.statusbar", true);
   document.getElementById('handleurl').checked = this.getBoolPref("extensions.ietab2.handleUrlBar", false);
   document.getElementById('runinprocess').checked = this.getBoolPref("extensions.ietab2.runinprocess", false);
   document.getElementById('alwaysnew').checked = this.getBoolPref("extensions.ietab2.alwaysNewTab", false);
   document.getElementById('focustab').checked  = this.getBoolPref("extensions.ietab2.focustab", true);

   //context
   document.getElementById('pagelink.embed').checked = this.getBoolPref("extensions.ietab2.pagelink", true);
   document.getElementById('tabsmenu.embed').checked = this.getBoolPref("extensions.ietab2.tabsmenu", true);
   document.getElementById('bookmark.embed').checked = this.getBoolPref("extensions.ietab2.bookmark", true);

   document.getElementById('pagelink.extapp').checked = this.getBoolPref("extensions.ietab2.pagelink.extapp", true);
   document.getElementById('tabsmenu.extapp').checked = this.getBoolPref("extensions.ietab2.tabsmenu.extapp", true);
   document.getElementById('bookmark.extapp').checked = this.getBoolPref("extensions.ietab2.bookmark.extapp", true);

   document.getElementById('pagelink.icon').checked = this.getBoolPref("extensions.ietab2.icon.pagelink", false);
   document.getElementById('tabsmenu.icon').checked = this.getBoolPref("extensions.ietab2.icon.tabsmenu", false);
   document.getElementById('bookmark.icon').checked = this.getBoolPref("extensions.ietab2.icon.bookmark", false);

   //external
   var path = this.getStrPref("extensions.ietab2.extAppPath", "");
   document.getElementById('pathbox').value = (path == "" ? this.IExploreExePath : path);
   document.getElementById('parambox').value = this.getStrPref("extensions.ietab2.extAppParam", "%1");
   document.getElementById('ctrlclick').checked = this.getBoolPref("extensions.ietab2.ctrlclick", true);

   //fill urlbox
   var newurl = (window.arguments ? window.arguments[0] : ""); //get CurrentTab's URL
   document.getElementById('urlbox').value = ( this.startsWith(newurl,"about:") ? "" : newurl);
   document.getElementById('urlbox').select();

   //updateStatus
   this.updateDialogPositions();
   this.updateDialogAllStatus();
   this.updateApplyButton(false);
   
   //compatibility mode
   var mode = this.getStrPref("extensions.ietab2.compatMode");
   document.getElementById("iemode").selectedItem = document.getElementById(mode);
}

IeTab2.prototype.updateApplyButton = function(e) {
   document.getElementById("myExtra1").disabled = !e;
}

IeTab2.prototype.init = function() {
   this.initDialog();
   this.addEventListenerByTagName("checkbox", "command", this.updateApplyButton);
   this.addEventListener("filterChilds", "DOMAttrModified", this.updateApplyButton);
   this.addEventListener("filterChilds", "DOMNodeInserted", this.updateApplyButton);
   this.addEventListener("filterChilds", "DOMNodeRemoved", this.updateApplyButton);
   this.addEventListener("parambox", "input", this.updateApplyButton);
   this.addEventListener("toolsmenu", "command", this.updateToolsMenuStatus);
}

IeTab2.prototype.destory = function() {
   this.removeEventListenerByTagName("checkbox", "command", this.updateApplyButton);
   this.removeEventListener("filterChilds", "DOMAttrModified", this.updateApplyButton);
   this.removeEventListener("filterChilds", "DOMNodeInserted", this.updateApplyButton);
   this.removeEventListener("filterChilds", "DOMNodeRemoved", this.updateApplyButton);
   this.removeEventListener("parambox", "input", this.updateApplyButton);
   this.removeEventListener("toolsmenu", "command", this.updateToolsMenuStatus);
}

IeTab2.prototype.updateInterface = function() {
   var statusbar = this.getBoolPref("extensions.ietab2.statusbar", true);
   var icon = (window.arguments ? window.arguments[1] : null); //get status-bar icon handle
   if (icon) this.setAttributeHidden(icon, statusbar);
}

IeTab2.prototype.setOptions = function() {
   var requiresRestart = false;
   
   //filter
   var filter = document.getElementById('filtercbx').checked;
   this.setBoolPref("extensions.ietab2.filter", filter);
   this.setStrPref("extensions.ietab2.filterlist", this.getFilterListString());

   //general
   var toolsmenu = document.getElementById('toolsmenu').checked;
   this.setBoolPref("extensions.ietab2.toolsmenu", toolsmenu);
   this.setBoolPref("extensions.ietab2.toolsmenu.icon", document.getElementById('toolsmenu.icon').checked);

   var statusbar = document.getElementById('statusbar').checked;
   this.setBoolPref("extensions.ietab2.statusbar", statusbar);

   // Deal with the process mode
   var runInProcess = document.getElementById('runinprocess').checked;
   if(runInProcess != this.getBoolPref("extensions.ietab2.runinprocess"))
   {
	   requiresRestart = true;
	   this.setBoolPref("extensions.ietab2.runinprocess", runInProcess);
	   this.setBoolPref("dom.ipc.plugins.enabled.npietab2.dll", !runInProcess );
   }

   this.setBoolPref("extensions.ietab2.handleUrlBar", document.getElementById('handleurl').checked);
   this.setBoolPref("extensions.ietab2.alwaysNewTab", document.getElementById('alwaysnew').checked);
   this.setBoolPref("extensions.ietab2.focustab", document.getElementById('focustab').checked);

   //context (ietab)
   this.setBoolPref("extensions.ietab2.pagelink", document.getElementById('pagelink.embed').checked);
   this.setBoolPref("extensions.ietab2.tabsmenu", document.getElementById('tabsmenu.embed').checked);
   this.setBoolPref("extensions.ietab2.bookmark", document.getElementById('bookmark.embed').checked);

   //context (extapp)
   this.setBoolPref("extensions.ietab2.pagelink.extapp", document.getElementById('pagelink.extapp').checked);
   this.setBoolPref("extensions.ietab2.tabsmenu.extapp", document.getElementById('tabsmenu.extapp').checked);
   this.setBoolPref("extensions.ietab2.bookmark.extapp", document.getElementById('bookmark.extapp').checked);

   //showicon
   this.setBoolPref("extensions.ietab2.icon.pagelink", document.getElementById('pagelink.icon').checked);
   this.setBoolPref("extensions.ietab2.icon.tabsmenu", document.getElementById('tabsmenu.icon').checked);
   this.setBoolPref("extensions.ietab2.icon.bookmark", document.getElementById('bookmark.icon').checked);

   //external
   var path = document.getElementById('pathbox').value;
   this.setStrPref("extensions.ietab2.extAppPath", (path == this.IExploreExePath ? "" : path));

   var param = document.getElementById('parambox').value;
   this.setStrPref("extensions.ietab2.extAppParam", this.trim(param).split(/\s+/).join(" "));

   this.setBoolPref("extensions.ietab2.ctrlclick", document.getElementById('ctrlclick').checked);

   //update UI
   this.updateApplyButton(false);
   this.updateInterface();
   
   // Deal with compatibility mode
   var newMode = "ie7mode";
   var item = document.getElementById("iemode").selectedItem;
   if(item)
       newMode = item.getAttribute("id");
   if(this.getStrPref("extensions.ietab2.compatMode") != newMode)
   {
       requiresRestart = true;
       this.setStrPref("extensions.ietab2.compatMode", newMode);
       this.updateIEMode();
   }
   
   //notify of restart requirement
   if(requiresRestart)
	  alert("Firefox must be restarted for all changes to take effect.");
}

IeTab2.prototype.updateIEMode = function() {
   var mode = this.getStrPref("extensions.ietab2.compatMode");
   var wrk = Components.classes["@mozilla.org/windows-registry-key;1"]
						.createInstance(Components.interfaces.nsIWindowsRegKey);
   wrk.create(wrk.ROOT_KEY_CURRENT_USER,
	    "SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION",
	    wrk.ACCESS_ALL);
	
   var value = 7000;
   if(mode == "ie8mode")
      value = 8000;
   else if(mode == "ie8forced")
      value = 8888;
   else if(mode == "ie9mode")
      value = 9000;
  else if(mode == "ie9forced")
      value = 9999;

   wrk.writeIntValue("firefox.exe", value);
   wrk.writeIntValue("plugin-container.exe", value);
}

IeTab2.prototype.setAttributeHidden = function(obj, isHidden) {
   if (!obj) return;
   if (isHidden){
      obj.removeAttribute("hidden");
   }else{
      obj.setAttribute("hidden", true);
   }
}

IeTab2.prototype.getFilterListString = function() {
   var list = [];
   var filter = document.getElementById('filterList');
   var count = filter.view.rowCount;

   for (var i=0; i<count; i++) {
      var rule = filter.view.getCellText(i, filter.columns['columnRule']);
      var enabled = filter.view.getCellValue(i, filter.columns['columnEnabled']);
      var item = rule + (enabled=="true" ? "" : "\b");
      list.push(item);
   }
   list.sort();
   return list.join(" ");
}

IeTab2.prototype.updateToolsMenuStatus = function() {
   document.getElementById("toolsmenu.icon").disabled = !document.getElementById("toolsmenu").checked;
}

IeTab2.prototype.updateDelButtonStatus = function() {
   var en = document.getElementById('filtercbx').checked;
   var delbtn = document.getElementById('delbtn');
   var filter = document.getElementById('filterList');
   delbtn.disabled = (!en) || (filter.view.selection.count < 1);
}

IeTab2.prototype.updateAddButtonStatus = function() {
   var en = document.getElementById('filtercbx').checked;
   var addbtn = document.getElementById('addbtn');
   var urlbox = document.getElementById('urlbox');
   addbtn.disabled = (!en) || (this.trim(urlbox.value).length < 1);
}

IeTab2.prototype.updateDialogAllStatus = function() {
   var en = document.getElementById('filtercbx').checked;
   document.getElementById('filterList').disabled = (!en);
   document.getElementById('filterList').editable = (en);
   document.getElementById('urllabel').disabled = (!en);
   document.getElementById('urlbox').disabled = (!en);
   this.updateAddButtonStatus();
   this.updateDelButtonStatus();
   this.updateToolsMenuStatus();
}

IeTab2.prototype.updateDialogPositions = function() {
   var em = [document.getElementById('tabsmenu.embed'),
             document.getElementById('pagelink.embed'),
             document.getElementById('bookmark.embed')]
   var ex = [document.getElementById('tabsmenu.extapp'),
             document.getElementById('pagelink.extapp'),
             document.getElementById('bookmark.extapp')]
   var emMax = Math.max(em[0].boxObject.width, em[1].boxObject.width, em[2].boxObject.width);
   var exMax = Math.max(ex[0].boxObject.width, ex[1].boxObject.width, ex[2].boxObject.width);
   for (var i=0 ; i<em.length ; i++) em[i].width = emMax;
   for (var i=0 ; i<ex.length ; i++) ex[i].width = exMax;
}

IeTab2.prototype.findRule = function(value) {
   var filter = document.getElementById('filterList');
   var count = filter.view.rowCount;
   for (var i=0; i<count; i++) {
      var rule = filter.view.getCellText(i, filter.columns['columnRule']);
      if (rule == value) return i;
   }
   return -1;
}

IeTab2.prototype.addNewURL = function() {
   var filter = document.getElementById('filterList');
   var urlbox = document.getElementById('urlbox');
   var rule = this.trim(urlbox.value);
   if (rule != "") {
      if ((rule != "about:blank") && (rule.indexOf("://") < 0)) {
         rule = (/^[A-Za-z]:/.test(rule) ? "file:///"+rule.replace(/\\/g,"/") : rule);
         if (/^file:\/\/.*/.test(rule)) rule = encodeURI(rule);
      }
      if (!/^\/(.*)\/$/.exec(rule)) rule = rule.replace(/\/$/, "/*");
      rule = rule.replace(/\s/g, "%20");
      var idx = this.findRule(rule);
      if (idx == -1) { idx = this.addFilterRule(rule, true); urlbox.value = ""; }
      filter.view.selection.select(idx);
      filter.boxObject.ensureRowIsVisible(idx);
   }
   filter.focus();
   this.updateAddButtonStatus();
}

IeTab2.prototype.delSelected = function() {
   var filter = document.getElementById('filterList');
   var rules = document.getElementById('filterChilds');
   if (filter.view.selection.count > 0) {
      for (var i=rules.childNodes.length-1 ; i>=0 ; i--) {
         if (filter.view.selection.isSelected(i))
            rules.removeChild(rules.childNodes[i]);
      }
   }
   this.updateDelButtonStatus();
}

IeTab2.prototype.onClickFilterList = function(e) {
   var filter = document.getElementById('filterList');
   if (!filter.disabled && e.button == 0 && e.detail >= 2) {
      if (filter.view.selection.count == 1) {
         var urlbox = document.getElementById('urlbox');
         urlbox.value = filter.view.getCellText(filter.currentIndex, filter.columns['columnRule']);
         urlbox.select();
         this.updateAddButtonStatus();
      }
   }
}

IeTab2.prototype.modifyTextBoxValue = function(textboxId, newValue) {
   var box = document.getElementById(textboxId);
   if (box.value != newValue) {
      box.value = newValue;
      this.updateApplyButton(true);
   }
}

IeTab2.prototype.browseAppPath = function() {
   const nsIFilePicker = Components.interfaces.nsIFilePicker;
   var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
   fp.init(window, null, nsIFilePicker.modeOpen);
   fp.appendFilters(nsIFilePicker.filterApps);
   fp.appendFilters(nsIFilePicker.filterAll);
   var rv = fp.show();
   if (rv == nsIFilePicker.returnOK) {
      this.modifyTextBoxValue("pathbox", fp.file.target);
   }
}

IeTab2.prototype.resetAppPath = function() {
   this.modifyTextBoxValue("pathbox", this.IExploreExePath);
   this.modifyTextBoxValue("parambox", "%1");
}

IeTab2.prototype.saveToFile = function(aList) {
   var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(Components.interfaces.nsIFilePicker);
   var stream = Components.classes["@mozilla.org/network/file-output-stream;1"].createInstance(Components.interfaces.nsIFileOutputStream);
   var converter = Components.classes["@mozilla.org/intl/converter-output-stream;1"].createInstance(Components.interfaces.nsIConverterOutputStream);

   fp.init(window, null, fp.modeSave);
   fp.defaultExtension = "txt";
   fp.defaultString = "IETabPref";
   fp.appendFilters(fp.filterText);

   if (fp.show() != fp.returnCancel) {
      try {
         if (fp.file.exists()) fp.file.remove(true);
         fp.file.create(fp.file.NORMAL_FILE_TYPE, 0666);
         stream.init(fp.file, 0x02, 0x200, null);
         converter.init(stream, "UTF-8", 0, 0x0000);

         for (var i = 0; i < aList.length ; i++) {
            aList[i] = aList[i] + "\n";
            converter.writeString(aList[i]);
         }
      } finally {
         converter.close();
         stream.close();
      }
   }
}

IeTab2.prototype.loadFromFile = function() {
   var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(Components.interfaces.nsIFilePicker);
   var stream = Components.classes["@mozilla.org/network/file-input-stream;1"].createInstance(Components.interfaces.nsIFileInputStream);
   var converter = Components.classes["@mozilla.org/intl/converter-input-stream;1"].createInstance(Components.interfaces.nsIConverterInputStream);

   fp.init(window, null, fp.modeOpen);
   fp.defaultExtension = "txt";
   fp.appendFilters(fp.filterText);

   if (fp.show() != fp.returnCancel) {
      try {
         var input = {};
         stream.init(fp.file, 0x01, 0444, null);
         converter.init(stream, "UTF-8", 0, 0x0000);
         converter.readString(stream.available(), input);
         var linebreak = input.value.match(/(((\n+)|(\r+))+)/m)[1];
         return input.value.split(linebreak);
      } finally {
         converter.close();
         stream.close();
      }
   }
   return null;
}

IeTab2.prototype.exportSettings = function() {
   var aList = this.getAllSettings();
   if (aList) this.saveToFile(aList);
}

IeTab2.prototype.importSettings = function() {
   var aList = this.loadFromFile();
   if (aList) {
      this.setAllSettings(aList);
      this.initDialog();
      this.updateInterface();
   }
}

IeTab2.prototype.importOldSettings = function() {
   var aList = this.getAllOldIETabSettings();
   if (aList) {
      this.setAllSettings(aList);
   }
}

IeTab2.prototype.checkGetIETab1Settings = function() {
    var hasChecked = gIeTab2.getBoolPref("extensions.ietab2.prefsMigrated", false);
    if(!hasChecked)
    {
        gIeTab2.setBoolPref("extensions.ietab2.prefsMigrated", true);
        var oldList = gIeTab2.getStrPref("ietab.filterlist", null);
        if(oldList)
        {
            var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
			var res = ps.confirmEx(
						window.opener,
						"IE Tab 2",
						"It appears you had an older version of IE Tab installed.\r\n" +
                            "Would you like to import your settings from the older version?",
						ps.STD_YES_NO_BUTTONS,
						null,
						null,
						null,
						null,
						{});
			if(res == 0)
			{
                gIeTab2.importOldSettings();
			}
        }
        gIeTab2.flushPrefs();
    }
}

IeTab2.prototype.restoreDefault = function() {
   var aTemp = this.getAllSettings(false);
   var aDefault = this.getAllSettings(true);
   this.setAllSettings(aDefault);
   this.initDialog();
   this.setAllSettings(aTemp);
   this.updateApplyButton(true);
}

var gIeTab2 = new IeTab2();

