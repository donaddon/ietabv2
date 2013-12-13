/*
 *  The extension API is provided for integration between the extension and ietab.net
 */

//
// This is exposed to the ietab.net web pages
//
IeTab2.prototype.extensionApi = {
    // Explicitly declare what is exposed to the web page by this object (required as of Fx 15)
    __exposedProps__: {
        isIeTabButtonVisible: "r",
        isAddonbarEnabled: "r",
        addIeTabButtonToNavbar: "r",
        addIeTabButtonToAddonbar: "r"
    },

    isIeTabButtonVisible: function() {
        return !!document.getElementById("ietab2-button");
    },

    isAddonbarEnabled: function() {
        var bar = document.getElementById("addon-bar");
        if (!bar)
            return false;
        return !bar.collapsed;
    },

    addIeTabButtonToNavbar: function() {
        gIeTab2.installButton("nav-bar", "ietab2-button", "urlbar-container");
    },

    addIeTabButtonToAddonbar: function() {
        gIeTab2.installButton("addon-bar", "ietab2-button", null);
    }
}

IeTab2.prototype.attachExtensionApi = function(doc) {
    // Only attach for Gecko 1.9 due to security enhancements for wrappedJSObject
    // Technique taken from here:  http://forums.mozillazine.org/viewtopic.php?f=25&t=705865
    var gecko19 = !!(navigator.oscpu && document.getElementsByClassName);
	if(!gecko19)
		return;

	try {
		var win = doc.defaultView.wrappedJSObject;
		win.ietabExtensionApi = gIeTab2.extensionApi;
	}
	catch(ex) {
    }
}

IeTab2.initExtensionApiSupport = function() {
    var regex = /^https?:\/\/(www\.)?(dev2?\.)?ietab\.net\//;
    window.addEventListener("DOMContentLoaded", function(e) {
        if (!gIeTab2)
            return;
        var doc = e.target;
        if (!doc)
            return;

        var url = doc.location.toString();
        if (!regex.test(url))
            return;

        // It's one of our documents, attach the api
        gIeTab2.attachExtensionApi(doc);
    }, true);
}

IeTab2.initExtensionApiSupport();
