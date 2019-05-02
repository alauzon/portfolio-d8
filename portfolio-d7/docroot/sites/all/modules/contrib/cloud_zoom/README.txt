Overview
--------
This module provides an field formatter to zoom an image while hovering over 
it. An image style is selected for the default display image, and an additional 
style is selected to be used as the zoomed image. When a user hovers over the 
displayed image, the zoomed image appears and is positioned relative to the 
current mouse position.


Requirements
------------
This module depends on the core Image module being enabled.


Install
-------
Install the module by following the instructions at http://drupal.org/node/70151


Configuration
-------------
To configure the Image Zoom display, go to Administration > Structure > Content 
types and select the content type you would want to use Image Zoom with. If you 
do not already have an Image field defined, add one by going to the Manage Fields 
tab. After you have an Image field, go to the Manage Display tab. Change the 
Format for your Image field to Image Zoom. To change which styles are displayed 
for the displayed image and the zoomed image, click the gear icon at the end of 
the row, select the desired image styles, and click Update.


Changing default styles
-----------------------
All styles provided by this moodule are able to be overridden to change the
position, size, border color, etc... to your liking. To do this, find the
css property you would like to change in the css/imagezoom.css file, copy it
to your theme's .css file, and change it. Note that any changes you make to
the imagezoom.css file will be overwritten when you update the module. Using
a css file in your theme prevents this.