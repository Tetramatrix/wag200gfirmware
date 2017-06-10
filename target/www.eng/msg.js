// Javascript messsage file
// NOTE:  
//  %s will be replaced with either a number or a text string
//  \n means start new line

//  shared  messages - used in more than 1 file
var msg_blank = "%s can not be blank.\n";
var msg_space = "Blanks or spaces are not allowed in %s\n";
var msg_blank_in = "Blanks are not allowed in %s\n";
var msg_allspaces = "%s cannot consist solely of spaces\n";
var msg_invalid = "Invalid character or characters in %s\nValid characters are: \n%s\n\n";
var msg_check_invalid = "%s contains an invalid number\n";
var msg_valid_range = "%s is invalid. Valid range is %s to %s\n";
var msg_second = "Second";
var msg_invalid_ip = "Invalid IP address, please enter again.\n";
var msg_invalid_portnumber = "Invalid Port number, valid range is 1 to 65534.\n";
var msg_nameNULL = "The name can not be blank.\n";
var msg_invalid_qos_portnumber = "Specific Port is invalid, valid range is 0 to 65534.\n";


//  AccessRes.htm (Internet Access Policy), PortrangeTriggering.htm
var msg_endtime_less = "The end time must be after the start time.\n";
var msg_policyname = "Policy Name";
var msg_invalid_keyword = "Invalid Keyword: Can not include \":\" in Keyword.\n" ;
var msg_wrong_URLformat = "Invalid URL, please check the format of the URL.\n";
var msg_blockedsv0_startport = "1st Blocked Service Start Port";
var msg_blockedsv1_startport = "2nd Blocked Service Start Port";
var msg_blockedsv0_endport = "1st Blocked Service Finish Port";
var msg_blockedsv1_endport = "2nd Blocked Service Finish Port";
var msg_portbigger = "Finish Port number must be greater than the Start Port number.\n";


// administration.htm
var msg_nomatch_pwd = "The password and verification do not match, please enter again.\n";
var msg_rm_port = "Management Port";
var msg_invalid_remote_allow_ipaddr = "The allowed ip address is invalid.";
var msg_invalid_remote_start_ip = "The allowed start ip address is invalid.\n";
var msg_invalid_remote_end_ip = "The allowed end ip address is invalid.\n";
var msg_invalid_start_end_ip = "The IP Range is invalid.";

var msg_invalid_snmp_ipaddr = "The snmp ip address is invalid";
var msg_invalid_snmp_start_ip = "The snmp start ip address is invalid.\n";
var msg_invalid_snmp_end_ip = "The snmp end ip address is invalid.\n";
var msg_invalid_snmp_start_end_ip = "The snmp ip range is invalid";
var msg_device_name = "Device Name";
var msg_nofile = "Filename can not be blank";
var msg_confirmCfile = "Warning!\nRestoring settings from a config file will erase all the current settings.\nAre you sure you wish to do this?";
var msg_snmp_get_comm = "SNMP Get Community";
var msg_snmp_set_comm = "SNMP Set Community";
var msg_invalid_snmp_trap = "SNMP Trap IP Address is invalid.\n";
var msg_default_pwd = "The Router is currently set to its default password. As a security measure, you must change the password before the Remote Management feature can be enabled. Click the OK button to change your password or leave the Remote Management feature disabled.";

// AdvancedWSettings.htm (Advanced Wireless)
var msg_beacon = "Beacon Interval";
var msg_dtim = "DTIM Interval";
var msg_frag = "Fragmentation Threshold";
var msg_rts = "RTS Threshold";


//  appgaming.htm  (Port Range Forwarding)
var msg_r_name = "Application %s: Name ";
var msg_start_port = "Application %s: Start Port";
var msg_end_port = "Application %s: End Port";
var msg_rf_ip = "Application %s: IP";
var msg_Entry = "Entry "; //  e.g. Entry 2 
var msg_portg = "Application %s: End port must be greater than Start port.\n";


// Diagnostics.htm
var msg_ping_size = "Ping Size";
var msg_ping_number = "Number of Pings";
var msg_ping_interval = "Ping Interval";
var msg_ping_timeout = "Ping Timeout";

// DMZ.htm
var msg_dmzIP = "DMZ IP Address";

//  EditList.htm  (Internet Access PCs List)
var msg_bad_mac_pc = "MAC address %s is invalid.\n";  
var msg_invalid_ipnum = "IP Address %s is invalid.\n";
var msg_invalid_range1 = "IP address range 1 is invalid.\n";
var msg_invalid_range2 = "IP address range 2 is invalid.\n";


// factorydefaults.htm
var msg_confirmDefault = "Warning!\nLoading the Factory Default Settings will erase all the current settings.\nAre you sure you wish to do this?";

