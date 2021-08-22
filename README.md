Field Visibility Condition
================

Description
-----------

Field Visiblity Condition extends the BackdropCMS block visibility condition to set a condition based on the selected fields data.

This makes it possilble to show or hide another block or even a layout, based on the value of a particular field.

How to install
--------------

Install this module using the official Backdrop CMS instructions at https://backdropcms.org/guide/modules

Usage
-----

This module will add a new Visibility Condition called 'Field value' which provides access to an entity field value.

Layouts:

1. Navigate to your layout of choice and click the 'Configure Layout' tab.
2. Scroll down to 'Visibility conditions'
3. Click '+ Add visibility condition'
4. In the dropdown, choose 'Field value'
5. Select the field you want to use as the validation target
6. Choose which data you want to use as the validation data or add a checkmark in the 'NOT' box to show if it doesn't validate
7. Click the 'Add Visibility Condition' button
8. Click the 'Save Layout' button
9. Test and make sure it works the way you expect

Blocks:

1. Navigate to your layout of choice
2. On the 'Manage Blocks' tab, find the block you want to show or hide and click 'Configure'
3. Click on the 'Visibility condition' link to open it
4. Click '+ Add visibility condition'
5. Choose 'Field value' from the dropdown
6. Select the field you want to use as the validation target
7. Choose which data you want to use as the validation data or add a checkmark in the 'NOT' box to show if it doesn't validate
8. Click the 'Add Visibility Condition' button
9. Click the 'Update Block' button
10. Click the 'Save Layout' button
11. Test and make sure it works the way you expect

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

CURRENT MAINTAINERS
---------------    

Originally developed for Backdrop by @docwilmot
