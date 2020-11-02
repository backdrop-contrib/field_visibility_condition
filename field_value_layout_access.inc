<?php
/**
 * @file
 * Plugin to provide access control/visibility based on date and time.
 */
class FieldValueLayoutAccess extends LayoutAccessNegatable {

  /**
   * {@inheritdoc}
   */
  function summary() {
    $all_plugins = $this->field_value_layout_access_get_children(array(), 'field_value_layout_access');
    $plugin = $all_plugins[$this->settings['plugin_name']];

    list($parent, $entity_type, $bundle_type, $field_name) = explode(':', $plugin['name']);
    $instances   = field_info_instances($entity_type, $bundle_type);
    $instance    = $instances[$field_name];
    $field       = field_info_field($instance['field_name']);

    $entity_info = entity_get_info($entity_type);
    $entity      = (object)array(
      $entity_info['entity keys']['bundle'] => $bundle_type,
    );
    $keys   = array();
    $value_keys = array();
    $keyed_elements = array();

    foreach ($field['columns'] as $column => $attributes) {
      $settings_key = _field_sql_storage_columnname($field_name, $column);

      $keyed_elements["@{$column}_value"] = array();

      if (isset($this->settings[$settings_key])) {
        if (is_array($this->settings[$settings_key])) {
          $i = 0;
          foreach ($this->settings[$settings_key] as $this->settings_value) {
            if (!is_array($this->settings_value)) {
              $entity->{$field_name}[LANGUAGE_NONE][$i][$column] = $this->settings_value;
              $keyed_elements["@{$column}_value"][$i] = array('#markup' => $this->settings_value);
            }
            elseif (isset($this->settings_value[$column])) {
              $entity->{$field_name}[LANGUAGE_NONE][$i][$column] = $this->settings_value[$column];
              $keyed_elements["@{$column}_value"][$i] = array('#markup' => $this->settings_value[$column]);
            }
            $i++;
          }
        }
        else {
          $entity->{$field_name}[LANGUAGE_NONE][0][$column] = $this->settings[$settings_key];
          $keyed_elements["@{$column}_value"][0] = array('#markup' => $this->settings[$settings_key]);
        }
      }

      $keys['@' . $column] = $column;
      $value_keys[] = "@{$column}_value";
    }
    $elements = array();
    $items = isset($entity->{$field_name}[LANGUAGE_NONE]) ? $entity->{$field_name}[LANGUAGE_NONE] : array();
    $view_mode = 'full';

    $display = field_get_display($instance, $view_mode, $entity);
    if (!isset($display['module'])) {
      $display['module'] = $field['module'];
    }
    if (isset($display['module'])) {
      // Choose simple formatter for well known cases.
      switch ($display['module']) {
        case 'text':
          $display['type'] = 'text_default';
          break;

        case 'list':
          $display['type'] = 'list_default';
          if ($field['type'] == 'list_boolean') {
            $allowed_values = list_allowed_values($field, $instance, $entity_type, $entity);
            foreach ($items as $item) {
              if (isset($allowed_values[$item['value']])) {
                if ($allowed_values[$item['value']] == '') {
                  $display['type'] = 'list_key';
                  break;
                }
              }
              else {
                $display['type'] = 'list_key';
              }
            }
          }
          break;

        case 'taxonomy':
          $display['type'] = 'taxonomy_term_reference_plain';
          break;

        case 'entityreference':
          $display['type'] = 'entityreference_label';
          break;

        default:
          // Use field instance formatter setting.
          break;
      }

      $function = $display['module'] . '_field_formatter_view';
      if (function_exists($function)) {
        $entity_group = array(0 => $entity);
        $item_group = array(0 => $items);
        $instance_group = array(0 => $instance);
        field_default_prepare_view($entity_type, $entity_group, $field, $instance_group, LANGUAGE_NONE, $item_group, $display);
        $elements = $function($entity_type, $entity, $field, $instance, LANGUAGE_NONE, $item_group[0], $display);
      }
    }
    if (count($elements) > 0) {
      foreach ($field['columns'] as $column => $attributes) {
        if (count($field['columns']) == 1) {
          $keyed_elements["@{$column}_value"] = $elements;
        }
      }
    }
    $values = array();
    foreach ($value_keys as $key) {
      $output = array();
      $elements = $keyed_elements[$key];
      if (is_array($elements)) {
        foreach ($elements as $element_key => $element) {
          if (is_numeric($element_key)) {
            $value_str = strip_tags(backdrop_render($element));
            if (strlen($value_str) > 0) {
              $output[] = $value_str;
            }
          }
        }
      }
      else {
        $value_str = strip_tags(backdrop_render($elements));
        if (strlen($value_str) > 0) {
          $output[] = $value_str;
        }
      }
      $value = implode(', ', $output);
      if ($value !== '') {
        $values[$key] = implode(', ', $output);
      }
    }
    $string = '';
    $value_count = count($values);
    foreach ($keys as $key_name => $column) {
      if (isset($values[$key_name . '_value'])) {
        $string .= ($value_count > 1) ? " @{$column} = @{$column}_value" : "@{$column}_value";
      }
    }
    return t('@field is set to "!value"', array('@field' => $instance['label'], '!value' => format_string($string, array_merge($keys, $values))));
  }

