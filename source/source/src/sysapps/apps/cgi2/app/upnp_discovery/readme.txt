There is one issue let me confusion.

For Windows XP, the serviceList MUST be provided in your description xml file.
Even though all fields in service List are empty.
<?xml version="1.0"?>                                                              
<root xmlns="urn:schemas-upnp-org:device-1-0">                                     
<specVersion>                                                                      
        <major>1</major>                                                           
        <minor>0</minor>                                                           
</specVersion>                                                                     
<URLBase>http://192.168.1.2:6789</URLBase>                                         
<device>                                                                           
<deviceType>urn:schemas-upnp-org:device:NetworkCamera:1</deviceType>               
<friendlyName>WVC2300</friendlyName>                                               
<manufacturer>Linksys</manufacturer>                                               
<manufacturerURL>www.linksys.com</manufacturerURL>                                 
<modelDescription>WVC2300 Wireless-G Internet Video Camera</modelDescription>      
<modelName>WVC2300</modelName>                                                     
<modelNumber>WVC2300</modelNumber>                                                 
<UDN>uuid:upnp-Linksys_NetworkCamera-00C002110003</UDN>                            
<serviceList><service>  <serviceType></serviceType>     <serviceId></serviceId>   <controlURL></controlURL>                <eventSubURL></eventSubURL>             <SCPDURL></SCPDURL>        </service>                                                 </serviceList>                                                                     
<presentationURL>http://192.168.1.2</presentationURL></device>                     
</root>        


But, all fields in serviceList are empty is illegal.
Intel's Device Spy will reject the device.
If you remove the "serviceList", device will appear on device spy.
But, WindowsXP won't

It's a trade-off

Or. You must fill out an correct text into serviceList to let WindowsXP and device spy accept this.
Currently, I don't want to implement them. So, I just follow windowsXP's rule.