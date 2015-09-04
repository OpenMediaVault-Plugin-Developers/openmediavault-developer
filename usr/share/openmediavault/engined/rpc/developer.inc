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
    private static $commands = array("build","update","upload","buildpot","pushpot",
        "pullpo","install","reset","tx","gh","git","add","commit","push",
        "dchi","dcha","dchr","changelog","status");

    private static $plugins = array("acestream","active-directory","anacron","aufs",
        "autoshutdown","backup","btsync","calibre","couchpotato","cups","ddclient","deluge",
        "developer","dnsmasq","docker","docker-gui","downloader","emby","extplorer","fail2ban",
        "flexget","flashmemory","folding","gateone","git","glusterfs","greyhole","headphones",
        "hpraid","jdownloader","kerberos","links","locate","mcmyadmin","mediabrowser","mhddfs",
        "minidlna","mumble","mylar","mysql","nginx","nzbget","nzbdrone","offlineimap","omvextrasorg",
        "openvpn","openvpnas","openvpn-old","plexmediaserver","postgresql","pptp","processlist",
        "pxe","pyload","remotedesktop","remotenfs-share","remoteshare","roundcube","rsnapshot",
        "sabnzbd","sensors","shellinabox","sickbeard","skeleton","snapraid","sonarr","spideroak",
        "subsonic","subversion","supportinfo","syncthing","teamspeak3","themes","transmissionbt",
        "unionfilesystems","vdr","vdr-extras","virtualbox","virtualhosts","wakealarm",
        "webdav","website","websites","wol","wordpress","zfs");

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
            $curl, array(
                CURLOPT_RETURNTRANSFER => 1,
                CURLOPT_TIMEOUT => 30,
                CURLOPT_CONNECTTIMEOUT => 5
            )
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

        $this->registerMethod("getBintrayRepoList");
        $this->registerMethod("getBintrayPackageList");
        $this->registerMethod("getBintrayFileList");
        $this->registerMethod("getBintrayPackages");
        $this->registerMethod("getBintrayPackage");
        $this->registerMethod("setBintrayPackage");
        $this->registerMethod("deleteBintrayPackage");
        $this->registerMethod("publishBintrayFile");
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
        "btusername"      => $params['btusername'],
        "btapikey"        => $params['btapikey'],
        "btgpgpass"       => $params['btgpgpass'],
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

    if (!empty($object['sharedfolderref']))
    {
        $sfPath = OMVRpc::exec(
            "ShareMgmt",
            "getPath",
            array(
                "uuid" => $object['sharedfolderref']
            ),
            $context
        );
    }
    $objects = array();

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

    /**
     * Retrieve all Bintray repos that the user has access to
     * 
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getBintrayRepoList($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );

        $settings = $xmlConfig->get($this->getXpath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $objects = array();
        $url = "https://api.bintray.com/repos/" . $settings['btusername'];
        $data = json_decode($this->_doApiCall($url));
        foreach ($data as $repo) {
            if (preg_match('/^' . $params['omvversion'] . '.*$/', $repo->name)) {
                array_push(
                    $objects,
                    array("repository" => $repo->name)
                );
            }
        }
        return $objects;
    }

    /**
     * Retrieve all Bintray packages in a specific repo
     * 
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getBintrayPackageList($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );

        $settings = $xmlConfig->get($this->getXpath());
        if (is_null($settings)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $objects = array();
        $url = "https://api.bintray.com/repos/" .
            "openmediavault-plugin-developers/" .
            $params['repository'] . "/packages";
        $data = json_decode($this->_doApiCall($url));
        foreach ($data as $package) {
            array_push(
                $objects,
                array("bpackage" => $package->name)
            );
        }
        return $objects;
    }

    /**
     * Retrieve all files in location specified by package
     * 
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getBintrayFileList($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
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
                array(
                    "uuid" => $settings['sharedfolderref']
                ),
                $context
            );
        }

        $objects = array();
        $package = $xmlConfig->get($this->getXpath() . "/btpackages/btpackage[uuid='" . $params['uuid'] . "']");
        if (is_null($package)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $files = scandir($sfPath);
        foreach ($files as $file) {
            if (preg_match('/^' . $package['bpackage'] . '.*.deb$/', $file)) {
                if (!(is_dir($sfPath . "/" . $file))) {
                    array_push(
                        $objects,
                        array(
                            "filename" => $file,
                            "fullpath" => $sfPath . "/" . $file
                        )
                    );
                }
            }
        }

        return $objects;
    }


    /**
     * Retrieve all Bintray packages
     * 
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getBintrayPackages($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );

        $packages = $xmlConfig->getList($this->getXpath() . "/btpackages/btpackage");
        if (is_null($packages)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        return $packages;
    }

    /**
     * Retrieve a package object
     * 
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getBintrayPackage($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );

        $objects = array();
        $package = $xmlConfig->get($this->getXpath() . "/btpackages/btpackage[uuid='" . $params['uuid'] . "']");
        if (is_null($package)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        return $package;
    }

    /**
     * Set a package object
     * 
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An associative array with all the new settings
     */
    public function setBintrayPackage($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );
        $this->validateMethodParams(
            $params,
            '{' .
            '"type": "object",' .
            '"properties": {' .
            '"uuid": { "type": "string" },' .
            '"omvversion": { "type": "string" },' .
            '"repository": { "type": "string" },' .
            '"bpackage": { "type": "string" },' .
            '"dist": { "type": "string" },' .
            '"arch": { "type": "string" }' .
            '}}'
        );

        $version = "n/a";
        $object = array(
            "uuid" => (strcmp($params['uuid'], "undefined") === 0) ? OMVUtil::uuid() : $params['uuid'],
            "omvversion" => $params['omvversion'],
            "repository" => $params['repository'],
            "bpackage" => $params['bpackage'],
            "dist" => $params['dist'],
            "arch" => $params['arch']
        );

        if (strcmp($params['uuid'], "undefined") === 0) {
            //New package
            if (false === $xmlConfig->set($this->getXpath() . "/btpackages", array("btpackage" => $object))) {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                    $this->getXpath()
                );
            }
        } else {
            //Update existing package
            $xpath = $this->getXpath() . "/btpackages/btpackage[uuid='" . $object['uuid'] . "']";
            $oldObject = $xmlConfig->get($xpath);
            if (is_null($oldObject)) {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                    $xpath
                );
            }
            if (false === $xmlConfig->replace($xpath, $object)) {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                    $this->getXpath()
                );
            }
        }

        return $object;
    }

    /**
     * Delete a package object
     * 
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An associative array with all the new settings
     */
    public function deleteBintrayPackage($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );
        $this->validateMethodParams(
            $params,
            '{' .
            '"type": "object",' .
            '"properties": {' .
            '"uuid": { "type": "string" }' .
            '}}'
        );

        $xpath = $this->getXpath() . "/packages/package[uuid='" . $params['uuid'] . "']";
        if (false === $xmlConfig->delete($xpath)) {
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
    public function publishBintrayFile($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );
        $this->validateMethodParams(
            $params,
            '{' .
            '"type": "object",' .
            '"properties": {' .
            '"uuid": { "type": "string" },' .
            '"file": { "type": "string" }' .
            '}}'
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
                array(
                    "uuid" => $settings['sharedfolderref']
                ),
                $context
            );
        }

        $package = $xmlConfig->get($this->getXpath() . "/btpackages/btpackage[uuid='" . $params['uuid'] . "']");
        if (is_null($package)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        if (!(preg_match('/^.*_([\d]+\.[\d\.]+).*$/', $params['file'], $matches))) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                "Failed to get version from filename:" .
                $params['file']
            );

        }
        $version = $matches[1];

        //Can't get php curl to work with authentication...
        /*
        $url = "https://api.bintray.com/content/openmediavault-plugin-developers/" .
            $package['repository'] . "/" . $package['bpackage'] .
            "/" . $version . "/" . "pool/main/" . substr($package['bpackage'], 0, 1) .
            "/" . $package['bpackage'] . "/" . $params['file'] . ";deb_distribution=" .
            $package['dist'] . ";deb_component=main;deb_architecture=" . $package['arch'] .
            ";publish=1";

        $curl_options = array(
            CURLOPT_RETURNTRANSFER => 1,
            CURLOPT_TIMEOUT => 30,
            CURLOPT_CONNECTTIMEOUT => 5,
            CURLOPT_PUT => true,
            CURLOPT_INFILE => $package['srcpath'] . "/" . $params['file'],
            CURLOPT_INFILESIZE => filesize($package['srcpath'] . "/" .$params['file']),
            CURLOPT_HEADER => true,
            CURLOPT_HTTPHEADER => array(
                'X-GPG-PASSPHRASE: ' . $settings['gpgpass'],
                "Authorization: Basic " . base64_encode($settings['username'] . ":" . $settings['apikey'])
            )
        );


        $curl = curl_init();
        curl_setopt_array($curl, $curl_options);
        curl_setopt($curl, CURLOPT_URL, $url);
        if (!($response = curl_exec($curl))) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                'Error: "' . curl_error($curl) . '" - Code: ' . curl_errno($curl)
            );
        }
        curl_close($curl);
         */

        $url = "https://api.bintray.com/content/openmediavault-plugin-developers/" .
            $package['repository'] . "/" . $package['bpackage'] .
            "/" . $version . "/" . "pool/main/" . substr($package['bpackage'], 0, 1) .
            "/" . $package['bpackage'] . "/" . $params['file'];

        $cmd = "curl -X PUT -T " . $sfPath . "/" . $params['file'] . " " .
            "-H 'X-GPG-PASSPHRASE: " . $settings['btgpgpass'] . "' " .
            "-H 'X-Bintray-Debian-Distribution: " . $package['dist'] . "' " .
            "-H 'X-Bintray-Debian-Component: main' " .
            "-H 'X-Bintray-Debian-Architecture: " . $package['arch'] . "' " .
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
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceDeveloper());
