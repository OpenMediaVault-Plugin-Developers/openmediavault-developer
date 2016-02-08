<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2015 OpenMediaVault Plugin Developers
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
    private static $commands = [
        "build","update","upload","install","reset",
        "tx","buildpot","pushpot","pullpo",
        "git","gh","add","commit","push","tag","status","diff",
        "dchi","dcha","dchr","changelog"
    ];

    private static $plugins = [
        "acestream","active-directory","anacron","autoshutdown",
        "backup","btsync",
        "calibre","couchpotato","cups",
        "ddclient","deluge","developer","dnsmasq","docker",
            "docker-gui","downloader",
        "emby","extplorer",
        "fail2ban","flashmemory",
        "gateone","greyhole",
        "headphones","hpraid",
        "jdownloader",
        "kerberos",
        "letsencrypt","links","locate","luksencryption",
        "mcmyadmin","mediabrowser","minidlna","mpd","mumble",
            "mylar","mysql",
        "nginx","nzbget","nzbdrone",
        "offlineimap","omvextrasorg","openvpn","openvpnas",
        "plexmediaserver","postgresql","pxe","pyload",
        "remotedesktop","remoteshare","roundcube","rsnapshot",
        "sabnzbd","sensors","shellinabox","sickbeard","skeleton",
            "snapraid","sonarr","subsonic","syncthing",
        "teamspeak3","transmissionbt",
        "unionfilesystems",
        "vdr","vdr-extras","vdr-vnsiserver","virtualbox",
        "wakealarm","webdav","websites","wol","wordpress",
        "zfs"
    ];

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

    /**
     * Do a curl API call with given url
     *
     * @param string $url The URL to use in the call
     *
     * @return string
     */
    private function _doApiCall($url)
    {
        $curl = curl_init();
        curl_setopt_array(
            $curl, [
                CURLOPT_RETURNTRANSFER => 1,
                CURLOPT_TIMEOUT => 30,
                CURLOPT_CONNECTTIMEOUT => 5
            ]
        );
        curl_setopt($curl, CURLOPT_URL, $url);
        if (!($response = curl_exec($curl))) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                'Error: "' . curl_error($curl) . '" - Code: ' . curl_errno($curl)
            );
        }
        curl_close($curl);
        return $response;
    }

    private function _createTree(&$list, $parent){
        $tree = [];
        foreach ($parent as $k=>$l){
            if(isset($list[$l['id']])){
                $l['leaf'] = false;
                $l['data'] = $this->_createTree($list, $list[$l['id']]);
            } else {
                $l['leaf'] = true;
            }
            $tree[] = $l;
        }
        return $tree;
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
        $this->registerMethod("getBranches");
        $this->registerMethod("doChangeBranch");
        $this->registerMethod("doDch");
        $this->registerMethod("doOmvSvn");

        $this->registerMethod("getBintrayRepos");
        $this->registerMethod("addBintrayRepo");
        $this->registerMethod("addBintrayPackage");
        $this->registerMethod("getBintrayFiles");
        $this->registerMethod("publishFileBintray");
        $this->registerMethod("syncBintrayData");
    }

    public function getSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
                        "txusername"      : { "type" : "string", "optional" : true },
                        "btusername"      : { "type" : "string", "optional" : true },
                        "btapikey"        : { "type" : "string", "optional" : true },
                        "btgpgpass"       : { "type" : "string", "optional" : true }
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
    $object = [
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
        "btusername"      => $params['btusername'],
        "btapikey"        => $params['btapikey'],
        "btgpgpass"       => $params['btgpgpass'],
        "locations"       => isset($oldObject['locations']) ? $oldObject['locations'] : [],
        "repos"           => isset($oldObject['repos']) ? $oldObject['repos'] : [],
        "packages"        => isset($oldObject['packages']) ? $oldObject['packages'] : []
    ];

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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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

    if (!empty($object['sharedfolderref']))
    {
        $sfPath = OMVRpc::exec(
            "ShareMgmt",
            "getPath",
            [ "uuid" => $object['sharedfolderref'] ],
            $context
        );
    }
    $objects = [];

    foreach (self::$plugins as $plugin)
    {
        $pluginPath = sprintf("%s/openmediavault-%s", $sfPath, $plugin);
        $exists = file_exists($pluginPath);
        $version = "n/a";

        if ($exists)
        {
            if (file_exists(sprintf("%s/control", $pluginPath)))
            {
                foreach (new SplFileObject(sprintf("%s/control", $pluginPath)) as $line)
                {
                    if (preg_match("/^Version:/", $line))
                    {
                        $version = trim($line);
                        break;
                    }
                }
            }
            else
            {
                $f = fopen(sprintf("%s/debian/changelog", $pluginPath), 'r');
                $version = fgets($f);
                fclose($f);
            }
        }

        $objects[] = [
            "name"     => $plugin,
            "fullname" => sprintf("openmediavault-%s", $plugin),
            "url"      => sprintf("https://github.com/OpenMediaVault-Plugin-Developers/openmediavault-%s", $plugin),
            "exists"   => $exists,
            "version"  => $version
        ];
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
    $object = [
        "uuid"            => ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"]) ? OMVUtil::uuid() : $params["uuid"],
        "name"            => $params['name'],
        "remotehost"      => $params['remotehost'],
        "remotepath"      => $params['remotepath'],
        "port"            => intval($params['port']),
        "username"        => $params['username'],
        "password"        => $params['password']
    ];

    // Set the configuration object.
    $success = FALSE;
    if ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"])
    {
        // Append the configuration object.
        $success = $xmlConfig->set($this->getXPathLocations(), [ "location" => $object ]);
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
                "properties" : {
                    "command" : { "type" : "string" },
                        "plugin"  : { "type" : "string" },
                        "tag"  : { "type" : "string", "optional" : "true" },
                        "message"  : { "type" : "string", "optional" : "true" },
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

        $cmd = sprintf("omv-mkconf developer %s %s _%s _%s _%s2>&1",
            $params['command'], $params['plugin'], $params['commit'], $params['tag'], $params['message']);
        $this->exec($cmd, $output, $bgOutputFilename);

        $this->finalizeBgProcStatus($bgStatusFilename, $output);
        exit(0);
    } catch(Exception $e) {
        $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
        exit(1);
    }
    }

    public function doChangeBranch($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
                "properties" : {
                    "command" : { "type" : "string" },
                        "name"    : { "type" : "string" },
                        "branch"  : { "type" : "string", "optional" : "true" }
    }
    }');

    if (!in_array($params['name'], self::$plugins))
    {
        throw new OMVException(
            OMVErrorMsg::E_MISC_OPERATION_DENIED,
            sprinf("Unknown plugin - %s", $params['name'])
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

        $cmd = sprintf("omv-mkconf developer %s %s %s2>&1",
            "changebranch", $params['name'], $params['branch']);
        $this->exec($cmd, $output, $bgOutputFilename);

        $this->finalizeBgProcStatus($bgStatusFilename, $output);
        exit(0);
    } catch(Exception $e) {
        $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
        exit(1);
    }
    }

    public function getBranches($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        // Get configuration object
        $objects = [];
        $cmd = sprintf("omv-mkconf developer branches %s", $params['plugin']);
        exec($cmd, $out);

        foreach ($out as $branch) {
            $objects[] = [
                "branch" => trim(strstr($branch, " "))
            ];
        }
        return $objects;
    }

    public function doDch($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
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

    /**
     * Retrieve all Bintray repos and packages that the user has access to
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getBintrayRepos($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        $settings = $xmlConfig->get($this->getXpath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $objects = [];
        $reposXpath = $this->getXpath() . "/repos";
        $packagesXpath = $this->getXpath() . "/packages";
        $repos = $xmlConfig->getList($reposXpath . "/*");
        if (count($repos) === 0) {
            $this->syncBintrayData($params, $context);
            $repos = $xmlConfig->getList($reposXpath . "/*");
        }
        foreach ($repos as $repo) {
            array_push(
                $objects,
                [
                    "parentid" => "root",
                    "id" => $repo['name'],
                    "type" => "Repo",
                    "name" => $repo['name'],
                    "icon" => "images/folder.png"
                ]
            );
            $packages = $xmlConfig->getList($packagesXpath . "/package[repo='" . $repo['name'] . "']");
            foreach ($packages as $package) {
                array_push(
                    $objects,
                    [
                        "parentid" => $repo['name'],
                        "id" => $repo['name'] . "-" . $package['name'],
                        "type" => "Package",
                        "name" => $package['name'],
                        "icon" => "images/puzzle.png"
                    ]
                );
            }
        }
        $new = [];
        foreach ($objects as $a){
            $new[$a['parentid']][] = $a;
        }
        $tree['data'] = $this->_createTree($new, $new['root']);
        return $tree;
    }

    /**
     * Create a new Bintray repo
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function addBintrayRepo($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        $settings = $xmlConfig->get($this->getXpath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $url = "https://api.bintray.com/repos/openmediavault-plugin-developers/" . $params['repo'];
        $cmd = "curl -H \"Content-Type: application/json\" -u" . $settings['btusername'] . ":" . $settings['btapikey'] . " " .
            "-X POST -d '{ \"type\": \"debian\", \"private\": false, \"premium\": false, \"desc\": \"" . $params['desc'] . "\" }' " .
            $url;

        OMVutil::exec($cmd, $out, $res);

        if (!(preg_match('/^\{.*\"created\".*\}$/', $out[0]))) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                $out[0]
            );
        }
        $reposXpath = $this->getXpath() . "/repos";
        $object = [ "name" => $params['repo'] ];
        if (false === $xmlConfig->set($reposXpath, [ "repo" => $object ])) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXpath()
            );
        }
    }

    /**
     * Create a new Bintray package
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function addBintrayPackage($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        $settings = $xmlConfig->get($this->getXpath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        if (!empty($settings['sharedfolderref'])) {
            $sfPath = OMVRpc::exec(
                "ShareMgmt",
                "getPath",
                [ "uuid" => $settings['sharedfolderref'] ],
                $context
            );
        }

        $pluginPath = sprintf("%s/%s", $sfPath, $params['name']);
        $desc = "";
        if (file_exists(sprintf("%s/debian/control", $pluginPath))) {
            $data = rtrim(file_get_contents(sprintf("%s/debian/control", $pluginPath)));
            $lines = explode("\n", $data);
            for ($i = 0; $i < count($lines); $i++) {
                if (preg_match('/^Description:(.*)$/', $lines[$i], $matches)) {
                    $desc .= trim($matches[1]) . "\n";
                    for ($j = $i+1; $j < count($lines); $j++) {
                        if (preg_match('/^[a-zA-Z]+:/', $lines[$j])) {
                            break;
                        } else {
                            $desc .= trim($lines[$j]) . "\n";
                        }
                    }
                    break;
                }
            }
        }
        $desc = trim($desc);

        $url = "https://api.bintray.com/packages/openmediavault-plugin-developers/" . $params['repo'];
        $cmd = "curl -H \"Content-Type: application/json\" -u" . $settings['btusername'] . ":" . $settings['btapikey'] . " " .
            "-X POST -d '{ \"name\": \"" . $params['name'] . "\", \"desc\": \"" . $desc . "\", \"licenses\": [\"GPL-3.0\"], " .
            "\"vcs_url\": \"https://github.com/OpenMediaVault-Plugin-Developers/" . $params['name'] . ".git\", " .
            "\"github_repo\": \"OpenMediaVault-Plugin-Developers/" . $params['name'] . "\", " .
            "\"public_download_numbers\": true }' " .
            $url;

        OMVutil::exec($cmd, $out, $res);

        if (!(preg_match('/^\{.*\"created\".*\}$/', $out[0]))) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                $out[0]
            );
        }
        $packagesXpath = $this->getXpath() . "/packages";
        $object = [
            "name" => $params['name'],
            "repo" => $params['repo']
        ];
        if (false === $xmlConfig->set($packagesXpath, [ "package" => $object ])) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXpath()
            );
        }
    }

    /**
     * Publish a file to Bintray
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An associative array with all the new settings
     */
    public function publishFileBintray($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        $settings = $xmlConfig->get($this->getXpath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        if (!empty($settings['sharedfolderref'])) {
            $sfPath = OMVRpc::exec(
                "ShareMgmt",
                "getPath",
                [ "uuid" => $settings['sharedfolderref'] ],
                $context
            );
        }

        if (!(preg_match('/^.*_([\d]+\.[\d\.]+)_(.*)\.deb$/', $params['file'], $matches))) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                "Failed to get data from filename:" .
                $params['file']
            );

        }
        $version = $matches[1];
        $arch = $matches[2];

        if ($arch == "all") {
            if (!empty($params['arch'])) {
                $arch = trim($params['arch']);
            }
        }

        $url = "https://api.bintray.com/content/openmediavault-plugin-developers/" .
            $params['repo'] . "/" . $params['bpackage'] .
            "/" . $version . "/" . "pool/main/" . substr($params['bpackage'], 0, 1) .
            "/" . $params['bpackage'] . "/" . $params['file'];

        $cmd = "curl -X PUT -T " . $sfPath . "/" . $params['file'] . " " .
            "-H 'X-GPG-PASSPHRASE: " . $settings['btgpgpass'] . "' " .
            "-H 'X-Bintray-Debian-Distribution: " . $params['dist'] . "' " .
            "-H 'X-Bintray-Debian-Component: main' " .
            "-H 'X-Bintray-Debian-Architecture: " . $arch . "' " .
            "-H 'X-Bintray-Publish: 1' " .
            "-u" . $settings['btusername'] . ":" . $settings['btapikey'] . " " . $url;

        OMVutil::exec($cmd, $out, $res);

        if (!(strcmp($out[0], "{}") === 0)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                $out[0]
            );
        }
    }

    /**
     * Sync local data with Bintray
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An associative array with all the new settings
     */
    public function syncBintrayData($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        $settings = $xmlConfig->get($this->getXpath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $reposXpath = $this->getXpath() . "/repos";
        $packagesXpath = $this->getXpath() . "/packages";
        $xmlConfig->delete($reposXpath);
        $xmlConfig->delete($packagesXpath);
        if (false === $xmlConfig->set($this->getXpath(), [ "repos" => [] ])) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXpath()
            );
        }
        if (false === $xmlConfig->set($this->getXpath(), [ "packages" => [] ])) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $url = "https://api.bintray.com/repos/openmediavault-plugin-developers";
        $repodata = json_decode($this->_doApiCall($url));
        foreach ($repodata as $repo) {
            $object = [ "name" => $repo->name ];
            if (false === $xmlConfig->set($reposXpath, [ "repo" => $object ])) {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                    $this->getXpath()
                );
            }
            $url = "https://api.bintray.com/repos/openmediavault-plugin-developers/" . $repo->name . "/packages";
            $packagedata = json_decode($this->_doApiCall($url));
            foreach ($packagedata as $package) {
                $object = [
                    "name" => $package->name,
                    "repo" => $repo->name
                ];
                if (false === $xmlConfig->set($packagesXpath, [ "package" => $object ])) {
                    throw new OMVException(
                        OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                        $this->getXpath()
                    );
                }
            }
        }
    }

    /**
     * Retrieve all files in location specified by package
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getBintrayFiles($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            [ "role" => OMV_ROLE_ADMINISTRATOR ]
        );

        // Get configuration object
        $settings = $xmlConfig->get($this->getXPath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        if (!empty($settings['sharedfolderref'])) {
            $sfPath = OMVRpc::exec(
                "ShareMgmt",
                "getPath",
                [ "uuid" => $settings['sharedfolderref'] ],
                $context
            );
        }

        $objects = [];

        $files = scandir($sfPath);
        foreach ($files as $file) {
            if (preg_match('/^' . $params['bpackage'] . '.*.deb$/', $file)) {
                if (!(is_dir($sfPath . "/" . $file))) {
                    array_push(
                        $objects,
                        [
                            "filename" => $file,
                            "fullpath" => $sfPath . "/" . $file
                        ]
                    );
                }
            }
        }

        return $objects;
    }

}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceDeveloper());
