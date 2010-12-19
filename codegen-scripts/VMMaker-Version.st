"Currently this script is manually prepared.
In future it will be automatically generated from image.

This file contains an information for recreating an image used
for generating VM source code using VMMaker "


#(	
	"base image"
		baseImage 'Squeak4.1 latest update #10304'
		
	"FFI Pools"
		package 'http://source.squeak.org/FFI/FFI-Pools-eem.2.mcz'

	"Freetype (need only the FT2Constants, errors during load)"
	
		package 'http://squeaksource.com/FreeTypePlus/FreeType-tween.435.mcz'
		
	"VMMaker"
		package 'http://squeaksource.com/VMMaker/VMMaker-oscog-Igor.Stasenko.38.mcz'
		
	"Freetype plugin"
	
		package 'http://squeaksource.com/FreetypePlugin/Freetype-Plugin-dtl.56.mcz'


	"NativeBoost stuff"
	
		package 'http://squeaksource.com/NativeBoost/NativeBoost-Pools-Igor.Stasenko.5.mcz'
		package 'http://squeaksource.com/NativeBoost/NativeBoost-CogPlugin-Igor.Stasenko.3.mcz'

)

"string used to generate the code"
"
		| rootDirectory |
		rootDirectory := FileDirectory on: '/Users/sig/projects/cog/sig-cog'.

		NBCoInterpreter initialize.

		CogNativeBoostPlugin setTargetPlatform: #Mac32PlatformId.
		(VMMaker
		       generate: NBCoInterpreter
		       to: (rootDirectory / 'src') fullName
		       platformDir: (rootDirectory / 'platforms') fullName
		       excluding:#(BrokenPlugin IA32ABIPluginSimulator SlangTestPlugin TestOSAPlugin))

		
"
