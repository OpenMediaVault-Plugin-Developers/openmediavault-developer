/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2014-2017 OpenMediaVault Plugin Developers
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
// require("js/omv/Rpc.js")
// require("js/omv/WorkspaceManager.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/form/field/SharedFolderComboBox.js")
// require("js/omv/util/Format.js")
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")

Ext.define("OMV.module.admin.service.developer.Plugins", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Model",
        "OMV.data.Store",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format"
    ],

    locationId : "",

    hidePagingToolbar : false,
    hideAddButton     : true,
    hideEditButton    : true,
    hideDeleteButton  : true,
    autoReload        : false,
    rememberSelected  : true,
    stateful          : true,
    stateId           : "b317a72d-1804-1632-a31b-1f48f0ea6aae",
    columns           : [{
        xtype     : "textcolumn",
        text      : _("Name"),
        sortable  : true,
        dataIndex : "name",
        stateId   : "name"
    },{
        xtype     : "textcolumn",
        text      : _("Folder Exists"),
        sortable  : true,
        dataIndex : "exists",
        stateId   : "exists",
        renderer    : function (value) {
            var content;
            if ( value )
                content = _("Yes");
            else
                content = _("No");
            return content;
        }
    },{
        xtype     : "textcolumn",
        text      : _("Branch"),
        sortable  : true,
        dataIndex : "branch",
        stateId   : "branch"
    },{
        xtype     : "textcolumn",
        text      : _("Version"),
        sortable  : true,
        dataIndex : "version",
        stateId   : "version",
        flex      : 1
    }],

    initComponent: function() {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "name",
                    fields     : [
                        { name : "name", type : "string" },
                        { name : "url", type : "string" },
                        { name : "exists", type : "boolean" },
                        { name : "branch", type : "string" },
                        { name : "version", type : "string" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Developer",
                        method  : "getPluginList"
                    }
                },
                remoteSort : true,
                sorters    : [{
                    direction : "ASC",
                    property  : "name"
                }]
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var me = this;
        var items = me.callParent(arguments);

        Ext.Array.insert(items, 0, [{
            xtype : "button",
            text  : _("GitHub"),
            icon  : "images/folder.png",
            scope : this,
            menu  : [{
                text     : _("Update All"),
                icon     : "images/refresh.png",
                handler  : Ext.Function.bind(me.onUpdateButton, me, [ "all" ]),
                disabled : false
            },{
                text     : _("Update"),
                icon     : "images/refresh.png",
                handler  : Ext.Function.bind(me.onUpdateButton, me, [ "" ]),
                disabled : true,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1
                }
            },{
                text     : _("Reset"),
                icon     : "images/reboot.png",
                handler  : Ext.Function.bind(me.onResetButton, me, [ me ]),
                disabled : true,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1
                }
            },{
                text     : _("Reset Cache"),
                icon     : "images/reboot.png",
                handler  : Ext.Function.bind(me.onResetCacheButton, me, [ me ]),
                disabled : false
            },{
                text     : _("Open repo page"),
                icon     : "images/home.png",
                handler  : Ext.Function.bind(me.onOpenRepoButton, me, [ me ]),
                disabled : true,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1
                }
            }]
        },{
            xtype    : "button",
            text     : _("Package"),
            scope    : this,
            icon     : "images/software.png",
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1
            },
            menu     : [{
                text     : _("Build"),
                icon     : "images/software.png",
                handler  : Ext.Function.bind(me.onBuildButton, me, [ me ])
            },{
                text     : _("Install"),
                icon     : "images/add.png",
                handler  : Ext.Function.bind(me.onInstallButton, me, [ me ])
            }]
        },{
            id       : me.getId() + "-upload",
            xtype    : "button",
            text     : _("Upload"),
            icon     : "images/upload.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onUploadButton, me, [ me ]),
            scope    : me,
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1
            }
        },{
            id            : me.getId() + "-location",
            xtype         : "combo",
            allowBlank    : false,
            editable      : false,
            triggerAction : "all",
            displayField  : "name",
            valueField    : "uuid",
            store         : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "name",
                    fields     : [
                        { name : "uuid", type : "string" },
                        { name : "name", type : "string" }
                    ]
                }),
                proxy : {
                    type    : "rpc",
                    rpcData : {
                        service : "Developer",
                        method  : "getLocationList"
                    }
                }
            }),
            listeners     : {
                scope  : me,
                change : function(combo, value) {
                    me.locationId = value;
                }
            }
        },{
            xtype    : "button",
            text     : _("Transifex"),
            scope    : this,
            icon     : "images/access.png",
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1
            },
            menu     : [{
                text     : _("Build POT"),
                icon     : "images/software.png",
                handler  : Ext.Function.bind(me.onTxButton, me, [ "buildpot" ])
            },{
                text     : _("Push POT"),
                icon     : "images/upload.png",
                handler  : Ext.Function.bind(me.onTxButton, me, [ "pushpot" ])
            },{
                text     : _("Pull PO"),
                icon     : "images/download.png",
                handler  : Ext.Function.bind(me.onTxButton, me, [ "pullpo" ])
            }]
        },{
            xtype    : "button",
            text     : _("Git"),
            scope    : this,
            icon     : "images/logs.png",
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1
            },
            menu     : [{
                text     : _("git add"),
                icon     : "images/add.png",
                handler  : Ext.Function.bind(me.onGitButton, me, [ "add" ])
            },{
                text     : _("git commit"),
                icon     : "images/software.png",
                handler  : Ext.Function.bind(me.onGitButton, me, [ "commit" ])
            },{
                text     : _("git push"),
                icon     : "images/upload.png",
                handler  : Ext.Function.bind(me.onGitButton, me, [ "push" ])
            },{
                text     : _("git tag"),
                icon     : "images/puzzle.png",
                handler  : Ext.Function.bind(me.onGitButton, me, [ "tag" ])
            },{
                text     : _("git status"),
                icon     : "images/pulse.png",
                handler  : Ext.Function.bind(me.onGitButton, me, [ "status" ])
            },{
                text     : _("git diff"),
                icon     : "images/edit.png",
                handler  : Ext.Function.bind(me.onGitButton, me, [ "diff" ])
            },{
                text     : _("Change Branch"),
                icon     : "images/edit.png",
                handler  : Ext.Function.bind(me.onBranchButton, me, [ me ])
            }]
        },{
            id       : me.getId() + "-dch",
            xtype    : "button",
            text     : _("Changelog"),
            scope    : this,
            icon     : "images/document.png",
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1
            },
            menu     : [{
                text     : _("dch -i"),
                icon     : "images/arrow-up.png",
                handler  : Ext.Function.bind(me.onDchButton, me, [ "dchi" ])
            },{
                text     : _("dch -a"),
                icon     : "images/add.png",
                handler  : Ext.Function.bind(me.onDchButton, me, [ "dcha" ])
            },{
                text     : _("dch -r"),
                icon     : "images/software.png",
                handler  : Ext.Function.bind(me.onDchButton, me, [ "dchr" ])
            },{
                text     : _("Show changelog"),
                icon     : "images/document.png",
                handler  : Ext.Function.bind(me.onChangelogButton, me, [ me ])
            }]
        }]);
        return items;
    },

    onUpdateButton : function(plugin) {
        var me = this;
        var title = "";
        var cmd = "";
        if(plugin == "all") {
            title = _("Updating all plugins ...");
            cmd = "all";
        } else {
            var record = me.getSelected();
            name = record.get("name");
            title = _("Updating ") + name + " ...";
            cmd = record.get("name");
        }
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command" : "update",
                "plugin"  : cmd
            },
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
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onResetButton : function(plugin) {
        var me = this;
        var record = me.getSelected();
        name = record.get("name");
        cmd = record.get("name");

        var wnd = Ext.create("OMV.window.Execute", {
            title           : _("Resetting ") + name + _(" to current github files ..."),
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command" : "reset",
                "plugin"  : cmd
            },
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
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onResetCacheButton : function(plugin) {
        var me = this;
        OMV.MessageBox.wait(null, _("Resetting plugin list cache ..."));
        OMV.Rpc.request({
            scope       : me,
            relayErrors : false,
            rpcData     : {
                service  : "Developer",
                method   : "doResetCache"
            },
            success : function(id, success, response) {
                this.doReload();
                OMV.MessageBox.hide();
            }
        });
    },

    onBuildButton : function() {
        var me = this;
        var record = me.getSelected();
        var title = _("Updating ") + record.get("name") + " ...";
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command" : "build",
                "plugin"  : record.get("name")
            },
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
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onChangelogButton : function() {
        var me = this;
        var record = me.getSelected();
        var title = _("Changelog for ") + record.get("name") + " ...";
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command" : "changelog",
                "plugin"  : record.get("name")
            },
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
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onUploadButton : function() {
        var me = this;
        var record = me.getSelected();
        var title = _("Uploading ") + record.get("name") + " ...";
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command"  : "upload",
                "plugin"   : record.get("name"),
                "location" : me.locationId
            },
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
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onTxButton : function(cmd) {
        var me = this;
        var title = "";
        var record = me.getSelected();
        switch(cmd) {
            case "buildpot":
                title = _("Building translations ...");
                break;
            case "pushpot":
                title = _("Sending translations ...");
                break;
            default:
                title = _("Pulling translations ...");
        }
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command" : cmd,
                "plugin"  : record.get("name")
            },
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
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onInstallButton : function() {
        var me = this;
        var record = me.getSelected();
        var wnd = Ext.create("OMV.window.Execute", {
            title           : _("Installing ") + record.get("name") + " ...",
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command" : "install",
                "plugin"  : record.get("name")
            },
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
    },

    onGitButton : function(cmd) {
        var me = this;
        var commit = "";
        var title = "";
        var tag = "";
        var message = "";
        var record = me.getSelected();
        switch(cmd) {
            case "add":
                title = _("Adding files for commit ...");
                break;
            case "commit":
                title = _("Creating commit ...");
                commit = prompt("Enter commit message", "");
                break;
            case "tag":
                title = _("Creating tag ...");
                tag = prompt("Enter tag name", "");
                message = prompt("Enter message", "");
                break;
            case "status":
                title = _("Status ...");
                break;
            case "diff":
                title = _("Showing diff ...");
                break;
            default:
                title = _("Pushing files to Github ...");
        }
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doGit",
            rpcParams       : {
                "command" : cmd,
                "plugin"  : record.get("name"),
                "commit"  : commit,
                "tag"     : tag,
                "message" : message
            },
            rpcIgnoreErrors : true,
            hideStartButton : true,
            hideStopButton  : true,
            listeners       : {
                scope     : me,
                finish    : function(wnd, response) {
                    wnd.appendValue(_("Done..."));
                    wnd.setButtonDisabled("close", false);
                    this.doReload();
                },
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                    wnd.setButtonDisabled("close", false);
                },
                close     : function() {
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onDchButton : function(cmd) {
        var me = this;
        var commit = "";
        var title = "";
        var record = me.getSelected();
        switch(cmd) {
            case "dchi":
                title = _("Increment version ...");
                commit = prompt("Enter changelog line", "");
                break;
            case "dcha":
                title = _("Add changelog line ...");
                commit = prompt("Enter changelog line", "");
                break;
            default:
                title = _("Release version ...");
        }
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doDch",
            rpcParams       : {
                "command" : cmd,
                "plugin"  : record.get("name"),
                "commit"  : commit
            },
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
                    this.doReload();
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onBranchButton: function() {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.workspace.window.Form", {
            title        : _("Change Branch ..."),
            rpcService   : "Developer",
            rpcSetMethod : "doChangeBranch",
            getFormItems : function() {
                var me = this;
                return [{
                    xtype : "combo",
                    name  : "branch",
                    store : Ext.create("OMV.data.Store", {
                        autoLoad : true,
                        pageSize : 100,
                        model    : OMV.data.Model.createImplicit({
                            fields : [
                                { name : "branch", type: "string" }
                            ]
                        }),
                        proxy : {
                            type    : "rpc",
                            rpcData : {
                                service : "Developer",
                                method  : "getBranches",
                                params  : {
                                    plugin : record.get("name")
                                }
                            }
                        }
                    }),
                    editable     : false,
                    valueField   : "branch",
                    displayField : "branch",
                    fieldLabel   : _("Branch"),
                    allowBlank   : false
                },{
                    xtype : "hiddenfield",
                    name  : "plugin",
                    value : record.get("name")
                }];
            },
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onOpenRepoButton : function() {
        var record = this.getSelected();
        window.open("https://github.com/OpenMediaVault-Plugin-Developers" + record.get("url"), "_blank");
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "plugins",
    path      : "/service/developer",
    text      : _("Plugins"),
    position  : 10,
    className : "OMV.module.admin.service.developer.Plugins"
});
