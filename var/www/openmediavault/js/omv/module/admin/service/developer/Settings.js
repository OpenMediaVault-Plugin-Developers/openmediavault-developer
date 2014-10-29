/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2014 OpenMediaVault Plugin Developers
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
            }]
        },{
            xtype    : "fieldset",
            title    : _("Upload Settings"),
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "textfield",
                name       : "remotehost",
                fieldLabel : _("Remote Host"),
                allowBlank : false
            },{
                xtype      : "textfield",
                name       : "remotepath",
                fieldLabel : _("Remote Path"),
                allowBlank : false
            },{
                xtype         : "numberfield",
                name          : "port",
                fieldLabel    : _("Port"),
                vtype         : "port",
                minValue      : 1,
                maxValue      : 65535,
                allowDecimals : false,
                allowBlank    : false,
                value         : 22
            },{
                xtype      : "usercombo",
                name       : "username",
                fieldLabel : _("User"),
                value      : "nobody",
                userType   : "normal",
                editable   : true
            },{
                xtype      : "passwordfield",
                name       : "password",
                fieldLabel : _("Password"),
                allowBlank : false
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
                allowBlank : false
            },{
                xtype      : "textfield",
                name       : "gitemail",
                fieldLabel : _("Email"),
                allowBlank : false
            },{
                xtype   : "button",
                name    : "gitconfig",
                text    : _("Create .gitconfig"),
                scope   : this,
                handler : Ext.Function.bind(me.onGitConfigButton, me, [ me ]),
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
                allowBlank : false
            },{
                xtype      : "passwordfield",
                name       : "txpassword",
                fieldLabel : _("Password"),
                allowBlank : false
            },{
                xtype      : "textfield",
                name       : "txtoken",
                fieldLabel : _("Token"),
                allowBlank : true
            },{
                xtype      : "textfield",
                name       : "txusername",
                fieldLabel : _("Username"),
                allowBlank : false
            },{
                xtype   : "button",
                name    : "txconfig",
                text    : _("Create Transifex configs"),
                scope   : this,
                handler : Ext.Function.bind(me.onTxConfigButton, me, [ me ]),
                margin  : "5 0 8 0"
            }]
        }];
    },

    onGitConfigButton : function() {
        var me = this;
        me.doSubmit();
        OMV.MessageBox.wait(null, _("Creating .gitconfig file ..."));
        OMV.Rpc.request({
            scope       : me,
            relayErrors : false,
            rpcData     : {
                service  : "Developer",
                method   : "createGitConfig"
            },
            success : function(id, success, response) {
                me.doReload();
                OMV.MessageBox.hide();
            }
        });
    },

    onTxConfigButton : function() {
        var me = this;
        me.doSubmit();
        OMV.MessageBox.wait(null, _("Creating Transifex configs ..."));
        OMV.Rpc.request({
            scope       : me,
            relayErrors : false,
            rpcData     : {
                service  : "Developer",
                method   : "createTxConfig"
            },
            success : function(id, success, response) {
                me.doReload();
                OMV.MessageBox.hide();
            }
        });
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/developer",
    text      : _("Settings"),
    position  : 20,
    className : "OMV.module.admin.service.developer.Settings"
});
