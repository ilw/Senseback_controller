%Initialize file for capturing data with realterm:
if exist('d:\temp\cap.bin', 'file')
    handles.readfile = fopen('C:\temp\cap.bin','r'); %open capture file for read, not write since write access will be requested by realterm.
    if handles.readfile < 3
        errordlg('Data capture file not opened. Check existence of file at C:\temp\cap.bin, permissions and whether the file is opened in another program.',...
        'Error','modal')
    end
  frewind(handles.readfile);
  fclose(handles.readfile);
else
  % File does not exist. Create file.
  fid = fopen('C:\temp\cap.bin', 'w');
  fclose(fid);
end


handles.hrealterm=actxserver('realterm.realtermintf');
handles.hrealterm.baud=1000000; %1M speed for the UART-SPI interface
handles.hrealterm.flowcontrol=0; %no handshaking currently
handles.hrealterm.Port='3'; %Make sure the COM port number (here 3) is the correct one for the USB UART device here; I can change it if needed, it depends on the computer.
handles.hrealterm.PortOpen=1; %open the comm port


is_open=(handles.hrealterm.PortOpen~=0); %check that serial COM port is open
if (is_open == 0) % if some error occurs while trying to open serial port
    errordlg('Unable to open serial port. Make sure serial port device is connected properly and registered in control panel. Ensure no other program is using the port.',...
        'Error','modal')    
    uicontrol(hObject)
    invoke(handles.hrealterm,'close'); 
    delete(handles.hrealterm);
    return
end

%Test realterm instance to make sure it's alive, useful for callbacks that have to use realterm
if (isfield(handles,'hrealterm'))
    try handles.hrealterm.PortOpen=1; %Checks if realterm is alive
    catch
        errordlg('Realterm not alive. Toggle connect button.',...
                'Error','modal')
        uicontrol(hObject)
        return
    end
    is_open=(handles.hrealterm.PortOpen~=0); %check that serial COM port is open
        if (is_open == 0) 
            errordlg('No connection to device. Toggle connect button.',...
                'Error','modal')
            uicontrol(hObject)
            return
        end
else
        errordlg('No Realterm instance, toggle connect button. If problem persists reinstall Realterm.',...
            'Error','modal')
        uicontrol(hObject)
        return
end






