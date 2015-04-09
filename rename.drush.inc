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
      'destination' => 'The destination folder for the new module',
	),
	'options' => array(
	),
	'aliases' => array('mmv'),
	'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  $items['copy-module'] = array(
	'description' => 'Copy and Renames a module and its hooks.',
    'arguments' => array(
	  'module' => 'The module that will be renamed.',
      'destination' => 'The destination folder for the new module',
    ),
    'options' => array(
	),
	'aliases' => array('mcp'),
	'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  $items['module-form-git'] = array(
    'description' => 'Clone a gitted module and Renames it along its hooks.',
    'arguments' => array(
      'git_url' => 'The git reprository.',
      'destination' => 'The destination folder for the new module',
    ),
    'options' => array(
      'projectname' => 'The project name of the gitted module',
    ),
    'aliases' => array('mfg'),
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
  $help = array(
    'drush:move-module' => dt('Move and Renames a module and its hooks. Usage is "drush mmv fun work"'),
    'drush:copy-module' => dt('Copy and Renames a module and its hooks. Usage is "drush mcp fun work"'),
    'drush:module-form-git' => dt('Clone a gitted module and Renames it along its hooks. Usage is "drush mfg git@github.com:foo/bar.git baz"')
  );
  if(!empty($help[$section])) return $help[$section];
}



function drush_rename_move_module($module = NULL, $destination = NULL) {
  include_once 'rename_process.inc';
	$p = new DrushRenameProcess($module, $destination, $op = 'mv');
	return $p->exec();
}
function drush_rename_copy_module($module = NULL, $destination = NULL) {
  include_once 'rename_process.inc';
	$p = new DrushRenameProcess($module, $destination, $op = 'cp -R');
	return $p->exec();
}

function drush_rename_module_form_git($repo = NULL, $destination = NULL) {
  $folder = (dirname($destination) == '.' ? 'sites/default/modules' : dirname($destination));
  $projectname = drush_get_option('projectname', basename($repo,'.git'));
  if(!drush_shell_exec('git archive --format=tar --prefix=%s/ --remote=%s master | tar -xf - -C %s',$projectname,$repo,$folder)){
    return drush_set_error("Could not get the source.");
  }
  include_once 'rename_process.inc';
	$p = new DrushRenameProcess($folder.'/'.$projectname, $destination, $op = 'mv');
	return $p->exec();
}


?>
