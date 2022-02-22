# RDP screen sharing
Proof of Concept of RDP screen sharing

This proof of concept of RDP screen sharing provides a very simple RDP server (ServidorRDPJanela) and a very simple RDP client (Cliente).
You must use two computers: one for running the server and other for running the client.
The server must be started first. It will generate an XML invitation file (containing the information necessary for the client to connect to the server). The file name is hardcoded in the variable fileName, inside the procedure GenerateInvitation, in the file ServidorRDPJanela.cpp. You must set this variable with the path you want to write the file to. Don´t forget that back slahes must be doubled, since it is an escape character inside a string.
The server now is ready to accept connections from the clients. Don´t close the small window that is opened by the server in the screen (you can minimize it), otherwise the server will be closed.
After the server has been started and the XML invitation file has been created, copy it to a location where the client can read it (it must be done manually; please keep in mind this is just a proof of concept, not a commercial project...).
The path where the client is going to read the XML from is hardcoded in the variable invitationFile, in the file Cliente.cpp. Please edit this file and set the variable content to the path of the XML where the client is going to read from.
A window will be opened in the client and in few seconds you should start viewing the sharer screen (the screen of the server computer) in this window.
These solutions (the server and the client) can be compiled using Visual Studio 2022, using the x86 platform configuration. I didn´t compile the solutions using x64 platform configuration. I also had to use the Multibyte character set (open the solution; in the taskbar select Project -> Properties of <project name> -> Configuration Properties -> Advanced -> Characters set and select Multibyte Characters set).

Once again I would like to highlight that this is just a proof of concept, with all settings hardcoded, and absolutely no warranty.

