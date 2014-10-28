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
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/util/Format.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/form/field/SharedFolderComboBox.js")

Ext.define("OMV.module.admin.service.developer.Plugins", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format"
    ],

    hidePagingToolbar : false,
    hideAddButton     : true,
    hideEditButton    : true,
    hideDeleteButton  : true,
    autoReload        : false,
    stateful          : true,
    stateId           : "b317a72d-1804-1632-a31b-1f48f0ea6aae",
    columns           : [{
        text      : _("Name"),
        sortable  : true,
        dataIndex : "name",
        stateId   : "name"
    },{
        text      : _("Full Name"),
        sortable  : true,
        dataIndex : "fullname",
        stateId   : "fullname"
    },{
        text      : _("Github URL"),
        sortable  : true,
        dataIndex : "url",
        stateId   : "url",
        renderer  : function(value) {
            return "<a href=\"" + value + "\" target=\"_blank\">" + value + "</a>";
        }
    }],

    initComponent: function() {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "uuid",
                    fields     : [
                        { name : "uuid", type: "string" },
                        { name : "name", type: "string" },
                        { name : "fullname", type: "string" },
                        { name : "url", type: "string" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Developer",
                        method  : "getPluginList"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var me = this;
        var items = me.callParent(arguments);

        Ext.Array.insert(items, 0, [{
            id       : me.getId() + "-updateall",
            xtype    : "button",
            text     : _("Update All"),
            icon     : "images/refresh.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onUpdateButton, me, [ "all" ]),
            scope    : me
        },{
            id       : me.getId() + "-update",
            xtype    : "button",
            text     : _("Update"),
            icon     : "images/refresh.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onUpdateButton, me, [ "" ]),
            scope    : me,
            disabled : true
        },{
            id       : me.getId() + "-build",
            xtype    : "button",
            text     : _("Build"),
            icon     : "images/software.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onBuildButton, me, [ me ]),
            scope    : me,
            disabled : true
        },{
            id       : me.getId() + "-upload",
            xtype    : "button",
            text     : _("Upload"),
            icon     : "images/upload.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onUploadButton, me, [ me ]),
            scope    : me,
            disabled : true
        }]);
        return items;
    },

    onSelectionChange: function(model, records) {
        var me = this;
        me.callParent(arguments);
        // Process additional buttons.
        var tbarBtnDisabled = {
            "update" : true,
            "build"  : true,
            "upload" : true
        };
        if(records.length == 1) {
            tbarBtnDisabled["update"] = false;
            tbarBtnDisabled["build"] = false;
            tbarBtnDisabled["upload"] = false;
        }
        // Update the button controls.
        Ext.Object.each(tbarBtnDisabled, function(key, value) {
            this.setToolbarButtonDisabled(key, value);
        }, me);
    },

    onUpdateButton : function(plugin) {
        var me = this;
        var title = "";
        var cmd = "";
        if(plugin == "all") {
            title = _("Updating all plugins...");
            cmd = "all";
        } else {
            var record = me.getSelected();
            name = record.get("fullname");
            title = _("Updating ") + name + "...";
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
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    },

    onBuildButton : function() {
        var me = this;
        var record = me.getSelected();
        var title = _("Updating ") + record.get("fullname") + "...";
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
        var title = _("Uploading ") + record.get("fullname") + "...";
        var wnd = Ext.create("OMV.window.Execute", {
            title           : title,
            rpcService      : "Developer",
            rpcMethod       : "doCommand",
            rpcParams       : {
                "command" : "upload",
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
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "plugins",
    path      : "/service/developer",
    text      : _("Plugins"),
    position  : 10,
    className : "OMV.module.admin.service.developer.Plugins"
});