  /**
   * {@inheritdoc}
   */
  function checkAccess() {
    $all_plugins = $this->field_value_layout_access_get_children(array(), 'field_value_layout_access');
    $plugin = $all_plugins[$this->settings['plugin_name']];

    list($parent, $entity_type, $bundle_type, $field_name) = explode(':', $plugin['name']);

    $context = $this->contexts[$entity_type];
    if ((!is_object($context)) || (empty($context->data))) {
      // If the context doesn't exist -- for example, a newly added entity
      // reference is used as a pane visibility criteria -- we deny access.
      return FALSE;
    }

    if ($field_items = field_get_items($entity_type, $context->data, $field_name)) {
      $langcode = field_language($entity_type, $context->data, $field_name);
      // Get field storage columns.
      $instance = field_info_instance($entity_type, $field_name, $bundle_type);
      $field = field_info_field($instance['field_name']);
      $columns = array();
      foreach ($field['columns'] as $column => $attributes) {
        $columns[$column] = _field_sql_storage_columnname($field_name, $column);
      }

      if (isset($this->settings[$field_name])) {
        // We have settings for this field.
        $this->settings_value_array = $this->field_value_layout_access_get_settings($this->settings[$field_name], $langcode);
        if (empty($this->settings_value_array)) {
          return FALSE;
        }

        // Check field value.
        foreach ($field_items as $field_value) {
          // Iterate through this->settingsig values.
          foreach ($this->settings_value_array as $this->settings_value) {
            $match = FALSE;
            foreach ($field_value as $field_column => $value) {
              // Check access only for stored in this->settingsig column values.
              if (isset($this->settings_value[$field_column])) {
                if ($value == $this->settings_value[$field_column]) {
                  $match = TRUE;
                }
                else {
                  $match = FALSE;
                  break;
                }
              }
            }
            if ($match) {
              return TRUE;
            }
          }
        }
        return FALSE;
      }
    }

    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);

    $all_plugins = $this->field_value_layout_access_get_children(array(), 'field_value_layout_access');
    $options = array();
    foreach ($all_plugins as $plugins) {
      $options[$plugins['name']] = $plugins['title'];
    }
    if (isset($form_state['values']['plugin'])) {
      $form_state['plugin'] = $all_plugins[$form_state['values']['plugin']];
    }
    elseif (!empty($this->settings['plugin_name'])) {
      $form_state['plugin'] = $all_plugins[$this->settings['plugin_name']];
    }
    else {
      $form_state['plugin'] = reset($all_plugins);
    }
    $plugin = $form_state['plugin'];

    list($parent, $entity_type, $bundle_type, $field_name) = explode(':', $plugin['name']);
    $form['plugin'] = array(
      '#type' => 'select',
      '#title' => t('Fields'),
      '#default_value' => $plugin['name'],
      '#options' => $options,
      '#description' => t('Only the checked roles will be granted access.'),
      '#ajax' => array(
        'wrapper' => 'plugin_form',
        'callback' => array($this, 'field_value_plugin_form_ajax'),
      ),
    );
    $form['plugin_form'] = array(
      '#type' => 'container',
      '#id' => 'plugin_form',
      '#parents' => array('plugin_form'),
    );

    $entity_info = entity_get_info($entity_type);
    $instances   = field_info_instances($entity_type, $bundle_type);
    $instance    = $instances[$field_name];
    $field       = field_info_field($instance['field_name']);
    foreach ($field['columns'] as $column => $attributes) {
      $columns[$column] = _field_sql_storage_columnname($field_name, $column);
    }

    $entity = (object) array(
      $entity_info['entity keys']['bundle'] => $bundle_type,
    );

