<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2020 OpenMediaVault Plugin Developers
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
    private $omvDeveloper = "/usr/sbin/omv-developer";

    private $packageCache = "/var/cache/openmediavault/dev_package_cache";
    private $pluginCache  = "/var/cache/openmediavault/dev_plugin_cache";
    private $repoCache    = "/var/cache/openmediavault/dev_repo_cache";

    private $plugins = [];

    public function getName()
    {
        return "Developer";
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getPluginList");

        $this->registerMethod("doCommand");
        $this->registerMethod("createConfig");
        $this->registerMethod("doGit");
        $this->registerMethod("getBranches");
        $this->registerMethod("doChangeBranch");
        $this->registerMethod("doDch");
        $this->registerMethod("doOmvGit");
        $this->registerMethod("doResetCache");
    }

    public function getSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.developer");
        // Return the configuration object.
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
            $pluginPath = sprintf("%s/%s", $sfPath, $plugin);
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
                } elseif (file_exists(sprintf("%s/debian/changelog", $pluginPath))) {
                    $f = fopen(sprintf("%s/debian/changelog", $pluginPath), 'r');
                    $version = fgets($f);
                    fclose($f);
                } else {
                    $version = 'n/a';
                }
                if (file_exists(sprintf("%s/.git/HEAD", $pluginPath))) {
                    $f = fopen(sprintf("%s/.git/HEAD", $pluginPath), 'r');
                    $branch = explode("/", fgets($f));
                    fclose($f);
                }
            }
            $objects[] = [
                "name"     => $plugin,
                "url"      => sprintf("%s/%s", $this->githubURL, $plugin),
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
            $list = [];
            for ($i = 1; $i <= 3; $i++) {
                $json = file_get_contents(
                    'https://api.github.com/orgs/openmediavault-plugin-developers/repos?per_page=100&page=' . $i,
                    false, $context
                );
                $list = array_merge($list, json_decode($json));
            }
            // Get repos info and put in cache file
            file_put_contents($this->pluginCache, json_encode($list));
        }
        $this->plugins = [];
        $data = json_decode(file_get_contents($this->pluginCache), true);
        foreach ($data as $plugin) {
            if (empty($plugin))
                continue;
            $this->plugins[] = $plugin["name"];
        }
        asort($this->plugins);
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
                    $cmdArgs[] = $params['command'];
                    $cmdArgs[] = $params['plugin'];
                    $cmdArgs[] = sprintf("\"%s\"", $params['location']);
                    $cmd = new \OMV\System\Process($this->omvDeveloper, $cmdArgs);
                    $cmd->setRedirect2to1(true);
                    $cmdLine = $cmd->getCommandLine();
                    $this->exec($cmdLine, $output, $bgOutputFilename);
                } else {
                    $this->fillPlugins();
                    foreach ($this->plugins as $plugin) {
                        $cmdArgs = [];
                        $cmdArgs[] = "update";
                        $cmdArgs[] = $plugin;
                        $cmd = new \OMV\System\Process($this->omvDeveloper, $cmdArgs);
                        $cmd->setRedirect2to1(true);
                        $cmdLine = $cmd->getCommandLine();
                        $this->exec($cmdLine, $output, $bgOutputFilename);
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
                exec("omv-salt deploy run --quiet netrc", $output);
                break;

            case "tx":
                $configFile = "/root/.transifexrc";
                $config = '[https://www.transifex.com]\napi_hostname = https://api.transifex.com';
                file_put_contents($configFile, $config);

                $this->fillPlugins();
                file_put_contents("/root/.repos", implode(PHP_EOL, $this->plugins));

                $cmd = sprintf("export lang=c; %s tx", $this->omvDeveloper);
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
                $cmdArgs[] = $params['command'];
                $cmdArgs[] = $params['plugin'];
                $cmdArgs[] = sprintf("\"%s\"", $params['commit']);
                $cmdArgs[] = sprintf("\"%s\"", $params['tag']);
                $cmdArgs[] = sprintf("\"%s\"", $params['message']);
                $cmd = new \OMV\System\Process($this->omvDeveloper, $cmdArgs);
                $cmdLine = $cmd->getCommandLine();
                $this->exec($cmdLine, $output, $bgOutputFilename);
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
                $cmdArgs[] = "changebranch";
                $cmdArgs[] = $params['plugin'];
                $cmdArgs[] = trim(str_replace("*", "", $params['branch']));
                $cmd = new \OMV\System\Process($this->omvDeveloper, $cmdArgs);
                $cmdLine = $cmd->getCommandLine();
                $this->exec($cmdLine, $output, $bgOutputFilename);
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
        $cmd = sprintf("%s branches %s", $this->omvDeveloper, $params['plugin']);
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
                $cmdArgs[] = $params['command'];
                $cmdArgs[] = $params['plugin'];
                $cmdArgs[] = sprintf("\"%s\"", $params['commit']);
                $cmd = new \OMV\System\Process($this->omvDeveloper, $cmdArgs);
                $cmdLine = $cmd->getCommandLine();
                $this->exec($cmdLine, $output, $bgOutputFilename);
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
                $cmd = new \OMV\System\Process($this->omvDeveloper, "omvgit");
                $cmdLine = $cmd->getCommandLine();
                $this->exec($cmdLine, $output, $bgOutputFilename);
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
}
