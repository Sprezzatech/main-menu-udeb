Template: netcfg/no_dhcp_client
Type: note
Description: No dhcp client found. I cannot continue.
 This package requires pump or dhcp-client. 
Description-ru: ������ dhcp �� ������. ������ ��������������.
 ��� ����� ������ ��������� ���������� ����� pump ��� dhcp-client.

Template: netcfg/dhcp_hostname
Type: string
Description: What is your dhcp hostname?
 In some situations, you may need to supply a DHCP host name.  These
 situations are rare.  Most commonly, it applies to cable modem users. If
 you are a cable modem user, you might often need to specify an account
 number here.  Otherwise, you can just leave this blank. 
Description-ru: ����� ��� � ����� ������ dhcp?
 � ��������� ���������, ��� ����� ������������ DHCP ��� ������. ��� ������
 ������. � ����������� ����� ��� �������� ������������� ��������� �������.
 ���� �� ����������� ��������� �������, �� ����� �� ��� �������� �������
 ����� ����� ��������. � ��������� ������ ������ �������� ���� ������.

Template: netcfg/do_dhcp
Type: note
Description: I will now configure the network.
 This may take some time.  It shouldn't take more than a minute or two. 
Description-ru: ������ � ��������� � ��������� ����.
 ��� ����� ������ ��������� �����. �� ����� �����-���� �����.

Template: netcfg/confirm_dhcp
Type: boolean
Default: true
Description: Is this information correct?
Description-ru: ��� ���������� ���������?
