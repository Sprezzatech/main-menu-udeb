Template: ethdetect/detection_type
Type: select
Choices: passive, full, none
Choices-es: pasiva, completa, ninguna
Default: passive
Default-es: pasiva
Description: What level of hardware detection would you like?
 I can automatically detect some hardware.  If you want me to try to detect
 your hardware you can choose between full and passive detection.  Full
 detection may involve probing which could cause the computer to hang.  Some
 hardware, like ISA cards, can only be detected with full detection.  If you
 don't want me to do any detection, choose 'none' and you will be prompted.
Description-es: �Qu� nivel de detecci�n de hardware desea?
 Puedo detectar autom�ticamente alg�n hardware. Si quiere que intente
 detectar su hardware, puede elegir entre detecci�n pasiva o completa. La
 detecci�n completa puede implicar probar el hardware, lo cual puede
 colgar el sistema. Alg�n hardware, como las tarjetas ISA, s�lo pueden
 detectarse de este modo. Si no quiere que intente detectar nada, elija
 'ninguna' y se le solicitar� la informaci�n necesaria.

Template: ethdetect/module_select
Type: select
Choices: 3c509, ne, ne2000, 3c59x, acenic, dgrs, dmfe, eepro100, epic100, hp100, ibmtr, ne2k-pci, old_tulip, rtl8139, sis900, sktr, tlan, tulip, other 
Choices-es: 3c509, ne, ne2000, 3c59x, acenic, dgrs, dmfe, eepro100, epic100, hp100, ibmtr, ne2k-pci, old_tulip, rtl8139, sis900, sktr, tlan, tulip, otro
Default: unknown
Default-es: desconocido
Description: What module does your ethernet card require?
 This is a list of modules that I know about.  Choose the module from the list
 that supports your card.  If your card requires a different module, choose
 'other' and you will be prompted for the location of that module.
Description-es: �Qu� m�dulo necesita su tarjeta ethernet?
 Esta es una lista de m�dulos que conozco. Escoja de esta lista el m�dulo
 que incluya soporte para su tarjeta. Si su tarjeta requiere un m�dulo
 distinto, elija 'otro' y se le pedir� la ubicaci�n de ese m�dulo.

Template: ethdetect/module_prompt
Type: string
Description: Where is the module for your ethernet card?
 Please enter the full path to the module for your ethernet card.
Description-es: �D�nde se encuentra el m�dulo para su tarjeta ethernet?
 Por favor, introduzca la ruta completa del m�dulo para su tarjeta
 ethernet.

Template: ethdetect/module_params
Type: string
Description: Please enter any additional parameters.
 Some modules accept load-time parameters to customize their operation.  These
 parameters are often I/O port and IRQ numbers that vary from machine to
 machine and cannot be determined from the hardware. An example string looks
 something like "IRQ=7 IO=0x220"
Description-es: Por favor introduzca los par�metros adicionales.
 Algunos m�dulos aceptan al par�metros en tiempo de carga para ajustar su
 funcionamiento. Estos par�metros se refieren con frecuencia al puerto de
 entrada/salida (IO) y n�meros de petici�n de interrupci�n (IRQ) que
 pueden variar entre m�quinas y no se puede determinar a partir del
 hardware. Un ejemplo ser�a algo parecido a esto: "IRQ=7 IO=0x220".

Template: ethdetect/load_module
Type: boolean
Default: true
Description: Would you like me to attempt to load the '${module}' module?
 An ethernet card has been detected on the ${bus} bus.  In order for the operating
 system to use this card the module '${module}' must be loaded.
Description-es: �Quiere que intente cargar el m�dulo '${module}'?
 Se ha detectado una tarjeta ethernet en el bus ${bus}. Para que el
 sistema operativo use esta tarjeta se debe cargar el m�dulo '${module}'.

Template: ethdetect/error
Type: note
Description: An error occured.
 Something went wrong.
Description-es: Ha ocurrido un error.
 Algo ha fallado.

Template: ethdetect/nothing_detected
Type: note
Description: No ethernet cards were detected.
 Make sure the card is firmly seated and try again.
Description-es: No se ha detectado ninguna tarjeta ethernet
 Aseg�rese de que la tarjeta est� correctamente enchufada e int�ntelo de
 nuevo.

