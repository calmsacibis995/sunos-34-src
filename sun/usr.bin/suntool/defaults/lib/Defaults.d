SunDefaults_Version 2
;	@(#)Defaults.d	1.1	86/09/25 SMI
/Defaults	""

//Private_only	"True"
	$Enumeration ""
	$Help	"If true, use only your private .defaults file."
	False	""
	False/$Help	"Include the master database too."
	True	""
	True/$Help	"Use only your private .defaults file."

//Error_Action "Continue"
	$Help	"Action to take when an error is encountered in SunDefaults."
	$Enumeration	""
	Continue	""
	Continue/$Help	"On each error, print an error message and continue."
	Abort		""
	Abort/$Help	"On first error, print error message and then dump core."
	Suppress		""
	Suppress/$Help	"Whenever an error occurs, simply continue without printing any error message."

//Maximum_Errors "10"
	$Help	"Maximum number of errors that SunDefaults will write out."

//Test_Mode	"Disabled"
	$Enumeration	""
	$Help		"Enable/disable SunDefaults test mode."
	Enabled		""
	Enabled/$Help	"SunDefaults will make believe that the database is totally inaccessable."
	Disabled	""
	Disabled/$Help	"Normal mode of operation."

//Directory	"/usr/lib/defaults"
	$Help		"Directory to get defaults database from, i.e., where the *.d files are to be obtained."

//Private_Directory	""
	$Help		"Directory for developing new *.d files. Searched in addition to standard directory."
