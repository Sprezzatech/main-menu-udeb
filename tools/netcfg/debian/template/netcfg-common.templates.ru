Template: netcfg/get_domain
Type: string
Description: Choose the domain name.
 The domain name is the part of your Internet address to the right
 of your host name.  It is often something that ends in .com, .net,
 .edu, or .org.  If you are setting up a home network, you can make
 something up, but make sure you use the same domain name on all
 your computers. 
Description-ru: �������� ��� ������.
 ��� ������ - ��� ����� ������ ��������-������, ������ �� ����� �����.
 �������� ��� ������������� �� .com, .net, .edu ��� .org. ���� ��
 �������������� �������� ����, �� �� ������ ������� ���-������ ����, ��
 ������ �������, ��� ����������� ���������� ��� ������ �� ���� ����� �������.

Template: netcfg/get_nameservers
Type: string
Description-ru: �������� ������ �������� DNS.
 ����������, ������� IP ������ (�� �������� �����) �� 3 �������� ����,
 ����������� ���������. �� ����������� �������. ������� ����� ������������ �
 ������� �� ��������. ���� �� ������ �� ������ ������������ ������� ������ ����,
 �� �������� ���� ������.

Template: netcfg/choose_interface
Type: select
Choices: ${ifchoices}
Description: Choose an interface.
 The following interfaces were detected. Choose the type of your primary
 network interface that you will need for installing the Debian system (via NFS
 or HTTP).
Choices-ru: ${ifchoices}
Description-ru: �������� ���������.
 ���� ���������� ��������� ����������. �������� ��� ������ ���������� ��������
 ����������, ������� ��� ����� ��� ��������� ������� Debian (�� NFS ��� HTTP).

Template: netcfg/error_cfg
Type: note
Description: An error occured.
 Something went wrong when I tried to activate your network.  
Description-ru: ��������� ������.
 ���-�� ����� �� ���, ����� � ��������� �������������� ���� ����.

Template: netcfg/get_hostname
Type: string
Default: debian
Description: Enter the system's hostname.
 The hostname is a single word that identifies your system to the
 network.  If you don't know what your hostname should be, consult
 your network administrator.  If you are setting up your own home
 network, you can make something up here. 
Description-ru: ������� ��������� ��� �����.
 ��� ����� - ��� ���� �����, ������� �������������� ���� ������� �
 ����. ���� �� �� ������ ����� ������ ���� ��� ����� �������, ��
 ������������� � ����� ������� ���������������. ���� �� ��������������
 ���� ����������� �������� ����, �� ������� ���-���� �� ������ �����.

Template: netcfg/error
Type: note
Description: An error occured and I cannot continue.
 Feel free to retry.
Description-ru: ��������� ������, � � �� ���� ����������.
 �� ������ ����������� ���������.

Template: netcfg/no_interfaces
Type: note 
Description: No interfaces were detected.
 No network interfaces were found.   That means that the installation system
 was unable to find a network device.  If you do have a network card, then it
 is possible that the module for it hasn't been selected yet.  Go back to 
 'Configure Network Hardware'.
Description-ru: ���������� �� ����������.
 ������� ���������� �� ����������. ��� ��������, ��� ������� ��������� ��
 ������ ����� ���� ������� �����. ���� � ��� ���� ������� �����, �� ��������,
 ��� ��� ��� ���� �� ��� ������ ������. ��������� ����� � ������
 'Configure Network Hardware'.
