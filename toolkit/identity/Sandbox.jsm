/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const EXPORTED_SYMBOLS = ["Sandbox"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const PREF_DEBUG = "toolkit.identity.debug";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

/**
 * An object that represents a sandbox in an iframe loaded with aURL. The
 * callback provided to the constructor will be invoked when the sandbox is
 * ready to be used. The callback will receive this object as its only argument.
 *
 * Please call free() when you are finished with the sandbox to explicitely free
 * up all associated resources.
 *
 * @param aURL
 *        (string) URL to load in the sandbox.
 *
 * @param aCallback
 *        (function) Callback to be invoked with a Sandbox, when ready.
 */
function Sandbox(aURL, aCallback) {
  this._debug = Services.prefs.getBoolPref(PREF_DEBUG);
  this._url = aURL;
  this._createFrame();
  this._createSandbox(aCallback);
}

Sandbox.prototype = {

  /**
   * Use the outer window ID as the identifier of the Sandbox.
   */
  get id() {
    return this._frame.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
             .getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  },

  /**
   * Load or reload the url
   */
  load: function Sandbox_load() {
    this._log("load: " + this.id + " : " + this._url);
    this._createSandbox(function createdSandbox(aSandbox){
      this._log("load sandbox id: ", aSandbox.id);
    });
  },

  /**
   * Frees the sandbox and releases the iframe created to host it.
   */
  free: function Sandbox_free() {
    this._log("free: " + this.id);
    this._container.removeChild(this._frame);
    this._frame = null;
    this._container = null;
  },

  /**
   * Creates an empty, hidden iframe and sets it to the _frame
   * property of this object.
   */
  _createFrame: function Sandbox__createFrame() {
    // TODO: What if there is no most recent browser window? (bug 745415).
    // Or use hiddenWindow
    let doc = Services.wm.getMostRecentWindow("navigator:browser").document;

    // Insert iframe in to create docshell.
    let frame = doc.createElement("iframe");
    frame.setAttribute("type", "content");
    frame.setAttribute("collapsed", "true");
    doc.documentElement.appendChild(frame);

    // Stop about:blank from being loaded.
    let webNav = frame.docShell.QueryInterface(Ci.nsIWebNavigation);
    webNav.stop(Ci.nsIWebNavigation.STOP_NETWORK);

    // Disable some types of content
    webNav.allowAuth = false;
    webNav.allowPlugins = false;
    webNav.allowImages = false;
    webNav.allowWindowControl = false;
    // TODO: disable media (bug 759964)

    // Disable stylesheet loading since the document is not visible.
    let markupDocViewer = frame.docShell.contentViewer
                            .QueryInterface(Ci.nsIMarkupDocumentViewer);
    markupDocViewer.authorStyleDisabled = true;

    // Set instance properties.
    this._frame = frame;
    this._container = doc.documentElement;
  },

  _createSandbox: function Sandbox__createSandbox(aCallback) {
    this._container.addEventListener(
      "DOMWindowCreated",
      function _makeSandboxContentLoaded(event) {
        this._log("_makeSandboxContentLoaded  " + event.target.location.toString());
        if (event.target.location.toString() != this._url) {
          return;
        }
        event.target.removeEventListener(
          "DOMWindowCreated", _makeSandboxContentLoaded, false
        );
        /* TODO?
         let workerWindow = this._frame.contentWindow;
         this._sandbox = new Cu.Sandbox(workerWindow, {
         wantXrays:        false,
         sandboxPrototype: workerWindow
         });
         */
        aCallback(this);
      }.bind(this),
      true
    );

    // Load the iframe.
    this._frame.webNavigation.loadURI(
      this._url,
      this._frame.docShell.LOAD_FLAGS_BYPASS_CACHE,
      null, // referrer
      null, // postData
      null  // headers
    );

  },

  _log: function Sandbox__log(msg) {
    if (!this._debug)
      return;
    dump("Sandbox.jsm: " + msg + "\n");
  },

};
