<?php
/**
 * Impements hook_drush_command().
 *
 * @ return
 *  An associative array describing commands.
 */

function rename_drush_command () {
  $items = array();

  $items['move-module'] = array(
	'description' => 'Move and Renames a module and its hooks.',
	'arguments' => array(
	  'module' => 'The module that will be renamed.',
	  'destination' => 'The full destination folder for the new module',
	),
	'options' => array(
	),
	'aliases' => array('mmv'),
	'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  $items['copy-module'] = array(
	'description' => 'Copy and Renames a module and its hooks.',
	'options' => array(
	  'module' => 'The module that will be renamed.',
	  'destination' => 'The full destination folder for the new module',
	),
	'aliases' => array('mcp'),
	'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'. This hook is optional. If a command
 * does not implement this hook, the command's description is used instead.
 *
 * This hook is also used to look up help metadata, such as help
 * category title and summary.  See the comments below for a description.
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function rename_drush_help($section) {
  switch ($section) {
	case 'drush:module-rename':
	  return dt('Renames a module and its hooks. Usage is drush module-rename --module=fun --name=work');
  }
}



function drush_rename_move_module($module = NULL, $destination = NULL) {
	$p = new DrushRenameProcess($module, $destination, $op = 'mv');
	return $p->exec();
}
function drush_rename_copy_module($module = NULL, $destination = NULL) {
	$p = new DrushRenameProcess($module, $destination, $op = 'cp -R');
	return $p->exec();
}

/**
 * Run the actual renaming.
 *
 * The function name should be same as command name but with dashes turned to
 * underscores and 'drush_commandfile_' prepended, where 'commandfile' is
 * taken from the file 'commandfile.drush.inc', which in this case is 'sandwich'.
 * Note also that a simplification step is also done in instances where
 * the commandfile name is the same as the beginning of the command name,
 * "drush_example_example_foo" is simplified to just "drush_example_foo".
 * To also implement a hook that is called before your command, implement
 * "drush_hook_pre_example_foo".  For a list of all available hooks for a
 * given command, run drush in --debug mode.
 *
 * @see drush_invoke()
 * @see drush.api.php
 */
function drush_rename_module($module, $destination, $op = 'mv') {
	
	if (!empty($destination) && !is_dir(destination)) {
	  drush_mkdir($destination);
	}
	else {
	  drush_print('The destination directory for the new module already exists, moving on.');
	}

	$path = drupal_get_path('module', $options['module']);
	$files = scandir($path);

	$name = array_reverse(explode('/', $options['destination']));

	foreach($files as $file) {
	  $filename = explode('.', $file);
	  if (is_dir($options['destination']) && !empty($filename[1])) {
		drush_shell_cd_and_exec($path, 'cp %s %s/%s', $file, $options['destination'], $filename[0] . '.' . $filename[1]);
		//drush_print(print_r($filename));
	  }
	}
  
}

class DrushRenameHelper {
	
	
	static function get_project_path($module){
		$module_path = drupal_get_path('module', $module);
		if(!$module_path){
			$module_path = drupal_get_path('theme', $module);
		}
		return $module_path;
	}
	static function gess_dest_path($module_path,$destination){
		if(preg_match('/[\/\\\\]/',$destination)){
			return $destination;
		}
		return dirname($module_path) . '/' . $destination;
	}
	
}

class DrushRenameProcess {
	
	var $checks = array(
		'/\.php$/' => 'replace_names',
		'/\.inc$/' => 'replace_names',
		'/\.module$/' => 'replace_names',
		'/\.info$/' => 'replace_names',
	);
	
	public function __construct($module, $destination, $op = 'mv') {
		$this->module = $module;
		$this->destination = $destination;
		$this->op = $op;
	}
	
	public function exec(){
		$module_path = DrushRenameHelper::get_project_path($this->module);
		if(!$module_path){
			return drush_set_error("The module that was specified does not exist.");
		}
		$this->module_path = $module_path;
		
		$this->dest_path = DrushRenameHelper::gess_dest_path($module_path,$this->destination);
		$this->dest_module = basename($this->destination);
		drush_print("$module_path to $this->dest_path");

		if(!drush_shell_exec($this->op.' %s %s', $this->module_path, $this->dest_path)){
			return drush_set_error("Could not copy module.");
		}
		
		$this->check_files($this->dest_path);
	}
	public function check_files($path){
		
		$files = scandir($path);
		foreach($files as $file) {
			if(!in_array($file,array('.','..'))){
				$ffile = $path.'/'.$file;
				if(is_dir($ffile)){
					$this->check_files($ffile);
				}else{
          $ffile = $this->rename_file($ffile);
					foreach($this->checks as $reg => $functs){
						if(preg_match($reg,$file)){
							foreach ((array)$functs as $funct) {
								$this->{$funct}($ffile);
							}
						}
					}
					drush_print($file);
				}
			}
		}
	}
	public function rename_file($file){
    $dir = dirname($file);
    $name = basename($file);
    if(strpos($name,$this->module) !== false){
      $new_file = $dir .'/'. str_replace($this->module, $this->dest_module,$name);
      if(!drush_shell_exec('mv %s %s', $file, $new_file)){
        return drush_set_error("rename file : $file.");
      }
      return $new_file;
    }
    return $file;
  }
	public function replace_names($file){
		$str=file_get_contents($file);
		$str=str_replace($this->module, $this->dest_module,$str);
		file_put_contents($file, $str);
	}
}


?>