// FirmwareUpgrade.htm  ( copy to FirmwareUpgrade.htm )
var up_msg = "Continue?\nAll existing Internet connections will be terminated.";
var nofile_msg = "No filename provided. Please select the correct file.\n";
"\nPlease check LEDs to see if Router is ready, then re-connect.";
var msg_up_fail = "Upgrade has failed!";
var msg_sel_file = "Please select a file to upgrade !";
var msg_incrt_file = "Incorrect image file!";

// log.htm
var msg_log_ip = "Logviewer IP Address";
var msg_dos_thresholds = "Denial of Service Thresholds.\n";
var msg_smtp_in = "SMTP Mail Server.\n";
var msg_email_in = "Email Address for Alert Logs is invalid.\n";
var msg_email_re = "Return Email Address is invalid .\n";

// QoS
var msg_standard_portnum = "Invalid Specific Port. \nPorts assigned to well-known applications can not be used.\n";

// Setup.htm
var msg_invalid_wan_ip = "Internet IP address is invalid, please enter again.\n";
var msg_invalid_wan_gw = "Internet Gateway address is invalid, please enter again.\n";
var msg_invalid_wan_mask = "Internet Subnet Mask  is invalid, please enter again.\n";
var msg_primary_dns = "Primary DNS is invalid, please enter again.\n";
var msg_secondary_dns = "Secondary DNS is invalid, please enter again.\n";
var msg_pppoe_idle = "Idle time";
var msg_pppoe_redial = "Redial period";
var msg_mtu_size = "MTU size";
var msg_invalid_lan_ip = "Invalid LAN IP Address, please enter again.\n";
var msg_dhcp_lease = "DHCP Lease Time";
var msg_user_name = "User name";
var msg_dhcp_start = "DHCP start IP address range";
var msg_wan_pcr = "Pcr Rate";
var msg_wan_scr = "Scr Rate";
var msg_dhcp_num = "Maximum Number of DHCP Users";
var msg_invalid_dhcp_ip = "DHCP start IP is not in the same subnet as LAN IP, please enter again.\n";
var msg_wan_vpi = "VPI value";
var msg_wan_vci = "VCI value";
var msg_lan_in_dhcp_ip = "LAN IP address can not be within the DHCP IP address range.\n";
var msg_static_dns_1 = "Static DNS 1 is invalid, please enter again.\n";
var msg_static_dns_2 = "Static DNS 2 is invalid, please enter again.\n";
var msg_static_dns_3 = "Static DNS 3 is invalid, please enter again.\n";
var msg_static_wins = "WINS IP address is invalid, please enter again.\n";
var msg_dhcp_servers = "DHCP Server is invalid, please enter again.\n";
var msg_time_interval = "Time interval";

// setup_ddns.htm
var msg_username = "User name";
var msg_password = "Password";
var msg_hostname = "Hostname";
var msg_email_addr = "User Email Address";
var msg_key = "TZO Password (Key)";
var msg_domain = "Domain Name";
var msg_ddns_info = "Connecting the server, please wait ...";

// Setup_routing.htm
var msg_invalid_rdest_ip = "Invalid Destination IP address, please enter again.\n";
var msg_invalid_rmask = "Invalid Network Mask, please enter again.\n";
var msg_invalid_rgw = "Invalid Gateway Address, please enter again.\n";
var msg_hopcount = "Hop Count";


// SinglePortForwarding.htm
var msg_s_name = "Application %s: Name";
var msg_sf_ip = "Application %s: IP";
var msg_ext_port = "Application %s: External Port";
var msg_int_port = "Application %s: Internal Port";


// status
var msg_button_release = "DHCP Release";
var msg_button_renew = "DHCP Renew";
var msg_button_connect = " Connect ";
var msg_button_disconnect =  "Disconnect";


// wireless.HTM
var msg_ssid = "SSID";
var msg_out_c = "length is out of range";

// WSecurity.htm  (Wireless Security)
var msg_wep_pass = "Passphrase can not be blank\n";
var msg_hexkey = "Invalid Key. \nHex keys can only include the characters 0~9 and A~F.\nKey size is 10 chars (64bit) or 26 chars (128bit).\n";
var msg_asciikey = "Invalid Key. \nKey size is 5 chars (64bit) or 13 chars (128bit).\n";
var msg_maxpass = "Maximum Passphrase Size is ";
var msg_psk_keysize = "PSK must be from 8 to 63 characters\n";
var msg_key_renew = "Key Renewal";
var msg_key_renew_ivd = "key renewal value is invalid(must be 1~99999)!";

// WL_FilterTable.htm (Wireless MAC Filter)
var msg_item_mac = "Invalid entry for MAC address ";  // e.g. Invalid entry for MAC address 6 
var msg_mac_same = "MAC address %s and MAC address %s are identical.\n";  
var msg_mac_full = "MAC address filter list full.";
// e.g. MAC address 1 and MAC address 4 are identical.

// Reboot.htm
var msg_reboot_hard = "Will you really want to hard reboot the device?";
var msg_reboot_soft = "Will you really want to soft reboot the device?";


