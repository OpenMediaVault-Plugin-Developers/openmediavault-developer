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
                userType   : "normal"
            },{
                xtype      : "textfield",
                name       : "password",
                fieldLabel : _("Password"),
                allowBlank : false
            }]
        }];
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/developer",
    text      : _("Settings"),
    position  : 20,
    className : "OMV.module.admin.service.developer.Settings"
});
