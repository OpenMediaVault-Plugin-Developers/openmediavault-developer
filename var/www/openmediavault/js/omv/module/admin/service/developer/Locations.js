/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2019 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")

Ext.define("OMV.module.admin.service.developer.Location", {
    extend : "OMV.workspace.window.Form",
    uses   : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    rpcService   : "Developer",
    rpcGetMethod : "getLocation",
    rpcSetMethod : "setLocation",
    plugins      : [{
        ptype : "configobject"
    }],

    getFormItems : function () {
        var me = this;
        return [{
                xtype      : "textfield",
                name       : "name",
                fieldLabel : _("Name"),
                allowBlank : false
            },{
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
                xtype      : "textfield",
                name       : "username",
                fieldLabel : _("User"),
                allowBlank : false
            },{
                xtype      : "passwordfield",
                name       : "password",
                fieldLabel : _("Password"),
                allowBlank : true
        }];
    }
});

Ext.define("OMV.module.admin.service.developer.Locations", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc"
    ],
    uses     : [
        "OMV.module.admin.service.developer.Location"
    ],

    hidePagingToolbar : false,
    stateful          : true,
    stateId           : "3339057b-b2c0-4c48-a4c1-8c9b1fb34d1b",
    columns           : [{
        xtype     : "textcolumn",
        text      : _("Name"),
        sortable  : true,
        dataIndex : "name",
        stateId   : "name"
    },{
        xtype     : "textcolumn",
        text      : _("Remote Host"),
        sortable  : true,
        dataIndex : "remotehost",
        stateId   : "remotehost",
    },{
        xtype     : "textcolumn",
        text      : _("Remote Path"),
        sortable  : true,
        dataIndex : "remotepath",
        stateId   : "remotepath",
    },{
        xtype     : "textcolumn",
        text      : _("Port"),
        sortable  : true,
        dataIndex : "port",
        stateId   : "port",
    },{
        xtype     : "textcolumn",
        text      : _("Username"),
        sortable  : true,
        dataIndex : "username",
        stateId   : "username",
    }],

    initComponent : function () {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "uuid",
                    fields     : [
                        { name  : "uuid", type: "string" },
                        { name  : "name", type: "string" },
                        { name  : "remotehost", type: "string" },
                        { name  : "remotepath", type: "string" },
                        { name  : "port", type: "integer" },
                        { name  : "username", type: "string" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Developer",
                        method  : "getLocationList"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    onAddButton: function () {
        var me = this;
        Ext.create("OMV.module.admin.service.developer.Location", {
            title     : _("Add upload location"),
            uuid      : OMV.UUID_UNDEFINED,
            listeners : {
                scope  : me,
                submit : function () {
                    this.doReload();
                }
            }
        }).show();
    },

    onEditButton: function () {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.module.admin.service.developer.Location", {
            title     : _("Edit upload location"),
            uuid      : record.get("uuid"),
            listeners : {
                scope  : me,
                submit : function () {
                    this.doReload();
                }
            }
        }).show();
    },

    doDeletion: function (record) {
        var me = this;
        OMV.Rpc.request({
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "Developer",
                method  : "deleteLocation",
                params  : {
                    uuid: record.get("uuid")
                }
            }
        });
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "locations",
    path      : "/service/developer",
    text      : _("Locations"),
    position  : 30,
    className : "OMV.module.admin.service.developer.Locations"
});
