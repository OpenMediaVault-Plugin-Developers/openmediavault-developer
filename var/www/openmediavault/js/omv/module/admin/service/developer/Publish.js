/**
 * Copyright (c) 2015 OpenMediaVault Plugin Developers
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

// require("js/omv/workspace/window/Form.js")
// require("js/omv/Rpc.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")

Ext.define("OMV.module.admin.service.developer.Publish", {
    extend: "OMV.workspace.window.Form",

    title: _("Publish file"),
    layout: "fit",
    width: 600,
    maxHeight: 700,
    closable: true,
    resizable: true,
    buttonAlign: "center",
    grow: true,

    rpcService   : "Developer",
    rpcSetMethod : "publishBintrayFile",

    plugins: [{
        ptype : "configobject"
    }],
    uuid : OMV.UUID_UNDEFINED,

    initComponent: function() {
        var me = this;
        if (me.uuid === "") {
            me.uuid = OMV.UUID_UNDEFINED;
        }
        me.callParent(arguments);
    },

    getFormItems : function() {
        var me = this;
        var items = [];

        //Add parameters fieldset
        items.push({
            xtype: "fieldset",
            title: _("Parameters"),
            items: [{
                xtype: "combo",
                name: "file",
                store: Ext.create("OMV.data.Store", {
                    autoLoad: true,
                    fields: [
                        { name: "filename", type: "string" },
                        { name: "fullpath", type: "string" }
                    ],
                    proxy: {
                        type: "rpc",
                        rpcData: {
                            service: "Developer",
                            method: "getBintrayFileList",
                            params: {
                                uuid: me.uuid
                            }
                        }
                    }
                }),
                editable: false,
                valueField: "filename",
                displayField: "filename",
                fieldLabel: _("File"),
                allowBlank: false,
            }]
        });
        return items;
    }

});

