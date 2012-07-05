// LCS 3021  May 6, 2003

//public message
var msg_blank = "%s can not be blank.\n";
var msg_nospaces = "Blanks or spaces are not allowed in %s\n\n";
var msg_invalid = "Invalid character or characters in %s. Valid characters are: \n%s\n\n";
var msg_notallow = "Invalid character or characters in %s\nThe following characters are not allowed:\n%s\n";
var msg_check_invalid = "%s must be a number.\n";
var msg_outofrange = "%s is invalid. Valid range is %s to %s \n";
var msg_validIP = "%s is invalid. Valid range is 1.0.0.1 to 254.255.255.254\n";
var msg_validMask = "%s is invalid. Valid range is 0.0.0.0 to 255.255.255.255\n";
var msg_mail = "%s is invalid. Please enter a valid e-mail address\n";
var msg_ipadd = "IP address is invalid. Valid range is 1.0.0.1 to 254.255.255.254\n";
var msg_restart = "Camera must now restart. \nPlease wait %s seconds before attempting to continue";
var msg_changeIP = "Since the Camera's IP address has changed,\nthis connection must be terminated.\n\nPlease wait %s seconds for the restart to be completed,\nand then re-connect using the new IP address";
var msg_activeX = "MPEG-4 Video is only available with Microsoft Internet Explorer.\nFor Netscape users, please use the Cisco Viewer & Recorder Utility.";
var msg_connect_livevideo =  "Connecting to Live Video stream ..." ;
var msg_name =  "User Name";
var msg_uname =  "DDNS Username";
var msg_pw =  "Password";
var dw_error = 	"Video is unavailable through the L1 Devices tab. Please access the camera directly."; 				  
var dw_serror = 	"Video is unavailable through the HTTPS Protocol. Please access the camera directly."; 				  


// options
//var msg_send_test_mail = "This will send E-mail to the recipients you have enabled and entered.";
var msg_send_test_mail = "An e-mail will be sent to the recipients you have listed.";
var	msg_ddns_username = "DDNS User Name";
var msg_ddns_email = "E-mail Address for TZO DDNS";
var msg_ddns_key = "TZO Password Key";
var msg_ddns_password = "DDNS Password";
var msg_ddns_host = "DDNS Host Name";
var msg_ddns_domain = "DDNS Domain Name";
var msg_ip_sche_hr = "Check Internet IP address time (hr)";
var msg_ip_sche_min = "Check Internet IP address time (min)";

var msg_emaildest1 = "E-mail address [1]";
var msg_emaildest2 = "E-mail address [2]";
var msg_emaildest3 = "E-mail address [3]";

var msg_emailsrc = "'From' E-mail address";
var msg_smtp_server = "Outgoing Mail SMTP Server";
var msg_smtp_login = "Mail Server Account Name" ;
var msg_port = "Alternate Port Number";
var msg_ftp_port = "FTP Port Number";
var msg_smtp_port = "SMTP Port Number";
var msg_blank_smtp_name = "Mail Server Account Name";
var msg_blank_smtp_pw = "Mail Server Account Password";
var msg_blank_pop_name = "POP Server Name";

var msg_v_url = "Video Streaming URL Address" ;

// ddns
var msg_wanport = 'Changing the WAN port number also changes the "Alternate Port" on the "Options" screen.\n';
var msg_newuser = "Make sure you are connected to the Internet to register";

var msg_daname = "DDNS Account Name";
var msg_dpw = "DDNS Password";
var msg_ddns = "DDNS Host Name";
var msg_selsite = "Please select a DDNS Service provider";
var msg_ddvn = "DDNS Device Name";
var msg_v_ddns_hname ="Host Name is invalid. Please enter a valid host name!\n";
var msg_v_hname = "Host Name can not be blank.\n";

// basic
var msg_dname = "Device Name";
var msg_hr = "Time (Hrs)";
var msg_min = "Time (Min)";
var msg_day = "Date (Day)";
var msg_year = "Date (Year)";
var msg_adm = "Admin Name";
var msg_update_hr = "Update schedule (hr)";
var msg_update_min = "Update schedule (min)";
var msg_cname = "Camera Name";
var msg_cdesc = "Camera Description";
var msg_ntp_s = "NTP Server Address";
var msg_ntp_port = "NTP Port";
var msg_cname_empty = "Camera Name can not be empty";
var msg_bonj_empty = "Bonjour Name can not be empty";

var msg_ip_field = "IP Address";
var msg_mask_field = "Subnet Mask";
var msg_gateway = "Gateway IP address";
var msg_dns1 = "Primary DNS address";
var msg_dns2 = "Secondary DNS address";
var msg_need_gwdns = "Warning!\nE-mail notification, DDNS, and Network Time Protocol\ncannot work without a Gateway and DNS.\n\nClick OK to confirm current settings, or click Cancel to abort changes.";
var msg_datetime_pcsync = "Camera's Date/Time will be set to match your PC, but this will be lost on power down.\nOn start up, the Camera will use an Internet Time Server to obtain the current Date and Time.\n";

