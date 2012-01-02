//
// Updates:
//
// 2/24/10: ietab.net -- Converted to new ietab2 namespace.
//
var IeTab2ExtApp = {

   HKEY_CLASSES_ROOT: 0,
   HKEY_CURRENT_CONFIG: 1,
   HKEY_CURRENT_USER: 2,
   HKEY_LOCAL_MACHINE: 3,
   HKEY_USERS: 4,

   getRegistryEntry: function(regRoot, regPath, regName) {
      try {
         if ("@mozilla.org/windows-registry-key;1" in Components.classes) {
            var nsIWindowsRegKey = Components.classes["@mozilla.org/windows-registry-key;1"].getService(Components.interfaces.nsIWindowsRegKey);
            var regRootKey = new Array(0x80000000, 0x80000005, 0x80000001, 0x80000002, 0x80000003);
            nsIWindowsRegKey.open(regRootKey[regRoot], regPath, Components.interfaces.nsIWindowsRegKey.ACCESS_READ);
            if (nsIWindowsRegKey.valueCount)
               return nsIWindowsRegKey.readStringValue(regName);
         }
      } catch(e) {}
      return null;
   },

   getIExploreExePath: function() {
      var regRoot = this.HKEY_LOCAL_MACHINE;
      var regPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";
      var regName = "";
      return this.getRegistryEntry(regRoot, regPath, regName);
   },

   removeArrayNullElements: function(a) {
      var result = [];
      while(a.length) {
         var elmt = a.shift();
         if (elmt) result.push(elmt);
      }
      return result;
   },

   runApp: function(filename, parameter) {
      if ((!filename) || (filename == "")) filename = this.getIExploreExePath();
      var nsILocalFile = Components.classes["@mozilla.org/file/local;1"].getService(Components.interfaces.nsILocalFile);
      nsILocalFile.initWithPath(filename);
      if (nsILocalFile.exists()) {
         var paramArray = parameter ? parameter.split(/\s*\"([^\"]*)\"\s*|\s+/) : [];
         paramArray = this.removeArrayNullElements(paramArray);
         var nsIProcess = Components.classes["@mozilla.org/process/util;1"].createInstance(Components.interfaces.nsIProcess);
         nsIProcess.init(nsILocalFile);
         nsIProcess.run(false, paramArray, paramArray.length);
         return true;
      }
      return false;
   }
};
