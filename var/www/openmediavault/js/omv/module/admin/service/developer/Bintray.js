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

// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/panel/Panel.js")
// require("js/omv/module/admin/service/developer/Packages.js")
// require("js/omv/module/admin/service/developer/Publish.js")

Ext.define("OMV.module.admin.service.developer.Bintray", {
    extend: "OMV.workspace.grid.Panel",

    rpcService: "Developer",
    rpcGetMethod: "getBintrayPackages",
    requires: [
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
    ],

    stateful: true,
    stateId: "9e468ecc-904d-427a-bda0-02e916bb796c",

    defaults: {
        flex: 1
    },
    anchor: "100%",

    columns: [{
        text: _("Version"),
        dataIndex: 'omvversion',
        sortable: true,
        stateId: 'omvversion'
    },{
        text: _("Repository"),
        dataIndex: 'repository',
        sortable: true,
        stateId: 'repository'
    },{
        text: _("Package"),
        dataIndex: 'bpackage',
        sortable: true,
        stateId: 'bpackage'
    },{
        text: _("Distribution"),
        dataIndex: 'dist',
        sortable: true,
        stateId: 'dist'
    },{
        text: _("Architecture"),
        dataIndex: 'arch',
        sortable: true,
        stateId: 'arch'
    }],

    initComponent: function() {
        var me = this;
        
        Ext.apply(me, {
            store: Ext.create("OMV.data.Store", {
                autoLoad: true,
                model: OMV.data.Model.createImplicit({
                    fields: [
                        { name: "uuid", type: "string" },
                        { name: "omvversion", type: "string" },
                        { name: "repository", type: "string" },
                        { name: "bpackage", type: "string" },
                        { name: "dist", type: "string" },
                        { name: "arch", type: "string" }
                    ]
                }),
                proxy: {
                    type: "rpc",
                    rpcData: {
                        service: "Developer",
                        method: "getBintrayPackages",
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function(c) {
        var me = this;

        return [{
            id: me.getId() + "-add",
            xtype: "button",
            text: _("Add package"),
            icon: "images/add.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: false,
            handler: Ext.Function.bind(me.onAddButton, me, [ me ]),
            scope: me
        },{
            id: me.getId() + "-edit",
            xtype: "button",
            text: _("Edit package"),
            icon: "images/edit.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: true,
            handler: Ext.Function.bind(me.onEditButton, me, [ me ]),
            scope: me
        },{
            id: me.getId() + "-publish",
            xtype: "button",
            text: _("Publish file"),
            icon: "images/upload.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: true,
            handler: Ext.Function.bind(me.onPublishButton, me, [ me ]),
            scope: me
        },{
            id: me.getId() + "-delete",
            xtype: "button",
            text: _("Delete package"),
            icon: "images/delete.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: true,
            handler: Ext.Function.bind(me.onDeleteButton, me, [ me ]),
            scope: me
        }];
    },

    onSelectionChange: function(model, records) {
        var me = this;
        if(me.hideTopToolbar)
            return;
        var tbarBtnName = [ "add", "edit", "publish", "delete" ];
        var tbarBtnDisabled = {
            "add": false,
            "edit": false,
            "publish": false,
            "delete": false
        };
        // Enable/disable buttons depending on the number of selected rows.
        if(records.length <= 0) {
            tbarBtnDisabled["edit"] = true;
            tbarBtnDisabled["publish"] = true;
            tbarBtnDisabled["delete"] = true;
        } else if(records.length == 1) {
        } else {
            tbarBtnDisabled["edit"] = true;
            tbarBtnDisabled["publish"] = true;
        }

        // Update the button controls.
        Ext.Array.each(tbarBtnName, function(name) {
            var tbarBtnCtrl = me.queryById(me.getId() + "-" + name);
            if(!Ext.isEmpty(tbarBtnCtrl)) {
                if(true == tbarBtnDisabled[name]) {
                    tbarBtnCtrl.disable();
                } else {
                    tbarBtnCtrl.enable();
                }
            }
        });
    },

    onAddButton : function() {
        var me = this;
        Ext.create("OMV.module.admin.service.developer.Packages", {
            title: _("Add package"),
            uuid: "",
            listeners: {
                scope: me,
                submit: function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onEditButton : function() {
        var me = this;
        var sm = me.getSelectionModel();
        var records = sm.getSelection();
        var record = records[0];
        Ext.create("OMV.module.admin.service.developer.Packages", {
            title: _("Edit package"),
            uuid: record.get("uuid"),
            listeners: {
                scope: me,
                submit: function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onPublishButton: function() {
        var me = this;
        var sm = me.getSelectionModel();
        var records = sm.getSelection();
        var record = records[0];
        Ext.create("OMV.module.admin.service.developer.Publish", {
            uuid: record.get("uuid"),
            listeners: {
                scope: me,
                submit: function() {
                    OMV.MessageBox.show({
                        title: _("Success"),
                        msg: _("File successfully published"),
                        scope: me,
                        buttons: Ext.Msg.OK
                    });
                }
            }
        }).show();
    },

    doDeletion: function(record) {
        var me = this;
        OMV.Rpc.request({
            scope: me,
            callback: me.onDeletion,
            rpcData: {
                service: "Developer",
                method: "deleteBintrayPackage",
                params: {
                    uuid: record.get('uuid')
                }
            }
        });
    }

});

OMV.WorkspaceManager.registerPanel({
    id: "bintray",
    path: "/service/developer",
    text: _("Bintray"),
    position: 70,
    className: "OMV.module.admin.service.developer.Bintray"
});

