<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
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
require_once("openmediavault/object.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/util.inc");
require_once("openmediavault/rpcservice.inc");
require_once("openmediavault/notify.inc");

class OMVRpcServiceDeveloper extends OMVRpcServiceAbstract
{
    public function getName()
    {
        return "Developer";
    }

    private function getPluginName()
    {
        return strtolower( $this->getName() );
    }

    private function getXPath()
    {
        return sprintf( "//services/%s", $this->getPluginName() );
    }

    private function getEventMessagePath()
    {
        return sprintf( "org.openmediavault.services.%s", $this->getPluginName() );
    }

    private function getPlugins()
    {
        return array("acestream","active-directory","anacron","aufs",
            "autoshutdown","backup","btsync","calibre", "couchpotato","cups",
            "deluge","dnsmasq","downloader","extplorer","fail2ban","git",
            "glusterfs","greyhole","headphones","jdownloader","links",
            "mcmyadmin","mediabrowser","mhddfs","minidlna","mumble","mysql",
            "nginx","offlineimap","omvextrasorg","openvpn","openvpnas",
            "openvpn-old","plexmediaserver","pptp","processlist","pxe",
            "pyload","remotenfs-share","remoteshare","roundcube","rsnapshot",
            "sabnzbd","sensors","shellinabox","sickbeard","skeleton","snapraid",
            "subsonic","subversion","supportinfo","teamspeak3","transmissionbt",
            "vdr","virtualbox","virtualhosts","wakealarm","webdav","website",
            "wol","wordpress","zfs");
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");

        $this->registerMethod("getPluginList");

        $this->registerMethod("doUpdate");
        $this->registerMethod("doBuild");
    }

    public function getSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Get configuration object
        $object = $xmlConfig->get($this->getXPath());
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        return $object;
    }

    public function setSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "sharedfolderref" : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID"] . ' }
            }
        }');

        // Prepare configuration data
        $object = array(
            "sharedfolderref" => $params["sharedfolderref"]
        );

        // Set configuration object
        if (false === $xmlConfig->replace($this->getXPath(), $object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $object
        );

        return $object;
    }

    public function getPluginList($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
            }
        }');

        $objects = array();
        $plugins = $this->getPlugins();
        
        foreach ($plugins as $plugin)
        {
            $objects = array(
                "name"     => $plugin,
                "fullname" => sprintf("openmediavault-%s", $plugin),
                "url"      => sprintf("https://github.com/OpenMediaVault-Plugin-Developers/openmediavault-%s", $plugin)
            );
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function doCommand($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "build"   : { "type" : "boolean" },
                "command" : { "type" : "string" }
            }
        }');
        
        $plugins = $this->getPlugins();
        
        if ($params['command'] != "all")
        {
            if (!in_array($params['command'], $plugins))
            {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_OPERATION_DENIED,
                    sprinf("Unknown plugin - %s", $params['command'])
                );
            }
        }

        if (boolval($params['build'] === true)
            $type = "build";
        else
            $type = "update";
            
        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();

        if ($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            if ($params['command'] != "all")
            {
                $cmd = sprintf("omv-mkconf developer %s %s 2>&1", $type, $params['command']);
                $this->exec($cmd, $output, $bgOutputFilename);
                $this->finalizeBgProcStatus($bgStatusFilename, $output);            
            }
            else
            {
                foreach ($plugins as $plugin)
                {   
                    $cmd = sprintf("omv-mkconf developer %s %s 2>&1", $type, $plugin);
                    $this->exec($cmd, $output, $bgOutputFilename);
                    $this->finalizeBgProcStatus($bgStatusFilename, $output);
                }
            }
            
            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceDeveloper());