    foreach ($columns as $column => $sql_column) {
      if (isset($this->settings[$sql_column])) {
        if (is_array($this->settings[$sql_column])) {
          foreach ($this->settings[$sql_column] as $delta => $this->settings_value) {
            if (is_numeric($delta)) {
              if (is_array($this->settings_value)) {
                $entity->{$field_name}[LANGUAGE_NONE][$delta][$column] = $this->settings_value[$column];
              }
              else {
                $entity->{$field_name}[LANGUAGE_NONE][$delta][$column] = $this->settings_value;
              }
            }
          }
        }
        else {
          $entity->{$field_name}[LANGUAGE_NONE][0][$column] = $this->settings[$sql_column];
        }
      }
    }

    $form['#parents'] = array('settings');
    $langcode = field_valid_language(NULL);
    $form['plugin_form']['content'] = (array) $this->field_value_layout_access_field_invoke_field($instance, 'form', $entity_type, $entity, $form, $form_state, array('default' => TRUE, 'language' => $langcode));
    // Weight is really not important once this is populated and will only interfere with the form layout.
    // foreach (element_children($form['settings']) as $element) {
      // unset($form['settings'][$element]['#weight']);
    // }
  }

  /**
   * AJAX callback to update the style settings within the block configure form.
   */
  function field_value_plugin_form_ajax($form, $form_state) {
    return $form['condition_settings']['content']['plugin_form'];
  }

  function formSubmit($form, &$form_state) {
    parent::formSubmit($form, $form_state);

    $plugin = $form_state['plugin'];
    list($parent, $entity_type, $bundle_type, $field_name) = explode(':', $plugin['name']);
    $langcode  = field_valid_language(NULL);
    $langcode  = isset($form_state['input']['settings'][$field_name][$langcode]) ? $langcode : LANGUAGE_NONE;
    $instances = field_info_instances($entity_type, $bundle_type);
    $instance  = $instances[$field_name];
    $field     = field_info_field($instance['field_name']);
    foreach ($field['columns'] as $column => $attributes) {
      $columns[$column] = _field_sql_storage_columnname($field_name, $column);
    }
    $items = $this->field_value_layout_access_get_proper_form_items($field, $form_state['values'][$field_name][$langcode], array_keys($columns));
    foreach ($columns as $column => $sql_column) {
      $column_items = $this->field_value_layout_access_filter_items_by_column($items, $column);
      $this->settings[$sql_column] = $column_items;
    }
    
    $this->settings[$field_name][$langcode] = $items;
    $this->settings['plugin_name'] = $plugin['name'];
  }

  function field_value_layout_access_get_child($plugin, $parent, $child) {
    $plugins = &backdrop_static(__FUNCTION__, array());
    if (empty($plugins[$parent . ':' . $child])) {
      list($entity_type, $bundle_type, $field_name) = explode(':', $child);
      $plugins[$parent . ':' . $child] = $this->field_value_layout_access_get_get_child($plugin, $parent, $entity_type, $bundle_type, $field_name);
    }

    return $plugins[$parent . ':' . $child];
  }

  function field_value_layout_access_get_children($plugin, $parent) {
    $plugins = &backdrop_static(__FUNCTION__, array());
    if (!empty($plugins)) {
      return $plugins;
    }
    $entities = entity_get_info();
    foreach ($entities as $entity_type => $entity) {
      foreach ($entity['bundles'] as $bundle_type => $bundle) {
        foreach (field_info_instances($entity_type, $bundle_type) as $field_name => $field) {
          if (!isset($plugins[$parent . ':' . $entity_type . ':' . $bundle_type . ':' . $field_name])) {
            $plugin = $this->field_value_layout_access_get_get_child($plugin, $parent, $entity_type, $bundle_type, $field_name, $entity, $bundle, $field);
            $plugins[$parent . ':' . $entity_type . ':' . $bundle_type . ':' . $field_name] = $plugin;
          }
        }
      }
    }

    return $plugins;
  }

  function field_value_layout_access_get_get_child($plugin, $parent, $entity_type, $bundle_type, $field_name, $entity = NULL, $bundle = NULL, $field = NULL) {
    // Check that the entity, bundle and field arrays have a value.
    // If not, load theme using machine names.
    if (empty($entity)) {
      $entity = entity_get_info($entity_type);
    }

    if (empty($bundle)) {
      $bundle = $entity['bundles'][$bundle_type];
    }

    if (empty($field)) {
      $field_instances = field_info_instances($entity_type, $bundle_type);
      $field = $field_instances[$field_name];
    }

    $plugin['title'] = t('@entity @type: @field Field', array('@entity' => $entity['label'], '@type' => $bundle_type, '@field' => $field['label']));
    $plugin['keyword'] = $entity_type;
    $plugin['description'] = t('Control access by @entity entity bundle.', array('@entity' => $entity_type));
    $plugin['name'] = $parent . ':' . $entity_type . ':' . $bundle_type . ':' . $field_name;
    $plugin['required context'] = array();//layout_create_context(t(ucfirst($entity_type)), $entity_type, array(
      // 'type' => $bundle_type,
    // ));

    return $plugin;
  }

  function field_value_layout_access_get_proper_form_items($field, $form_items, $columns) {
    $items = array();

    // Single value item.
    if (!is_array($form_items)) {
      foreach ($columns as $column) {
        $items[0][$column] = $form_items;
      }
      return $items;
    }

    foreach ($form_items as $delta => $value) {
      $item = array();
      if (is_numeric($delta)) { // Array of field values.
        if (!is_array($value)) {  // Single value in array.
          foreach ($columns as $column) {
            $item[$column] = $value;
          }
        }
        else { // Value has colums.
          foreach ($columns as $column) {
            $item[$column] = isset($value[$column]) ? $value[$column] : '';
          }
        }
      }
      $items[] = $item;
    }

    // Check if $form_items is an array of columns.
    $item = array();
    $has_columns = FALSE;
    foreach ($columns as $column) {
      if (isset($form_items[$column])) {
        $has_columns = TRUE;
        $item[$column] = $form_items[$column];
      }
      else {
        $item[$column] = '';
      }
    }
    if ($has_columns) {
      $items[] = $item;
    }

    // Remove empty values.
    $items = _field_filter_items($field, $items);
    return $items;
  }

  function field_value_layout_access_filter_items_by_column($items, $column) {
    $column_items = array();
    foreach ($items as $delta => $values) {
      $column_items[$delta] = isset($values[$column]) ? $values[$column] : '';
    }
    return $column_items;
  }

  /**
   * Check for access.
   */

  function field_value_layout_access_get_settings($values, $langcode = LANGUAGE_NONE) {
    if (!is_array($values) || !isset($values[$langcode])) {
      return NULL;
    }
    $settings_values = array();

    foreach ($values[$langcode] as $delta => $value) {
      $settings_values[$delta] = $value;
    }

    return $settings_values;
  }

  function field_value_layout_access_field_invoke_field($field_name, $op, $entity_type, $entity, &$a = NULL, &$b = NULL, $options = array()) {
    if (is_array($field_name)) {
      $instance = $field_name;
      $field = empty($field_name['field']) ? field_info_field($instance['field_name']) : $field_name['field'];
      $field_name = $instance['field_name'];
    }
    else {
      list(, , $bundle) = entity_extract_ids($entity_type, $entity);
      $instance = field_info_instance($entity_type, $field_name, $bundle);
    }
    if (empty($instance)) {
      return;
    }

    // Merge default options.
    $default_options = array(
      'default' => FALSE,
      'deleted' => FALSE,
      'language' => NULL,
    );
    $options += $default_options;
    $return = array();

    // Everything from here is unmodified code from _field_invoke() formerly
    // inside a foreach loop over the instances.
    $function = $options['default'] ? 'field_default_' . $op : $field['module'] . '_field_' . $op;
    if (function_exists($function)) {

      // Determine the list of languages to iterate on.
      $available_languages = field_available_languages($entity_type, $field);
      $languages = _field_language_suggestion($available_languages, $options['language'], $field_name);
      foreach ($languages as $langcode) {
        $items = isset($entity->{$field_name}[$langcode]) ? $entity->{$field_name}[$langcode] : array();
      dpm($entity);
        $result = $function($entity_type, $entity, $field, $instance, $langcode, $items, $a, $b);
        if (isset($result)) {

          // For hooks with array results, we merge results together.
          // For hooks with scalar results, we collect results in an array.
          if (is_array($result)) {
            $return = array_merge($return, $result);
          }
          else {
            $return[] = $result;
          }
        }

        // Populate $items back in the field values, but avoid replacing missing
        // fields with an empty array (those are not equivalent on update).
        if ($items !== array() || isset($entity->{$field_name}[$langcode])) {
          $entity->{$field_name}[$langcode] = $items;
        }
      }
    }
    return $return;
  }

  function setContexts(array $all_contexts) {
    $all_plugins = $this->field_value_layout_access_get_children(array(), 'field_value_layout_access');
    $plugin = $all_plugins[$this->settings['plugin_name']];

    list($parent, $entity_type, $bundle_type, $field_name) = explode(':', $plugin['name']);
    $required_contexts = array($entity_type => $entity_type);
    return layout_set_handler_contexts($this->contexts, $all_contexts, $required_contexts, $this->settings['contexts']);
  }
}