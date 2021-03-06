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
  var $version = '0.1.0';
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
  var $outputBuffering = false;
  
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
        if(file_exists($this->dest_path.'/rename_choices')){
          $this->lastChoices = $this->load_choices();
          $choices_loaded = true;
          if(drush_confirm('Do you want to use the same settings as the last time ?')){
            $this->choices = $this->lastChoices;
          }
        }
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
    if(!empty($choices_loaded)){
      $this->save_choices();
    }
    $this->load_module_config();
    $this->check_files($this->dest_path);
    $this->save_choices();
  }
  
  function lastChoice($key,$def=null){
    if(!empty($this->lastChoices)){
      if(is_array($key)){
        $p = $key;
      }else{
        if(array_key_exists($key,$this->lastChoices)) return $this->lastChoices[$key];
        $p = explode('.',$key);
      }
      if(drupal_array_nested_key_exists($this->lastChoices, $p)) return drupal_array_get_nested_value($this->lastChoices, $p);
    }
    return $def;
  }
  
  function load_choices(){
    $choices = array();
    include $this->dest_path.'/rename_choices';
    return $choices;
  }
  
  function save_choices(){
    $content = '<?php' . "\n" .
      '  $renameVersion = \'' . $this->version . "';\n" .
      '  $template = \'' . $this->module . "';\n" .
      '  $templateVersion = \'' . $this->config->version . "';\n" .
      '  $choices = ' . $this->indent(var_export($this->choices,true),1) . ";\n" .
      '?>';
    file_put_contents( $this->dest_path.'/rename_choices' , $content);
  }
  
  function add_checks($additions){
    foreach ($additions as $key => $val) {
      if(empty($this->checks[$key])){
        $this->checks[$key] = $val;
      }else{
        $this->checks[$key] = array_merge((array)$this->checks[$key],(array)$val);
      }
    }
  }
  
  var $choices = array();
  function prompt($q,$cacheKey = null){
    if(!empty($cacheKey) && !empty($this->choices[$cacheKey])) 
      return $this->choices[$cacheKey];
    if($this->outputBuffering){
      $tmp = ob_get_clean();
    }
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
    if($this->outputBuffering){
      ob_start(); 
      echo $tmp;
    }
    if(!empty($cacheKey))
      $this->choices[$cacheKey] = $res;
    return $res;
  }
  
  function output($txt){
    if($this->outputBuffering){
      $tmp = ob_get_clean();
    }
    drush_print($txt);
    if($this->outputBuffering){
      ob_start(); 
      echo $tmp;
    }
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
      unlink($config_file);
    }else{
      $this->config = new RenameOptions($this);
    }
  }
  
  public function run_check_funct($name,$file){
    if(method_exists($this->config, $name)){
      $this->config->{$name}($file);
    }else if(method_exists($this, $name)){
      $this->{$name}($file);
    }
  }
  
  public function check_files($path){
    $files = scandir($path);
        drush_print_r($this->checks);
    foreach($files as $file) {
      $ffile = $path.'/'.$file;
      if(in_array($file,array('.git'))){
        if(!drush_shell_exec('rm -rf %s', $ffile)){
          return drush_set_error("Could not remove : ".$ffile);
        }
      }elseif(!in_array($file,array('.','..'))){
        
        $ffile = $this->rename_file($ffile);
        if(is_dir($ffile)){
          $this->check_files($ffile);
        }else{
          if(preg_match('/\.rename\.tpl$/',$file)){
            $rendered = $this->renderTemplate($ffile);
            $ffile = $rendered;
            $file = basename($rendered);
          }
          if(empty($rendered) or $this->config->checkRendered){
          foreach($this->checks as $reg => $functs){
            if(preg_match($reg,$file)){
              foreach ((array)$functs as $funct) {
                $this->run_check_funct($funct,$ffile);
              }
            }
          }
          }
          drush_print($file);
        }
      }
    }
  }
  
  public function renderTemplate($file){
    $inflector = Inflector::get();
    $this->outputBuffering = true;
    ob_start(); 
    include $file;
    $content = ob_get_clean();
    $this->outputBuffering = false;
    $content = str_replace(array('<!--?php','?-->'),array('<?php','?>'),$content);
    $newFile = preg_replace('/\.rename\.tpl$/','',$file);
    file_put_contents( $newFile , $content );
    unlink($file);
    return $newFile;
  }
  
  public function file_new_name($file){
    if(method_exists($this->config, 'file_new_name')){
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
    $str=$this->replace_promp('[todo] name', array('prompt'=>'Module Human Name','default'=>$this->lastChoice('title',$def)),$str,'title');
    $str=$this->replace_promp('[todo] description', array('prompt'=>'Module description','default'=>$this->lastChoice('description',$def)),$str,'description');
    file_put_contents($file, $str);
  }
  
  function indent($txt,$level=1,$space='  '){
    return str_replace("\n","\n".str_repeat($space , $level),$txt);
  }
  function arrayKeysExport($arr){
    return preg_replace('/^array\s*\(\n(.*)\n\)$/s','\1',var_export($arr,true));
  }
  
}
class RenameOptions {
  
    var $checks = array();
    var $checkRendered = false;
    var $version = '0.0.1';
    
    function __construct($process) {
      $this->process = $process;
    }
}
?>