<?php
use ICanBoogie\Inflector;

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
		'/\.info$/' => array('replace_names','replace_info'),
		'/^readme/i' => array('replace_names','replace_info'),
		'/\.php$/' => 'replace_names',
		'/\.inc$/' => 'replace_names',
		'/\.module$/' => 'replace_names',
		'/\.install$/' => 'replace_names',
		'/\.po$/' => 'replace_names',
		'/\.pot$/' => 'replace_names',
	);
	
	public function __construct($module, $destination, $op = 'mv') {
    
    include_once 'vendor/autoload.php';

    if(dirname($module) == '.') {
		$this->module = $module;
    }else{
      $this->module = basename($module);
      $this->module_path = $module;
    }
		$this->destination = $destination;
		$this->op = $op;
	}
	
	public function exec(){
    if(empty($this->module_path)){
		$module_path = DrushRenameHelper::get_project_path($this->module);
		if(!$module_path){
			return drush_set_error("The module that was specified does not exist.");
		}
		$this->module_path = $module_path;
    }
		
		$this->dest_path = DrushRenameHelper::gess_dest_path($this->module_path,$this->destination);
		$this->dest_module = basename($this->destination);
		drush_print("$this->module_path to $this->dest_path");

    if(file_exists($this->dest_path)){
      if(drush_confirm('The target folder already exists, do you want to overwrite it?')){
        if(!drush_shell_exec('rm -rf %s', $this->dest_path)){
          return drush_set_error("Could not remove old folder.");
        }
      }else{
        return;
      }
    }
		if(!drush_shell_exec($this->op.' %s %s', $this->module_path, $this->dest_path)){
			return drush_set_error("Could not copy module.");
		}
		$this->load_module_config();
		$this->check_files($this->dest_path);
	}
  
  function add_checks($additions){
    foreach ($additions as $key => $val) {
      if(empty($this->checks[$key])){
        $this->checks[$key] = $val;
      }else{
        $this->checks[$key] = array_merge($this->checks[$key],$val);
      }
    }
  }
  
  var $_cachedPrompts = array();
  function prompt($q,$cacheKey = null){
    if(!empty($cacheKey) && !empty($this->_cachedPrompts[$cacheKey])) 
      return $this->_cachedPrompts[$cacheKey];
    if(is_array($q)){
      $def = array(
        'default' => null,
        'required' => true
      );
      $q = array_merge($def,$q);
      $res = drush_prompt($q['prompt'],$q['default'],!$q['required']);
    }else{
      $res = drush_prompt($q);
    }
    if(!empty($cacheKey))
      $this->_cachedPrompts[$cacheKey] = $res;
    return $res;
  }
  
  function replace_promp($search,$q,$text,$cacheKey = null){
    if($cacheKey === true) $cacheKey = $search;
    if(strpos($text,$search) !== false){
      $replace = $this->prompt($q,$cacheKey);
      $text = str_replace($search, $replace, $text);
    }
    return $text;
  }
  
  function load_module_config(){
    $inflector = Inflector::get();
    $className = $inflector->camelize($this->module) . 'RenameOptions';
    $config_file = $this->dest_path . '/' . $this->module . '.rename.inc';
    if(file_exists($config_file)){
      include_once $config_file;
      class_exists($className);
      $this->config = new $className($this);
      if(!empty($this->config->checks)){
        $this->add_checks($this->config->checks);
      }
    }
  }
  
  public function run_check_funct($name,$file){
    if($this->config && method_exists($this->config, $name)){
      $this->config->{$name}($file);
    }else if(method_exists($this, $name)){
      $this->{$name}($file);
    }
  }
  
	public function check_files($path){
		
		$files = scandir($path);
		foreach($files as $file) {
			if(!in_array($file,array('.','..'))){
				$ffile = $path.'/'.$file;
        $ffile = $this->rename_file($ffile);
				if(is_dir($ffile)){
					$this->check_files($ffile);
				}else{
					foreach($this->checks as $reg => $functs){
						if(preg_match($reg,$file)){
							foreach ((array)$functs as $funct) {
                $this->run_check_funct($funct,$ffile);
							}
						}
					}
					drush_print($file);
				}
			}
		}
	}
  
	public function file_new_name($file){
    if($this->config && method_exists($this->config, 'file_new_name')){
      $new_file = $this->config->file_new_name($file);
      if($file != $new_file) return $new_file;
    }
    $dir = dirname($file);
    $name = basename($file);
    if(strpos($name,$this->module) !== false){
      $new_file = $dir .'/'. str_replace($this->module, $this->dest_module,$name);
      return $new_file;
    }else{
      return $file;
    }
  }
  
	public function rename_file($file){
    $new_file = $this->file_new_name($file);
    if($file != $new_file){
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
	public function replace_info($file){
    $inflector = Inflector::get();
    $def = $inflector->humanize($this->dest_module); 
    
		$str=file_get_contents($file);
		$str=$this->replace_promp('[todo] name', array('prompt'=>'Module Human Name','default'=>$def),$str,true);
		$str=$this->replace_promp('[todo] description', array('prompt'=>'Module description','default'=>$def),$str,true);
		file_put_contents($file, $str);
	}
}
?>