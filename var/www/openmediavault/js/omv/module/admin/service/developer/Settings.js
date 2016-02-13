/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2014-2016 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/form/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/form/field/SharedFolderComboBox.js")
// require("js/omv/form/field/UserComboBox.js")

Ext.define("OMV.module.admin.service.developer.Settings", {
    extend : "OMV.workspace.form.Panel",
    requires : [
        "OMV.form.field.SharedFolderComboBox",
        "OMV.form.field.UserComboBox"
    ],
    uses   : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    rpcService   : "Developer",
    rpcGetMethod : "getSettings",
    rpcSetMethod : "setSettings",

    getButtonItems : function() {
        var me = this;
        var items = me.callParent(arguments);
        items.push({
            id       : me.getId() + "-omvsvn",
            xtype    : "button",
            text     : _("Install OMV from svn"),
            icon     : "images/add.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            scope    : me,
            handler  : Ext.Function.bind(me.onOmvSvnButton, me, [ me ])
        });
        return items;
    },

    getFormItems    : function() {
        var me = this;
        return [{
            xtype    : "fieldset",
            title    : _("Settings"),
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "sharedfoldercombo",
                name       : "sharedfolderref",
                fieldLabel : _("Directory")
            },{
                xtype      : "usercombo",
                name       : "owner",
                fieldLabel : _("Owner"),
                value      : "nobody",
                userType   : "normal"
            },{
                xtype      : "textfield",
                name       : "copylocation",
                fieldLabel : _("Copy Location"),
                allowBlank : true
            }]
        },{
            xtype    : "fieldset",
            title    : _("git Config"),
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "textfield",
                name       : "gitname",
                fieldLabel : _("Name"),
                allowBlank : true
            },{
                xtype      : "textfield",
                name       : "gitemail",
                fieldLabel : _("Email"),
                allowBlank : true
            },{
                xtype   : "button",
                name    : "gitconfig",
                text    : _("Create git config"),
                scope   : this,
                handler : Ext.Function.bind(me.onConfigButton, me, [ "git" ]),
                margin  : "5 0 8 0"
            }]
        },{
            xtype    : "fieldset",
            title    : _("Github Config"),
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "textfield",
                name       : "ghusername",
                fieldLabel : _("Username"),
                allowBlank : true
            },{
                xtype      : "passwordfield",
                name       : "ghpassword",
                fieldLabel : _("Password"),
                allowBlank : true
            },{
                xtype   : "button",
                name    : "ghconfig",
                text    : _("Create Github .netrc"),
                scope   : this,
                handler : Ext.Function.bind(me.onConfigButton, me, [ "gh" ]),
                margin  : "5 0 8 0"
            }]
        },{
            xtype    : "fieldset",
            title    : _("Transifex Config"),
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "textfield",
                name       : "txhostname",
                fieldLabel : _("Hostname"),
                allowBlank : true
            },{
                xtype      : "passwordfield",
                name       : "txpassword",
                fieldLabel : _("Password"),
                allowBlank : true
            },{
                xtype      : "textfield",
                name       : "txtoken",
                fieldLabel : _("Token"),
                allowBlank : true
            },{
                xtype      : "textfield",
                name       : "txusername",
                fieldLabel : _("Username"),
                allowBlank : true
            },{
                xtype   : "button",
                name    : "txconfig",
                text    : _("Create Transifex configs"),
                scope   : this,
                handler : Ext.Function.bind(me.onConfigButton, me, [ "tx" ]),
                margin  : "5 0 8 0"
            }]
        },{
            xtype: "fieldset",
            title: _("Bintray settings"),
            fieldDefaults: {
                labelSeparator: ""
            },
            items: [{
                xtype: "textfield",
                name: "btusername",
                fieldLabel : _("Username"),
                allowBlank : true
            },{
                xtype: "passwordfield",
                fieldLabel : _("API key"),
                name: "btapikey",
                allowBlank : true
            },{
                xtype: "passwordfield",
                fieldLabel : _("GPG-key passphrase"),
                name: "btgpgpass",
                allowBlank : true
            }]
        }];
    },

    onConfigButton : function(cmd) {
        var me = this;
        me.doSubmit();
        switch(cmd) {
            case "gh":
                title = _("Creating Github .netrc ...");
            break;
            case "tx":
                title = _("Creating Transifex config ...");
            break;
            default:
                title = _("Creating git config ...");
        }
        OMV.MessageBox.wait(null, title);
        OMV.Rpc.request({
            scope       : me,
            relayErrors : false,
            rpcData     : {
                service  : "Developer",
                method   : "createConfig",
                params       : {
                    command : cmd
                },
            },
            success : function(id, success, response) {
                me.doReload();
                OMV.MessageBox.hide();
            }
        });
    },

    onOmvSvnButton: function() {
        var me = this;
        var wnd = Ext.create("OMV.window.Execute", {
            title           : _("Installing OMV from svn ..."),
            rpcService      : "Developer",
            rpcMethod       : "doOmvSvn",
            rpcIgnoreErrors : true,
            hideStartButton : true,
            hideStopButton  : true,
            listeners       : {
                scope     : me,
                finish    : function(wnd, response) {
                    wnd.appendValue(_("Done..."));
                    wnd.setButtonDisabled("close", false);
                },
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                    wnd.setButtonDisabled("close", false);
                },
                close     : function() {
                    document.location.reload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/developer",
    text      : _("Settings"),
    position  : 20,
    className : "OMV.module.admin.service.developer.Settings"
});
