# Drush Rename Module

This module allows you to renames  or copy a module and its hooks via Drush.

## Installation

The module must by placed in a folder named `content_sections` inside `sites/all/modules` or `sites/default/modules` 
```sh
drush en drush_rename
```

## Exemples

```sh
  drush mmv module_v1 module_v2   # Move and Renames a module and its hooks.
  drush mcp module1 module2       # Copy and Renames a module and its hooks.
  drush mfg mfg git@github.com:kevthunder/my_blank_module.git baz   # Clone a gitted module and Renames it along its hooks. Usage is "drush mfg git@github.com:foo/bar.git baz"
```