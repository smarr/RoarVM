Gofer new	squeaksource: 'MetacelloRepository';	package: 'ConfigurationOfCog';	load.	((Smalltalk at: #ConfigurationOfCog) project version: '1.6') load.((MCHttpRepository 	
	location: 'http://www.squeaksource.com/VMMaker' 
		user: '' 
		password: '')	versionFromFileNamed: 'CMakeVMMaker-EstebanLorenzano.79.mcz') 
	load.	(Smalltalk saveAs: 'generator.image') 
	ifFalse: [ Smalltalk snapshot: false andQuit: true ]