// Advance
var msg_http_port = "User-defined HTTP port";
var msg_port_conflict = "The Secondary Port number can not be equal to the RTSP Port number.\n";
var msg_conflict = "The HTTP Port number can not be equal to the HTTPS Port number.\n";
var msg_rtsp_port = "RTSP Port";
var msg_rtp_src_port = "RTP Data Port";
var msg_rtp_pkt_len = "Max RTP Data Packet Length";
var msg_cos_vid = "VLAN ID";
var msg_q_dscp = "QoS DSCP";

var msg_video_add = "Video Multicast Address";
var msg_video_port = "Video Multicast Port Number";
var msg_audio_add = "Audio Multicast Address";
var msg_audio_port = "Audio Multicast Port Number";
var msg_v_port = "Invalid Video Port number. Enter an even value between 1024 and 65534.\n";
var msg_a_port = "Invalid Audio Port number. Enter an even value between 1024 and 65534.\n";
var msg_rtp_time = "Time of hop(s)";
var msg_bonj = "Bonjour Name";
var msg_mcast_group_name ="Multicast Group Name";

// W_sec
var msg_blank_wpa2e_name = "WPA2 Enterprise User Name";
var msg_blank_wpa2e_pw = "WPA2 Enterprise Password";
var msg_blank_urid = "User ID";
var msg_blank_urpw = "Password";
var msg_warn_auth = "This will allow you to download the certification (CA) from untrust publishers, continue?\n";
/*var ca_msg = "Be sure the certificate includes a digital signature from a certificate authority (CA)\n which vouches for correctness of the data. Continue uploading the file?";*/
var ca_msg = "Certificate already exists! Continue uploading another Certificate?";
var NoCertificate_msg = "No Certificate has been provided. Please provide a Certificate";
var fill_msg = 'Certificate already exists! Continue uploading another Certificate?\n';

// wireless
var msg_ssid = "SSID";
var msg_auto_ch  = "Auto"; // channel
var msg_wiresec = "Do you want to enable wireless security ?";
var msg_keysize = "The key length is not correct.\nKeys must consist of %s hexadecimal characters";
var msg_hexkey = "The key value is not correct. \nKeys must consist of hexadecimal characters ( 0~9 or A~F )";
var msg_smallpassphase = "Passphrase must have at least one character.";
var msg_default_key = "Selected key [%s] cannot be blank.";
var msg_psk_size = "WPA Pre-shared key must be from 8 to 63 characters in length.\n";

var msg_wep_64 = "64 Bit Key table";
var msg_wep_128 = "128 Bit Key table";

// options
var msg_motiondetect = "Warning!\nMotion detection can be triggered by sudden changes in lighting levels,\nas well as by moving objects.";
var msg_nomotiondetect = "Motion detection is not available with MJPEG video streams";
var msg_noaudiomotion_mjpeg = "Motion detection and audio are not available with MJPEG video streams,\nso they have been disabled.";
var msg_change_port = "Changing the Alternate port number also changes the WAN Port on the DDNS screen.";

// password
var msg_adm_login = "Administrator Login Name";
var msg_passNoMatch = "Password entries do not match.\n";
var msg_bigpw = "Please limit the password field to 8 characters";
var nofile_msg = "No filename provided. Please select the correct file.";
var msg_confirmCfile = "Warning!\nRestoring settings from a config file will erase all the current settings.\nClick OK to continue , Cancel to abort." ;
var msg_bigpw = "Please limit the password field to 8 characters";

//Pan/Tilt
var msg_cycle_limit = "Unable to add. Sequence can contain a maximum of %s presets.\n";
var msg_select_preset = "No Preset position selected, please select a preset from the list.\n";
var msg_exclusive_time = "Exclusive control time";
var msg_standby_time = "Standby time";
var msg_position_time = "Time at each position";
var msg_in_presetname = "The preset name has existed.\n";
var msg_in_max = "Maximum added items is 9.\n";
var msg_in_ptname = "The name has existed.\n";
var msg_pname_blank = "Current Position can not be blank.\n";
var msg_add_blank = "Can not add blank Preset Position in Sequence.\n";

// image
var msg_quality_change = "Warning!\nWhen the image bit rate or quality setting is changed, all existing connections will be terminated.\nAnyone viewing the video will need to re-connect.";
var msg_res_change = "Warning!\nWhen the image size setting is changed, all existing connections will be terminated.\nAnyone viewing the video will need to re-connect.";
var msg_preview = "In this browser, click the image pane to start and stop the preview";
var msg_typechange = "Warning!\nWhen the image type setting is changed, all existing connections will be terminated.\nAnyone viewing the video will need to re-connect.";
var msg_typechange_full = "Warning!\n1) Audio is not available with this video type.\n2) When the image type is changed, all existing connections will be terminated.\nAnyone viewing the video will need to re-connect.";

