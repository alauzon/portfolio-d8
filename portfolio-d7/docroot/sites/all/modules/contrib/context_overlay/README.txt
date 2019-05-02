Context Overlay creates two context reactions:

* Force overlay: Displays pages that match the context in an overlay dialog
* Customize Overlay Properties: Allows you to set overlay properties including the width, margin-top, and whether to hide the title


By matching these reactions, you can create an elegant UI for your site's visitors. Here's a couple of ideas:

* Easily make the login / registration appear in a properly-sized modal dialog
* Combine with the Contact Importer module to make a nice contact importing experience
* Add to a context selecting all webform nodes to make forms appear in a properly-sized modal dialog


TODO
---------
* Figure out a way to keep a page from appearing in an overlay (for example, force /admin/dashboard to be a normal page).
* Allow the theme to be set (set the overlay theme to administration theme by default?). Currently can be done with themekey. Similar functionality could be provided by context_reaction_theme.
* Look into possible implementations of hook_admin_paths() and hook_admin_paths_alter(
