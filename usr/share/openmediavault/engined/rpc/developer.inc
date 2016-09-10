<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2016 OpenMediaVault Plugin Developers
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
require_once("openmediavault/globals.inc");
require_once("openmediavault/functions.inc");

class OMVRpcServiceDeveloper extends \OMV\Rpc\ServiceAbstract
{
    private $packageCache = "/var/cache/openmediavault/dev_package_cache";
    private $pluginCache  = "/var/cache/openmediavault/dev_plugin_cache";
    private $repoCache    = "/var/cache/openmediavault/dev_repo_cache";

    private $plugins = [];

    public function getName()
    {
        return "Developer";
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
        $this->registerMethod("doOmvGit");
        $this->registerMethod("doResetCache");

        $this->registerMethod("getBintrayRepos");
        $this->registerMethod("addBintrayRepo");
        $this->registerMethod("addBintrayPackage");
        $this->registerMethod("getBintrayFiles");
        $this->registerMethod("publishFileBintray");
        $this->registerMethod("syncBintrayData");
    }

    public function getSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer");

        // Remove useless properties from the object.
        $object->remove("locations");

        return $object->getAssoc();
    }

    public function setSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.setsettings");

        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer");
        $object->setAssoc($params);
        $db->set($object);

        // Remove useless properties from the object.
        $object->remove("locations");

        // Return the configuration object.
        return $object->getAssoc();
    }

    public function getPluginList($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.getlist");

        // Get the configuration objects.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer");

        // Remove useless properties from the object.
        $object->remove("locations");

        $sfPath = "/tmp/";

        if (!empty($object->get("sharedfolderref"))) {
            $sfObject = $db->get("conf.system.sharedfolder", $object->get("sharedfolderref"));

            // Get the mount point configuration object to build the absolute shared folder path.
            $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));

            // Return the absolute shared folder path.
            $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
        }

        // Add plugins.
        $objects = [];
        $this->fillPlugins();
        foreach ($this->plugins as $plugin) {
            $pluginPath = sprintf("%s/openmediavault-%s", $sfPath, $plugin);
            $exists  = file_exists($pluginPath);
            $version = "n/a";
            $branch  = [ "", "", "n/a" ];

            if ($exists) {
                if (file_exists(sprintf("%s/control", $pluginPath))) {
                    foreach (new SplFileObject(sprintf("%s/control", $pluginPath)) as $line) {
                        if (preg_match("/^Version:/", $line)) {
                            $version = trim($line);
                            break;
                        }
                    }
                } else {
                    $f = fopen(sprintf("%s/debian/changelog", $pluginPath), 'r');
                    $version = fgets($f);
                    fclose($f);
                }
                if (file_exists(sprintf("%s/.git/HEAD", $pluginPath))) {
                    $f = fopen(sprintf("%s/.git/HEAD", $pluginPath), 'r');
                    $branch = explode("/", fgets($f));
                    fclose($f);
                }
            }
            $objects[] = [
                "name"     => $plugin,
                "fullname" => sprintf("openmediavault-%s", $plugin),
                "url"      => sprintf("%s/openmediavault-%s", $this->githubURL, $plugin),
                "exists"   => $exists,
                "branch"   => $branch[2],
                "version"  => $version
            ];
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    private function fillPlugins()
    {
        // Get current time
        $time = time();

        // if the cache doesn't exist or is older than 1 day, download it
        if ( !file_exists($this->pluginCache) or
           ( $time - filemtime($this->pluginCache) >= 60*60*24) ) {
            // Github requires an user-agent:
            //   https://developer.github.com/v3/#user-agent-required
            $context = stream_context_create([
                'http' => ['user_agent' => 'openmediavault-plugin-developers'],
            ]);
            $json = file_get_contents(
                'https://api.github.com/orgs/openmediavault-plugin-developers/repos?per_page=100',
                false, $context
            );
            // Get repos info and put in cache file
            file_put_contents($this->pluginCache, $json);
        }
        $this->plugins = [];
        $data = json_decode(file_get_contents($this->pluginCache), true);
        foreach ($data as $plugin) {
            if (empty($plugin))
                continue;
            if (substr($plugin["name"],0,15) !== "openmediavault-")
                continue;
            $this->plugins[] = str_replace("openmediavault-","",$plugin["name"]);
        }
        asort($this->plugins);
    }

    public function getLocation($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.objectuuid");

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();

        return $db->getAssoc("conf.service.developer.location", $params['uuid']);
    }

    public function setLocation($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.setlocation");

        // Prepare the configuration object.
        $object = new \OMV\Config\ConfigObject("conf.service.developer.location");
        $object->setAssoc($params);

        // Set the configuration object.
        $isNew = $object->isNew();
        $db = \OMV\Config\Database::getInstance();
        if (TRUE === $isNew) {
            // Check uniqueness.
            $db->assertIsUnique($object, "name");
        }
        $db->set($object);

        // Return the configuration object.
        return $object->getAssoc();
    }

    public function getLocationList($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.getlist");

        // Get the list of configuration objects.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->getAssoc("conf.service.developer.location");

        // Filter the result objects.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function deleteLocation($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.objectuuid");

        // Delete the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer.location", $params['uuid']);
        $db->delete($object);

        // Return the deleted configuration object.
        return $object->getAssoc();
    }

    public function doCommand($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.docommand");

        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($params) {
                if ($params['plugin'] != "all") {
                    $cmdArgs = [];
                    $cmdArgs[] = "developer";
                    $cmdArgs[] = $params['command'];
                    $cmdArgs[] = $params['plugin'];
                    $cmdArgs[] = sprintf("\"%s\"", $params['location']);
                    $cmd = new \OMV\System\Process("omv-mkconf", $cmdArgs);
                    $cmd->setRedirect2to1(true);
                    $cmdLine = $cmd->getCommandLine();
                    if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename))
                        throw new \OMV\ExecException($cmdLine, $output);
                } else {
                    $this->fillPlugins();
                    foreach ($this->plugins as $plugin) {
                        $cmdArgs = [];
                        $cmdArgs[] = "developer";
                        $cmdArgs[] = "update";
                        $cmdArgs[] = $plugin;
                        $cmd = new \OMV\System\Process("omv-mkconf", $cmdArgs);
                        $cmd->setRedirect2to1(true);
                        $cmdLine = $cmd->getCommandLine();
                        if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename))
                            throw new \OMV\ExecException($cmdLine, $output);
                    }
                }
                return $output;
            }
        );
    }

    public function createConfig($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.createconfig");

        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer");

        // Remove useless properties from the object.
        $object->remove("locations");

        $configFile = "";

        switch ($params['command']) {
            case "git":
                $configFile = "/etc/gitconfig";
                $config = sprintf("[user]\n\tname = %s\n\temail = %s\n",
                    $object->get("gitname"), $object->get("gitemail")
                );
                file_put_contents($configFile, $config);

                chmod($configFile, 0644);
                break;

            case "gh":
                $configFile = "/root/.netrc";
                $config = sprintf("machine github.com\nlogin %s\npassword %s\nprotocol https\n",
                    $object->get("ghusername"), $object->get("ghpassword")
                );

                if ( file_exists($configFile) === false) {
                    file_put_contents($configFile, $config);
                } else {
                    if ( strpos(file_get_contents($configFile), "machine github.com") === false )
                        file_put_contents($configFile, $config, FILE_APPEND);
                }
                chmod($configFile, 0600);
                break;

            case "tx":
                $configFile = "/root/.transifexrc";
                $config = sprintf("[%s]\nhostname = %s\npassword = %s\ntoken = %s\nusername = %s\n",
                    $object->get("txhostname"), $object->get("txhostname"),$object->get("txpassword"),
                    $object->get("txtoken"), $object->get("txusername")
                );
                file_put_contents($configFile, $config);

                $this->fillPlugins();
                file_put_contents("/root/.repos", implode(PHP_EOL, $this->plugins));

                $cmd = "export lang=c; omv-mkconf developer tx";
                exec($cmd, $output);
                break;
        }
    }

    public function doGit($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.dogit");

        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($params) {
                $cmdArgs = [];
                $cmdArgs[] = "developer";
                $cmdArgs[] = $params['command'];
                $cmdArgs[] = $params['plugin'];
                $cmdArgs[] = sprintf("\"%s\"", $params['commit']);
                $cmdArgs[] = sprintf("\"%s\"", $params['tag']);
                $cmdArgs[] = sprintf("\"%s\"", $params['message']);
                $cmd = new \OMV\System\Process("omv-mkconf", $cmdArgs);
                $cmd->setRedirect2to1(true);
                $cmdLine = $cmd->getCommandLine();
                if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename))
                    throw new \OMV\ExecException($cmdLine, $output);
                return $output;
            }
        );
    }

    public function doChangeBranch($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.changebranch");

        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($params) {
                $cmdArgs = [];
                $cmdArgs[] = "developer";
                $cmdArgs[] = "changebranch";
                $cmdArgs[] = $params['plugin'];
                $cmdArgs[] = trim(str_replace("*", "", $params['branch']));
                $cmd = new \OMV\System\Process("omv-mkconf", $cmdArgs);
                $cmd->setRedirect2to1(true);
                $cmdLine = $cmd->getCommandLine();
                if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename))
                    throw new \OMV\ExecException($cmdLine, $output);
                return $output;
            }
        );
    }

    public function getBranches($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.getbranches");

        // Get configuration object
        $objects = [];
        $cmd = sprintf("omv-mkconf developer branches %s", $params['plugin']);
        exec($cmd, $output);

        foreach ($output as $branch) {
            $objects[] = [
                "branch" => trim($branch)
            ];
        }
        return $objects;
    }

    public function doDch($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.developer.dodch");

        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($params) {
                $cmdArgs = [];
                $cmdArgs[] = "developer";
                $cmdArgs[] = $params['command'];
                $cmdArgs[] = $params['plugin'];
                $cmdArgs[] = sprintf("\"%s\"", $params['commit']);
                $cmd = new \OMV\System\Process("omv-mkconf", $cmdArgs);
                $cmd->setRedirect2to1(true);
                $cmdLine = $cmd->getCommandLine();
                if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename))
                    throw new \OMV\ExecException($cmdLine, $output);
                return $output;
            }
        );
    }

    public function doOmvGit($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($params) {
                $cmd = new \OMV\System\Process("omv-mkconf", "developer", "omvgit");
                $cmd->setRedirect2to1(true);
                $cmdLine = $cmd->getCommandLine();
                if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename))
                    throw new \OMV\ExecException($cmdLine, $output);
                return $output;
            }
        );
    }

    public function doResetCache($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        unlink($this->pluginCache);
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
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $packages = [];
        $repos = [];

        // Get the existing configuration object.
        if (file_exists($this->packageCache)) {
            $packages = explode(PHP_EOL, rtrim(file_get_contents($this->packageCache), PHP_EOL));
        }
        if (file_exists($this->repoCache)) {
            $repos = explode(PHP_EOL, rtrim(file_get_contents($this->repoCache), PHP_EOL));
        }
        if (count($repos) === 0) {
            $this->syncBintrayData($params, $context);
            $packages = explode(PHP_EOL, rtrim(file_get_contents($this->packageCache), PHP_EOL));
            $repos = explode(PHP_EOL, rtrim(file_get_contents($this->repoCache), PHP_EOL));
        }

        $objects = [];

        foreach ($repos as $repo) {

            array_push(
                $objects,
                [
                    'parentid' => 'root',
                    'id' => $repo,
                    'type' => 'Repo',
                    'name' => $repo,
                    'icon' => 'images/folder.png'
                ]
            );
            foreach ($packages as $package) {
                $item = explode(",", $package);
                if ($item[1] == $repo) {
                    array_push(
                        $objects,
                        [
                            'parentid' => $repo,
                            'id' => sprintf('%s-%s', $repo, $item[0]),
                            'type' => 'Package',
                            'name' => $item[0],
                            'icon' => 'images/puzzle.png'
                        ]
                    );
                }
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
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.developer');
        $object->remove('locations');

        $url = "https://api.bintray.com/repos/openmediavault-plugin-developers/" . $params['repo'];
        $cmd = "curl -H \"Content-Type: application/json\" -u" . $object->get('btusername') . ":" . $object->get('btapikey') . " " .
            "-X POST -d '{ \"type\": \"debian\", \"private\": false, \"premium\": false, \"desc\": \"" . $params['desc'] . "\" }' " .
            $url;
        exec($cmd, $output);

        if (!(preg_match('/^\{.*\"created\".*\}$/', $output[0]))) {
            throw new \OMV\Exception($output[0]);
        }
        $fp = fopen($this->repoCache, "w");
        fwrite($fp, $params['name'] . PHP_EOL);
        fclose($fp);
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
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.developer');
        $object->remove('locations');

        if (!empty($object->get('sharedfolderref'))) {
            $sfObject = $db->get('conf.system.sharedfolder', $object->get('sharedfolderref'));

            // Get the mount point configuration object to build the absolute shared folder path.
            $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));

            // Return the absolute shared folder path.
            $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
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
        $cmd = "curl -H \"Content-Type: application/json\" -u" . $object->get('btusername') . ":" . $object->get('btapikey') . " " .
            "-X POST -d '{ \"name\": \"" . $params['name'] . "\", \"desc\": \"" . $desc . "\", \"licenses\": [\"GPL-3.0\"], " .
            "\"vcs_url\": \"https://github.com/OpenMediaVault-Plugin-Developers/" . $params['name'] . ".git\", " .
            "\"github_repo\": \"OpenMediaVault-Plugin-Developers/" . $params['name'] . "\", " .
            "\"public_download_numbers\": true }' " .
            $url;
        exec($cmd, $output);

        if (!(preg_match('/^\{.*\"created\".*\}$/', $output[0]))) {
            throw new \OMV\Exception($output[0]);
        }
        $fp = fopen($this->packageCache, "w");
        fwrite($fp, sprintf("%s,%s\n", $params['name'], $params['repo']));
        fclose($fp);
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
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.developer');
        $object->remove('locations');

        if (!empty($object->get("sharedfolderref"))) {
            $sfObject = $db->get("conf.system.sharedfolder", $object->get("sharedfolderref"));

            // Get the mount point configuration object to build the absolute shared folder path.
            $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));

            // Return the absolute shared folder path.
            $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
        }

        if (!(preg_match('/^.*_(.*)_(.*)\.deb$/', $params['file'], $matches))) {
            throw new \OMV\Exception(
                "Failed to get data from filename:" . $params['file']
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
            "-H 'X-GPG-PASSPHRASE: " . $object->get('btgpgpass') . "' " .
            "-H 'X-Bintray-Debian-Distribution: " . $params['dist'] . "' " .
            "-H 'X-Bintray-Debian-Component: main' " .
            "-H 'X-Bintray-Debian-Architecture: " . $arch . "' " .
            "-H 'X-Bintray-Publish: 1' " .
            "-u" . $object->get('btusername') . ":" . $object->get('btapikey') . " " . $url;
        exec($cmd, $output);

        if (!(strcmp($output[0], "{}") === 0)) {
            throw new \OMV\Exception($output[0]);
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
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer");

        // Remove useless properties from the object.
        $object->remove("locations");

        if (file_exists($this->packageCache)) {
            unlink($this->packageCache);
        }
        if (file_exists($this->repoCache)) {
            unlink($this->repoCache);
        }

        touch($this->packageCache);
        touch($this->repoCache);

        $fp1 = fopen($this->repoCache, "w");
        $fp2 = fopen($this->packageCache, "w");

        $url = "https://api.bintray.com/repos/openmediavault-plugin-developers";
        $repodata = json_decode($this->_doApiCall($url));
        foreach ($repodata as $repo) {
            fwrite($fp1, $repo->name . PHP_EOL);

            $url = "https://api.bintray.com/repos/openmediavault-plugin-developers/" . $repo->name . "/packages";
            $packagedata = json_decode($this->_doApiCall($url));
            foreach ($packagedata as $package) {
                fwrite($fp2, sprintf("%s,%s\n", $package->name, $repo->name));
            }
        }
        fclose($fp1);
        fclose($fp2);
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
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer");

        if (!empty($object->get("sharedfolderref"))) {
            $sfObject = $db->get("conf.system.sharedfolder", $object->get("sharedfolderref"));

            // Get the mount point configuration object to build the absolute shared folder path.
            $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));

            // Return the absolute shared folder path.
            $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
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
