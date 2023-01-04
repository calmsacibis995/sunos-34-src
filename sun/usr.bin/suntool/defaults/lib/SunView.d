SunDefaults_Version 2
;	@(#)SunView.d	1.3	87/01/07 SMI
/SunView
/SunView/$Help	"Miscellaneous SunView options"

//Click_to_Type	"Disabled"
	$Help "Enable/disable click-to-type model of keyboard interaction."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Click-to-type: left mouse button sets caret; middle mouse button restores caret."
	Disabled ""
	Disabled/$Help "Keyboard follows mouse: cursor over window directs type-in."


//Font	""
	$Help "Name of the default font used to display text. Empty string means default system font."


//Walking_Menus	"Disabled"
	$Help "Enable/disable walking menus. Provided for backwards compatibility of user interface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Use new SunView features.  Tools capable will use new style walking menus."
	Disabled ""
	Disabled/$Help "Remain compatible with pre-SunView1.0.  Tools will use old style, stacking menus."


//Rootmenu_filename ""
	$Help "Name of file containing a customized rootmenu.  Empty string means default rootmenu."


//Icon_gravity	"North"
	$Help "Specifies against which edge of the screen icons are placed."
	$Enumeration ""
	North  ""
	North/$Help "Icons will be placed against top edge of screen."
	South  ""
	South/$Help "Icons will be placed against bottom edge of screen."
	East  ""
	East/$Help "Icons will be placed against right edge of screen."
	West  ""
	West/$Help "Icons will be placed against left edge of screen."


//Icon_close_level "Ahead_of_all"
	$Help "The front-to-back level for the icon when a window closes."
	$Enumeration ""
	Ahead_of_all
	Ahead_of_all/$Help "Icon will be on top of all windows."
 	Ahead_of_icons
	Ahead_of_icons/$Help "Icon will be behind open windows, but ahead of other icons."
	Behind_all
	Behind_all/$Help "Icon will be behind ALL windows, open and iconic."

//Jump_cursor_on_resize	"Disabled"
	$Help "Enable/disable jumping the cursor when the user resizes a window."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "On a resize operation, move the cursor to the window edge."
	Disabled ""
	Disabled/$Help "On a resize operation, move the window edge to the current cursor position."


//Audible_Bell	"Enabled"
	$Help "Enable/disable audible bell."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "An audible tone will be emitted upon receipt of a bell command."
	Disabled ""
	Disabled/$Help "No audible tone will be emitted upon receipt of a bell command."


//Visible_Bell	"Enabled"
	$Help "Enable/disable visible bell."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Screen will flash upon receipt of a bell command."
	Disabled ""
	Disabled/$Help "Screen will not flash upon receipt of a bell command."


//Embolden_Labels	"Disabled"
	$Help "Enable/disable displaying all tool labels in boldface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Tool labels will be bold."
	Disabled ""
	Disabled/$Help "Tool labels will not be bold."


//Ttysubwindow/Retained
	$Help "This item has been moved to /Tty/Retained."


//Root_Pattern	"on"
	$Help "Root window 'pattern'; 'on', 'off', 'gray' or the name of a file produced with iconedit"


