// require("js/omv/WorkspaceManager.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/tree/Panel.js")
// require("js/omv/form/field/CheckboxGrid.js")
// require("js/omv/workspace/window/Form.js")

Ext.define("OMV.module.admin.service.developer.Bintray", {
    extend: "OMV.tree.Panel",

    rpcService: "Developer",
    rpcGetMethod: "getBintrayRepos",
    requires: [
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc"
    ],

    rootVisible: false,
    stateful: true,
    stateId: "b32945c9-3f96-4859-a231-784fe946e957",

    border: false,
    rowLines: false,
    columnLines: true,
    selModel: {
        allowDeselect: true,
        mode: "SINGLE"
    },

    hideTopToolbar: false,
    hidePagingToolbar: false,
    deletionConfirmRequired: true,
    deletionWaitMsg: _("Deleting selected item(s)"),
    mode: "remote",
    rememberSelected: false,
    autoReload: false,

    columns: [{
        text: _("Name"),
        xtype: 'treecolumn',
        dataIndex: 'name',
        sortable: true,
        stateId: 'name'
    }],

    getTopToolbarItems: function(c) {
        var me = this;
        return [{
            id: me.getId() + "-addRepo",
            xtype: "button",
            text: _("Add repo"),
            icon: "images/add.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: false,
            hidden: false,
            handler: Ext.Function.bind(me.onAddRepoButton, me, [ me ]),
            scope: me
        },{
            id: me.getId() + "-addPackage",
            xtype: "button",
            text: _("Add package"),
            icon: "images/add.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: true,
            hidden: false,
            handler: Ext.Function.bind(me.onAddPackageButton, me, [ me ]),
            scope: me
        },{
            id: me.getId() + "-publish",
            xtype: "button",
            text: _("Publish file"),
            icon: "images/upload.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: true,
            hidden: false,
            handler: Ext.Function.bind(me.onPublishFileButton, me, [ me ]),
            scope: me
        },{
            id: me.getId() + "-sync",
            xtype: "button",
            text: _("Synchronize"),
            icon: "images/refresh.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled: false,
            hidden: false,
            handler: Ext.Function.bind(me.onSyncButton, me, [ me ]),
            scope: me
        }]
    },

    onSelectionChange: function(model, records) {
        var me = this;
        if(me.hideTopToolbar)
            return;
        var tbarBtnName = [ "addRepo", "addPackage", "publish", "sync" ];
        var tbarBtnDisabled = {
            "addRepo": false,
            "addPackage": true,
            "publish": true,
            "sync": false
        };
        // Enable/disable buttons depending on the number of selected rows.
        if(records.length <= 0) {
        } else if(records.length == 1) {
            // Enable 'addPackage' button if selected node a Repo
            Ext.Array.each(records, function(record) {
                if(record.get("type") === "Repo") {
                    tbarBtnDisabled["addPackage"] = false;
                    return false;
                }
            });
            // Enable 'Publish' button if selected node is a Package
            Ext.Array.each(records, function(record) {
                if(record.get("type") === "Package") {
                    tbarBtnDisabled["publish"] = false;
                    return false;
                }
            });
        } else {
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

    initComponent: function() {
        var me = this;
        me.dockedItems = [];
        me.dockedItems.push(me.topToolbar = Ext.widget({
            xtype: "toolbar",
            dock: "top",
            items: me.getTopToolbarItems(me)
        }));
        Ext.apply(me, {
            store: Ext.create("Ext.data.TreeStore", {
                autoLoad: true,
                model: OMV.data.Model.createImplicit({
                    fields: [
                        { name: "name", type: "string" },
                        { name: "id", type: "string" },
                        { name: "type", type: "string" },
                        { name: "parentid", type: "string" }
                    ]
                }),
                proxy: {
                    type: "rpc",
                    rpcData: {
                        service: "Developer",
                        method: "getBintrayRepos"
                    }
                },
                folderSort: true
            })
        });
        me.callParent(arguments);
        var selModel = me.getSelectionModel();
        selModel.on("selectionchange", me.onSelectionChange, me);
    },

    onAddRepoButton : function() {
        var me = this;
        Ext.create("OMV.workspace.window.Form", {
            title: _("Create Bintray repo"),
            rpcService   : "Developer",
            rpcSetMethod : "addBintrayRepo",

            getFormItems : function() {
                var me = this;
                return [{
                    xtype: "textfield",
                    name: "repo",
                    fieldLabel: _("Name"),
                    allowBlank: false
                },{
                    xtype: "textareafield",
                    name: "desc",
                    fieldLabel: _("Description")
                }];
            },

            listeners: {
                scope: me,
                submit: function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onAddPackageButton: function() {
        var me = this;
        var sm = me.getSelectionModel();
        var records = sm.getSelection();
        var record = records[0];
        Ext.create("OMV.workspace.window.Form", {
            title: _("Create Bintray package"),
            rpcService   : "Developer",
            rpcSetMethod : "addBintrayPackage",

            getFormItems : function() {
                var me = this;
                return [{
                    xtype: "combo",
                    name: "name",
                    store: Ext.create("OMV.data.Store", {
                        autoLoad: true,
                        pageSize: 100,
                        model: OMV.data.Model.createImplicit({
                            fields: [
                                { name: "name", type: "string" }
                            ]
                        }),
                        proxy: {
                            type: "rpc",
                            rpcData: {
                                service: "Developer",
                                method: "getPluginList"
                            }
                        },
                        remoteSort: true,
                        sorters: [{
                            direction: "ASC",
                            property: "name"
                        }]
                    }),
                    editable: false,
                    valueField: "name",
                    displayField: "name",
                    fieldLabel: _("Plugin"),
                    allowBlank: false
                },{
                    xtype: "hiddenfield",
                    name: "repo",
                    value: record.get("name")
                }];
            },

            listeners: {
                scope: me,
                submit: function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onPublishFileButton: function() {
        var me = this;
        var sm = me.getSelectionModel();
        var records = sm.getSelection();
        var record = records[0];
        Ext.create("OMV.workspace.window.Form", {
            title: _("Publish file on Bintray"),
            width: 500,
            rpcService   : "Developer",
            rpcSetMethod : "publishFileBintray",

            getFormItems : function() {
                var me = this;
                return [{
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
                                method: "getBintrayFiles",
                                params: {
                                    bpackage: record.get("name")
                                }
                            }
                        }
                    }),
                    editable: false,
                    valueField: "filename",
                    displayField: "filename",
                    fieldLabel: _("File"),
                    allowBlank: false
                },{
                    xtype: "combo",
                    name: "dist",
                    store: Ext.create("OMV.data.Store", {
                        autoLoad: true,
                        fields: [
                            { name: "distribution", type: "string" }
                        ],
                        data: [
                            { distribution: "wheezy" },
                            { distribution: "jessie" },
                            { distribution: "stretch" }
                        ]
                    }),
                    editable: false,
                    valueField: "distribution",
                    displayField: "distribution",
                    value: "jessie",
                    fieldLabel: _("Dist"),
                    allowBlank: false
                },{
                    xtype: "combo",
                    name: "arch",
                    fieldLabel: _("Architecture"),
                    mode: "local",
                    store: new Ext.data.SimpleStore({
                        fields: [ "value", "text" ],
                        data: [
                            [ "all", _("All") ],
                            [ "amd64", _("amd64 only") ],
                            [ "arm64", _("arm64 only") ],
                            [ "armel", _("armel only") ],
                            [ "armhf", _("armhf only") ],
                            [ "i386", _("i386 only") ],
                            [ "powerpc", _("powerpc only") ],
                            [ "amd64,i386", _("amd64,i386") ],
                            [ "amd64,armhf,i386", _("amd64,armhf,i386") ],
                            [ "amd64,armel,armhf,i386", _("amd64,armel,armhf,i386") ],
                            [ "amd64,armel,armhf,i386", _("amd64,arm64,armhf,i386") ],
                            [ "amd64,arm64,armel,armhf,i386", _("amd64,arm64,armel,armhf,i386") ]
                        ]
                    }),
                    displayField: "text",
                    valueField: "value",
                    allowBlank: false,
                    editable: false,
                    triggerAction: "all",
                    value: "all"
                },{
                    xtype: "hiddenfield",
                    name: "bpackage",
                    value: record.get("name")
                },{
                    xtype: "hiddenfield",
                    name: "repo",
                    value: record.get("parentid")
                }];
            },

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

    onSyncButton : function() {
        var me = this;
        OMV.Rpc.request({
            scope: me,
            relayErrors: false,
            callback: function(id, success, response) {
                this.doReload();
            },
            rpcData: {
                service: "Developer",
                method: "syncBintrayData",
                params: {
                    name: "Test"
                }
            }
        });
    },

    /**
     * Helper function to get the top toolbar object.
     * @return The paging toolbar object or NULL.
     */
    getTopToolbar: function() {
        return this.topToolbar;
    },

    /**
     * Helper function to get the paging toolbar object.
     * @return The paging toolbar object or NULL.
     */
    getPagingToolbar: function() {
        return this.pagingToolbar;
    },

    /**
     * Reload the grid content.
     */
    doReload: function() {
        var me = this;
        if(me.mode === "remote") {
            me.store.reload();
        }
    }
});

OMV.WorkspaceManager.registerPanel({
    id: "bintray",
    path: "/service/developer",
    text: _("Bintray"),
    position: 50,
    className: "OMV.module.admin.service.developer.Bintray"
});
