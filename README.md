Field Visibility Condition
================

Description
-----------

Field Visibility Condition extends Backdrop CMS block visibility conditions to show or hide blocks and layouts based on entity field values.

This makes it possible to show or hide a block or even a layout based on the value of a particular field. For example, you could show a sidebar block only on posts where a "Featured" checkbox is checked.

**Important:** This module requires that the layout has an entity context available. It will only work on layouts bound to a specific path (e.g. `node/%`), not on the Default layout.

Condition Types
---------------

Four condition types are available:

- **Is equal to** - Show/hide when a field matches a specific value.
- **Is not empty (NOT NULL)** - Show/hide when a field has any value.
- **Is empty (NULL)** - Show/hide when a field has no value.
- **Count** - Show/hide when a field has number of values that meet the condition.

Each condition type also supports a **Reverse (NOT)** option to negate the check.

**Note:** For boolean fields, a value of "No" is treated as empty by Backdrop core. Use "Is equal to" to reliably match a specific boolean value.

How to Install
--------------

Install this module using the official Backdrop CMS instructions at https://backdropcms.org/guide/modules

Usage
-----

This module adds a new visibility condition called "Field value" which provides access to entity field values.

### Blocks

1. Navigate to your layout of choice (must be bound to a path like `node/%`).
2. On the "Manage Blocks" tab, find the block you want to show or hide and click "Configure".
3. Open the "Visibility conditions" section.
4. Click "+ Add condition".
5. Choose "Field value" from the dropdown.
6. Select the field you want to evaluate.
7. Choose a condition type and configure the value if needed.
8. Optionally check "Reverse (NOT)" to negate the condition.
9. Click "Add Condition".
10. Click "Update Block", then "Save Layout".

### Layouts

1. Navigate to your layout of choice and click the "Configure Layout" tab.
2. Scroll down to "Visibility conditions".
3. Click "+ Add visibility condition".
4. Choose "Field value" from the dropdown.
5. Select the field and configure the condition as described above.
6. Click "Add Visibility Condition", then "Save Layout".

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

Current Maintainers
---------------

 - Tim Erickson (https://github.com/stpaultim)
 - Openings available....

Credits
---------------

Originally developed for Backdrop by @docwilmot