var msg_text_is_empty = "Warning!\nThe text is empty, Do you want to save the change?";
var msg_overlay = "Text display";
var msg_posx = "Position X";
var msg_posy = "Position Y";
var msg_tr_color = "Transparent Color";
var up_img_msg = "Be sure the file size should not over 64Kb.\nContinue uploading the file?";
var up_img_f1 = "Please only upload file that end in types:  ";
var up_img_f2 = "\nPlease select a new ";
var up_img_f3 = "file to upload again.";
var msg_mv_pw = "Access Code";


// Status
var resetDefault_msg = "Reset to factory defaults ?\n\nClick OK to continue, or click Cancel to abort";
var restart_msg = "Restart the Camera ? \nAll existing connections will be terminated.\n\nClick OK to continue, or click Cancel to abort";

var msg_syslog_serv_addr = "Syslog Server Address";
var msg_syslog_port = "Syslog Port";

var msg_jab_serv_addr_empty = "Warning!\nThe Jabber server address can not be empty.\n";
var msg_blank_jab_name = "Jabber Server Login name";
var msg_blank_jab_pw = "Jabber Server Password";
var msg_jab_serv_addr_illegal_str = "Jabber Server Address";
var msg_send_jab_addr_illegal_str = "Server Address";
var msg_send_jab_address = "E-mail address";
var msg_endtime_less = "The end time must be after the start time.\n";
var msg_send_jab_addr_empty = 'Warning!\nThe "Send To" E-mail address can not be empty.\n';


// User
var button_label_add = "Add User";
var button_label_update = "Update User";
var button_label_clear =  "Clear Fields";
var button_label_cancel = "Cancel";
var msg_username = "User name";
var msg_nameused ="This name is already used. Please use another name.\n";
var msg_passwd_nomatch = "Password entries do not match, please retype.\n";
var msg_name_empty = "The name can not be empty if password or Admin option is filled.\n";     // , Pan/Tilt

var msg_dbfull = "User database is full, no more users can be added.\n";
var msg_select_user = "No user selected, please select a user from the list.\n";
var msg_del_user = "Delete user: ";
var msg_del_allusers = "Delete all users ? \n";

var msg_user = "User";


// upgrade
var up_msg = "Continue upgrading the firmware?\nAll existing connections will be terminated.";
var finish_msg = "\Firmware Upgrade completed... Camera restarting." + 
"\nPlease wait for restart to be completed before continuing.";


//FTP
var msg_ftp = "Warning!\nWhen FTP Upload is enabled, you must enter details of the FTP Server.\n";
var msg_aftp_name = "FTP Server Name";
var msg_aftp_login = "FTP Server Login name";
var msg_aftp_pw = "FTP Server Password";
var msg_in_inter = "Invaild Ftp Upload Interval"; 


// Event
var msg_eventname = "Event Name";
var msg_event_over = "It is not possible to add any more events. The maximum number of events is 10.\n";
var msg_noevent_selected = "No event selected. Please select a event from the list.\n";
var msg_delevent = "Delete selected event or events?\n";
var msg_photo_filename = "File Path Name";
var msg_no_option = "No option selected. Please select one from the list.\n";
var msg_emaildest = "E-mail address";
var msg_e_subject = "E-mail Subject";
// schedule
var msg_delperiod = "Delete selected period or periods?\n";
var msg_noperiod_selected = "No period selected. Please select a period from the list.\n";
var msg_finish_after_start = "Invalid period. Finish time must be after start time.\n";
var msg_start_end = "Invalid period!  Start time and end time cannot be the same.\n";

// I/O
var msg_Output_time = "Action duration time";

//RS485
var msg_hex485key = 'Invalid Command. Hex keys can only include the characters 0~9, A~F, a~f and ",".\n';
var msg_p_ad = "Logical address";
var msg_command_name = "Command name";
var msg_cdm = "Pan / Tilt Command";
var msg_fill = "You must key-in the valid %s and command to operate the connected device you wish to control.\n";
var msg_img_file = "Warning!\nImage file will be upload.\nClick OK to continue , Cancel to abort." ;

// IM
var msg_message_size = "Message can not exceed 384 characters; please shorten it.\n";

//samba
var msg_smb_server_illegal_str = "SMB/CIFS Server";
var msg_smb_path_illegal_str = "Upload path";
var msg_smb_path_empty = "Warning! The upload path can not be empty.\n";
var msg_smb_server_empty = "Warning! The SMB/CIFS server can not be empty.\n";
var msg_smb_max = "Maximum size of each file";