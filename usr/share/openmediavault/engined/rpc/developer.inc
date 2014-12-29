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
    private static $commands = array("build","update","upload","buildpot","pushpot",
        "pullpo","install","reset","tx","gh","git","add","commit","push",
        "dchi","dcha","dchr","changelog","status");

    private static $plugins = array("acestream","active-directory","anacron","aufs",
        "autoshutdown","backup","btsync","calibre","couchpotato","cups","deluge",
        "developer","dnsmasq","downloader","extplorer","fail2ban","folding","gateone",
        "git","glusterfs","greyhole","headphones","hpraid","jdownloader","links",
        "mcmyadmin","mediabrowser","mhddfs","minidlna","mumble","mysql","nginx",
        "nzbget","nzbdrone","offlineimap","omvextrasorg","openvpn","openvpnas",
        "openvpn-old","plexmediaserver","pptp","processlist","pxe","pyload",
        "remotenfs-share","remoteshare","roundcube","rsnapshot","sabnzbd","sensors",
        "shellinabox","sickbeard","skeleton","snapraid","sonarr","subsonic",
        "subversion","supportinfo","teamspeak3","themes","transmissionbt",
        "unionfilesystems","vdr","virtualbox","virtualhosts","wakealarm","webdav",
        "website","wol","wordpress","zfs");

    const LOCATION = 'location';

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

    private function getXPathLocations()
    {
        return sprintf( "%s/%ss", $this->getXPath(), self::LOCATION );
    }

    private function getXPathLocation()
    {
        return sprintf( "%s/%s", $this->getXPathLocations(), self::LOCATION );
    }

    private function getEventMessagePath()
    {
        return sprintf( "org.openmediavault.services.%s", $this->getPluginName() );
    }

    private function getEventMessagePathLocation()
    {
        return sprintf( "%s.%ss.%s", $this->getEventMessagePath(), self::LOCATION, self::LOCATION );
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getPluginList");

        $this->registerMethod("getLocation");
        $this->registerMethod("setLocation");
        $this->registerMethod("getLocationList");
        $this->registerMethod("deleteLocation");

        $this->registerMethod("doCommand");
        $this->registerMethod("createConfig");
        $this->registerMethod("doGit");
        $this->registerMethod("doDch");
        $this->registerMethod("doOmvSvn");
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

        unset($object['locations']);

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
                "sharedfolderref" : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID"] . ' },
                "owner"           : { "type" : "string" },
                "gitname"         : { "type" : "string", "optional" : true },
                "gitemail"        : { "type" : "string", "optional" : true },
                "ghusername"      : { "type" : "string", "optional" : true },
                "ghpassword"      : { "type" : "string", "optional" : true },
                "txhostname"      : { "type" : "string", "optional" : true },
                "txpassword"      : { "type" : "string", "optional" : true },
                "txtoken"         : { "type" : "string", "optional" : true },
                "txusername"      : { "type" : "string", "optional" : true }
            }
        }');

        // Get the existing configuration object.
        $oldObject = $xmlConfig->get( $this->getXPath() );
        if (is_null($oldObject))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        // Prepare configuration data
        $object = array(
            "sharedfolderref" => $params["sharedfolderref"],
            "owner"           => $params['owner'],
            "gitname"         => $params['gitname'],
            "gitemail"        => $params['gitemail'],
            "ghusername"      => $params['ghusername'],
            "ghpassword"      => $params['ghpassword'],
            "txhostname"      => $params['txhostname'],
            "txpassword"      => $params['txpassword'],
            "txtoken"         => $params['txtoken'],
            "txusername"      => $params['txusername'],
            "locations"       => isset($oldObject['locations']) ? $oldObject['locations'] : array()
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

        // Get configuration object
        $object = $xmlConfig->get($this->getXPath());
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        unset($object['locations']);

        $sfPath = OMVRpc::exec(
            "ShareMgmt",
            "getPath",
            array(
                "uuid" => $object['sharedfolderref']
            ),
            $context
        );

        $objects = array();

        foreach (self::$plugins as $plugin)
        {
            $pluginPath = sprintf("%s/openmediavault-%s", $sfPath, $plugin);
            $exists = file_exists($pluginPath);
            $version = "n/a";

            if ($exists)
            {
                $f = fopen(sprintf("%s/debian/changelog", $pluginPath), 'r');
                $version = fgets($f);
                fclose($f);
            }

            $objects[] = array(
                "name"     => $plugin,
                "fullname" => sprintf("openmediavault-%s", $plugin),
                "url"      => sprintf("https://github.com/OpenMediaVault-Plugin-Developers/openmediavault-%s", $plugin),
                "exists"   => $exists,
                "version"  => $version
            );
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getLocation($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Get configuration object
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathLocation(), $params['uuid']);
        $object = $xmlConfig->get($xPath);
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        $object['port'] = intval($object['port']);

        return $object;
    }

    public function setLocation($params, $context)
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
                "uuid"       : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID_UNDEFINED"] . ' },
                "name"       : { "type" : "string" },
                "remotehost" : { "type" : "string" },
                "remotepath" : { "type" : "string" },
                "port"       : { "type" : "integer" },
                "username"   : { "type" : "string" },
                "password"   : { "type" : "string" }
            }
        }');

        // Prepare configuration data
        $object = array(
            "uuid"            => ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"]) ? OMVUtil::uuid() : $params["uuid"],
            "name"            => $params['name'],
            "remotehost"      => $params['remotehost'],
            "remotepath"      => $params['remotepath'],
            "port"            => intval($params['port']),
            "username"        => $params['username'],
            "password"        => $params['password']
        );

        // Set the configuration object.
        $success = FALSE;
        if ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"])
        {
            // Append the configuration object.
            $success = $xmlConfig->set($this->getXPathLocations(), array("location" => $object));
        }
        else
        {
            // Update the existing configuration object.
            $xPath = sprintf("%s[uuid='%s']", $this->getXPathLocation(), $params['uuid']);
            $success = $xmlConfig->replace($xPath, $object);
        }

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePathLocation(),
            $object
        );

        return $object;
    }

    public function getLocationList($params, $context)
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

        // Get configuration data.
        $objects = $xmlConfig->getList( $this->getXPathLocation() );
        if (is_null($objects))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPathLocation()
            );
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function deleteLocation($params, $context)
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
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathLocation(), $params['uuid']);
        return $this->deleteConfigObjectByPath(
            $xPath,
            $this->getEventMessagePathLocation()
        );
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
                "command"  : { "type" : "string" },
                "plugin"   : { "type" : "string" },
                "location" : { "type" : "string", "optional" : true }
            }
        }');

        if (!in_array($params['command'], self::$commands))
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_OPERATION_DENIED,
                sprinf("Unknown command - %s", $params['command'])
            );
        }

        if ($params['plugin'] != "all")
        {
            if (!in_array($params['plugin'], self::$plugins))
            {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_OPERATION_DENIED,
                    sprinf("Unknown plugin - %s", $params['plugin'])
                );
            }
        }

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

            if ($params['plugin'] != "all")
            {
                $cmd = sprintf("omv-mkconf developer %s %s %s 2>&1", $params['command'], $params['plugin'], $params['location']);
                $this->exec($cmd, $output, $bgOutputFilename);
            }
            else
            {
                foreach (self::$plugins as $plugin)
                {
                    $cmd = sprintf("omv-mkconf developer update %s 2>&1",$plugin);
                    $this->exec($cmd, $output, $bgOutputFilename);
                }
            }
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    public function createConfig($params, $context)
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
                "command"  : { "type" : "string" }
            }
        }');

        if (!in_array($params['command'], self::$commands))
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_OPERATION_DENIED,
                sprinf("Unknown command - %s", $params['command'])
            );
        }

        // Get configuration object
        $object = $xmlConfig->get($this->getXPath());
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        $configFile = "";

        switch ($params['command'])
        {
            case "git":
                $configFile = "/etc/gitconfig";
                $config = sprintf("[user]\n\tname = %s\n\temail = %s\n", $object['gitname'], $object['gitemail']);
                file_put_contents($configFile, $config);

                chmod($configFile, 0644);
                break;

            case "gh":
                $configFile = "/root/.netrc";
                $config = sprintf("machine github.com\nlogin %s\npassword %s\nprotocol https\n", $object['ghusername'], $object['ghpassword']);

                if ( file_exists($configFile) === false)
                {
                    file_put_contents($configFile, $config);
                }
                else
                {
                    if( strpos(file_get_contents($configFile), "machine github.com") === false)
                        file_put_contents($configFile, $config, FILE_APPEND);
                }
                chmod($configFile, 0600);
                break;

            case "tx":
                $configFile = "/root/.transifexrc";
                $config = sprintf("[%s]\nhostname = %s\npassword = %s\ntoken = %s\nusername = %s\n",
                    $object['txhostname'], $object['txhostname'], $object['txpassword'],
                    $object['txtoken'], $object['txusername']);
                file_put_contents($configFile, $config);

                foreach (self::$plugins as $plugin)
                {
                    $repo = sprintf("openmediavault-%s\n", $plugin);
                    file_put_contents("/tmp/repos", $repo, FILE_APPEND);
                }

                $cmd = "omv-mkconf developer tx 2>&1";
                OMVUtil::exec($cmd, $output, $result);
                break;
        }
    }

    public function doGit($params, $context)
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
                "command" : { "type" : "string" },
                "plugin"  : { "type" : "string" },
                "commit"  : { "type" : "string", "optional" : true }
            }
        }');

        if (!in_array($params['command'], self::$commands))
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_OPERATION_DENIED,
                sprinf("Unknown command - %s", $params['command'])
            );
        }

        if (!in_array($params['plugin'], self::$plugins))
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_OPERATION_DENIED,
                sprinf("Unknown plugin - %s", $params['plugin'])
            );
        }

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

            $cmd = sprintf("omv-mkconf developer %s %s _%s 2>&1", $params['command'], $params['plugin'], $params['commit']);
            $this->exec($cmd, $output, $bgOutputFilename);

            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    public function doDch($params, $context)
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
                "command" : { "type" : "string" },
                "plugin"  : { "type" : "string" },
                "commit"  : { "type" : "string", "optional" : true }
            }
        }');

        if (!in_array($params['command'], self::$commands))
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_OPERATION_DENIED,
                sprinf("Unknown command - %s", $params['command'])
            );
        }

        if (!in_array($params['plugin'], self::$plugins))
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_OPERATION_DENIED,
                sprinf("Unknown plugin - %s", $params['plugin'])
            );
        }

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

            $cmd = sprintf("omv-mkconf developer %s %s _%s 2>&1", $params['command'], $params['plugin'], $params['commit']);
            $this->exec($cmd, $output, $bgOutputFilename);

            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    public function doOmvSvn($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if ($pid > 0)   // Parent process.
        {
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $cmd = "omv-mkconf developer omvsvn 2>&1";
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceDeveloper());
