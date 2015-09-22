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
// require("js/omv/window/RootFolderBrowser.js")

Ext.define("OMV.module.admin.service.developer.Packages", {
    extend: "OMV.workspace.window.Form",

    title: _("Add package"),
    layout: "fit",
    width: 600,
    maxHeight: 700,
    closable: true,
    resizable: true,
    buttonAlign: "center",
    grow: true,

    rpcService   : "Developer",
    rpcGetMethod : "getBintrayPackage",
    rpcSetMethod : "setBintrayPackage",

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
                name: "omvversion",
                store: Ext.create("OMV.data.Store", {
                    autoLoad: true,
                    fields: [
                        { name: "version", type: "string" }
                    ],
                    data: [
                        { version: "stoneburner" }
                    ]
                }),
                editable: false,
                valueField: "version",
                displayField: "version",
                fieldLabel: _("OMV version"),
                allowBlank: false,
                listeners: {
                    change: function(combo, newValue, oldValue, eOpts) {
                        me.getForm().findField("repository").setDisabled(false);
                        if (me.getForm().findField("repository").getStore().getProxy().rpcData.params === undefined) {
                            me.getForm().findField("repository").getStore().getProxy().rpcData.params = {omvversion: newValue};
                        } else {
                            me.getForm().findField("repository").getStore().getProxy().rpcData.params.omvversion = newValue;
                        }
                        me.getForm().findField("repository").getStore().load();
                    }
                }
            },{
                xtype: "combo",
                name: "repository",
                store: Ext.create("OMV.data.Store", {
                    autoLoad: true,
                    model: OMV.data.Model.createImplicit({
                        fields: [
                            { name: "repository", type: "string" }
                        ]
                    }),
                    proxy: {
                        type: "rpc",
                        rpcData: {
                            service: "Developer",
                            method: "getBintrayRepoList",
                        }
                    }
                }),
                editable: false,
                valueField: "repository",
                displayField: "repository",
                fieldLabel: _("Bintray repository"),
                allowBlank: false,
                listeners: {
                    change: function(combo, newValue, oldValue, eOpts) {
                        me.getForm().findField("bpackage").setDisabled(false);
                        if (me.getForm().findField("bpackage").getStore().getProxy().rpcData.params === undefined) {
                            me.getForm().findField("bpackage").getStore().getProxy().rpcData.params = {repository: newValue};
                        } else {
                            me.getForm().findField("bpackage").getStore().getProxy().rpcData.params.repository = newValue;
                        }
                        me.getForm().findField("bpackage").getStore().load();
                    }
                },
                disabled: true
            },{
                xtype: "combo",
                name: "bpackage",
                store: Ext.create("OMV.data.Store", {
                    autoLoad: false,
                    model: OMV.data.Model.createImplicit({
                        fields: [
                            { name: "package", type: "string" }
                        ]
                    }),
                    proxy: {
                        type: "rpc",
                        rpcData: {
                            service: "Developer",
                            method: "getBintrayPackageList",
                            params: {}
                        }
                    }
                }),
                editable: false,
                valueField: "bpackage",
                displayField: "bpackage",
                fieldLabel: _("Bintray package"),
                allowBlank: false,
                disabled: true
            },{
                xtype: "textfield",
                name: "dist",
                fieldLabel: _("Dist"),
                plugins: [{
                    ptype: "fieldinfo",
                    text: _("e.g. wheezy")
                }],
                allowBlank: false
            },{
                xtype: "textfield",
                name: "arch",
                fieldLabel: _("Architecture"),
                plugins: [{
                    ptype: "fieldinfo",
                    text: _("Comma seperated list of architectures e.g. i386,amd64")
                }],
                allowBlank: false
            }]
        });
        return items;

    }

});

